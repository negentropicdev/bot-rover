#ifndef AVR_I2C_REGISTERS_H_
#define AVR_I2C_REGISTERS_H_

#include "avr_i2c.h"

#define REG_FLOAT(x) {FLOAT, {.f32 = (x)}, false}
#define REG_U16(x) {U16, {.u16 = (x)}, false}
#define REG_I16(x) {I16, {.i16 = (x)}, false}
#define REG_U8(x) {U8, {.u8 = (x)}, false}

#define REG_U8_PTR(regs, num) &(regs[num].data.u8)
#define REG_U16_PTR(regs, num) &(regs[num].data.u16)
#define REG_I16_PTR(regs, num) &(regs[num].data.i16)
#define REG_FLOAT_PTR(regs, num) &(regs[num].data.f32)

typedef enum {FLOAT, U16, I16, U8} I2CRegisterType;

typedef union {
    float f32;
    uint16_t u16;
    int16_t i16;
    uint8_t u8;
    uint8_t bytes[4];

    uint32_t u32; //used only for copying
} I2CBuffer;

typedef struct {
    I2CRegisterType type;
    I2CBuffer data;
    bool updated;
} I2CRegister;

class I2C_Registers {
public:
    I2C_Registers();
    void init(uint8_t address, I2CRegister *registers, uint8_t count, bool genCall);

    bool updated(uint8_t i);
};

extern I2C_Registers I2C_Reg;

#endif //AVR_I2C_REGISTERS_H_