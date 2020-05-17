#include "avr_i2c.h"

//#define DEBUG_I2C_SER

//from AVR Libc, TWI interface status defines
#include <util/twi.h>

#include <avr/io.h>
#include <avr/interrupt.h>

#ifdef DEBUG_I2C_SER
#include <stdio.h>
#endif

// The predefined I2C instance
AVR_I2C I2C;

ISR(TWI_vect) {
    I2C._interrupt();
}

void start() {
    //set start and clear interrupt flag to resume
    TWCR = (1<<TWINT) | (1<<TWSTA) | (1<<TWEN) | (1<<TWIE);
}

void next(bool ack) {
    //optionally set ack and clear interrupt flag to resume
    TWCR = (1<<TWINT) | (1<<TWIE) | (1<<TWEN) | ((ack ? 1 : 0)<<TWEA);
}

void stop() {
    //set stop and clear interrupt flag to resume
    TWCR |= (1<<TWINT) | (1<<TWSTO) | (1<<TWEN) | (1<<TWIE);
}

void reset() {
    TWCR |= (1<<TWINT) | (1<<TWIE);
}

void AVR_I2C::_interrupt() {
    _status = TW_STATUS;

    #ifdef DEBUG_I2C_SER
        putchar('#');
    #endif

    //shortcut switch to respond to stops ASAP to alleviate bus issues.
    if (_status == TW_SR_STOP) {
        #ifdef DEBUG_I2C_SER
            printf(".\n");
        #endif
        TWCR = (1 << TWINT) | (1 << TWEA) | (1 << TWEN) | (1 << TWIE);
        return;
    }

    bool ack = false; //used in slave modes to know how to respond to master

    switch (_status) {
        //start condition transmitted
        case TW_START:
            #ifdef DEBUG_I2C_SER
                putchar(12);
                putchar('S');
            #endif

            if (_master_register) {
                TWDR = (_sla<<1); //Always start in MT when needing to send register
                #ifdef DEBUG_I2C_SER
                    putchar('T');
                #endif
            } else {
                TWDR = (_sla<<1) | (_read ? 1 : 0); //will be MR/MT depending on _read
                #ifdef DEBUG_I2C_SER
                    putchar(_read ? 'R' : 'T');
                #endif
            }
            next(false);
            break;

        //repeated start condition transmitted
        case TW_REP_START:
            #ifdef DEBUG_I2C_SER
                putchar('R');
            #endif

            TWDR = (_sla<<1) | (_read ? 1 : 0);
            next(false);
            break;

        //SLA+W transmitted, ACK received
        case TW_MT_SLA_ACK:
            #ifdef DEBUG_I2C_SER
                putchar('A');
            #endif

            if (_master_register) {
                _master_register = false;

                sendRegister();
            }
            break;

        //SLA+W transmitted, NACK received
        case TW_MT_SLA_NACK:
            #ifdef DEBUG_I2C_SER
                putchar('!');
            #endif

            //reset();
            _master_cb(I2C_ADDR_NACK);
            break;

        //data transmitted, ACK received
        case TW_MT_DATA_ACK:
            #ifdef DEBUG_I2C_SER
                putchar('D');
            #endif
            //Might be in MT from register being written
            if (_read) {
                //Change to MR by initiating repeated start
                start();
            }
            break;

        //data transmitted, NACK received
        case TW_MT_DATA_NACK:
            #ifdef DEBUG_I2C_SER
                putchar('@');
            #endif

            reset();
            _master_cb(I2C_DATA_NACK);
            break;

        //arbitration lost in SLA+W/R or data
        case TW_MT_ARB_LOST: //Also TW_MR_ARB_LOST
            #ifdef DEBUG_I2C_SER
                putchar('&');
            #endif

            reset();
            _master_cb(I2C_LOST_ARB);
            break;

        //SLA+R transmitted, ACK received
        case TW_MR_SLA_ACK:
            #ifdef DEBUG_I2C_SER
                putchar('A');
            #endif
            next(_data_n > 1);
            break;

        //SLA+R transmitted, NACK received
        case TW_MR_SLA_NACK:
            #ifdef DEBUG_I2C_SER
                putchar('#');
            #endif

            reset();
            _master_cb(I2C_ADDR_NACK);
            break;

        //data received, ACK returned
        case TW_MR_DATA_ACK:
            #ifdef DEBUG_I2C_SER
                putchar('D');
            #endif
            sendData();
            break;

        //data received, NACK returned
        case TW_MR_DATA_NACK:
            #ifdef DEBUG_I2C_SER
                putchar('$');
            #endif
            readData();
            break;

        //SLA+R received, ACK returned
        case TW_ST_SLA_ACK:
            #ifdef DEBUG_I2C_SER
                putchar('?');
            #endif

            if (_slave_mode == REGISTER && _register_write_cb != 0) {
                ack = _register_read_cb(_slave_reg);
                if (_slave_auto_inc) _slave_reg++;
            } else if (_slave_mode == RAW && _raw_write_cb != 0) {
                ack = _raw_read_cb();
            }

            #ifdef DEBUG_I2C_SER
                printf(">%02x", TWDR);
            #endif

            if (ack) {
                TWCR = (1 << TWINT) | (1<< TWEA) | (1 << TWEN) | (1 << TWIE);
            } else {
                TWCR = (1 << TWINT) | (1 << TWEN) | (1 << TWIE);
            }
            break;

        //arbitration lost in SLA+RW, SLA+R received, ACK returned
        case TW_ST_ARB_LOST_SLA_ACK:
            break;

        //data transmitted, ACK received
        case TW_ST_DATA_ACK:
            if (_slave_mode == REGISTER && _register_write_cb != 0) {
                ack = _register_read_cb(_slave_reg);
                if (_slave_auto_inc) _slave_reg++;
            } else if (_slave_mode == RAW && _raw_write_cb != 0) {
                ack = _raw_read_cb();
            }

            #ifdef DEBUG_I2C_SER
                printf(">%02x", TWDR);
            #endif

            if (ack) {
                TWCR = (1 << TWINT) | (1<< TWEA) | (1 << TWEN) | (1 << TWIE);
            } else {
                TWCR = (1 << TWINT) | (1 << TWEN) | (1 << TWIE);
            }
            break;

        //data transmitted, NACK received
        case TW_ST_DATA_NACK:
            #ifdef DEBUG_I2C_SER
                printf("!\n");
            #endif

            TWCR = (1 << TWINT) | (1 << TWEN) | (1 << TWIE) | (1 << TWEA);
            break;

        //last data byte transmitted, ACK received
        case TW_ST_LAST_DATA:
            #ifdef DEBUG_I2C_SER
                printf(">%02x.\n", *_data);
            #endif
            
            //just re-enable the I2C engine to be ready for more slave ops.
            //#TODO: check for pending master op and send start
            TWCR = (1 << TWINT) | (1 << TWIE) | (1 << TWEN) | (1 << TWEA);
            break;

        //SLA+W received, ACK returned
        case TW_SR_SLA_ACK:
            #ifdef DEBUG_I2C_SER
                printf("W");
            #endif
            //mark whether expecting a register
            _slave_set_reg = _slave_mode == REGISTER;

            //Continue bus ops
            TWCR = (1 << TWINT) | (1 << TWIE) | (1 << TWEN) | (1 << TWEA);
            break;

        //arbitration lost in SLA+RW, SLA+W received, ACK returned
        case TW_SR_ARB_LOST_SLA_ACK:
            break;

        //general call received, ACK returned
        case TW_SR_GCALL_ACK:
            break;

        //arbitration lost in SLA+RW, general call received, ACK returned
        case TW_SR_ARB_LOST_GCALL_ACK:
            break;

        //data received, ACK returned
        case TW_SR_DATA_ACK:
            if (_slave_set_reg) {
                #ifdef DEBUG_I2C_SER
                    putchar('r');
                #endif

                _slave_set_reg = false;
                _slave_reg = TWDR;
                ack = true;
            } else {
                if (_slave_mode == REGISTER && _register_write_cb != 0) {
                    ack = _register_write_cb(_slave_reg);
                    if (_slave_auto_inc) ++_slave_reg;
                } else if (_slave_mode == RAW && _raw_write_cb != 0) {
                    ack = _raw_write_cb();
                }
                #ifdef DEBUG_I2C_SER
                    putchar('<');
                #endif
            }

            #ifdef DEBUG_I2C_SER
                printf("%02x", TWDR);
            #endif

            if (ack) {
                TWCR = (1 << TWINT) | (1<< TWEA) | (1 << TWEN) | (1 << TWIE);
            } else {
                TWCR = (1 << TWINT) | (1 << TWEN) | (1 << TWIE);
            }
            break;

        //data received, NACK returned
        case TW_SR_DATA_NACK:
            if (_slave_mode == REGISTER && _register_write_cb != 0) {
                _register_write_cb(_slave_reg);
                if (_slave_auto_inc) ++_slave_reg;
            } else if (_slave_mode == RAW && _raw_write_cb != 0) {
                _raw_write_cb();
            }

            #ifdef DEBUG_I2C_SER
                printf("<%02x!", TWDR);
            #endif

            TWCR = (1 << TWINT) | (1<< TWEA) | (1 << TWEN) | (1 << TWIE);
            break;

        //general call data received, ACK returned
        case TW_SR_GCALL_DATA_ACK:
            break;

        //general call data received, NACK returned
        case TW_SR_GCALL_DATA_NACK:
            break;

        //stop or repeated start condition received while selected
        case TW_SR_STOP:
            // !!!!!!!!!! Handled by if statement before switch ^^^^
            break;

        //no state information available
        case TW_NO_INFO:
            break;

        //illegal start or stop condition
        case TW_BUS_ERROR:
            break;
    }
}

