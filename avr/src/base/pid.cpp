#include "base/pid.h"

#include <stdlib.h>
#include <math.h>

//#define DEBUG_PID

#ifdef DEBUG_PID
    #include <stdio.h>
    #include "../system/avr_serial.h"
#endif

PID::PID(float Rp, float Ti, float Td, float min, float max, float dT) {
    _Rp = Rp;
    _Ti = Ti;
    _Td = Td;
    _min = min;
    _max = max;
    _dT = dT;
    _reset = true;
    _Ff = 0;
}

float PID::run(const float &pv, const float &setpoint, const bool reset, char l) {

    #ifdef DEBUG_PID
        putchar(l);
        printf(" sp:");
        printDec(setpoint);
        printf(" pv:");
        printDec(pv);
    #endif

    float error = setpoint - pv;

    #ifdef DEBUG_PID
        printf(" e:");
        printDec(error);
    #endif

    bool r = reset || _reset;
    _reset = false;

    #ifdef DEBUG_PID
        putchar(' ');
        putchar(r ? 'r' : ' ');
    #endif

    _outF = setpoint * _Ff;
    float output = _outF;

    #ifdef DEBUG_PID
        printf(" F:");
        printDec(_outF);
    #endif
    
    //Calc P
    bool inRange = true;

    float limit = 1;//_max / _Rp;

    #ifdef DEBUG_PID
        printf(" l:");
        printDec(limit);
    #endif

    if (_Rp != 0) {
        float normalizedP = error / _Rp;
        if (normalizedP > limit) {
            normalizedP = limit;
            inRange = false;
        } else if (normalizedP < -limit) {
            normalizedP = -limit;
            inRange = false;
        }

        #ifdef DEBUG_PID
            printf(" n:");
            printDec(normalizedP);
        #endif

        _outP = normalizedP * _max;

        output += _outP;
    }

    #ifdef DEBUG_PID
        printf(" P:");
        printDec(_outP);
    #endif

    //Calc I
    if (_Ti != 0 && !r && inRange) {
        float i = (_dT / (_Ti)) * (_Rp) * error;
        _sum += i;

        if (_sum + output > _max) _sum = output - _max;
        else if (_sum + output < _min) _sum = output - _min;

        output += _sum;
    } else {
        _sum = 0;
    }

    _outI = _sum;

    #ifdef DEBUG_PID
        printf(" i:");
        printDec(_outI);
    #endif

    //Calc D
    if (_Td != 0 && inRange) {
        _outD = (_last - pv) / (_dT * _Td);
        output += _outD;
    } else {
        _outD = 0;
    }

    #ifdef DEBUG_PID
        printf(" d:");
        printDec(_outD);
        printf(" o:");
        printDec(output);
    #endif
    
    _last = pv;

    #ifdef DEBUG_PID
        putchar('\n');
    #endif
    
    //clamp
    if (output > _max) output = _max;
    else if (output < _min) output = _min;

    return output;
}

void PID::setF(float f) {
    _Ff = f;
}

void PID::setP(float p) {
    _Rp = p;
}

void PID::setI(float i) {
    _Ti = i;
}

void PID::setD(float d) {
    _Td = d;
}

void PID::reset() {
    _reset = true;
}

float PID::getOutF() {
    return _outF;
}

float PID::getOutP() {
    return _outP;
}

float PID::getOutI() {
    return _outI;
}

float PID::getOutD() {
    return _outD;
}
