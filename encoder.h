// encoder.h

#ifndef MAGNETIC_ENCODER_H
#define MAGNETIC_ENCODER_H

void init_encoders(void);
void read_encoders(uint16_t *result_a, uint16_t *result_b);

#endif