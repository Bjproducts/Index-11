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
        //ONCE THE RECEPTION HAS TIMED OUT RANSMIT THE TOTAL BYTE COUNT BACK THE APPLIVATION OVER USART 2 USING THE EXACT FORMAT (BYTES RECIEIVED :10)


        //CONFIG ADC1 WITH A CLOCK PRESCALLER OF 1- SO THE ADC RUNS AT 5MHZ 
        //CONFIG A CONVERSION SEQUENCE TO SAMPLE AN4(PA4) AND AN9(PB1).
        //TRIGGER THE CONVERSION SEQUNCE EVERY 100US USING A HARDWARE IMERS UPDATE EVENT AS AN EXTERNAL TRIGGER SOURCE (TRGO)
        //tming verifcaions pins 
        //drive pa6 high when the conversion seqeince os triggered amd LOW wjen nd of sequence interupt occurs tp measure the sequemce conversion time 
        //toggle pa5 everytime a new set of avergages is calculated and printed to verify the update period
        //collct and average 500 samples per channel , convert averages to volatage and print the results to the debig termina formatted to 2 decimal place one chanel per line 
        //if  configured properly the debug termnals values and Pa6 will update every 50ms 

        //the micro will transmit raw binary data rather than ASCII text , standard serial text terminals will not siplay values properly 
        //keep  the debug termnail from q2 actie anf simi transmit 11 byte binary packet over usart2 containing the avergaed AN4 and An9 volatges at the the same 50ms update rate 
    }
}

/*********************************************************************
  Functions
*********************************************************************/

/*********************************************************************
  ISRs
*********************************************************************/
