#ifndef HCSR04_H_
#define HCSR04_H_

#include <stdint.h>

typedef struct {
    volatile uint8_t *port;
    volatile uint8_t *ddr;
    volatile uint8_t *pin;
    uint8_t bit;
} SonarDef;

class HCSR04 {
public:
    HCSR04(const SonarDef &def);
    HCSR04(volatile uint8_t *port, volatile uint8_t *ddr, volatile uint8_t *pin, uint8_t bit);

    unsigned long range();

private:
    unsigned long _range();

    volatile uint8_t *_port;
    volatile uint8_t *_ddr;
    volatile uint8_t *_pin;
    uint8_t _bitmask;
};

#endif //HCSR04_H_