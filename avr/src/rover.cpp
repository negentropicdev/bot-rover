#include "system/avr_serial.h"
#include "system/avr_i2c.h"
#include "system/avr_timer.h"

#include "base/pid.h"
#include "base/encoder.h"
#include "base/odometry.h"

#include <avr/io.h>
#include <avr/interrupt.h>

#define DEBUG
#define DEBUG_I2C

#define FF "%d.%d"
#define FV(fv) (int)(fv), ((int)(fv * 1000) % 1000)

void printDec(float f) {
    printf(FF, FV(f));
}

typedef struct {        // i - Register(s)
    int16_t encA;       // 0 - 0, 1
    int16_t encB;       // 1 - 2, 3
    float ticksPerUnit; // 2 - 4, 5, 6, 7
    float width;        // 3 - 8, 9, 10, 11
    uint8_t flags;      // 4 - 12
    float velP;         // 5 - 13, 14, 15, 16
    float velI;         // 6 - 17, 18, 19, 20
    float velD;         // 7 - 21, 22, 23, 24
    float velMin;       // 8 - 25, 26, 27, 28
    float velMax;       // 9 - 29, 30, 31, 32
    float poseX;        //10 - 33, 34, 35, 36
    float poseY;        //11 - 37, 38, 39, 40
    float poseA;        //12 - 41, 42, 43, 44
    float cmdDrive;     //13 - 45, 46, 47, 48
    float cmdTurn;      //14 - 49, 50, 51, 52
    float velocity;     //15 - 53, 54, 55, 56
    float turn;         //16 - 57, 58, 59, 60
    float velL;         //17 - 61, 62, 63, 64
    float velR;         //18 - 65, 66, 67, 68
    uint8_t run;        //19 - 69
} DataReg;

DataReg data;

uint8_t maxReg = sizeof(DataReg) - 1;
uint8_t *dataReg = (uint8_t*)&data;
uint8_t dataOffsets[20] = {0, 2, 4, 8, 12, 13, 17, 21, 25, 29, 33, 37, 41, 45, 49, 53, 57, 61, 65, 69};
uint8_t dataLen[20] =     {2, 2, 4, 4, 1,  4,  4,  4,  4,  4,  4,  4,  4,  4,  4,  4,  4,  4,  4, 1};
uint8_t dataI[70] = {
    0, 0,
    1, 1,
    2, 2, 2, 2,
    3, 3, 3, 3,
    4,
    5, 5, 5, 5,
    6, 6, 6, 6,
    7, 7, 7, 7,
    8, 8, 8, 8,
    9, 9, 9, 9,
    10, 10, 10, 10,
    11, 11, 11, 11,
    12, 12, 12, 12,
    13, 13, 13, 13,
    14, 14, 14, 14,
    15, 15, 15, 15,
    16, 16, 16, 16,
    17, 17, 17, 17,
    18, 18, 18, 18,
    19
};

uint8_t lastDataI = 255;

uint8_t i2c_buffer[4] = {0, 0, 0, 0};
uint8_t i2c_buffer_off = 0;
uint8_t i2c_buffer_i = 0;

volatile bool dataLock = false;

bool i2c_read_cb(uint8_t reg) {

    uint8_t regI = dataI[reg];

    #ifdef DEBUG_I2C
        printf("r %d i:%d", reg, regI);
    #endif

    if (regI != lastDataI) {
        i2c_buffer_off = dataOffsets[regI];

        for (uint8_t i = 0; i < dataLen[regI]; ++i) {
            i2c_buffer[dataLen[regI] - i - 1] = dataReg[i2c_buffer_off + i];
        }

        lastDataI = regI;
    }

    uint8_t di = reg - i2c_buffer_off;

    #ifdef DEBUG_I2C
        printf(" d:%d", di);
    #endif

    TWDR = dataReg[di];

    #ifdef DEBUG_I2C
        putchar('\n');
    #endif

    return reg < maxReg;
}

bool i2c_write_cb(uint8_t reg) {
    uint8_t regI = dataI[reg];
    #ifdef DEBUG_I2C
        printf("w %d i:%d", reg, regI);
    #endif

    if (regI != lastDataI) {
        i2c_buffer_off = dataOffsets[regI];
        i2c_buffer_i = 0;

        for (uint8_t i = 0; i < dataLen[regI]; ++i) {
            i2c_buffer[i] = dataReg[i2c_buffer_off + i];
        }

        lastDataI = regI;
    }

    i2c_buffer[i2c_buffer_i++] = TWDR;
    if (i2c_buffer_i == dataLen[regI]) {
        for (uint8_t i = 0; i < dataLen[regI]; ++i) {
            dataReg[i2c_buffer_off + i] = i2c_buffer[i];
        }

        #ifdef DEBUG_I2C
            putchar('\n');
            switch (regI) {
            case 2:
                printDec(data.ticksPerUnit);
                break;
            case 3:
                printDec(data.width);
                break;
            }
            putchar('\n');
        #endif
    }

    #ifdef DEBUG_I2C
        putchar('\n');
    #endif

    return reg < maxReg;
}

void init() {
    initTimer();
    initEncoders();
    initSerial(57600);

    I2C.initSlave(0x40, i2c_read_cb, i2c_write_cb, false);
    I2C.begin();

    sei();
}

int main() {
    init();

    Odometry odom;

    unsigned long odomPeriod = 10;
    unsigned long curMillis = millis();
    unsigned long lastOdom = curMillis;

    float dT = odomPeriod / 1000.0;

    #ifdef DEBUG
        unsigned long statusPeriod = 500;
        unsigned long lastStatus = curMillis;
    #endif

    while(1) {
        curMillis = millis();
        if (curMillis - lastOdom >= odomPeriod) {
            getEncoders(data.encA, data.encB);
            
            odom.update(data.encA, data.encB, dT);
            odom.getPose(&data.poseX, &data.poseY, &data.poseA);
            odom.getVelocity(&data.velocity, &data.turn);
            odom.getWheelVel(&data.velL, &data.velR);
        }

        #ifdef DEBUG
            if (curMillis - lastStatus >= statusPeriod) {
                lastStatus = curMillis; //Don't need this to be exactly on period
                printf("A:%d B:%d\n", data.encA, data.encB);
                printf("X:"FF" Y:"FF" A:"FF"\n", FV(data.poseX), FV(data.poseY), FV(data.poseA));

                putchar('\n');
            }
        #endif
    }
}