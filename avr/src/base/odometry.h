#ifndef BASE_H_
#define BASE_H_

#include <stdint.h>
#include <math.h>

template <typename T> bool isNeg(const T &val);

template <typename T> int sgn(T val);

void normalizeRadians(float *heading);

//includes functions for velocity control and odometry

class Odometry {
public:
    Odometry(const float &width, const float &tpu);
    
    void update(const volatile int16_t &encL, const volatile int16_t &encR, const float &dT);
    
    const void getPose(volatile float *x, volatile float *y, volatile float *a);
    const void getVelocity(volatile float *velocity, volatile float *turn);
    const void getWheelVel(volatile float *left, volatile float *right);

    void setWidth(float width);
    void setTicksPerUnit(float tpu);
    
private:
    
    //odometry variables
    float _ticksPerUnit, _width;
    int16_t _lastEncL, _lastEncR;
    float _x, _y, _a;
    
    //measured values
    float _velL, _velR;
    float _vel, _turn;
};

#endif //Odometry_H_
