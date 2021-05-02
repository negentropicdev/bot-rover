#include "../system/avr_serial.h"
#include "../system/avr_timer.h"

#include "../base/pid.h"
#include "../base/encoder.h"
#include "../base/odometry.h"
#include "../base/wheel.h"

#include <avr/io.h>
#include <avr/interrupt.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>

#define DEBUG
//#define DEBUG_I2C

#define FF "%c%d.%03d"
#define FV(fv) (fv < 0 ? '-' : ' '), abs((int)(fv)), abs((int)(fv * 1000) % 1000)

/*void printDec(float f) {
    printf(FF, FV(f));
}*/

Odometry odom (0.15, 825.0);

float velF, velP, velI, velD;
int16_t outMin, outMax;
int16_t outR;

int16_t encA, encB;

volatile float velL, velR;

float targL, targR;

float outF, outP, outI, outD;

const int filter_count = 5;

float vel_filtered[filter_count];
int cur_filter_i = 0;

void init() {
    initTimer();
    initSerial(115200);

    initEncoders();
    initMotors();

    for (int i = 0; i < filter_count; ++i) {
        vel_filtered[i] = 0;
    }

    velF = 40000;
    velP = 1.4;
    velI = 0.00001;
    velD = 0.0007;

    outMin = -1 * 0x7fff;
    outMax = 0x7fff;

    sei();
}

int main() {
    init();

    float targ = 0.2;

    float vel = 0;

    unsigned long odomPeriod = 100;
    unsigned long curMillis = millis();
    unsigned long lastOdom = curMillis;

    unsigned long outPeriod = 100;
    unsigned long lastOut = curMillis - 5;
    float dT = odomPeriod / 1000.0;
    float outDT = outPeriod / 1000.0;

    PID pidR (velP, velI, velD, outMin, outMax, outDT);
    pidR.setF(velF);

    #ifdef DEBUG
        unsigned long statusPeriod = 500;
        unsigned long lastStatus = curMillis;
    #endif

    bool proc;

    while(1) {
        proc = false;
        curMillis = millis();
        if (curMillis - lastOdom >= odomPeriod) {
            lastOdom += odomPeriod;
            proc = true;

            //with current wiring A is right and B is left
            getEncoders(encA, encB);
            
            odom.update(encA, encB, dT);
            odom.getWheelVel(&velL, &velR);
        }

        if (curMillis - lastOut >= outPeriod) {
            lastOut += outPeriod;
            proc = true;

            targR = targ;

            vel = velR;

            vel_filtered[cur_filter_i] = vel;
            cur_filter_i = ++cur_filter_i % filter_count;

            printf("(%d/%d)", cur_filter_i, filter_count);

            float vel_filt = 0;
            for (int i = 0; i < filter_count; ++i) {
                vel_filt += vel_filtered[i];
            }

            #ifdef DEBUG
                printf(" Vs:");
                printDec(vel_filt);
            #endif
            
            vel_filt /= filter_count;

            outR = (int16_t)pidR.run(vel_filt, targR, false, 'R');
            wheelR.drive(outR);
        }
    }
}