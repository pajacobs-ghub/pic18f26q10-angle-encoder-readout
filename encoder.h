// encoder.h

#ifndef MAGNETIC_ENCODER_H
#define MAGNETIC_ENCODER_H

void init_AEAT_encoders(void);
void read_AEAT_encoders(uint16_t *result_a, uint16_t *result_b, uint8_t nbits);

#endif