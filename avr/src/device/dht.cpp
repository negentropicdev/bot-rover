#include "device/dht.h"

#include <avr/io.h>
#include <avr/interrupt.h>

#include "system/avr_timer.h"

#define DHT_TIMEOUT (F_CPU/40000)

DHT::DHT(volatile uint8_t *port, volatile uint8_t *ddr, volatile uint8_t *pin, uint8_t bit) {
    _port = port;
    _ddr = ddr;
    _pin = pin;
    _bitmask = (1<<bit);

    _temp = 0;
    _humidity = 0;
}

float DHT::getTemp() {
    return _temp;
}

float DHT::getHumidity() {
    return _humidity;
}

int8_t DHT::read() {
    //cli();
    int8_t res = _read();
    //sei();

    if (res != DHT_OK) {
        return res;
    }

    // these _buf are always zero, masking them reduces errors.
    _buf[0] &= 0x03;
    _buf[2] &= 0x83;

    // CONVERT AND STORE
    _humidity = (_buf[0]*256 + _buf[1]) * 0.1;
    _temp = ((_buf[2] & 0x7F)*256 + _buf[3]) * 0.1;
    if (_buf[2] & 0x80)  // negative temperature
    {
        _temp = -_temp;
    }

    // TEST CHECKSUM
    uint8_t sum = _buf[0] + _buf[1] + _buf[2] + _buf[3];
    if (_buf[4] != sum)
    {
        return DHT_ERR_CHECKSUM;
    }
    return DHT_OK;
}

int8_t DHT::_read() {
    uint8_t mask = 128;
    uint8_t idx = 0;

    uint8_t data = 0;
    uint8_t state = 0;
    uint8_t pstate = 0;
    uint16_t zeroLoop = DHT_TIMEOUT;
    uint16_t delta = 0;

    uint8_t leadingZeroBits = 34;

    // REQUEST SAMPLE
    *_ddr |= _bitmask; //pinMode(pin, OUTPUT);
    *_port &= ~_bitmask; //digitalWrite(pin, LOW); // T-be
    delay_us(1000UL);
    // digitalWrite(pin, HIGH); // T-go
    *_ddr &= ~_bitmask; //pinMode(pin, INPUT);

    uint16_t loopCount = DHT_TIMEOUT * 2;  // 200uSec max
    // while(digitalRead(pin) == HIGH)
    while ((*_pin & _bitmask) != 0 )
    {
        if (--loopCount == 0) 
        {
          return DHT_ERR_CONNECT;
        }
    }

    // GET ACKNOWLEDGE or TIMEOUT
    loopCount = DHT_TIMEOUT;
    // while(digitalRead(pin) == LOW)
    while ((*_pin & _bitmask) == 0 )  // T-rel
    {
        if (--loopCount == 0) 
        {
          return DHT_ERR_ACK_L;
        }
    }

    loopCount = DHT_TIMEOUT;
    // while(digitalRead(pin) == HIGH)
    while ((*_pin & _bitmask) != 0 )  // T-reh
    {
        if (--loopCount == 0)
        {
          return DHT_ERR_ACK_H;
        }
    }

    loopCount = DHT_TIMEOUT;

    // READ THE OUTPUT - 40 BITS => 5 BYTES
    for (uint8_t i = 40; i != 0; )
    {
        // WAIT FOR FALLING EDGE
        state = (*_pin & _bitmask);
        if (state == 0 && pstate != 0)
        {
            if (i > leadingZeroBits) // DHT22 first 6 bits are all zero !!   DHT11 only 1
            {
                zeroLoop = (zeroLoop < loopCount ? zeroLoop : loopCount);
                delta = (DHT_TIMEOUT - zeroLoop)/4;
            }
            else if ( loopCount <= (zeroLoop - delta) ) // long -> one
            {
                data |= mask;
            }
            mask >>= 1;
            if (mask == 0)   // next byte
            {
                mask = 128;
                _buf[idx] = data;
                idx++;
                data = 0;
            }
            // next bit
            --i;

            // reset timeout flag
            loopCount = DHT_TIMEOUT;
        }
        pstate = state;
        // Check timeout
        if (--loopCount == 0)
        {
          return DHT_ERR_TIMEOUT;
        }

    }

    return DHT_OK;
}