// spi-max7219.c
// Use the SPI2 peripheral on the pic18f26q10-i/sp
// to send data to a max7219 display driver and its 8 7-segment digits.
//
// PJ, 2023-03-07
//

#include <xc.h>
#include "global_defs.h"
#include <stdint.h>
#include "spi-max7219.h"

#define CSn LATBbits.LATB0

void spi2_init(void)
{
    ANSELBbits.ANSELB1 = 0; TRISBbits.TRISB1 = 0; LATBbits.LATB1 = 0; // SCK2
    ANSELBbits.ANSELB2 = 0; TRISBbits.TRISB2 = 1; WPUBbits.WPUB2 = 1; // SDI2
    ANSELBbits.ANSELB3 = 0; TRISBbits.TRISB3 = 0; LATBbits.LATB3 = 0; // SDO2
    ANSELBbits.ANSELB0 = 0; TRISBbits.TRISB0 = 0; CSn = 1; // CSn
    // Configure SPI2 peripheral device.
    GIE = 0;
    PPSLOCK = 0x55;
    PPSLOCK = 0xaa;
    PPSLOCKED = 0;
    SSP2CLKPPS = 0b01001; // RB1
    RB1PPS = 0x11; // SCK2
    SSP2DATPPS = 0b01010; // SDI is RB2
    RB3PPS = 0x12; // SDO2
    PPSLOCK = 0x55;
    PPSLOCK = 0xaa;
    PPSLOCKED = 1;
    //
    SSP2STATbits.SMP = 0; // Sample in middle of data output time
    SSP2STATbits.CKE = 1; // Transmit data on active to idle level of clock
    SSP2CON1bits.CKP = 0; // Clock idles low
    SSP2CON1bits.SSPM = 0b0010; // Mode is master, clock is FOSC/64
    SSP2CON1bits.SSPEN = 1; // Enable
}

void spi2_write(uint8_t addr, uint8_t data)
{
    unsigned char dummy;
    CSn = 0; __delay_us(1);
    if (SSP2CON1bits.WCOL) SSP2CON1bits.WCOL = 0;
    // Send instruction byte followed by gain byte.
    PIR3bits.SSP2IF = 0;
    SSP2BUF = addr;
    while (!PIR3bits.SSP2IF) { /* wait for transmission to complete */ }
    dummy = SSP2BUF; // Discard incoming data.
    PIR3bits.SSP2IF = 0;
    SSP2BUF = data;
    while (!PIR3bits.SSP2IF) { /* wait for transmission to complete */ }
    dummy = SSP2BUF; // Discard incoming data.
    __delay_us(1);
    CSn = 1;
}

void max7219_init(void)
{
    spi2_write(0x0c, 0x01); // shutdown register: normal operation
    spi2_write(0x0f, 0x00); // display test register: normal mode
    spi2_write(0x0a, 0x01); // intensity register: small
    spi2_write(0x0b, 0x07); // scan limit: display all digits
    spi2_write(0x09, 0xff); // decode mode: Code B decode all digits
    spi2_write(0x01, 0x00); // digit 1
    spi2_write(0x02, 0x01); // digit 2
    spi2_write(0x03, 0x02); // digit 3
    spi2_write(0x04, 0x03); // digit 4
    spi2_write(0x05, 0x04); // digit 5
    spi2_write(0x06, 0x05); // digit 6
    spi2_write(0x07, 0x06); // digit 7
    spi2_write(0x08, 0x07); // digit 8
}


void spi2_led_display(uint16_t a, uint16_t b)
{
    spi2_write(0x0b, 0x07); // display all digits
    spi2_write(0x01, 0x00); // digit 1
}
