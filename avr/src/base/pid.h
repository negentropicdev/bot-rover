#ifndef PID_H_
#define PID_H_

class PID {
public:
    PID(float Rp, float Ti, float Td, float min, float max, float dT);
    
    void setGains(float Rp, float Ti, float Td);
    void setRange(float min, float max);
    
    float run(const float &pv, const float &setpoint, const bool reset, char l);

    void setP(float p);
    void setI(float i);
    void setD(float d);
    
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

    float _dT;
};

#endif //PID_H_
