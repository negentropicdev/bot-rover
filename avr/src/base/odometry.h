#ifndef BASE_H_
#define BASE_H_

//includes functions for velocity control and odometry

//yummmm
#define PI 3.1415926536
#define TWO_PI (PI * 2.0)

#include "pid.h"

class Odometry {
public:
    Odometry();
    
    void setWidth(const float &width);
    void setTicksPerUnit(const float &tpu);
    
    void update(const long &encL, const long &encR, const float &dT);
    
    const void getPose(float *x, float *y, float *a);
    const void getVelocity(float *velocity, float *turn);
    const void getWheelVel(float *left, float *right);
    
private:
    
    //odometry variables
    float _ticksPerUnit, _width;
    long _lastEncL, _lastEncR;
    float _x, _y, _a;
    
    //measured values
    float _velL, _velR;
    float _vel, _turn;
};

#endif //Odometry_H_
