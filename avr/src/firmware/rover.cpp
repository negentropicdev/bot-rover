#include "system/avr_i2c_registers.h"
#include "system/avr_timer.h"

#include "base/wheel.h"

#include <avr/io.h>
#include <avr/interrupt.h>

//Flag bit positions
#define F_RESET 0
#define F_RUN   1
#define F_BRAKE 2
#define F_COAST 3

#define TFLAG(flag) (*flags & (1 << flag))

#define REG_COUNT 24
I2CRegister registers[REG_COUNT] = {
    REG_I16(0),            // 0 encA
    REG_I16(0),            // 1 encB
    REG_FLOAT(15498.331),  // 2 ticksPerUnit
    REG_FLOAT(0.17596),    // 3 width
    REG_U8(0),             // 4 flags
    REG_FLOAT(0),          // 5 velP 0.6
    REG_FLOAT(0),          // 6 velI 0.00001
    REG_FLOAT(0),          // 7 velD 0.0007
    REG_FLOAT(-1 * 0x7fff),// 8 outMin
    REG_FLOAT(0x7fff),     // 9 outMax
    REG_FLOAT(0),          //10 poseX
    REG_FLOAT(0),          //11 poseY
    REG_FLOAT(0),          //12 poseA
    REG_FLOAT(0),          //13 cmdVelocity
    REG_FLOAT(0),          //14 cmdTurn
    REG_FLOAT(0),          //15 velocity
    REG_FLOAT(0),          //16 turn
    REG_FLOAT(0),          //17 velL
    REG_FLOAT(0),          //18 velR
    REG_U8(0),             //19 run
    REG_I16(0),            //20 outL
    REG_I16(0),            //21 outR
    REG_I16(0x2000),       //22 outDeadband
    REG_FLOAT(40000)       //23 velF
};

float* outMax;
float* cmdVelocity;
float* cmdTurn;

int16_t* outDeadband;

int16_t* outL;
int16_t* outR;

uint8_t* flags;

void init() {
    initTimer();
    initMotors();

    I2C_Reg.init(0x40, registers, REG_COUNT, false);

    outMax = REG_FLOAT_PTR(registers, 9);
    outDeadband = REG_I16_PTR(registers, 22);

    cmdVelocity = REG_FLOAT_PTR(registers, 13);
    cmdTurn = REG_FLOAT_PTR(registers, 14);

    outL = REG_I16_PTR(registers, 20);
    outR = REG_I16_PTR(registers, 21);

    flags = REG_U8_PTR(registers, 4);

    sei();
}

int main() {
    init();

    float targL = 0;
    float targR = 0;

    unsigned long curMillis = millis();

    unsigned long outPeriod = 100;
    unsigned long lastOut = curMillis - 5;

    while(1) {
        curMillis = millis();

        if (curMillis - lastOut >= outPeriod) {
            lastOut += outPeriod;

            bool reset = TFLAG(F_RESET);
            bool run   = TFLAG(F_RUN);
            bool brake = TFLAG(F_BRAKE);
            bool coast = TFLAG(F_COAST);

            if (run) {
                if (!(brake || coast)) {
                    float targTurn = (*cmdTurn);
                    targL = *cmdVelocity - targTurn;
                    targR = *cmdVelocity + targTurn;

                    if (targL < -1) targL = -1;
                    else if (targL > 1) targL = 1;

                    if (targR < -1) targR = -1;
                    else if (targR > 1) targR = 1;
                }
            } else {
                targL = 0;
                targR = 0;
            }

            reset = reset || !run;

            int16_t _outL = (int16_t)(targL * (*outMax));
            int16_t _outR = (int16_t)(targR * (*outMax));

            int16_t db = *outDeadband;

            if (_outL > -db && _outL < db) _outL = 0;
            if (_outR > -db && _outR < db) _outR = 0;

            *outL = _outL;
            *outR = _outR;

            if (run) {
                wheelL.drive(_outL);
                wheelR.drive(_outR);
            } else if (brake) {
                wheelL.brake();
                wheelR.brake();
            } else {
                wheelL.coast();
                wheelR.coast();
            }
        }
    }
}