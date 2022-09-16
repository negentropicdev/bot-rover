#ifndef ENCODER_H_
#define ENCODER_H_

#include <stdint.h>

void initEncoders();

void getEncoders(volatile int16_t & A, volatile int16_t & B);
void getAndClearEncoders(int16_t & A, int16_t & B);

#endif //ENCODER_H_
