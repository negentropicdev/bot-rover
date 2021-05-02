#include "../system/avr_serial.h"
#include "../system/avr_i2c.h"
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

Odometry odom (0.15, 825.0);

void printDec(float f) {
    printf(FF, FV(f));
}

//Flag bit positions
#define F_RESET 0
#define F_RUN   1
#define F_BRAKE 2
#define F_COAST 3

#define TFLAG(flag) (flags & (1 << flag))

#define R_ENCA          0
#define R_ENCB          1
#define R_TICKSPERUNIT  2
#define R_WIDTH         3
#define R_FLAGS         4
#define R_VELP          5
#define R_VELI          6
#define R_VELD          7
#define R_OUTMIN        8
#define R_OUTMAX        9
#define R_POSEX         10
#define R_POSEY         11
#define R_POSEA         12
#define R_CMDDRIVE      13
#define R_CMDTURN       14
#define R_VELOCITY      15
#define R_TURN          16
#define R_VELL          17
#define R_VELR          18
#define R_RUN           19
#define R_OUTL          20
#define R_OUTR          21
#define R_OUTDEADBAND   22

volatile int16_t encA;       // 0 - 0, 1
volatile int16_t encB;       // 1 - 2, 3
volatile float ticksPerUnit; // 2 - 4, 5, 6, 7
volatile float width;        // 3 - 8, 9, 10, 11
volatile uint8_t flags;      // 4 - 12
volatile float velP;         // 5 - 13, 14, 15, 16
volatile float velI;         // 6 - 17, 18, 19, 20
volatile float velD;         // 7 - 21, 22, 23, 24
volatile float outMin;       // 8 - 25, 26, 27, 28
volatile float outMax;       // 9 - 29, 30, 31, 32
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
volatile float velF;         //23 - 76, 77, 78, 79

#define R_F32 1
#define R_I16 2
#define R_U8  3

#define R_COUNT 24
uint8_t dataLen[R_COUNT] =     {2, 2, 4, 4, 1, 4, 4, 4,  4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 1, 2, 2, 2, 4};

uint8_t dataType[R_COUNT] = {
    R_I16,
    R_I16,
    R_F32,
    R_F32,
    R_U8,
    R_F32,
    R_F32,
    R_F32,
    R_F32,
    R_F32,
    R_F32,
    R_F32,
    R_F32,
    R_F32,
    R_F32,
    R_F32,
    R_F32,
    R_F32,
    R_F32,
    R_U8,
    R_I16,
    R_I16,
    R_I16,
    R_F32
};

volatile bool updateReg[R_COUNT] = {
    false,
    false,
    false,
    false,
    false,
    false,
    false,
    false,
    false,
    false,
    false,
    false,
    false,
    false,
    false,
    false,
    false,
    false,
    false,
    false,
    false,
    false,
    false,
    false
};

static volatile union {
    float val;
    uint8_t bytes[4];
} floatbuf;

static volatile union {
    int16_t val;
    uint8_t bytes[2];
} intbuf;

volatile uint8_t bytebuf;

volatile uint8_t i2c_buffer_i = 0;
volatile uint8_t i2c_buffer_len = 0;
volatile uint8_t i2c_type = 0;
volatile uint8_t i2c_reg = 0;

