#include "odometry.h"
#include <math.h>

template <typename T> bool isNeg(const T &val) {
    return (val < T(0));
}

template <typename T> int sgn(T val) {
    return (T(0) < val) - (val < T(0));
}

void normalizeRadians(float *heading) {
    while (*heading < -PI) *heading += TWO_PI;
    while (*heading > PI) *heading -= TWO_PI;
}

Odometry::Odometry() {
    _lastEncL = 0;
    _lastEncR = 0;
}

void Odometry::update(const long &encL, const long &encR, const float &dT) {
    long diffL = encL - _lastEncL;
    long diffR = encR - _lastEncR;
    
    _lastEncL = encL;
    _lastEncR = encR;
    
    float distL = diffL / _ticksPerUnit;
    float distR = diffR / _ticksPerUnit;
    
    _velL = distL / dT;
    _velR = distR / dT;
    
    float forward = (distL + distR) / 2.0;
    _a += (distR - distL) / _width;
    if (_a > PI) {
        _a -= TWO_PI;
    } else if (_a < -PI) {
        _a += TWO_PI;
    }
    
    _x += cos(_a) * forward;
    _y += sin(_a) * forward;
}

void Odometry::setWidth(const float &width) {
    _width = width;
}

void Odometry::setTicksPerUnit(const float &tpu) {
    _ticksPerUnit = tpu;
}

const void Odometry::getVelocity(float *velocity, float *turn) {
    *velocity = _vel;
    *turn = _turn;
}

const void Odometry::getWheelVel(float *left, float *right) {
    *left = _velL;
    *right = _velR;
}

const void Odometry::getPose(float *x, float *y, float *a) {
    *x = _x;
    *y = _y;
    *a = _a;
}