#include "encoder.h"

#include <avr/interrupt.h>
#include <avr/io.h>

#include <stdio.h>

volatile int16_t _enc[2];

bool a, b;

ISR(INT0_vect) {
    a = (PIND & (1<<2)) > 0;
    b = (PIND & (1<<4)) > 0;
    
    if (a == b) {
        _enc[0]--;
    } else {
        _enc[0]++;
    }
}

ISR(INT1_vect) {
    a = (PIND & (1<<3)) > 0;
    b = (PIND & (1<<5)) > 0;
    
    if (a == b) {
        _enc[1]++;
    } else {
        _enc[1]--;
    }
}

void initEncoders() {
    EICRA |= 5; //set both external interrupts to level change
    EIMSK |= 3; //enable both external interrupts

    uint8_t oldSreg = SREG;
    cli();
    
    _enc[0] = 0;
    _enc[1] = 0;
    
    SREG = oldSreg;
}

void getEncoders(int16_t & A, int16_t & B) {
    uint8_t oldSreg = SREG;
    cli();
    A = _enc[0];
    B = _enc[1];
    SREG = oldSreg;
}

void getAndClear(int16_t & A, int16_t & B) {
    uint8_t oldSreg = SREG;
    cli();
    A = _enc[0];
    B = _enc[1];
    _enc[0] = 0;
    _enc[1] = 0;
    SREG = oldSreg;
}
