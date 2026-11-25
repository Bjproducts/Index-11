/*********************************************************************
File    : main.c
Program : Index-11
Purpose : USART2 packet counts and ADC voltage reporting
*********************************************************************/

#include <stdio.h>
#include <string.h>
#include "clock.h"
#include "gpio.h"
#include "timer.h"
#include "uart.h"

static void SendAdcPacket(float voltage4, float voltage9);

static volatile uint32_t sampleTicks;
static volatile uint32_t completedSums[2];
static volatile uint32_t completedBlocks;
static volatile uint32_t packetCounts[16];
static volatile unsigned countRead;
static volatile unsigned countWrite;

/*********************************************************************
  Main entry
*********************************************************************/
int main(void)
{
    uint32_t lastBlock = 0;
    uint32_t lastSerialTick = 0;
    float voltage4 = 0.0f;
    float voltage9 = 0.0f;
    int adcPacketPending = 0;

    // One-time initializations
    __disable_irq();
    Clock_InitPll(PLL_50MHZ);

    RCC->IOPENR |= RCC_IOPENR_GPIOAEN | RCC_IOPENR_GPIOBEN;
    RCC->APBENR1 |= RCC_APBENR1_TIM6EN | RCC_APBENR1_USART2EN;
    RCC->APBENR2 |= RCC_APBENR2_ADCEN;
    (void)RCC->APBENR2;

    GPIO_InitAlternateF(GPIOA, 2, 1);
    GPIO_InitAlternateF(GPIOA, 3, 1);
    UART_Init(USART2, 115200, 1);
    USART2->CR1 &= ~USART_CR1_UE;
    USART2->CR2 |= USART_CR2_RTOEN;
    USART2->RTOR = 115; // 115 bit times at 115200 baud is about 1 ms.
    USART2->ICR = USART_ICR_RTOCF;
    USART2->CR1 |= USART_CR1_RTOIE | USART_CR1_UE;
    NVIC_SetPriority(USART2_LPUART2_IRQn, 2);
    NVIC_EnableIRQ(USART2_LPUART2_IRQn);

    GPIO_Clear(GPIOA, 5);
    GPIO_Clear(GPIOA, 6);
    GPIO_InitOutput(GPIOA, 5);
    GPIO_InitOutput(GPIOA, 6);
    GPIOA->MODER |= GPIO_MODER_MODE4; // PA4 / AN4.
    GPIOB->MODER |= GPIO_MODER_MODE1; // PB1 / AN9.
    GPIO_SetPullMode(GPIOA, 4, PullMode_None);
    GPIO_SetPullMode(GPIOB, 1, PullMode_None);

    RCC->CCIPR &= ~RCC_CCIPR_ADCSEL; // ADC kernel clock is SYSCLK.
    ADC1->CFGR2 = 0; // Asynchronous ADC clock.
    ADC1_COMMON->CCR = ADC_CCR_PRESC_2 | ADC_CCR_PRESC_0; // 50 MHz / 10.
    ADC1->CR |= ADC_CR_ADVREGEN;
    for (unsigned i = 0; i < 1000; ++i)
    {
        __NOP(); // At least 20 us before calibration.
    }
    ADC1->CR |= ADC_CR_ADCAL;
    while (ADC1->CR & ADC_CR_ADCAL)
    {
    }
    for (unsigned i = 0; i < 50; ++i)
    {
        __NOP(); // At least five ADC clocks after calibration.
    }

    ADC1->ISR = ADC_ISR_CCRDY | ADC_ISR_ADRDY;
    ADC1->CHSELR = ADC_CHSELR_CHSEL4 | ADC_CHSELR_CHSEL9;
    while (!(ADC1->ISR & ADC_ISR_CCRDY))
    {
    }
    ADC1->SMPR = 5UL << ADC_SMPR_SMP1_Pos; // 39.5 sample cycles per channel.
    ADC1->CFGR1 = (5UL << ADC_CFGR1_EXTSEL_Pos) | ADC_CFGR1_EXTEN_0 | ADC_CFGR1_WAIT;
    ADC1->IER = ADC_IER_EOCIE | ADC_IER_EOSIE;
    ADC1->CR |= ADC_CR_ADEN;
    while (!(ADC1->ISR & ADC_ISR_ADRDY))
    {
    }
    NVIC_SetPriority(ADC1_COMP_IRQn, 1);
    NVIC_EnableIRQ(ADC1_COMP_IRQn);

    Timer_Init(TIM6, 50, 100); // 1 us ticks, 100 us update period.
    TIM6->CR2 = 2UL << TIM_CR2_MMS_Pos; // Update event drives ADC TRGO.
    NVIC_SetPriority(TIM6_DAC_LPTIM1_IRQn, 0);
    Timer_EnableInterrupt(TIM6, TIM6_DAC_LPTIM1_IRQn, TimUIE);
    ADC1->CR |= ADC_CR_ADSTART;
    __enable_irq();
    Timer_SetEnable(TIM6, 1);

    // Infinite loop
    while (1)
    {
        if (completedBlocks != lastBlock)
        {
            __disable_irq();
            lastBlock = completedBlocks;
            uint32_t sum4 = completedSums[0];
            uint32_t sum9 = completedSums[1];
            __enable_irq();

            voltage4 = (float)sum4 * (3.3f / (4095.0f * 500.0f));
            voltage9 = (float)sum9 * (3.3f / (4095.0f * 500.0f));
            unsigned mv4 = (unsigned)(voltage4 * 100.0f + 0.5f);
            unsigned mv9 = (unsigned)(voltage9 * 100.0f + 0.5f);
            printf("AN4: %u.%02u V\nAN9: %u.%02u V\n",
                   mv4 / 100, mv4 % 100, mv9 / 100, mv9 % 100);
            GPIO_Toggle(GPIOA, 5);
            adcPacketPending = 1;
        }

        // Give the desktop app's 5 ms serial read window time to close.
        if (sampleTicks - lastSerialTick >= 60)
        {
            if (adcPacketPending)
            {
                SendAdcPacket(voltage4, voltage9);
                adcPacketPending = 0;
            }
            else if (countRead != countWrite)
            {
                char reply[32];
                uint32_t count = packetCounts[countRead];
                countRead = (countRead + 1) % 16;
                snprintf(reply, sizeof reply, "BYTES RECIEIVED :%lu", (unsigned long)count);
                UART_TxStr(USART2, reply);
            }
            else
            {
                continue;
            }
            while (!(USART2->ISR & USART_ISR_TC))
            {
            }
            lastSerialTick = sampleTicks;
        }
    }
}

