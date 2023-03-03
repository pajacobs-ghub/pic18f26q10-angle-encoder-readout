// i2c.c
// Functions to drive the I2C1 module on the PIC16F18324 microcontroller
// as a single master I2C device, with 7-bit addressing.
// Modeled on the description given in the data sheet DS40001800D, Section 30.
// PJ, 2019-03-11
// Adapted to the PIC18F26Q10-I/SP MCU, mostly by changing the assigned pins.
// PJ, 2023-03-03

#include <xc.h>
#include <stdint.h>
#include "global_defs.h"

static uint8_t i2c1_error = 0; // Will be set if there is a timeout event.
uint8_t i2c1_get_error_flag(void) { return i2c1_error; }

void i2c1_init(void)
{
    // Default pin mapping for PIC18F26Q10-I/SP
    // SCL1 RC3
    // SDA1 RC4
    GIE = 0;
    PPSLOCK = 0x55;
    PPSLOCK = 0xaa;
    PPSLOCKED = 0;
    SSP1CLKPPS = 0b10011; // RC3
    RC3PPS = 0x13; // MSSP1 SCL
    SSP1DATPPS = 0b10100; // RC4
    RC4PPS = 0x10; // MSSP1 SDA
    PPSLOCK = 0x55;
    PPSLOCK = 0xaa;
    PPSLOCKED = 1;
    ANSELCbits.ANSELC3 = 0; // allow digital input
    ANSELCbits.ANSELC4 = 0;
    TRISCbits.TRISC3 = 1;
    TRISCbits.TRISC4 = 1;
    // Configure module
    SSP1STATbits.SMP = 1; // Disable slew-rate control for 100kHz
    SSP1CON1bits.SSPM = 0b1000; // master mode, clock=FOSC/(4*(SSP1ADD+1))
    SSP1ADD = 0x4f; // To get 100 kHz I2C clock when FOSC=32MHz
    SSP1CON1bits.SSPEN = 1; // Enable module.
    PIR3bits.SSP1IF = 0;
    PIR3bits.BCL1IF = 0;
    i2c1_error = 0;
    return;
}

void i2c1_close(void)
{
    SSP1CON1bits.SSPEN = 0;
    return;
}

uint8_t i2c1_read(uint8_t addr7bit, uint8_t n, uint8_t* buf)
{
    uint8_t n_read = 0;
    uint8_t retries, timeout;
    i2c1_error = 0;
    uint8_t addrbyte;
    //
    // TODO: Look for idle state, but timeout if necessary.
    //
    if (SSP1CON1bits.WCOL) {
        SSP1CON1bits.WCOL = 0;
        i2c1_error = 2;
        goto Quit;
    }
    if (PIR3bits.BCL1IF) {
        PIR3bits.BCL1IF = 0;
        i2c1_error = 3;
        goto Quit;
    }
    //
    // Start condition.
    PIR3bits.SSP1IF = 0;
    SSP1CON2bits.SEN = 1;
    retries = 255; timeout = 1;
    while (retries) {
        if (PIR3bits.SSP1IF) { timeout = 0; break; }
        __delay_us(2); --retries;
    }
    PIR3bits.SSP1IF = 0;
    if (timeout) { i2c1_error = 1; goto Quit; }
    //
    // Address slave.
    addrbyte = (uint8_t)(addr7bit << 1) | 1; // R/W bit is 1
    SSP1BUF = addrbyte;
    // Wait for transmission of address to complete. 
    retries = 255; timeout = 1;
    while (retries) {
        if (PIR3bits.SSP1IF) { timeout = 0; break; }
        __delay_us(2); --retries;
    }
    PIR3bits.SSP1IF = 0;
    if (timeout) { i2c1_error = 1; goto Quit; }
    //
    // Look for acknowledgment from slave.
    retries = 255; timeout = 1;
    while (retries) {
        if (!SSP1CON2bits.ACKSTAT) { timeout = 0; break; }
        __delay_us(2); --retries;
    }
    PIR3bits.SSP1IF = 0;
    if (timeout) { i2c1_error = 1; goto Quit; }
    //
    // Now read the data bytes.
    for (uint8_t i=0; i < n; ++i) {
        SSP1CON2bits.RCEN = 1;
        // Collect next data byte when it is available.
        // Looking up to 255 times with pauses of 20us gives us
        // a total wait time of 5.1ms, which should be long enough
        // for the Sensirion pressure sensors.
        retries = 255; timeout = 1;
        while (retries) {
            if (SSP1STATbits.BF) {
                PIR3bits.SSP1IF = 0;
                buf[i] = SSP1BUF;
                SSP1STATbits.BF = 0;
                timeout = 0; break;
            }
            __delay_us(20); --retries;
        }
        if (timeout) { i2c1_error = 1; goto Quit; }
        if (i == (n-1)) {
            SSP1CON2bits.ACKDT = 1; // NACK to indicate that we are done.
        } else {
            SSP1CON2bits.ACKDT = 0; // ACK to indicate that we want more.
        }
        // Initiate the !ACK clocking.
        PIR3bits.SSP1IF = 0;
        SSP1CON2bits.ACKEN = 1;
        retries = 255; timeout = 1;
        while (retries) {
            if (PIR3bits.SSP1IF) { timeout = 0; break; }
            __delay_us(2); --retries;
        }
        PIR3bits.SSP1IF = 0;
        if (timeout) { i2c1_error = 1; goto Quit; }
        ++n_read;
    }
    SSP1CON2bits.RCEN = 0;
    // Stop condition
    PIR3bits.SSP1IF = 0;
    SSP1CON2bits.PEN = 1;
    // Wait for stop condition.
    retries = 255; timeout = 1;
    while (retries) {
        if (PIR3bits.SSP1IF) { timeout = 0; break; }
        __delay_us(2); --retries;
    }
    PIR3bits.SSP1IF = 0;
    if (timeout) { i2c1_error = 1; }
    //
    Quit:
    SSP1CON2bits.RCEN = 0;
    return n_read;
} // end i2c1_read()

