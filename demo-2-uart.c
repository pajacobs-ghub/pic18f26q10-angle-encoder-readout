// demo-2-uart.c
// Demonstrate transmitting and receiving characters 
// with the PIC18F26Q10 EUSART1 peripheral.
// PJ, 2019-04-15 adapted from pic16f18324 to PIC16F18426
//     2023-02-03 adapted to PIC18F26Q10
//
// Configuration Bit Settings (generated from Config Memory View)
// CONFIG1L
#pragma config FEXTOSC = OFF
#pragma config RSTOSC = HFINTOSC_64MHZ
// CONFIG1H
#pragma config CLKOUTEN = OFF
#pragma config CSWEN = ON
#pragma config FCMEN = ON
// CONFIG2L
#pragma config MCLRE = EXTMCLR
#pragma config PWRTE = ON
#pragma config LPBOREN = OFF
#pragma config BOREN = SBORDIS
// CONFIG2H
#pragma config BORV = VBOR_190
#pragma config ZCD = OFF
#pragma config PPS1WAY = OFF
#pragma config STVREN = ON
#pragma config XINST = OFF
// CONFIG3L
#pragma config WDTCPS = WDTCPS_31
#pragma config WDTE = ON
// CONFIG3H
#pragma config WDTCWS = WDTCWS_7
#pragma config WDTCCS = SC
// CONFIG4L
#pragma config WRT0 = OFF
#pragma config WRT1 = OFF
#pragma config WRT2 = OFF
#pragma config WRT3 = OFF
// CONFIG4H
#pragma config WRTC = OFF
#pragma config WRTB = OFF
#pragma config WRTD = OFF
#pragma config SCANE = ON
#pragma config LVP = ON
// CONFIG5L
#pragma config CP = OFF
#pragma config CPD = OFF
// CONFIG5H
// CONFIG6L
#pragma config EBTR0 = OFF
#pragma config EBTR1 = OFF
#pragma config EBTR2 = OFF
#pragma config EBTR3 = OFF
// CONFIG6H
#pragma config EBTRB = OFF

#include <xc.h>
#include "global_defs.h"
#include "uart.h"
#include <stdio.h>
#include <string.h>

#define GREENLED LATBbits.LATB5


int main(void)
{
    char buf[80];
    char* buf_ptr;
    int n;
    OSCFRQbits.HFFRQ = 0b0110; // Select 32MHz.
    TRISBbits.TRISB5 = 0; // Pin as output for LED.
    GREENLED = 0;
    uart1_init(115200);
    n = printf("\r\nMagnetic encoder readout board with PIC18F26Q10.");
    while (1) {
        GREENLED ^= 1;
        __delay_ms(500);
        n = printf("\r\nEnter some text: ");
        // Characters are echoed as they are typed.
        // Backspace deleting is allowed.
        buf_ptr = gets(buf);
        n = printf("\rEntered text was: ");
        if (buf_ptr) { puts(buf); }
        if (strncmp(buf, "quit", 4) == 0) break;
        n = printf("\r\nNow, do it again.");
    }
    uart1_flush_rx();
    n = printf("Press any key to close the serial port.");
    while (!kbhit()) { CLRWDT(); }
    uart1_close();
    return 0; // Expect that the MCU will reset.
} // end main
