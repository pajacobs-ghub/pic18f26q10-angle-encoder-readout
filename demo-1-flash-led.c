/* demo-1-flash-led.c
 * First code to bring up PIC18F26Q10-I/SP on encoder readout board.
 * PJ 2023-02-02
 */
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

#define GREENLED LATBbits.LATB5

int main()
{
    OSCFRQbits.HFFRQ = 0b0110; // Select 32MHz.
    TRISBbits.TRISB5 = 0; // Pin as output for LED.
    GREENLED = 0;
    while (1) {
        GREENLED ^= 1;
        __delay_ms(1000);
        CLRWDT();
    }
    return 0; // We should never arrive here.
}