AVR_I2C::AVR_I2C() {
    _state = IDLE;

    _status = 0;

    _master_cb = 0;

    _data = 0;
    _data_n = 0;

    _slave_mode = NONE;

    _register_read_cb = 0;
    _register_write_cb = 0;

    _raw_read_cb = 0;
    _raw_write_cb = 0;
    _slave_reg = 0;
    _slave_auto_inc = true;
    _slave_set_reg = false;
}

void AVR_I2C::initMaster(unsigned long rate, i2c_master_complete cb) {
    //rate = F_CPU / (16 + 2(TWBR) * Prescale)
    //scaler = (F_CPU / (2 * Rate)) - 8 assuming prescale of 1
    
    //automatically determine prescale value to be as small as possible while
    //fitting the bit rate divider into 8 bits
    int bitRate = F_CPU / (2 * rate) - 8;
    
    uint8_t prescale = 0;
    
    while (bitRate > 255) {
        prescale++;
        bitRate /= 4;
        
        if (prescale > 3) {
            //error case, can't handle that large of a rate
            //for now do nothing
        }
    }
    
    TWSR = prescale & 0x3;
    TWBR = (uint8_t)bitRate;

    _master_cb = cb;
}

void AVR_I2C::initSlave(uint8_t address, i2c_register_read rcb, i2c_register_write wcb, bool genCall) {
    _register_read_cb = rcb;
    _register_write_cb = wcb;

    _slave_mode = REGISTER;

    configureSlave(address, genCall);
}

