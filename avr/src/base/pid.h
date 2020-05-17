#ifndef PID_H_
#define PID_H_

class PID {
public:
    PID();
    PID(const float &Rp, const float &Ti, const float &Td);
    
    void setGains(const float &Rp, const float &Ti, const float &Td);
    void setRange(const float &min, const float &max);
    
    float run(const float &value, const float &setpoint, const float dT, const bool reset = false);
    
    void reset();
    
private:
    float _Rp;
    float _Ti;
    float _Td;
    
    float _min;
    float _max;
    
    float _sum;

    float _last;
    
    bool _reset;

    float _outP;
    float _outI;
    float _outD;
};

#endif //PID_H_
