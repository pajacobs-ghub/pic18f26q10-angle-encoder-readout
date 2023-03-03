// uart.c
// Functions to provide a shim between the C standard library functions
// and UART1 peripheral device on the PIC16F1/PIC18FxxQ10 microcontroller.
// Build with linker set to link in the C99 standard library.
// PJ,
// 2018-12-28 Adapted from PIC16F1778 example.
// 2018-12-30 RTS/CTS flow control
// 2019-03-06 Adapted to PIC16F18324
// 2019-04-15 PIC16F18426
// 2023-02-03 PIC18F26Q10 for magnetic encoder readout
// 2023-03-03 change to linking with C99 library

#include <xc.h>
#include "global_defs.h"
#include "uart.h"
#include <stdio.h>
// #include <conio.h> // no longer used for C99

void uart1_init(long baud)
{
    unsigned int brg_value;
    // Configure PPS MCU_RX=RC7, MCU_TX=RC6 
    GIE = 0;
    PPSLOCK = 0x55;
    PPSLOCK = 0xaa;
    PPSLOCKED = 0;
    RX1PPS = 0b10111; // RC7
    RC6PPS = 0x09; // TX/CK
    PPSLOCK = 0x55;
    PPSLOCK = 0xaa;
    PPSLOCKED = 1;
    ANSELCbits.ANSELC6 = 0;
    TRISCbits.TRISC6 = 0; // TX is output
    LATCbits.LATC6 = 1; // idle high
    TRISCbits.TRISC7 = 1; // RX is input
    ANSELCbits.ANSELC7 = 0; // enable digital input
    // Hardware Flow Control
    // Host-CTS# RC5 pin 16, MCU output
    ANSELCbits.ANSELC5 = 0;
    TRISCbits.TRISC5 = 0;
    // Start out saying that it is not clear to send.
    LATCbits.LATC5 = 1;
    // Host-RTS# RC2 pin 13, MCU input
    ANSELCbits.ANSELC2 = 0;
    TRISCbits.TRISC2 = 1;
    // Use 8N1 asynchronous
    TX1STAbits.SYNC = 0;
    BAUD1CONbits.BRG16 = 1;
    TX1STAbits.BRGH = 1;
    brg_value = (unsigned int) (FOSC/baud/4 - 1);
    // For 32MHz, 115200 baud, expect value of 68.
    //              9600 baud                 832.
    SP1BRG = brg_value;
    TX1STAbits.TXEN = 1;
    RC1STAbits.CREN = 1;
    RC1STAbits.SPEN = 1;
    return;
}

void putch(char data)
{
    // Wait until PC/Host is requesting.
    while (PORTCbits.RC2) { CLRWDT(); }
    // Wait until shift-register empty, then send data.
    while (!TX1STAbits.TRMT) { CLRWDT(); }
    TX1REG = data;
    return;
}

__bit kbhit(void)
// Returns true if a character is waiting in the receive buffer.
// We look for a brief period and then discard any characters found.
{
    char c_arrived, c_discard;
    // Let the PC/Host know that it is clear to send.
    LATCbits.LATC5 = 0;
    // Wait long enough for a character to come through but
    // not long enough to mess up the super-loop period 
    // in the main program.
    __delay_us(20);
    // Let PC/Host know that it is not clear to send.
    LATCbits.LATC5 = 1;
    // Check if any characters have arrived and discard them,
    // so that we don't confuse subsequent calls to getch.
    c_arrived = PIR3bits.RC1IF;
    while (PIR3bits.RC1IF) { c_discard = RC1REG; }
    // Clear possible overflow error.
    if (RC1STAbits.OERR) { 
        RC1STAbits.CREN = 0;
        NOP();
        RC1STAbits.CREN = 1;
    }
    return (__bit)(c_arrived);
}

void uart1_flush_rx(void)
{
    char c_discard;
    while (PIR3bits.RC1IF) { c_discard = RC1REG; }
    // Clear possible overflow error.
    if (RC1STAbits.OERR) { 
        RC1STAbits.CREN = 0;
        NOP();
        RC1STAbits.CREN = 1;
    }
}

char getch(void)
{
    char c;
    // Let the PC/Host know that it is clear to send.
    LATCbits.LATC5 = 0;
    // Block until a character is available in buffer.
    while (!PIR3bits.RC1IF) { CLRWDT(); }
    // Let PC/Host know that it is not clear to send.
    LATCbits.LATC5 = 1;
    // Get the data that came in.
    c = RC1REG;
    // Clear possible overflow error.
    if (RC1STAbits.OERR) { 
        RC1STAbits.CREN = 0;
        NOP();
        RC1STAbits.CREN = 1;
    }
    return c;
}

char getche(void)
{
    char data = getch();
    putch(data); // echo the character
    return data;
}

void uart1_close(void)
{
    TX1STAbits.TXEN = 0;
    RC1STAbits.CREN = 0;
    RC1STAbits.SPEN = 0;
    return;
}
