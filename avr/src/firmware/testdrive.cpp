#include "system/avr_serial.h"
#include "system/avr_timer.h"

#include "base/pid.h"
#include "base/encoder.h"
#include "base/odometry.h"
#include "base/wheel.h"

#include <avr/io.h>
#include <avr/interrupt.h>
#include <stdlib.h>
#include <math.h>

#define FF "%d.%03d"
#define FV(fv) (int)(fv), abs((int)(fv * 1000) % 1000)

void printDec(float f) {
    printf(FF, FV(f));
}

void init() {
    initTimer();
    initSerial(115200);

    initEncoders();
    initMotors();

    sei();
}

int main() {
    init();

    float cmdTurn = 0.2;
    float cmdDrive = 0.4;

    float targL = 0;
    float targR = 0;

    float velP = 0.8;
    float velI = 0.01;
    float velD = 0;
    float max = 0x7fff;
    float min = -1 * max;

    float velL = 0;
    float velR = 0;

    float poseX = 0;
    float poseY = 0;
    float poseA = 0;

    float velocity = 0;
    float turn = 0;
    
    int16_t encA = 0;
    int16_t encB = 0;

    int16_t outL = 0;
    int16_t outR = 0;

    float width = 0.17596;
    float tpu = 15498.331;

    PID pidL (&velP, &velI, &velD, &min, &max, &velL);
    PID pidR (&velP, &velI, &velD, &min, &max, &velR);

    Odometry odom (width, tpu);

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
            getEncoders(encB, encA);
            
            odom.update(encA, encB, dT);
            odom.getPose(&poseX, &poseY, &poseA);
            odom.getVelocity(&velocity, &turn);
            odom.getWheelVel(&velL, &velR);
        }

        if (curMillis - lastOut >= outPeriod) {
            lastOut += outPeriod;

            float turn = 0.5 * width * cmdTurn;
            targL = cmdDrive - turn;
            targR = cmdDrive + turn;

            outL = (int16_t)pidL.run(targL, outDT);
            outR = (int16_t)pidR.run(targR, outDT);
        
            wheelL.drive(outL);
            wheelR.drive(outR);

            printf(FF" "FF" %d %d\n", FV(cmdDrive), FV(cmdTurn), outL, outR);
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