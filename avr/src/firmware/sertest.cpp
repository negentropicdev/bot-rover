#include "system/avr_serial.h"
#include "system/avr_timer.h"
//#include "system/avr_i2c.h"

#include <avr/io.h>
#include <avr/interrupt.h>

/*volatile uint8_t data[3] = {0, 0, 0};

bool i2c_read_cb(uint8_t reg, uint8_t& value) {
    value = data[0] + data[1];
    return false;
}

bool i2c_write_cb(uint8_t reg, uint8_t value) {
    data[reg] = value;

    return reg < 1;
}*/

int main() {
    initSerial(57600);
    initTimer();

    sei();

    printf("Serial initialized.\n");

    unsigned long pulseDelay = 1000;
    unsigned long curMillis = millis();
    unsigned long lastPulse = curMillis;
    unsigned long delta;

    uint8_t n = 0;

    while(1) {
        curMillis = millis();
        if (curMillis - lastPulse >= pulseDelay) {
            printf("%d", n);
            n = (n + 1) % 2;
            lastPulse += pulseDelay;
        }
    }
}