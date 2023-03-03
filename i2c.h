// i2c.h
// PJ, 2018-01-03

#ifndef MY_I2C
#define MY_I2C
#include <stdint.h>

uint8_t i2c1_get_error_flag(void);
void i2c1_init(void);
void i2c1_close(void);
uint8_t i2c1_read(uint8_t addr7bit, uint8_t n, uint8_t* buf);
uint8_t i2c1_write(uint8_t addr7bit, uint8_t n, uint8_t* buf);

#endif