uint8_t i2c1_write(uint8_t addr7bit, uint8_t n, uint8_t* buf)
{
    uint8_t n_sent = 0;
    i2c1_error = 0;
    uint8_t retries, timeout;
    uint8_t addrbyte;
    //
    // TODO: Look for idle state, but timeout if necessary.
    //
    if (SSP1CON1bits.WCOL) {
        SSP1CON1bits.WCOL = 0;
        i2c1_error = 2;
        goto Quit;
    }
    if (PIR3bits.BCL1IF) {
        PIR3bits.BCL1IF = 0;
        i2c1_error = 3;
        goto Quit;
    }
    //
    // Start condition.
    PIR3bits.SSP1IF = 0;
    SSP1CON2bits.SEN = 1;
    retries = 255; timeout = 1;
    while (retries) {
        if (PIR3bits.SSP1IF) { timeout = 0; break; }
        __delay_us(2); --retries;
    }
    PIR3bits.SSP1IF = 0;
    if (timeout) { i2c1_error = 1; goto Quit; }
    //
    // Address slave.
    addrbyte = (uint8_t)(addr7bit << 1); // R/W bit is 0
    SSP1BUF = addrbyte;
    // Wait for transmission of address to complete. 
    retries = 255; timeout = 1;
    while (retries) {
        if (PIR3bits.SSP1IF) { timeout = 0; break; }
        __delay_us(2); --retries;
    }
    PIR3bits.SSP1IF = 0;
    if (timeout) { i2c1_error = 1; goto Quit; }
    //
    // Look for acknowledgment from slave.
    retries = 255; timeout = 1;
    while (retries) {
        if (!SSP1CON2bits.ACKSTAT) { timeout = 0; break; }
        __delay_us(2); --retries;
    }
    PIR3bits.SSP1IF = 0;
    if (timeout) { i2c1_error = 1; goto Quit; }
    //
    // Now send the data bytes.
    for (uint8_t i=0; i < n; ++i) {
        // Send next data byte.
        retries = 255; timeout = 1;
        while (retries) {
            if (!SSP1STATbits.BF) {
                SSP1BUF = buf[i];
                timeout = 0; break;
            }
            __delay_us(2); --retries;
        }
        if (timeout) { i2c1_error = 1; goto Quit; }
        // Wait for transmission of data byte to complete.
        retries = 255; timeout = 1;
        while (retries) {
            if (PIR3bits.SSP1IF) { timeout = 0; break; }
            __delay_us(2); --retries;
        }
        PIR3bits.SSP1IF = 0;
        if (timeout) { i2c1_error = 1; goto Quit; }
        // Wait for slave acknowledge.
        retries = 255; timeout = 1;
        while (retries) {
            if (!SSP1CON2bits.ACKSTAT) { timeout = 0; break; }
            __delay_us(2); --retries;
        }
        PIR3bits.SSP1IF = 0;
        if (timeout) { i2c1_error = 1; goto Quit; }
        ++n_sent;
    }
    // Stop condition
    PIR3bits.SSP1IF = 0;
    SSP1CON2bits.PEN = 1;
    // Wait for stop condition.
    retries = 255; timeout = 1;
    while (retries) {
        if (PIR3bits.SSP1IF) { timeout = 0; break; }
        __delay_us(2); --retries;
    }
    PIR3bits.SSP1IF = 0;
    if (timeout) { i2c1_error = 1; }
    //
    Quit:
    return n_sent;
} // end i2c1_write()
