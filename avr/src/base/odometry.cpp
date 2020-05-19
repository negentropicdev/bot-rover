#include "odometry.h"

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#define FF "%c%d.%03d"
#define FV(fv) (fv < 0 ? '-' : ' '), abs((int)(fv)), abs((int)(fv * 1000) % 1000)

template <typename T> bool isNeg(const T &val) {
    return (val < T(0));
}

template <typename T> int sgn(T val) {
    return (T(0) < val) - (val < T(0));
}

void normalizeRadians(float *heading) {
    while (*heading < -M_PI) *heading += 2 * M_PI;
    while (*heading > M_PI) *heading -= 2 * M_PI;
}

Odometry::Odometry(const float &width, const float &tpu) {
    _width = width;
    _ticksPerUnit = tpu;
    _lastEncL = 0;
    _lastEncR = 0;
    _x = 0;
    _y = 0;
    _a = 0;
}

void Odometry::setWidth(float width) {
    _width = width;
}

void Odometry::setTicksPerUnit(float tpu) {
    _ticksPerUnit = tpu;
}

void Odometry::update(const volatile int16_t &encL, const volatile int16_t &encR, const float &dT) {
    int16_t diffL = encL - _lastEncL;
    int16_t diffR = encR - _lastEncR;
    
    _lastEncL = encL;
    _lastEncR = encR;
    
    float distL = diffL / _ticksPerUnit;
    float distR = diffR / _ticksPerUnit;
    
    _velL = distL / dT;
    _velR = distR / dT;
    
    float forward = (distL + distR) / 2.0;
    float da = (distR - distL) / _width;

    _a += da;

    normalizeRadians(&_a);
    
    //printf("x:"FF" y:"FF, FV(_x), FV(_y));
    _x += cos(_a) * forward;
    _y += sin(_a) * forward;
    //printf("  x:"FF" y:"FF"\n\n", FV(_x), FV(_y));

    bool fault = false;
    if (isnan(_x)) {
        fault = true;
        putchar('x');
    }
    
    if (isnan(_y)) {
        fault = true;
        putchar('y');
    }
    
    if (isnan(_a)) {
        fault = true;
        putchar('a');
    }
    
    if (fault) {
        printf("dL:"FF" dR:"FF" F:"FF" da:"FF" x:"FF" y:"FF" a:"FF"\n", FV(distL), FV(distR), FV(forward), FV(da), FV(_x), FV(_y), FV(_a));
        while(1);
    }
}

const void Odometry::getVelocity(volatile float *velocity, volatile float *turn) {
    *velocity = _vel;
    *turn = _turn;
}

const void Odometry::getWheelVel(volatile float *left, volatile float *right) {
    *left = _velL;
    *right = _velR;
}

const void Odometry::getPose(volatile float *x, volatile float *y, volatile float *a) {
    *x = _x;
    *y = _y;
    *a = _a;
}