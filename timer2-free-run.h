// timer2-free-run.h
// PJ, 2018-01-20, 2023-02-04
//
#ifndef MY_TIMER2_FREE_RUN
#define MY_TIMER2_FREE_RUN

#include <xc.h>
#include <stdint.h>

void timer2_init(uint8_t period_count, uint8_t postscale);
void timer2_close(void);
void timer2_wait(void);
#endif
