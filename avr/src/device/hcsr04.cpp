#include "hcsr04.h"

#include <avr/interrupt.h>

#include "../system/avr_timer.h"

#define DEBUG

#ifdef DEBUG
    #include <stdio.h>
#endif

HCSR04::HCSR04(const SonarDef &def) {
    _port = def.port;
    _ddr = def.ddr;
    _pin = def.pin;
    _bitmask = (1<<def.bit);
}

HCSR04::HCSR04(volatile uint8_t *port, volatile uint8_t *ddr, volatile uint8_t *pin, uint8_t bit) {
    _port = port;
    _ddr = ddr;
    _pin = pin;
    _bitmask = (1<<bit);
}

unsigned long HCSR04::range() {
    cli();
    unsigned long iter = _range();
    sei();

    return iter;
}

unsigned long HCSR04::_range() {
    volatile unsigned long d = 0;
    volatile unsigned long c = 0;

    *_ddr |= _bitmask; //set pin to output

    *_port |= _bitmask; // start pulse
    
    delay_us(36);

    *_port &= ~_bitmask; //end pulse

    *_ddr &= ~_bitmask; // change to input

    while ((*_pin & _bitmask) == 0) {
        ++d;
        if (d > 1000) return 0;
    }

    while ((*_pin & _bitmask) != 0) {
        ++c;

        if (c > 20000) return 1;
    }

    return c;
}