bool i2c_set_reg(uint8_t reg) {
    //Check register is in range
    if (reg >= R_COUNT) {
        return false;
    }

    i2c_reg = reg;

    i2c_buffer_i = 0;
    i2c_buffer_len = dataLen[reg];
    i2c_type = dataType[reg];

    //printf("S %d %d %d\n", reg, i2c_buffer_len, i2c_type);

    switch (reg) {
        case 0:
            intbuf.val = encA;
            break;

        case 1:
            intbuf.val = encB;
            break;

        case 2:
            floatbuf.val = ticksPerUnit;
            break;

        case 3:
            floatbuf.val = width;
            break;

        case 4:
            bytebuf = flags;
            break;
        
        case 5:
            floatbuf.val = velP;
            break;

        case 6:
            floatbuf.val = velI;
            break;
        
        case 7:
            floatbuf.val = velD;
            break;

        case 8:
            floatbuf.val = outMin;
            break;

        case 9:
            floatbuf.val = outMax;
            break;

        case 10:
            floatbuf.val = poseX;
            break;

        case 11:
            floatbuf.val = poseY;
            break;

        case 12:
            floatbuf.val = poseA;
            break;
        
        case 13:
            floatbuf.val = cmdDrive;
            break;

        case 14:
            floatbuf.val = cmdTurn;
            break;

        case 15:
            floatbuf.val = velocity;
            break;

        case 16:
            floatbuf.val = turn;
            break;

        case 17:
            floatbuf.val = velL;
            break;

        case 18:
            floatbuf.val = velR;
            break;

        case 19:
            bytebuf = run;
            break;

        case 20:
            intbuf.val = outL;
            break;

        case 21:
            intbuf.val = outR;
            break;
        
        case 22:
            intbuf.val = outDeadband;
            break;
        
        case 23:
            floatbuf.val = velF;
            break;
    }

    return true;
}

bool i2c_read_cb() {
    switch (i2c_type) {
        case R_F32:
            TWDR = floatbuf.bytes[i2c_buffer_i];
            break;

        case R_I16:
            TWDR = intbuf.bytes[i2c_buffer_i];
            break;

        case R_U8:
            TWDR = bytebuf;
            break;
    }

    //printf("%02x ", TWDR);
    
    ++i2c_buffer_i;
    if (i2c_buffer_i == i2c_buffer_len && i2c_reg < R_COUNT) {
        //printf("%d", i2c_buffer_i);
        i2c_set_reg(i2c_reg + 1);
    }

    //putchar('\n');
    
    return i2c_reg < R_COUNT;
}

bool i2c_write_cb() {
    switch (i2c_type) {
        case R_F32:
            floatbuf.bytes[i2c_buffer_i] = TWDR;
            break;

        case R_I16:
            intbuf.bytes[i2c_buffer_i] = TWDR;
            break;

        case R_U8:
            bytebuf = TWDR;
            break;
    }

    ++i2c_buffer_i;

    if (i2c_buffer_i == i2c_buffer_len) {
        switch (i2c_reg) {
            case 0:
                encA = intbuf.val;
                break;

            case 1:
                encB = intbuf.val;
                break;

            case 2:
                ticksPerUnit = floatbuf.val;
                break;

            case 3:
                width = floatbuf.val;
                break;

            case 4:
                flags = bytebuf;
                break;
            
            case 5:
                velP = floatbuf.val;
                break;

            case 6:
                velI = floatbuf.val;
                break;
            
            case 7:
                velD = floatbuf.val;
                break;

            case 8:
                outMin = floatbuf.val;
                break;

            case 9:
                outMax = floatbuf.val;
                break;

            case 10:
                poseX = floatbuf.val;
                break;

            case 11:
                poseY = floatbuf.val;
                break;

            case 12:
                poseA = floatbuf.val;
                break;
            
            case 13:
                cmdDrive = floatbuf.val;
                break;

            case 14:
                cmdTurn = floatbuf.val;
                break;

            case 15:
                velocity = floatbuf.val;
                break;

            case 16:
                turn = floatbuf.val;
                break;

            case 17:
                velL = floatbuf.val;
                break;

            case 18:
                velR = floatbuf.val;
                break;

            case 19:
                run = bytebuf;
                break;

            case 20:
                outL = intbuf.val;
                break;

            case 21:
                outR = intbuf.val;
                break;
            
            case 22:
                outDeadband = intbuf.val;
                break;

            case 23:
                velF = floatbuf.val;
                break;
        }

        i2c_set_reg(i2c_reg + 1);
    }

    return i2c_reg < R_COUNT;
}

