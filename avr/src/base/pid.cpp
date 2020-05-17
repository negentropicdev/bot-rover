#include "pid.h"

#include <stdlib.h>

PID::PID() {
    setGains(0.0, 0.0, 0.0);
    _reset = true;
    _min = 0.0;
    _max = 100.0;
}

PID::PID(const float &Rp, const float &Ti, const float &Td) {
    setGains(Rp, Ti, Td);
    _reset = true;
    _min = 0.0;
    _max = 100.0;
}

void PID::setGains(const float &Rp, const float &Ti, const float &Td) {
    _Rp = Rp;
    _Ti = Ti;
    _Td = Td;
}

void PID::setRange(const float &min, const float &max) {
    _min = min;
    _max = max;
}

float PID::run(const float &value, const float &setpoint, const float dT, const bool reset) {
    float error = setpoint - value;

    bool r = reset || _reset;
    _reset = false;
    
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

    if (_max > abs(_min)) {
        _outP = normalizedP * _max;
    } else {
        _outP = normalizedP * _min;
    }

    float output = _outP;

    //Calc I
    if (_Ti != 0 && !r && inRange) {
        float i = dT / _Ti * _Rp * error;
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

    //Calc D
    if (_Td != 0 && inRange) {
        _outD = (_last - value) / (dT * _Td);
        output += _outD;
    } else {
        _outD = 0;
    }
    
    _last = value;
    
    //clamp
    if (output > _max) output = _max;
    else if (output < _min) output = _min;

    return output;
}

void PID::reset() {
    _reset = true;
}
