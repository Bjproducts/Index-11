/*********************************************************************
File    : main.c
Program : Index-11
Purpose : TIM6-triggered ADC interrupt sampling and serial averages
*********************************************************************/

#include <stdio.h>
#include "clock.h"
#include "gpio.h"
#include "timer.h"
#include "uart.h"

static void TERM_TxStringXY(USART_TypeDef *uart, unsigned row, unsigned column,
                            const char *message);

static volatile uint32_t totals[2];
static volatile uint32_t completedTotals[2];
static volatile unsigned completedBlocks;

/*********************************************************************
  Main entry
*********************************************************************/
int main(void)
{
//   Configure SysTick to trigger an interrupt every 1 ms.
// Every 250 ms, transmit the fixed string "Hello CMPE2250\r\n" (16 bytes) via USART2 using DMA (DMA1_Channel2).
// GPIO Profiling on PA6:
// Drive PA6 HIGH immediately before triggering the DMA transmission.
// Enable the DMA Transfer Complete Interrupt (TCIE) on DMA1_Channel2.
// Clear the interrupt flag in DMA1->IFCR and drive PA6 LOW inside DMA1_Channel2_3_IRQHandler().
// Connect the oscilloscope:
// Channel 1 (Yellow): Pin PA6 (Profiling Pin)
// Channel 2 (Blue): Pin PA2 (USART2_TX)
// In your report, address the following theoretical questions:
// What does the pulse period of PA6 represent?
// What does the pulse width (HIGH time) of PA6 represent?
// At 115,200 baud (1 start bit, 8 data bits, 1 stop bit = 10 bits per frame): 
 
 

    char message[72];
    unsigned lastBlock = 0;

    __disable_irq();
    Clock_InitPll(PLL_50MHZ);

    RCC->IOPENR |= RCC_IOPENR_GPIOAEN;
    RCC->APBENR1 |= RCC_APBENR1_TIM6EN | RCC_APBENR1_USART2EN;
    RCC->APBENR2 |= RCC_APBENR2_ADCEN;
    (void)RCC->APBENR2;

    GPIOA->MODER |= GPIO_MODER_MODE0 | GPIO_MODER_MODE1; // ADC_IN0 / ADC_IN1.
    GPIO_SetPullMode(GPIOA, 0, PullMode_None);
    GPIO_SetPullMode(GPIOA, 1, PullMode_None);
    GPIO_InitAlternateF(GPIOA, 2, 1); // USART2 TX to the virtual COM port.
    UART_Init(USART2, 115200, 0);

    ADC1->CFGR2 = ADC_CFGR2_CKMODE_1; // PCLK / 4 = 12.5 MHz.
    ADC1->CR |= ADC_CR_ADVREGEN;
    for (unsigned i = 0; i < 1000; ++i)
    {
        __NOP(); // Allow at least 20 us for regulator startup.
    }
    ADC1->CR |= ADC_CR_ADCAL;
    while (ADC1->CR & ADC_CR_ADCAL)
    {
    }
    for (unsigned i = 0; i < 32; ++i)
    {
        __NOP(); // Allow at least four ADC clocks before enabling.
    }

    ADC1->ISR = ADC_ISR_CCRDY | ADC_ISR_ADRDY;
    ADC1->CHSELR = ADC_CHSELR_CHSEL0 | ADC_CHSELR_CHSEL1;
    while (!(ADC1->ISR & ADC_ISR_CCRDY))
    {
    }
    ADC1->SMPR = 6UL << ADC_SMPR_SMP1_Pos; // 79.5 sampling cycles.
    // TIM6_TRGO rising edge. Wait for each DR read before continuing.
    ADC1->CFGR1 = (5UL << ADC_CFGR1_EXTSEL_Pos) | ADC_CFGR1_EXTEN_0 | ADC_CFGR1_WAIT;
    ADC1->IER = ADC_IER_EOCIE | ADC_IER_EOSIE;
    ADC1->CR |= ADC_CR_ADEN;
    while (!(ADC1->ISR & ADC_ISR_ADRDY))
    {
    }
    NVIC_EnableIRQ(ADC1_COMP_IRQn);

    Timer_Init(TIM6, 50, 1000); // 1 us ticks, 1 ms between updates.
    TIM6->CR2 = 2UL << TIM_CR2_MMS_Pos; // Update event drives TRGO.
    ADC1->CR |= ADC_CR_ADSTART;
    __enable_irq();
    Timer_SetEnable(TIM6, 1);

    while (1)
    {
        __disable_irq();
        unsigned block = completedBlocks;
        uint32_t sum0 = completedTotals[0];
        uint32_t sum1 = completedTotals[1];
        __enable_irq();

        if (block != lastBlock)
        {
            lastBlock = block;

            // Convert 1000 raw samples to hundredths of a volt, rounded.
            uint32_t volts0 = (sum0 * 330UL + 2047500UL) / 4095000UL;
            uint32_t volts1 = (sum1 * 330UL + 2047500UL) / 4095000UL;
            snprintf(message, sizeof message, "AN0 Average: %lu.%02lu V | AN1 Average: %lu.%02lu V",
                     (unsigned long)(volts0 / 100), (unsigned long)(volts0 % 100),
                     (unsigned long)(volts1 / 100), (unsigned long)(volts1 % 100));
            TERM_TxStringXY(USART2, 10, 10, message);
        }
    }
}

/*********************************************************************
  Functions
*********************************************************************/
static void TERM_TxStringXY(USART_TypeDef *uart, unsigned row, unsigned column,
                            const char *message)
{
    char cursor[24];

    snprintf(cursor, sizeof cursor, "\033[%u;%uH", row, column);
    UART_TxStr(uart, cursor);
    UART_TxStr(uart, message);
}

/*********************************************************************
  ISRs
*********************************************************************/
void ADC1_COMP_IRQHandler(void)
{
    static unsigned channel;
    static unsigned sequences;
    uint32_t status = ADC1->ISR;

    if (status & ADC_ISR_EOC)
    {
        totals[channel] += ADC1->DR;
        channel = 1;
    }
    if (status & ADC_ISR_EOS)
    {
        ADC1->ISR = ADC_ISR_EOS;
        channel = 0;
        if (++sequences == 1000)
        {
            completedTotals[0] = totals[0];
            completedTotals[1] = totals[1];
            ++completedBlocks;
            totals[0] = 0;
            totals[1] = 0;
            sequences = 0;
        }
    }
}