void AVR_I2C::initSlave(uint8_t address, i2c_raw_read rcb, i2c_raw_write wcb, bool genCall) {
    _raw_read_cb = rcb;
    _raw_write_cb = wcb;

    _slave_mode = RAW;

    configureSlave(address, genCall);
}

void AVR_I2C::configureSlave(uint8_t address, bool genCall) {
    TWAR = (address<<1) | (genCall ? 1 : 0);
    TWCR |= (1<<TWEA);
}

void AVR_I2C::autoIncRegister(bool autoInc) {
    _slave_auto_inc = autoInc;
}

void AVR_I2C::begin() {
    //enable the I2C bus and enable interrupts
    TWCR |= (1<<TWEN) | (1<<TWIE);
}

bool AVR_I2C::busy() {
    return _state != IDLE;
}

void AVR_I2C::sendRegister() {
    #ifdef DEBUG_I2C_SER
        printf("%02x", _register);
    #endif

    _master_register = false;
    TWDR = _register;
    next(_data_n > 1);
}

void AVR_I2C::sendData() {
    #ifdef DEBUG_I2C_SER
        printf("%02x ", *_data);
    #endif

    TWDR = *_data;
    _data_n--;

    if (_data_n > 0) {
        _data++;
        next(_data_n > 1);
    } else {
        stop();
    }
}

void AVR_I2C::readData() {
    *_data = TWDR;

    #ifdef DEBUG_I2C_SER
        printf("%02x ", *_data);
    #endif

    _data_n--;

    if (_data_n > 0) {
        _data++;
        next (_data_n > 1);
    } else {
        stop();
    }
}

uint8_t AVR_I2C::write(uint8_t address, uint8_t reg, uint8_t value) {
    _value = value;
    return writen(address, reg, &_value, 1);
}

uint8_t AVR_I2C::writen(uint8_t address, uint8_t reg, uint8_t *data, uint8_t len) {
    _sla = address;
    _register = reg;
    _master_register = true;
    _data = data;
    _data_n = len;
    _read = false;

    _state = MT;

    start();

    return I2C_ASYNC;
}

uint8_t AVR_I2C::write(uint8_t address, uint8_t value) {
    _value = value;
    return writen(address, &_value, 1);
}

uint8_t AVR_I2C::writen(uint8_t address, uint8_t *data, uint8_t len) {
    _sla = address;
    _master_register = false;
    _data = data;
    _data_n = len;

    _state = MT;

    start();

    return I2C_ASYNC;
}

uint8_t AVR_I2C::read(uint8_t address, uint8_t reg, uint8_t &value) {
    return readn(address, reg, &value, 1);
}

uint8_t AVR_I2C::readn(uint8_t address, uint8_t reg, uint8_t *buf, uint8_t len) {
    _sla = address;
    _register = reg;
    _master_register = true;
    _data = buf;
    _data_n = len;
    _read = true;

    _state = MR;

    start();

    return I2C_ASYNC;
}

uint8_t AVR_I2C::read(uint8_t address, uint8_t &value) {
    return readn(address, &value, 1);
}

uint8_t AVR_I2C::readn(uint8_t address, uint8_t *buf, uint8_t len) {
    _sla = address;
    _master_register = false;
    _data = buf;
    _data_n = len;
    _read = true;

    _state = MR;

    start();

    return I2C_ASYNC;
}