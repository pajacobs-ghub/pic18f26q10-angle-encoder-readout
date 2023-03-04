// encoder-readout.c
// Read the magnetic encoder position and report via the serial port.
// PJ 2023-02-05
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
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include "uart.h"
#include "timer2-free-run.h"
#include "encoder.h"
#include "i2c.h"

#define GREENLED LATBbits.LATB5

// Things needed for the I2C-LCD
#define NCBUF 16
static char char_buffer[NCBUF];
#define ADDR 0x51

void display_to_lcd(uint16_t a, uint16_t b)
{
    int n;
    uint8_t* cptr;
    // Return to home position
    char_buffer[0] = 0xfe; char_buffer[1] =  0x46;
    n = i2c1_write(ADDR, 2, (uint8_t*)char_buffer);
    __delay_ms(5); // Give the LCD time
    n = sprintf(char_buffer, "A:%4u B:%4u", a, b);
    for (uint8_t i=0; i < NCBUF; ++i) {
        cptr = (uint8_t*)&char_buffer[i];
        if (*cptr == 0) break;
        n = i2c1_write(ADDR, 1, cptr);
        __delay_us(500); // Give the LCD time
    }
}

int main(void)
{
    int n;
    uint16_t a, b;
    //
    OSCFRQbits.HFFRQ = 0b0110; // Select 32MHz.
    TRISBbits.TRISB5 = 0; // Pin as output for LED.
    GREENLED = 0;
    init_encoders();
    uart1_init(115200);
    i2c1_init();
    __delay_ms(50); // Let the LCD get itself sorted at power-up.
    // Clear the LCD
    char_buffer[0] = 0xfe; char_buffer[1] =  0x51;
    n = i2c1_write(ADDR, 2, (uint8_t*)char_buffer);
    __delay_ms(5); // Give the LCD time
    timer2_init(2*15, 8); // 15 * 2.064ms * 8 = 248ms period
    //
    n = printf("\r\nMagnetic encoder readout.");
    timer2_wait();
    while (1) {
        read_encoders(&a, &b);
        n = printf("\r\n%4u %4u", a, b);
        display_to_lcd(a, b);
        uint8_t err = i2c1_get_error_flag();
        if (err) { printf("  i2c err=%u", err); }
        // Light LED to indicate slack time.
        // We can use the oscilloscope to measure the slack time,
        // in case we don't allow enough time for the tasks.
        GREENLED = 1;
        timer2_wait();
        GREENLED = 0;
    }
    timer2_close();
    uart1_close();
    return 0; // Expect that the MCU will reset.
} // end main
