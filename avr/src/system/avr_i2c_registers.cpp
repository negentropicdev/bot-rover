#include "system/avr_i2c_registers.h"

#include <avr/io.h>

//The pre-defined instance
I2C_Registers I2C_Reg;

//Byte counts for each type defined in I2CRegisterType enum
static uint8_t _size[4] = {4, 2, 2, 1};

//user configured registers
static uint8_t _count = 0;
static I2CRegister* _registers = 0;

//variables for current register access via bus
static volatile uint8_t _reg;
static volatile I2CBuffer _buf;
static volatile I2CRegisterType _type;
static volatile uint8_t _buf_i;
static volatile uint8_t _buf_len;

bool set_reg(uint8_t reg) {
    if (reg >= _count) {
        return false;
    }

    _reg = reg;
    _type = _registers[_reg].type;
    _buf_i = 0;
    _buf_len = _size[_type];

    _buf.u32 = _registers[_reg].data.u32;

    return reg < _count;
}

bool read_cb() {
    TWDR = _buf.bytes[_buf_i];

    ++_buf_i;
    if (_buf_i == _buf_len) {
        set_reg(_reg + 1);
    }

    return _reg < _count - 1;
}

bool write_cb() {
    _buf.bytes[_buf_i] = TWDR;

    ++_buf_i;
    if (_buf_i == _buf_len) {
        _registers[_reg].data.u32 = _buf.u32;
        _registers[_reg].updated = true;

        set_reg(_reg + 1);
    }

    return _reg < _count - 1;
}

I2C_Registers::I2C_Registers() {

}

void I2C_Registers::init(uint8_t address, I2CRegister *registers, uint8_t count, bool genCall) {
    _registers = registers;
    _count = count;

    I2C.initSlave(address, set_reg, read_cb, write_cb, genCall);
    I2C.begin();
}

bool I2C_Registers::updated(uint8_t i) {
    if (i >= _count) {
        return false;
    }

    bool u = _registers[i].updated;
    _registers[i].updated = false;
    return u;
}