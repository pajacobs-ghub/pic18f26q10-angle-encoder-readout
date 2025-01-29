// encoder-readout.c
// Read the magnetic encoder position and report via the serial port
// and a display (LCD and/or 7-segment LED)
// PJ 2023-02-05, 2023-03-08
// PJ 2023-08-13 Update to include reading of AS5600 encoder.
// PJ 2023-08-14 Get the output displaying values in degrees.
// PJ 2023-09-01 Alister's scaling for the friction-wheel drive.
// PJ 2023-09-02 Get the friction-wheel scaling right-way-up.
// PJ 2023-09-13 Tune friction-wheel scaling for actual diameters.
// PJ 2023-09-13 EEPROM storage for reference values.
// PJ 2023-09-15 Rework format of signed values sent to UART.
// PJ 2023-09-18 Increase LED refresh rate for Jeremy.
// PJ 2025-01-22 Backport some of the Lika AS36 code (1/100 degree units)
//               Handle the variety of resolutions for AEAT magnetic encoders.
// PJ 2025-01-23 Gerard's request for comma-separated values
// PJ 2025-01-29 Limit reference values restored from EEPROM
//               to the range of the specific encoder.
//
// This version string will be printed shortly after MCU reset.
#define VERSION_STR "\r\nv3.2 2025-01-29"
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
#include "eeprom.h"
#include "uart.h"
#include "timer2-free-run.h"
#include "encoder.h"
#include "i2c.h"
#include "spi-max7219.h"

#define GREENLED LATBbits.LATB5
#define SW0 PORTAbits.RA0
#define SW1 PORTAbits.RA1
#define SW2 PORTAbits.RA2
#define SW3 PORTAbits.RA3
#define PUSHBUTTONA PORTBbits.RB4
#define PUSHBUTTONB PORTCbits.RC1

// Things needed for the I2C-LCD and AS5600 encoder
#define NCBUF 20
static char char_buffer[NCBUF];
#define ADDR_LCD 0x51
#define ADDR_AS5600 0x36

void display_to_lcd_unsigned(uint16_t a, uint16_t b)
{
    int n;
    uint8_t* cptr;
    // Set cursor to DDRAM address 0
    char_buffer[0] = 0xfe; char_buffer[1] = 0x45; char_buffer[2] = 0x00;
    n = i2c1_write(ADDR_LCD, 3, (uint8_t*)char_buffer);
    __delay_ms(3); // Give the LCD time
    n = sprintf(char_buffer, "A:%4u B:%4u    ", a, b);
    for (uint8_t i=0; i < NCBUF; ++i) {
        cptr = (uint8_t*)&char_buffer[i];
        if (*cptr == 0) break;
        n = i2c1_write(ADDR_LCD, 1, cptr);
        __delay_ms(2); // Give the LCD time
    }
}

void values_to_string_buffer(int16_t a, int16_t b, char* chrs)
{
    // Assemble a string representation of the signed integer values
    // that represent the values (that arrive in 1/100 of a degree).
    // Expected values are in range -18000 through 18000.
    // chrs is an array of characters, length 16.
    // 0  1  2  3  4  5  6  7  8  9 10 11 12 13 14 15  index
    // -  1  8  0  .  0  0  ,  -  1  8  0  .  0  0 \0  content
    uint16_t val_a = (uint16_t) abs(a);
    uint16_t val_b = (uint16_t) abs(b);
    chrs[7] = ','; // comma between numbers
    chrs[15] = 0; // Terminator
    chrs[4] = '.';
    chrs[12] = '.';
    // Signs
    chrs[0] = (a < 0) ? '-' : ' ';
    chrs[8] = (b < 0) ? '-' : ' ';
    // Decimal digits for 1/100 degree.
    chrs[6] = '0' + val_a % 10; val_a /= 10;
    chrs[14] = '0' + val_b % 10; val_b /= 10;
    // Decimal digits for 1/10 degree
    chrs[5] = '0' + val_a % 10; val_a /= 10;
    chrs[13] = '0' + val_b % 10; val_b /= 10;
    for (uint8_t i=0; i < 3; ++i) {
        chrs[3-i] = '0' + val_a % 10; val_a /= 10;
        chrs[11-i] = '0' + val_b % 10; val_b /= 10;
    }
}

