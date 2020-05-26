#include "../system/avr_serial.h"
#include "../system/avr_i2c.h"
#include "../system/avr_timer.h"

#include "../device/dht.h"
#include "../device/hcsr04.h"

#include <avr/io.h>
#include <avr/interrupt.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>

#define DEBUG
//#define DEBUG_I2C

#define SONAR(port, bit) {&PORT ## port, &DDR ## port, &PIN ## port, bit}

SonarDef sd[8] = {
    SONAR(D, 3),
    SONAR(D, 4),
    SONAR(D, 5),
    SONAR(D, 6),
    SONAR(D, 7),
    SONAR(B, 0),
    SONAR(B, 1),
    SONAR(B, 2)
};

bool i2c_set_reg(uint8_t reg) {
        return true;
}

bool i2c_read_cb() {
    return false;
}

bool i2c_write_cb() {
    return false;
}

void init() {
    initTimer();
    initSerial(115200);

    I2C.initSlave(0x41, i2c_set_reg, i2c_read_cb, i2c_write_cb, false);
    I2C.begin();

    sei();
}

int main() {
    init();

    DHT dht(&PORTD, &DDRD, &PIND, 2);

    unsigned long echo_us[8];
    uint16_t range_cm[8];

    HCSR04 sonars[8] = {
        HCSR04(sd[0]),
        HCSR04(sd[1]),
        HCSR04(sd[2]),
        HCSR04(sd[3]),
        HCSR04(sd[4]),
        HCSR04(sd[5]),
        HCSR04(sd[6]),
        HCSR04(sd[7]),
    };

    unsigned long curMillis = millis();

    unsigned long dhtPeriod = 1000;
    unsigned long lastDht = curMillis;

    #ifdef DEBUG
        unsigned long statusPeriod = 500;
        unsigned long lastStatus = curMillis;
    #endif

    bool proc;

    while(1) {
        proc = false;
        curMillis = millis();

        if (curMillis - lastDht >= dhtPeriod) {
            lastDht += dhtPeriod;
            proc = true;

            dht.read();

            float t = dht.getTemp();
            float h = dht.getHumidity();

            //printDec(t);
            //printf("C  ");
            //printDec(h);
            //printf("%%\n");
            
            float scale = (331.4 + (0.606 * t) + (0.0124 * h)) / 20000.0;

            //printDec(scale);

            for (uint8_t i = 0; i < 1; ++i) {
                echo_us[i] = sonars[i].range();
            }

            for (uint8_t i = 0; i < 1; ++i) {
                range_cm[i] = echo_us[i] * scale;
                printf("%d: %d ", i, range_cm[i]);
            }
            printf("\n\n");
        }

        if (!proc) {
            //update from I2C volatiles
        }

        #ifdef DEBUG
            if (curMillis - lastStatus >= statusPeriod) {
                lastStatus = curMillis; //Don't need this to be exactly on period
            }
        #endif
    }
}