#ifndef MOTOR_H_
#define MOTOR_H_

#include <stdint.h>

#define MOTOR_MAX 0x7fff

void initMotors();

class Wheel {
public:
    Wheel(volatile uint8_t *port1, volatile uint8_t *port2, uint8_t pin1,
        uint8_t pin2, volatile uint16_t *pwm, volatile uint8_t *ddr1,
        volatile uint8_t *ddr2);
    
    void drive(int16_t val);
    void coast();
    void brake();
    
private:
    volatile uint8_t *_port1, *_port2;
    volatile uint16_t *_pwm;
    uint8_t _pin1, _pin2;    
};

extern Wheel wheelL;
extern Wheel wheelR;  

#endif //MOTOR_H_
