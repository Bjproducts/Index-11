/*********************************************************************
*                    SEGGER Microcontroller GmbH                     *
*                        The Embedded Experts                        *
**********************************************************************

-------------------------- END-OF-HEADER -----------------------------

File    : main.c
Purpose : Generic application start

*/

#include <stdio.h>
#include <stdlib.h>

/*********************************************************************
*
*       main()
*
*  Function description
*   Application entry point.
*/
int main(void) {
// In this version, the CPU must intervene after every single conversion to move data from the ADC to memory.

// Timer Setup: Configure TIM6 to update every 1ms and set its Master Mode (MMS) to generate a TRGO signal on every Update Event.

// ADC Setup: Configure the ADC to be triggered by TIM6_TRGO.

// Interrupts: Enable EOCIE (End of Conversion) and EOSIE (End of Sequence).

// Averaging: Accumulate the sum of AN0 and AN1 inside the ADC1_COMP_IRQHandler.

// Display: Once 1,000 sequences are complete, calculate the average and send the results to Tera Term using TERM_TxStringXY, so it displays always on the same position. 
}

/*************************** End of file ****************************/
