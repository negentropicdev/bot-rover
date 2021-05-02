#ifndef PID_H_
#define PID_H_

class PID {
public:
    PID(float Rp, float Ti, float Td, float min, float max, float dT);
    
    void setGains(float Rp, float Ti, float Td, float Ff);
    void setRange(float min, float max);
    
    float run(const float &pv, const float &setpoint, const bool reset, char l);

    void setF(float f);
    void setP(float p);
    void setI(float i);
    void setD(float d);

    float getOutF();
    float getOutP();
    float getOutI();
    float getOutD();
    
    void reset();
    
private:
    float _Ff;
    float _Rp;
    float _Ti;
    float _Td;
    
    float _min;
    float _max;
    
    float _sum;

    float _last;
    
    bool _reset;

    float _outF;
    float _outP;
    float _outI;
    float _outD;

    float _dT;
};

#endif //PID_H_
