#include "pid.h"

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
        putchar(r ? 'r' : ' ');
    #endif
    
    //Calc P
    bool inRange = true;

    float normalizedP = error / _Rp;
    if (normalizedP > 1) {
        normalizedP = 1;
        inRange = false;
    } else if (normalizedP < -1) {
        normalizedP = -1;
        inRange = false;
    }

    #ifdef DEBUG_PID
        printf(" n:");
        printDec(normalizedP);
    #endif

    _outP = normalizedP * _max;

    float output = _outP;

    #ifdef DEBUG_PID
        printf(" P:");
        printDec(_outP);
    #endif

    //Calc I
    if (_Ti != 0 && !r && inRange) {
        float i = (_dT / (_Ti)) * (_Rp) * error;
        _sum += i;

        float maxSum = _max - _outP;
        float minSum = _min - _outP;

        if (_sum > maxSum) _sum = maxSum;
        else if (_sum < minSum) _sum = minSum;

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
