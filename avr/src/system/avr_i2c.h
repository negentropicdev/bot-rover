#ifndef AVR_I2C_H_
#define AVR_I2C_H_

#include <stdint.h>

typedef enum {FLOAT, U16, I16, U8} I2CRegisterType;

typedef union {
    float f32;
    uint16_t u16;
    int16_t i16;
    uint8_t u8;
    uint8_t buf[4];
} I2CBuffer;

typedef struct {
    I2CRegisterType type;
    bool update;
    I2CBuffer data;
} I2CRegister;

/**
 * Signals back to app that a master operation has completed.
 * This will be the case whether it was a read or a write
 * and whether it was successful or not.
 */
typedef void (*i2c_master_complete)(uint8_t status);

/**
 * Signals back to app that a slave read has begun. The function
 * called should put the value to write in TWDR.
 * 
 * @return Whether to return an ack (true = more data available) or nack.
 */
typedef bool (*i2c_register_read)();

/**
 * Signals back to app that a slave write has occured. The function
 * should read the value from TWDR
 * 
 * @return Whether to return an ack (true = can receive more data) or nack.
 */
typedef bool (*i2c_register_write)();

/**
 * Tells the application that a register has been specified by a master.
 * 
 * @param reg The register addressed by the master
 * 
 * @return Whether the register is valid or not. Returning false results
 * in responding with a NACK to the master.
 */
typedef bool (*i2c_set_register)(uint8_t reg);

/**
 * Callback for a master requesting a byte when slave is in RAW mode.
 * The function called should put the value to write in TWDR.
 * 
 * @param value the value to write out to the bus.
 * 
 * @return Whether to return an ack (true = more data available) or nack.
 */
typedef bool (*i2c_raw_read)();

/**
 * Callback for a master writing value to this slave in RAW mode.
 * The function should read the value from TWDR.
 * 
 * @return Whether to return an ack (true = can receive more data) or nack.
 */
typedef bool (*i2c_raw_write)();


/** Catchall status value for non mode-specific errors. */
#define I2C_UNKNOWN 255

/** Operation completed as expected. */
#define I2C_SUCCESS 0

/** Operation was initialized correctly and the async callback will be
 * called to indicate result.
 */
#define I2C_ASYNC 1

/** Another operation is in progress that cannot be interrupted. */
#define I2C_BUSY 2

/** Invalid state was detected on the bus. */
#define I2C_BUS_ERROR 3

/** While attempting a master operation, bus control was lost due to
 * arbitration. Simply try again.
 */
#define I2C_LOST_ARB 4

/** While attempting a master operation, no ack was received while
 * addressing the slave.
 */
#define I2C_ADDR_NACK 5

/** While sending data to a slave there was no ack received. */
#define I2C_DATA_NACK 6

/**
 * Encapsulates AVR I2C hardware interactions such that an application
 * only has to worry about the data and reacting to errors.
 */
class AVR_I2C {
public:
    AVR_I2C();

    /** Initializes the instance as a master for initiating
     * reads and writes from this device.
     * 
     * @param rate The bitrate for I2C bus communication from this master.
     * @param cb The callback method that's called when an operation completes.
     */
    void initMaster(unsigned long rate, i2c_master_complete cb);

    /** Initializes the instance as a slave to respond to reads and writes
     * from other masters.
     * 
     * @param address The 7 bit address this device will respond to. This
     *  should be specified in the lower 7 bits (less than 128, 0x80).
     * 
     * @param sr The callback when a master specifies a register to read/write
     * 
     * @param rcb The read callback that is used to determine what value to send
     * 
     * @param wcb The write callback that is used to update local variables
     *  from writes from other masters.
     * 
     * @param genCall Whether the device will respond to general call (0x00) address.
     */
    void initSlave(uint8_t address, i2c_set_register sr, i2c_register_read rcb, i2c_register_write wcb, bool genCall);

    /** Initializes the instance as a slave to respond to reads and writes
     * from other masters.
     * 
     * @param address The 7 bit address this device will respond to. This
     *  should be specified in the lower 7 bits (less than 128, 0x80).
     * 
     * @param rcb The read callback that is used to determine what value to send
     * 
     * @param wcb The write callback that is used to update local variables
     *  from writes from other masters.
     * 
     * @param genCall Whether the device will respond to general call (0x00) address.
     */
    void initSlave(uint8_t address, i2c_raw_read rcb, i2c_raw_write wcb, bool genCall);

    /** Sets whether the library auto increments the register number
     * for slave operations, default is true.
     * 
     * @param autoInc True to have the library automatically address the next register
     * when calling the specified callbacks.
     */
    void autoIncRegister(bool autoInc);

    /** Enables I2C bus interaction after initializing as master and/or slave
     */
    void begin();

    /** Specifies whether the device is currently in the middle of I2C operation.
     * 
     * @Return true if a master operation cannot be started.
     */
    bool busy();

    /** Should only be used from interrupt vector */
    void _interrupt();

    uint8_t write(uint8_t address, uint8_t reg, uint8_t value);
    uint8_t writen(uint8_t address, uint8_t reg, uint8_t *data, uint8_t len);

    uint8_t write(uint8_t address, uint8_t value);
    uint8_t writen(uint8_t address, uint8_t *data, uint8_t len);

    uint8_t read(uint8_t address, uint8_t reg, uint8_t &value);
    uint8_t readn(uint8_t address, uint8_t reg, uint8_t *buf, uint8_t len);

    uint8_t read(uint8_t address, uint8_t &value);
    uint8_t readn(uint8_t address, uint8_t *buf, uint8_t len);

private:

    enum State { IDLE, MT, MR, ST, SR };

    enum SlaveMode { NONE, RAW, REGISTER };

    void configureSlave(uint8_t address, bool genCall);

    void sendRegister();
    void sendData();
    void readData();

    /** Stores current state and prevents operation collision */
    State _state;

    /** Last interrupt's status register */
    uint8_t _status;

    i2c_master_complete _master_cb;

    /** The address of the slave for a master operation. */
    uint8_t _sla;

    /** True when doing master read, false for write */
    bool _read;

    /** Register used for MT/MR */
    uint8_t _register;

    /** Whether to do a register (true) or raw (false) mode master op */
    bool _master_register;

    /** Value for single byte operations */
    uint8_t _value;

    /** Local pointer to current byte for read/write.
     * This is initialized at operation start and is incremented
     * until data_n becomes 0.
     */
    uint8_t *_data;

    /** Number of bytes to read/write as master.
     * This is set at start of operation and decremented for each byte.
     */
    uint8_t _data_n;

    /** Whether as a slave we're expecting to operate on registers or just
     * read/write byte sequences (raw).
     */
    SlaveMode _slave_mode;

    /** Callbacks when addressed as a slave and we're in register mode. */
    i2c_set_register _set_register_cb;
    i2c_register_read _register_read_cb;
    i2c_register_write _register_write_cb;

    /** Raw mode slave operation callbacks. */
    i2c_raw_read _raw_read_cb;
    i2c_raw_write _raw_write_cb;

    /** The next register number that will be read/write during slave operation. */
    uint8_t _slave_reg;

    /** Flag for first byte received as register address */
    bool _slave_set_reg;

    /** Whether registers are auto incremented by the library */
    bool _slave_auto_inc;
};

extern AVR_I2C I2C;

#endif //AVR_I2C_H_