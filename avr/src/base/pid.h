#ifndef PID_H_
#define PID_H_

class PID {
public:
    PID();
    PID(volatile float *Rp, volatile float *Ti, volatile float *Td, volatile float *min, volatile float *max, volatile float *pv);
    
    void setGains(volatile float *Rp, volatile float *Ti, volatile float *Td);
    void setRange(volatile float *min, volatile float *max);
    
    float run(const float &setpoint, const float &dT, const bool reset = false);
    
    void reset();
    
private:
    volatile float *_Rp;
    volatile float *_Ti;
    volatile float *_Td;
    
    volatile float *_min;
    volatile float *_max;

    volatile float *_pv;
    
    float _sum;

    float _last;
    
    bool _reset;

    float _outP;
    float _outI;
    float _outD;
};

#endif //PID_H_
