// encoder.c
// Functions to read the AEAT-6010 magnetic encoder from a PIC18F26Q10-I/SP.
// PJ 2023-02-05
#include <xc.h>
#include "global_defs.h"


// Pin assignments for the encoder's interface.
// Two encoders are accommodated, A and B.
#define CSn LATAbits.LATA4
#define CLK LATAbits.LATA5
#define DI_A PORTAbits.RA6
#define DI_B PORTAbits.RA7

void init_encoders(void)
{
    TRISAbits.TRISA4 = 0; // CSn output
    SLRCONAbits.SLRA4 = 1; // limit slew rate
    CSn = 1; // idle high
    //
    TRISAbits.TRISA5 = 0; // CLK output
    SLRCONAbits.SLRA5 = 1; // limit slew rate
    CLK = 1; // idle high
    //
    TRISAbits.TRISA6 = 1; // data input A
    ANSELAbits.ANSELA6 = 0;
    WPUAbits.WPUA6 = 1;
    //
    TRISAbits.TRISA7 = 1; // data input B
    ANSELAbits.ANSELA7 = 0;
    WPUAbits.WPUA7 = 1;
    
}

void read_encoders(uint16_t *result_a, uint16_t *result_b)
{
    uint8_t i;
    uint16_t bits_a, bits_b;
    // Presuming CLK = 1; CSn = 1; at the start.
    bits_a = 0;
    bits_b = 0;
    CSn = 0; // select the encoders
    __delay_us(1);
    for (i=0; i < 10; i++) {
        // Shift previous bits left.
        bits_a <<= 1; bits_b <<= 1;
        // Clock next bit.
        CLK = 0; __delay_us(1); CLK = 1; __delay_us(1);
        // Record the new bit for both encoders.
        bits_a |= DI_A; bits_b |= DI_B;
    }
    __delay_us(1);
    CSn = 1; // deselect encoders
    *result_a = bits_a; *result_b = bits_b;
}