int main(void)
{
    int n;
    uint16_t a_raw, b_raw;
    uint16_t a_ref;
    uint16_t b_ref;
    int32_t a_signed, b_signed; // 32-bit to store angles in 1/100 degree resolution.
    int32_t big; // working variable for scaling to degrees
    char digits_buffer[16]; // string of characters to display signed values
    //
    uint8_t lcd_count_display = 0;
    uint8_t lcd_count_clear = 0;
    uint8_t led_count_display = 0;
    //
    // Default/expected configuration.
    uint8_t use_uart = 1;
    uint8_t with_rts_cts = 1;
    uint8_t use_i2c_lcd = 0;
    uint8_t use_i2c_AS5600 = 0;
    uint8_t assume_AEAT_12bit = 1;
    uint8_t use_spi_led_display = 1;
    //
    OSCFRQbits.HFFRQ = 0b0110; // Select 32MHz.
    TRISBbits.TRISB5 = 0; // Pin as output for LED.
    GREENLED = 0;
    TRISBbits.TRISB4 = 1; ANSELBbits.ANSELB4 = 0; WPUBbits.WPUB4 = 1; // PUSHBUTTONA
    TRISCbits.TRISC1 = 1; ANSELCbits.ANSELC1 = 0; WPUCbits.WPUC1 = 1; // PUSHBUTTONB
    //
    // Configure the board by looking at the state of the switches.
    TRISAbits.TRISA0 = 1; ANSELAbits.ANSELA0 = 0; WPUAbits.WPUA0 = 1; // Input SW0
    TRISAbits.TRISA1 = 1; ANSELAbits.ANSELA1 = 0; WPUAbits.WPUA1 = 1; // Input SW1
    TRISAbits.TRISA2 = 1; ANSELAbits.ANSELA2 = 0; WPUAbits.WPUA2 = 1; // Input SW2
    TRISAbits.TRISA3 = 1; ANSELAbits.ANSELA3 = 0; WPUAbits.WPUA3 = 1; // Input SW3
    if (SW0) { use_uart = 1; } else { use_uart = 0; }
    if (SW1) { with_rts_cts = 1; } else { with_rts_cts = 0; }
    if (SW2) { use_i2c_AS5600 = 1; } else { use_i2c_AS5600 = 0; }
    if (SW3) { assume_AEAT_12bit = 1; } else { assume_AEAT_12bit = 0; }
    uint8_t aeat_nbits = (assume_AEAT_12bit) ? 12 : 10;
    //
    // Get ref values out of EEPROM.
    // With a freshly-programmed chip, all of the bits read from the EEPROM
    // will be 1, and the resulting reference value will be 0xffff and
    // out of range for a 10-bit or 12-bit encoder.
    a_ref = (uint16_t) (DATAEE_ReadByte(1) << 8) | DATAEE_ReadByte(0);
    b_ref = (uint16_t) (DATAEE_ReadByte(3) << 8) | DATAEE_ReadByte(2);
    if (use_i2c_AS5600 || assume_AEAT_12bit) {
        a_ref &= 0x0fff;
    } else {
        a_ref &= 0x03ff;
    }
    if (assume_AEAT_12bit) {
        b_ref &= 0x0fff;
    } else {
        b_ref &= 0x03ff;
    }
    //
    // Initialize the peripherals that are in play.
    init_AEAT_encoders();
    if (use_uart) {
        uart1_init(115200);
        __delay_ms(50); // Need a bit of delay to not miss the first characters.
        uart1_flush_rx();
        n = printf("\r\nReadout for AEAT-901x and AS5600 magnetic angle encoders.");
        n = printf(VERSION_STR);
        if (with_rts_cts) {
            n = printf("\r\nUsing RTS/CTS.");
        } else {
            n = printf("\r\nNOT using RTS/CTS.");
        }
        if (use_i2c_AS5600) {
            n = printf("\r\nUsing the AS5600 encoder on I2C.");
        } else {
            n = printf("\r\nNOT using AS5600 encoder on I2C.");
        }
        if (assume_AEAT_12bit) {
            n = printf("\r\nAssuming 12-bit AEAT-9012 encoders.");
        } else { 
            n = printf("\r\nAssuming 10-bit AEAT-9010 encoders.");
        }
        n = printf("\r\na_ref: %4u  b_ref: %4u", a_ref, b_ref);
    }
    if (use_i2c_lcd || use_i2c_AS5600) {
        i2c1_init();
        __delay_ms(50); // Let the LCD get itself sorted at power-up.
    }
    if (use_spi_led_display) {
        spi2_init();
        max7219_init();
    }
    timer2_init(7, 8); // 7 * 2.064ms * 8 = 116ms period
    //
    timer2_wait();
    while (1) {
        // 1. Read the raw values from the sensors.
        read_AEAT_encoders(&a_raw, &b_raw, aeat_nbits);
        if (use_i2c_AS5600) {
            // We are going to replace reading A with the AS5600 data.
            char_buffer[0] = 0x0c; // RAW ANGLE register, high byte
            n = i2c1_write(ADDR_AS5600, 1, (uint8_t*)char_buffer);
            n = i2c1_read(ADDR_AS5600, 2, (uint8_t*)char_buffer);
            uint8_t err = i2c1_get_error_flag();
            if (err && use_uart) { printf("  i2c err=%u", err); }
            a_raw = ((uint16_t)(char_buffer[0] & 0x0f)<<8) | (uint16_t)char_buffer[1];
        }
        // 2. If the push buttons are active (low), set the reference values.
        if (PUSHBUTTONA == 0) {
            a_ref = a_raw;
            DATAEE_WriteByte(0, (uint8_t)(a_ref & 0xff)); // low byte
            DATAEE_WriteByte(1, (uint8_t)((a_ref & 0xff00) >> 8)); // high byte
            __delay_ms(1000);
            n = printf("\r\na_ref = %4u", a_ref);
        }
        if (PUSHBUTTONB == 0) {
            b_ref = b_raw;
            DATAEE_WriteByte(2, (uint8_t)(b_ref & 0xff)); // low byte
            DATAEE_WriteByte(3, (uint8_t)((b_ref & 0xff00) >> 8)); // high byte
            __delay_ms(1000);
            n = printf("\r\nb_ref = %4u", b_ref);
        }
        a_signed = (int32_t)a_raw - (int32_t)a_ref;
        b_signed = (int32_t)b_raw - (int32_t)b_ref;
        // 3. Convert to units of 1/100 degree.
        //    12-bit sensor range is 4096. 36000/4096 == 1125/128
        //    10-bit sensor range is 1024. 36000/1024 == 1125/32
        big = a_signed * 1125;
        int16_t aeat_divisor = (assume_AEAT_12bit)? 128 : 32;
        if (use_i2c_AS5600) {
            a_signed = big/128;
        } else {
            a_signed = big/aeat_divisor;
        }
        // AEAT sensor range is 1024.
        big = b_signed * 1125;
        b_signed = big/aeat_divisor;
        // 4. Bring into -180 to 180 degree range by wrapping around.
        if (a_signed < -18000) a_signed += 36000;
        if (a_signed > 18000) a_signed -= 36000;
        if (b_signed < -18000) b_signed += 36000;
        if (b_signed > 18000) b_signed -= 36000;
        //
        // 5. Some output.
        if (use_uart) {
            uart1_flush_rx();
            values_to_string_buffer((int16_t)a_signed, (int16_t)b_signed, digits_buffer);
            n = printf("\r\n%4u,%4u,%s", a_raw, b_raw, digits_buffer);
        }
        if (use_i2c_lcd) {
            if (lcd_count_clear == 0) {
                // Clear the LCD very occasionally because we will
                // sometimes get bad data sent via the I2C bus.
                // Bad data should not happen, however,
                // the dangly wires on the prototype boards
                // are a potential source of trouble.
                char_buffer[0] = 0xfe; char_buffer[1] =  0x51;
                n = i2c1_write(ADDR_LCD, 2, (uint8_t*)char_buffer);
                __delay_ms(10); // Give the LCD time
                lcd_count_clear = 120;
            } else {
                lcd_count_clear--;
            }
            if (lcd_count_display == 0) {
                // Occasionally write the new data to the LCD.
                // We want this fast enough to inform the operator
                // of change but not too fast to be unreadable.
                display_to_lcd_unsigned(a_raw, b_raw);
                lcd_count_display = 4;
            } else {
                lcd_count_display--;
            }
            uint8_t err = i2c1_get_error_flag();
            if (err && use_uart) { printf("  i2c err=%u", err); }
        }
        if (use_spi_led_display) {
            if (led_count_display == 0) {
                // spi2_led_display_unsigned(a_raw, b_raw);
                // Display integral degrees only to 7-segment LED display.
                spi2_led_display_signed((int16_t)a_signed/100, (int16_t)b_signed/100);
                led_count_display = 0; // every pass for Jeremy
            } else {
                led_count_display--;
            }
        }
        // Light LED to indicate slack time.
        // We can use the oscilloscope to measure the slack time,
        // in case we don't allow enough time for the tasks.
        GREENLED = 1;
        timer2_wait();
        GREENLED = 0;
    }
    // Don't actually expect to arrive here but, just to keep things tidy...
    timer2_close();
    if (use_i2c_lcd || use_i2c_AS5600) {
        i2c1_close();
    }
    if (use_uart) uart1_close();
    return 0; // Expect that the MCU will reset if we arrive here.
} // end main
