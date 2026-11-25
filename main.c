/*********************************************************************
File    : main.c
Program : Index-11
Purpose : ADC averaging and periodic USART2 DMA profiling
*********************************************************************/


/*********************************************************************
  Main entry
*********************************************************************/
int main(void)
{
    while (1)
    {
        //configure system clock suing pll at 50mhz 
        //usart2 must be 115200
        //config usart 2 to recieve incoming bytes using recievncver timeout()RTO TO 1MS TRIGERRING HE RTO INTERUPT TO DETECT THE OEND OF A TRA 
        //THE DEKTOP APPLICAION DOESNT NOT APPEND A TERMINATION CHARATER 
        //USING DMA FOR RECEPTION IS RECOMMNED YOU MAY TRACK NUMBER OF RECIEVED BYTES USING dma ORrXNE INTERUPTS [ROVIIDED THE RTO ISR IS USED O SIGNAL PACKET COMPLETION 
        //ONCE THE RECEPTION HAS TIMED OUT RANSMIT THE TOTAL BYTE COUNT BACK THE APPLIVATION OVER USART 2 USING THE EXACT FORMAT 
    }
}

/*********************************************************************
  Functions
*********************************************************************/

/*********************************************************************
  ISRs
*********************************************************************/
