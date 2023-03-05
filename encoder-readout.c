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
#define NCBUF 20
static char char_buffer[NCBUF];
#define ADDR 0x51

void display_to_lcd(uint16_t a, uint16_t b)
{
    int n;
    uint8_t* cptr;
    // Set cursor to DDRAM address 0
    char_buffer[0] = 0xfe; char_buffer[1] = 0x45; char_buffer[2] = 0x00;
    n = i2c1_write(ADDR, 3, (uint8_t*)char_buffer);
    __delay_ms(3); // Give the LCD time
    n = sprintf(char_buffer, "A:%4u B:%4u    ", a, b);
    for (uint8_t i=0; i < NCBUF; ++i) {
        cptr = (uint8_t*)&char_buffer[i];
        if (*cptr == 0) break;
        n = i2c1_write(ADDR, 1, cptr);
        __delay_ms(2); // Give the LCD time
    }
}

int main(void)
{
    int n;
    uint16_t a, b;
    uint8_t lcd_count_display = 0;
    uint8_t lcd_count_clear = 0;
    //
    OSCFRQbits.HFFRQ = 0b0110; // Select 32MHz.
    TRISBbits.TRISB5 = 0; // Pin as output for LED.
    GREENLED = 0;
    init_encoders();
    uart1_init(115200);
    i2c1_init();
    __delay_ms(50); // Let the LCD get itself sorted at power-up.
    timer2_init(15, 8); // 15 * 2.064ms * 8 = 248ms period
    //
    n = printf("\r\nMagnetic encoder readout.");
    timer2_wait();
    while (1) {
        read_encoders(&a, &b);
        n = printf("\r\n%4u %4u", a, b);
        if (lcd_count_clear == 0) {
            // Clear the LCD very occasionally because we will
            // sometimes get bad data sent via the I2C bus.
            // Bad data should not happen, however,
            // the dangly wires on the prototype boards
            // are a potential source of trouble.
            char_buffer[0] = 0xfe; char_buffer[1] =  0x51;
            n = i2c1_write(ADDR, 2, (uint8_t*)char_buffer);
            __delay_ms(10); // Give the LCD time
            lcd_count_clear = 120;
        } else {
            lcd_count_clear--;
        }
        if (lcd_count_display == 0) {
            // Occasionally write the new data to the LCD.
            // We want this fast enough to inform the operator
            // of change but not too fast to be unreadable.
            display_to_lcd(a, b);
            lcd_count_display = 4;
        } else {
            lcd_count_display--;
        }
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
