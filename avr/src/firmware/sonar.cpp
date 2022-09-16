#include "system/avr_i2c_registers.h"
#include "system/avr_timer.h"

#include "device/dht.h"
#include "device/hcsr04.h"

#include <avr/io.h>
#include <avr/interrupt.h>

#define SONAR(port, bit) {&PORT ## port, &DDR ## port, &PIN ## port, bit}

SonarDef sd[6] = {
    SONAR(D, 3),
    SONAR(D, 4),
    SONAR(D, 5),
    SONAR(D, 6),
    SONAR(D, 7),
    SONAR(B, 0),
};

#define REG_COUNT 22
I2CRegister registers[REG_COUNT] = {
    REG_U8(54),  //Config
    REG_U8(0),   //RunMode
    REG_U8(60),  //SonarPeriod mS
    REG_U8(30),  //DHTPeriodCfg (val * 100 + 2000)
    REG_I16(200),//Temp * 10
    REG_I16(500),//Humidity * 10
    REG_I16(0),  //Sonar 1
    REG_I16(0),  //Sonar 2
    REG_I16(0),  //Sonar 3
    REG_I16(0),  //Sonar 4
    REG_I16(0),  //Sonar 5
    REG_I16(0),  //Sonar 6
    REG_I16(0),  //Sonar 7
    REG_I16(0),  //Sonar 8
    REG_I16(0),  //Sonar 9
    REG_I16(0),  //Sonar 10
    REG_I16(0),  //Sonar 11
    REG_I16(0),  //Sonar 12
    REG_I16(0),  //Sonar 13
    REG_I16(0),  //Sonar 14
    REG_I16(0),  //Sonar 15
    REG_I16(0),  //Sonar 16
};

#define RUN_RANGE 1
#define RUN_DHT   2

uint8_t* config;
uint8_t* runMode;
uint8_t* sonarPeriod;
uint8_t* dhtPeriodCfg;
int16_t* temp;
int16_t* humidity;
int16_t* range[16];
float _temp;
float _humidity;
float _range[16];

void init() {
    initTimer();

    I2C_Reg.init(0x41, registers, REG_COUNT, false);

    config = REG_U8_PTR(registers, 0);
    runMode = REG_U8_PTR(registers, 1);
    sonarPeriod = REG_U8_PTR(registers, 2);
    dhtPeriodCfg = REG_U8_PTR(registers, 3);
    temp = REG_I16_PTR(registers, 4);
    humidity = REG_I16_PTR(registers, 5);
    
    for (uint8_t i = 0; i < 16; ++i) {
        uint8_t r = 6 + i;
        range[i] = REG_I16_PTR(registers, r);
    }

    sei();
}

int main() {
    init();

    DHT dht(&PORTD, &DDRD, &PIND, 2);

    unsigned long echo_us[8] = {0, 0, 0, 0, 0, 0, 0, 0};

    HCSR04 sonars[6] = {
        HCSR04(sd[0]),
        HCSR04(sd[1]),
        HCSR04(sd[2]),
        HCSR04(sd[3]),
        HCSR04(sd[4]),
        HCSR04(sd[5]),
    };

    unsigned long curMillis = millis();

    unsigned long dhtPeriod = *dhtPeriodCfg * 100 + 2000;
    unsigned long lastDht = curMillis;

    unsigned long rangePeriod = *sonarPeriod;
    unsigned long lastRange = curMillis - 5;

    float scale = 0.017207; // Speed of sound (cm/uS) at 20C and 50% hum

    uint8_t rangeI = 0;
    uint8_t numSonar = 6;

    bool checkDHTPeriod = true;

    while(1) {
        curMillis = millis();

        if (*runMode & RUN_DHT && checkDHTPeriod) {
            if (curMillis - lastDht >= dhtPeriod) {
                lastDht = curMillis;

                int8_t res = dht.read();

                if (res == DHT_OK) {
                    _temp = dht.getTemp();
                    _humidity = dht.getHumidity();

                    *temp = (int16_t)(_temp * 10);
                    *humidity = (int16_t)(_humidity * 10);
                    
                    scale = (331.4 + (0.606 * _temp) + (0.0124 * _humidity)) / 20000.0;
                }
            }
        }

        if (*runMode & RUN_RANGE) {
            if (curMillis - lastRange >= rangePeriod) {
                lastRange += rangePeriod;

                echo_us[rangeI] = sonars[rangeI].range();

                _range[rangeI] = echo_us[rangeI] * scale * 2.375;
                if (echo_us[rangeI] == 1) {
                    *range[rangeI] = 0;
                } else if (echo_us[rangeI] == 0) {
                    *range[rangeI] = -1;
                } else {
                    *range[rangeI] = (int16_t)(_range[rangeI] * 10);
                }

                if (++rangeI == numSonar) {
                    rangeI = 0;
                    checkDHTPeriod = true;
                }
            }
        } else {
            checkDHTPeriod = true;
        }

        if (I2C_Reg.updated(2)) {
            rangePeriod = *sonarPeriod;
        }

        if (I2C_Reg.updated(3)) {
            dhtPeriod = *dhtPeriodCfg * 100 + 2000;
        }

        if (I2C_Reg.updated(1)) {
            if (((*runMode) & RUN_RANGE) == 0) {
                for (uint8_t i = 0; i < 16; ++i) {
                    _range[i] = -1;
                    *range[i] = -10;
                }
            }
        }
    }
}