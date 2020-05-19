#include "pid.h"

#include <stdlib.h>
#include <math.h>

//#include <stdio.h>
#define FF "%c%d.%03d"
#define FV(fv) (fv < 0 ? '-' : ' '), abs((int)(fv)), abs((int)(fv * 1000) % 1000)

PID::PID(volatile float *Rp, volatile float *Ti, volatile float *Td, volatile float *min, volatile float *max, volatile float *pv) {
    _Rp = Rp;
    _Ti = Ti;
    _Td = Td;
    _min = min;
    _max = max;
    _pv = pv;
    _reset = true;
}

float PID::run(const float &setpoint, const float &dT, const bool reset, char l) {

    //putchar(l);
    //printf(" sp:"FF" pv:"FF, FV(setpoint), FV(*_pv));

    float error = setpoint - *_pv;

    //printf(" e:"FF, FV(error));

    bool r = reset || _reset;
    _reset = false;
    
    //Calc P
    bool inRange = true;

    float normalizedP = error / *_Rp;
    if (normalizedP > 1) {
        normalizedP = 1;
        inRange = false;
    } else if (normalizedP < -1) {
        normalizedP = -1;
        inRange = false;
    }

    //printf(" n:"FF, FV(normalizedP));

    _outP = normalizedP * *_max;

    float output = _outP;

    //printf(" P:"FF, FV(_outP));

    //Calc I
    if (*_Ti != 0 && !r && inRange) {
        float i = (dT / (*_Ti)) * (*_Rp) * error;
        _sum += i;

        float maxSum = *_max - _outP;
        float minSum = *_min - _outP;

        if (_sum > maxSum) _sum = maxSum;
        else if (_sum < minSum) _sum = minSum;

        output += _sum;
    } else {
        _sum = 0;
    }

    _outI = _sum;

    //printf(" i:"FF, FV(_outI));

    //Calc D
    if (*_Td != 0 && inRange) {
        _outD = (_last - *_pv) / (dT * *_Td);
        output += _outD;
    } else {
        _outD = 0;
    }

    //printf(" d:"FF" o:"FF, FV(_outD), FV(output));
    
    _last = *_pv;

    //putchar('\n');
    
    //clamp
    if (output > *_max) output = *_max;
    else if (output < *_min) output = *_min;

    return output;
}

void PID::reset() {
    _reset = true;
}
