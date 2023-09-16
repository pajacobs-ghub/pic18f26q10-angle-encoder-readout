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
    // Display 76543210
    // Digits are numbered 1 through 8, starting from the right.
    for (uint8_t i=0; i < 8; ++i) {
        spi2_write(i+1, i);
    }
}


void spi2_led_display_unsigned(uint16_t a, uint16_t b)
{
    // Display aaaa bbbb decimal digits.
    uint8_t digits[8];
    uint16_t val_b = b;
    uint16_t val_a = a;
    for (uint8_t i=0; i < 4; ++i) {
        digits[i] = val_b % 10; val_b /= 10;
        digits[i+4] = val_a % 10; val_a /= 10;
    }
    for (uint8_t i=0; i < 8; ++i) {
        spi2_write(i+1, digits[i]);
    }
}

void spi2_led_display_signed(int16_t a, int16_t b)
{
    // Display SaaaSbbb signed decimal numbers
    //         76543210
    // Assuming that we are given two 3-digit numbers.
    uint8_t digits[8];
    uint16_t val_b = (uint16_t)abs(b);
    uint16_t val_a = (uint16_t)abs(a);
    // Use BCD Code B 0x0a for negative sign or 0x0f for blank.
    digits[3] = (b < 0) ? 0x0a : 0x0f;
    digits[7] = (a < 0) ? 0x0a : 0x0f;
    // Generate three digits for each number.
    for (uint8_t i=0; i < 3; ++i) {
        digits[i] = val_b % 10; val_b /= 10;
        digits[i+4] = val_a % 10; val_a /= 10;
    }
    for (uint8_t i=0; i < 8; ++i) {
        spi2_write(i+1, digits[i]);
    }
}