void init() {
    initTimer();
    initSerial(115200);

    initEncoders();
    initMotors();

    I2C.initSlave(0x40, i2c_set_reg, i2c_read_cb, i2c_write_cb, false);
    I2C.begin();

    width = 0.17596;
    ticksPerUnit = 15498.331;
    outMin = -1 * 0x7fff;
    outMax = 0x7fff;
    outDeadband = 0x2000;

    velF = 40000;
    velP = 0.6;
    velI = 0.00001;
    velD = 0.0007;

    poseX = 0;
    poseY = 0;
    poseA = 0;
    cmdDrive = 0;
    cmdTurn = 0;

    sei();
}

int main() {
    init();

    float targL = 0;
    float targR = 0;

    float leftVel = 0;
    float rightVel = 0;

    unsigned long odomPeriod = 100;
    unsigned long curMillis = millis();
    unsigned long lastOdom = curMillis;

    unsigned long outPeriod = 100;
    unsigned long lastOut = curMillis - 5;
    float dT = odomPeriod / 1000.0;
    float outDT = outPeriod / 1000.0;

    PID pidL (velP, velI, velD, outMin, outMax, outDT);
    pidL.setF(velF);

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

            getEncoders(encA, encB);
            
            odom.update(encA, encB, dT);
            odom.getPose(&poseX, &poseY, &poseA);
            odom.getVelocity(&velocity, &turn);
            odom.getWheelVel(&velL, &velR);
        }

        if (curMillis - lastOut >= outPeriod) {
            lastOut += outPeriod;
            proc = true;

            bool reset = TFLAG(F_RESET);
            bool run =   TFLAG(F_RUN);
            bool brake = TFLAG(F_BRAKE);
            bool coast = TFLAG(F_COAST);

            if (run) {
                if (!(brake || coast)) {
                    float turn = 0.5 * width * cmdTurn;
                    targL = cmdDrive - turn;
                    targR = cmdDrive + turn;
                }
            } else {
                targL = 0;
                targR = 0;
            }

            reset = reset || !run;

            leftVel = velL;
            rightVel = velR;

            outL = (int16_t)pidL.run(leftVel, targL, reset, 'L');
            outR = (int16_t)pidR.run(rightVel, targR, reset, 'R');

            if (run) {
                wheelL.drive(outL);
                wheelR.drive(outR);
            } else if (brake) {
                wheelL.brake();
                wheelR.brake();
            } else {
                wheelL.coast();
                wheelR.coast();
            }
            //printf(FF" "FF" "FF" "FF" "FF" "FF" %d %d %02x\n", FV(cmdDrive), FV(cmdTurn), FV(targL), FV(targR), FV(velL), FV(velR), outL, outR, flags);
            //printf(FF" "FF" "FF" "FF"\n", FV(velL), FV(velR), outL, outR);
        }

        if (!proc) {
            if (updateReg[R_VELP]) {
                //putchar('P');
                updateReg[R_VELP] = false;
                pidL.setP(velP);
                pidR.setP(velP);
            }

            if (updateReg[R_VELI]) {
                //putchar('I');
                updateReg[R_VELI] = false;
                pidL.setI(velI);
                pidR.setI(velI);
            }
            
            if (updateReg[R_VELD]) {
                //putchar('D');
                updateReg[R_VELD] = false;
                pidL.setD(velD);
                pidR.setD(velD);
            }
        }

        /*#ifdef DEBUG
            if (curMillis - lastStatus >= statusPeriod) {
                lastStatus = curMillis; //Don't need this to be exactly on period
                printf("tV:"FF" tT:"FF"\n", FV(cmdDrive), FV(cmdTurn));
                printf("oL:%d oR:%d\n", outL, outR);

                printf("%d %d\n", encA, encB);
            }
        #endif*/
    }
}