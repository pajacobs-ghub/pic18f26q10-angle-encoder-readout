// timer2-free-run.c
// PJ, 2019-01-20 PIC18F26K42 code
//     2019-04-22 Adapted to PIC16F18426.
//     2023-02-04 Adapted to allow up to 8 second period.
//                No other changes needed for PIC18F26Q10.

#include <xc.h>
#include <stdint.h>
#include "global_defs.h"
#include "timer2-free-run.h"

void timer2_init(uint8_t count, uint8_t postscale)
{
    // count (prplus1) is number of ticks that will be counted before reset
    // postscale also one more than we need to put into the postscaler
    T2CONbits.ON = 0;
    T2HLT = 0; // Free-running mode with software gate.
    T2CLKCONbits.CS = 0b0101; // Input ticks are MFINTOSC (31kHz), period 32.26us
    T2CONbits.CKPS = 0b110; // 1:64 prescale gives 2.064ms per PR increment
    if (postscale > 16) { postscale = 16; }
    T2CONbits.OUTPS = (uint8_t) (postscale - 1);
    T2PR = (uint8_t) (count - 1);
    T2TMR = 0;
    // TMR2IF will be set once period=prplus1*2.064ms*postscale has elapsed.
    PIR4bits.TMR2IF = 0;
    T2CONbits.ON = 1;
}

void timer2_close(void)
{
    T2CONbits.ON = 0;
    PIR4bits.TMR2IF = 0;
}

void timer2_wait(void)
{
    while (!PIR4bits.TMR2IF) { CLRWDT(); }
    // We reset the flag but leave the timer ticking
    // so that we have accurate periods.
    PIR4bits.TMR2IF = 0;
}
