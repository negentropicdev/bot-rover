#ifndef DHT_H_
#define DHT_H_

#include <stdint.h>

#define DHT_OK            0
#define DHT_ERR_UNKNOWN  -1
#define DHT_ERR_CHECKSUM -2
#define DHT_ERR_TIMEOUT  -3
#define DHT_ERR_CONNECT  -4
#define DHT_ERR_ACK_L    -5
#define DHT_ERR_ACK_H    -6

class DHT {
public:
    DHT(volatile uint8_t *port, volatile uint8_t *ddr, volatile uint8_t *pin, uint8_t bit);

    int8_t read();

    float getTemp();
    float getHumidity();

private:
    int8_t _read();
    
    uint8_t _buf[5];

    volatile uint8_t *_port;
    volatile uint8_t *_ddr;
    volatile uint8_t *_pin;
    uint8_t _bitmask;

    float _temp;
    float _humidity;
};

#endif //DHT_H_