/*********************************************************************
  Functions
*********************************************************************/
static void SendAdcPacket(float voltage4, float voltage9)
{
    // Cortex-M stores floats little-endian, matching BitConverter.ToSingle.
    uint8_t packet[11] = {0xC1, 4};
    memcpy(&packet[2], &voltage4, 4);
    packet[6] = 9;
    memcpy(&packet[7], &voltage9, 4);
    for (unsigned i = 0; i < sizeof packet; ++i)
    {
        UART_TxByte(USART2, packet[i]);
    }
}

/*********************************************************************
  ISRs
*********************************************************************/
void USART2_LPUART2_IRQHandler(void)
{
    static uint32_t byteCount;
    uint32_t status = USART2->ISR;

    if (status & USART_ISR_RTOF)
    {
        USART2->ICR = USART_ICR_RTOCF;
        unsigned next = (countWrite + 1) % 16;
        if (byteCount && next != countRead)
        {
            packetCounts[countWrite] = byteCount;
            countWrite = next;
        }
        byteCount = 0;
    }

    if (status & USART_ISR_RXNE_RXFNE)
    {
        (void)USART2->RDR;
        ++byteCount;
    }
}

void TIM6_DAC_LPTIM1_IRQHandler(void)
{
    TIM6->SR = ~TIM_SR_UIF;
    GPIO_Set(GPIOA, 6);
    ++sampleTicks;
}

void ADC1_COMP_IRQHandler(void)
{
    static uint32_t sums[2];
    static unsigned channel;
    static unsigned sequences;
    uint32_t status = ADC1->ISR;

    if (status & ADC_ISR_EOC)
    {
        sums[channel] += ADC1->DR;
        channel = 1;
    }
    if (status & ADC_ISR_EOS)
    {
        ADC1->ISR = ADC_ISR_EOS;
        GPIO_Clear(GPIOA, 6);
        channel = 0;
        if (++sequences == 500)
        {
            completedSums[0] = sums[0];
            completedSums[1] = sums[1];
            ++completedBlocks;
            sums[0] = 0;
            sums[1] = 0;
            sequences = 0;
        }
    }
}
