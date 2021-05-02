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

I2CRegister registers[22] = {
    {U8, false, {.u8 = (6 + 16 + 32)}}, //Config
    {U8, false, {.u8 = 0}},             //RunMode
    {U8, false, {.u8 = 60}},            //SonarPeriod mS
    {U8, false, {.u8 = 30}},            //DHTPeriod (mS * 100 + 2000)
    {FLOAT, false, {.f32 = 0}},         //Temp
    {FLOAT, false, {.f32 = 0}},         //Humidity
    {FLOAT, false, {.f32 = 0}},         //Sonar 1
    {FLOAT, false, {.f32 = 0}},         //Sonar 2
    {FLOAT, false, {.f32 = 0}},         //Sonar 3
    {FLOAT, false, {.f32 = 0}},         //Sonar 4
    {FLOAT, false, {.f32 = 0}},         //Sonar 5
    {FLOAT, false, {.f32 = 0}},         //Sonar 6
    {FLOAT, false, {.f32 = 0}},         //Sonar 7
    {FLOAT, false, {.f32 = 0}},         //Sonar 8
    {FLOAT, false, {.f32 = 0}},         //Sonar 9
    {FLOAT, false, {.f32 = 0}},         //Sonar 10
    {FLOAT, false, {.f32 = 0}},         //Sonar 11
    {FLOAT, false, {.f32 = 0}},         //Sonar 12
    {FLOAT, false, {.f32 = 0}},         //Sonar 13
    {FLOAT, false, {.f32 = 0}},         //Sonar 14
    {FLOAT, false, {.f32 = 0}},         //Sonar 15
    {FLOAT, false, {.f32 = 0}},         //Sonar 16
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

    unsigned long echo_us[8] = {0, 0, 0, 0, 0, 0, 0, 0};
    float range_cm[8] = {0, 0, 0, 0, 0, 0, 0, 0};

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

    unsigned long dhtPeriod = 5000;
    unsigned long lastDht = curMillis;

    unsigned long rangePeriod = 60;
    unsigned long lastRange = curMillis - 5;

    #ifdef DEBUG
        unsigned long statusPeriod = 500;
        unsigned long lastStatus = curMillis;
    #endif
    
    float temp = 20;
    float humidity = 50;
    float scale = 0.017207; // Speed of sound (cm/uS) at 20C and 50% hum

    uint8_t rangeI = 0;
    uint8_t numSonar = 6;

    while(1) {
        curMillis = millis();

        if (curMillis - lastDht >= dhtPeriod) {
            lastDht += dhtPeriod;

            int8_t res = dht.read();

            if (res == DHT_OK) {
                temp = dht.getTemp();
                humidity = dht.getHumidity();
                
                scale = (331.4 + (0.606 * temp) + (0.0124 * humidity)) / 20000.0;
            }

            //printDec(t);
            //printf("C  ");
            //printDec(h);
            //printf("%%\n");

            //printDec(scale);
        }

        if (curMillis - lastRange >= rangePeriod) {
            lastRange += rangePeriod;

            echo_us[rangeI] = sonars[rangeI].range();
            range_cm[rangeI] = echo_us[rangeI] * scale * 2.375;
            printf("%d: ", rangeI);
            printDec(range_cm[rangeI]);
            putchar(' ');

            if (++rangeI == numSonar) {
                putchar('\n');
                rangeI = 0;
            }
        }
    }
}