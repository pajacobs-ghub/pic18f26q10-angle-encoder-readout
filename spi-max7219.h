// spi-max7219.h
// PJ 2023-03-07

#ifndef SPI_MAX7219
#define SPI_MAX7219

void spi2_init(void);
void spi2_write(uint8_t addr, uint8_t data);
void max7219_init(void);
void spi2_led_display_unsigned(uint16_t a, uint16_t b);
void spi2_led_display_signed(int16_t a, int16_t b);

#endif