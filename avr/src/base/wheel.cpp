#include "wheel.h"

#include <avr/io.h>
#include <stdlib.h>

#include <stdio.h>

Wheel wheelL(&PORTC, &PORTC, 0, 1, &OCR1A, &DDRC, &DDRC);
Wheel wheelR(&PORTC, &PORTC, 2, 3, &OCR1B, &DDRC, &DDRC);

void initMotors() {
    //setup timer1 for motor pwm output
    TCCR1A = 0b10100000; //non-inverted pwm mode, phase & freq correct, top in ICR1
    TCCR1B = 0b00010001; //no prescaler
    ICR1 = MOTOR_MAX; //set PWM resolution
    OCR1A = 0;
    OCR1B = 0;
}

Wheel::Wheel(volatile uint8_t *port1, volatile uint8_t *port2, uint8_t pin1,
        uint8_t pin2, volatile uint16_t *pwm, volatile uint8_t *ddr1,
        volatile uint8_t *ddr2) {
    _port1 = port1;
    _port2 = port2;
    _pin1 = pin1;
    _pin2 = pin2;
    _pwm = pwm;
    
    *ddr1 |= (1<<_pin1);
    *ddr2 |= (1<<_pin2);
    
    if (pwm == &OCR1A) {
        DDRB |= (1<<1);
    } else if (pwm == &OCR1B) {
        DDRB |= (1<<2);
    }
}
    
void Wheel::drive(int16_t val) {
    if (val < 0) {
        val = -val;
        *_port1 |= (1<<_pin1);
        *_port2 &= ~(1<<_pin2);
    } else {
        *_port1 &= ~(1<<_pin1);
        *_port2 |= (1<<_pin2);
    }
    
    *_pwm = val;
}

void Wheel::coast() {
    *_pwm = 0;
}

void Wheel::brake() {
    *_port1 |= (1<<_pin1);
    *_port2 |= (1<<_pin2);
    *_pwm = MOTOR_MAX;
}
