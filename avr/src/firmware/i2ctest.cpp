#include "system/avr_serial.h"
#include "system/avr_timer.h"
#include "system/avr_i2c.h"

#include <avr/io.h>
#include <avr/interrupt.h>

#define BIN "%c%c%c%c%c%c%c%c"
#define BYBIN(byte)  \
  (byte & 0x80 ? '1' : '0'), \
  (byte & 0x40 ? '1' : '0'), \
  (byte & 0x20 ? '1' : '0'), \
  (byte & 0x10 ? '1' : '0'), \
  (byte & 0x08 ? '1' : '0'), \
  (byte & 0x04 ? '1' : '0'), \
  (byte & 0x02 ? '1' : '0'), \
  (byte & 0x01 ? '1' : '0') 

volatile uint8_t data[3] = {0, 0, 0};

bool i2c_read_cb(uint8_t reg) {
    TWDR = data[0] + data[1];
    return false;
}

bool i2c_write_cb(uint8_t reg) {
    data[reg] = TWDR;

    return true;
}

void init() {
    initSerial(57600);

    printf("Serial initialized.\n");

    printf("Initializing timer...");
    initTimer();
    printf("Done.\n");

    printf("Initializing I2C...");
    I2C.initSlave(0x40, i2c_read_cb, i2c_write_cb, false);
    I2C.begin();
    printf("Done.\n");

    printf("Initializing LED...");
    DDRB = (1 << 5);
    PORTB = 0;
    printf("Done.\n");

    sei();
}

int main() {
    init();

    unsigned long curMillis = millis();

    unsigned long pulseDelay = 500;
    unsigned long lastPulse = curMillis;
    
    //unsigned long statusDelay = 2000;
    //unsigned long lastStatus = curMillis;

    while(1) {
        curMillis = millis();
        if (curMillis - lastPulse >= pulseDelay) {
            lastPulse += pulseDelay;
            PINB |= (1 << 5);
        }

        /*if (curMillis - lastStatus >= statusDelay) {
            lastStatus += statusDelay;
            printf("\nSR: %02x CR: "BIN"\n", (TWSR), BYBIN(TWCR));
        }*/
    }
}