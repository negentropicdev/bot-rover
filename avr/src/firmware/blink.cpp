//#include "system/avr_serial.h"
#include "system/avr_timer.h"
//#include "system/avr_i2c.h"

#include <avr/io.h>
#include <avr/interrupt.h>

int main() {
    initTimer();

    sei();

    unsigned long pulseDelay = 1000;
    unsigned long curMillis = millis();
    unsigned long lastPulse = curMillis;
    uint8_t n = 0;

    DDRB = (1<<5);
    PORTB = 0;

    while(1) {
        curMillis = millis();
        if (curMillis - lastPulse >= pulseDelay) {
            PINB |= (1 << 5);
            lastPulse += pulseDelay;
        }
    }
}