#include "system/avr_serial.h"
#include "system/avr_i2c.h"
#include "system/avr_timer.h"

#include "base/pid.h"
#include "base/encoder.h"
#include "base/odometry.h"
#include "base/l298.h"

#include <avr/io.h>
#include <avr/interrupt.h>
#include <stdlib.h>
#include <math.h>

#define DEBUG
//#define DEBUG_I2C

#define FF "%d.%03d"
#define FV(fv) (int)(fv), abs((int)(fv * 1000) % 1000)

void printDec(float f) {
    printf(FF, FV(f));
}

typedef struct {                 // i - Register(s)
    volatile int16_t encA;       // 0 - 0, 1
    volatile int16_t encB;       // 1 - 2, 3
    volatile float ticksPerUnit; // 2 - 4, 5, 6, 7
    volatile float width;        // 3 - 8, 9, 10, 11
    volatile uint8_t flags;      // 4 - 12
    volatile float velP;         // 5 - 13, 14, 15, 16
    volatile float velI;         // 6 - 17, 18, 19, 20
    volatile float velD;         // 7 - 21, 22, 23, 24
    volatile float velMin;       // 8 - 25, 26, 27, 28
    volatile float velMax;       // 9 - 29, 30, 31, 32
    volatile float poseX;        //10 - 33, 34, 35, 36
    volatile float poseY;        //11 - 37, 38, 39, 40
    volatile float poseA;        //12 - 41, 42, 43, 44
    volatile float cmdDrive;     //13 - 45, 46, 47, 48
    volatile float cmdTurn;      //14 - 49, 50, 51, 52
    volatile float velocity;     //15 - 53, 54, 55, 56
    volatile float turn;         //16 - 57, 58, 59, 60
    volatile float velL;         //17 - 61, 62, 63, 64
    volatile float velR;         //18 - 65, 66, 67, 68
    volatile uint8_t run;        //19 - 69
    volatile int16_t outL;       //20 - 70, 71
    volatile int16_t outR;       //21 - 72, 73
    volatile int16_t outDeadband;//22 - 74, 75
} DataReg;

volatile DataReg data;

uint8_t maxReg = sizeof(DataReg) - 1;
uint8_t *dataReg = (uint8_t*)&data;
uint8_t dataOffsets[23] = {0, 2, 4, 8, 12, 13, 17, 21, 25, 29, 33, 37, 41, 45, 49, 53, 57, 61, 65, 69, 70, 72, 74};
uint8_t dataLen[23] =     {2, 2, 4, 4, 1,  4,  4,  4,  4,  4,  4,  4,  4,  4,  4,  4,  4,  4,  4, 1, 2, 2, 2};
uint8_t dataI[76] = {
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
    19,
    20, 20,
    21, 21,
    22, 22
};

//Flag bit positions
#define F_RESET 0
#define F_RUN   1
#define F_BRAKE 2
#define F_COAST 3

#define TFLAG(flag) (data.flags & (1 << flag))

uint8_t lastDataI = 255;

volatile uint8_t i2c_buffer[4] = {0, 0, 0, 0};
volatile uint8_t i2c_buffer_off = 0;
volatile uint8_t i2c_buffer_i = 0;

volatile bool dataLock = false;

Odometry odom (0.17596, 15498.331);

bool i2c_read_cb(uint8_t reg) {
    uint8_t regI = dataI[reg];

    if (regI != lastDataI) {
        i2c_buffer_off = dataOffsets[regI];

        for (uint8_t i = 0; i < dataLen[regI]; ++i) {
            i2c_buffer[i] = dataReg[i2c_buffer_off + i];
        }

        lastDataI = regI;
    }

    uint8_t di = reg - i2c_buffer_off;

    TWDR = i2c_buffer[di];

    return reg < maxReg;
}

bool i2c_write_cb(uint8_t reg) {
    uint8_t regI = dataI[reg];

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

        switch (regI) {
            case 2: //ticksPerUnit
                odom.setTicksPerUnit(*((float *)i2c_buffer));
                break;
            
            case 3: //width
                odom.setWidth(*((float *)i2c_buffer));
                break;
        }
    }

    return reg < maxReg;
}

void init() {
    initTimer();
    initEncoders();
    initSerial(57600);

    I2C.initSlave(0x40, i2c_read_cb, i2c_write_cb, false);
    I2C.begin();

    data.width = 0.17596;
    data.ticksPerUnit = 15498.331;
    data.velMin = -1 * 0x7fff;
    data.velMax = 0x7fff;
    data.outDeadband = 3000;

    sei();
}

int main() {
    init();

    float targL = 0;
    float targR = 0;

    PID pidL (&data.velP, &data.velI, &data.velD, &data.velMin, &data.velMax, &data.velL);
    PID pidR (&data.velP, &data.velI, &data.velD, &data.velMin, &data.velMax, &data.velR);

    unsigned long odomPeriod = 10;
    unsigned long curMillis = millis();
    unsigned long lastOdom = curMillis;

    unsigned long outPeriod = 100;
    unsigned long lastOut = curMillis;
    float outDT = outPeriod / 1000.0;

    float dT = odomPeriod / 1000.0;

    #ifdef DEBUG
        unsigned long statusPeriod = 500;
        unsigned long lastStatus = curMillis;
    #endif

    while(1) {
        curMillis = millis();
        if (curMillis - lastOdom >= odomPeriod) {
            lastOdom += odomPeriod;

            //with current wiring A is right and B is left
            getEncoders(data.encB, data.encA);
            
            odom.update(data.encA, data.encB, dT);
            odom.getPose(&data.poseX, &data.poseY, &data.poseA);
            odom.getVelocity(&data.velocity, &data.turn);
            odom.getWheelVel(&data.velL, &data.velR);
        }

        if (curMillis - lastOut >= outPeriod) {
            lastOut += outPeriod;

            bool reset = TFLAG(F_RESET);
            bool run = TFLAG(F_RUN);
            bool brake = TFLAG(F_BRAKE);
            bool coast = TFLAG(F_COAST);

            if (run) {
                if (!(brake || coast)) {
                    float turn = 0.5 * data.width * data.cmdTurn;
                    targL = data.cmdDrive - turn;
                    targR = data.cmdDrive + turn;

                    data.outL = (int16_t)pidL.run(targL, outDT, reset);
                    data.outR = (int16_t)pidR.run(targR, outDT, reset);
                }
            } else {
                data.outL = 0;
                data.outR = 0;
            }


        }

        #ifdef DEBUG
            if (curMillis - lastStatus >= statusPeriod) {
                lastStatus = curMillis; //Don't need this to be exactly on period
                //printf("tV:"FF" tT:"FF"\n", FV(data.cmdDrive), FV(data.cmdTurn));
                //printf("oL:%d oR:%d\n", data.outL, data.outR);
            }
        #endif
    }
}