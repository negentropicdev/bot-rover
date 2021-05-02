#include "../system/avr_serial.h"
#include "../system/avr_timer.h"

#include "../base/encoder.h"

#include <avr/io.h>
#include <avr/interrupt.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>

#define DEBUG
//#define DEBUG_I2C

#define FF "%c%d.%03d"
#define FV(fv) (fv < 0 ? '-' : ' '), abs((int)(fv)), abs((int)(fv * 1000) % 1000)


void printDec(float f) {
    printf(FF, FV(f));
}

void init() {
    initTimer();
    initSerial(115200);

    initEncoders();

    sei();
}

int main() {
    init();

    unsigned long odomPeriod = 10;
    unsigned long curMillis = millis();
    unsigned long lastOdom = curMillis;

    unsigned long outPeriod = 50;
    unsigned long lastOut = curMillis - 5;
    float dT = odomPeriod / 1000.0;
    float outDT = outPeriod / 1000.0;

    #ifdef DEBUG
        unsigned long statusPeriod = 500;
        unsigned long lastStatus = curMillis;
    #endif

    bool proc;

    volatile int16_t encA, encB;

    while(1) {
        proc = false;
        curMillis = millis();
        if (curMillis - lastOdom >= odomPeriod) {
            lastOdom += odomPeriod;
            proc = true;

            //with current wiring A is right and B is left
            getEncoders(encA, encB);
        }

        #ifdef DEBUG
            if (curMillis - lastStatus >= statusPeriod) {
                lastStatus = curMillis; //Don't need this to be exactly on period
                //printf("tV:"FF" tT:"FF"\n", FV(cmdDrive), FV(cmdTurn));
                //printf("oL:%d oR:%d\n", outL, outR);
                printf("%d %d\n", encA, encB);
            }
        #endif
    }
}