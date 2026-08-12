/*
    Copyright 2009-2026, The AROS Development Team. All rights reserved.
*/

#ifndef _DRM_COMPAT_I2C_
#define _DRM_COMPAT_I2C_

#include <exec/types.h>

/* I2C handling */

#define I2C_FUNC_I2C                    (1 << 1)
#define I2C_FUNC_SMBUS_EMUL             (1 << 2)
#define I2C_FUNC_SMBUS_BLOCK_PROC_CALL  (1 << 3)
#define I2C_FUNC_10BIT_ADDR             (1 << 4)
#define I2C_FUNC_SMBUS_READ_BLOCK_DATA  (1 << 5)

#define I2C_M_RD    0x0001
#define I2C_M_STOP  0x0002

struct i2c_msg
{
    UWORD addr;
    UWORD flags;
    UWORD len;
    UBYTE *buf;
};

#define ADAP_TYPE_DEFAULT   1
#define ADAP_TYPE_ALGO      2

struct i2c_adapter
{
    IPTR i2cdriver;     /* OOP_Object * */
    APTR algo_data;
    CONST_APTR algo;
    BYTE type;
};

struct i2c_algorithm
{
    unsigned int (*functionality)(struct i2c_adapter *adap);
    int (*master_xfer)(struct i2c_adapter *adap, struct i2c_msg *msgs, int num);
};

struct i2c_client;

#define I2C_BOARD_INFO(x, y) \
    .type = (x), .addr = (y)

struct i2c_board_info
{
    BYTE type[20]; /* Name? */
    UWORD addr;
};

struct i2c_driver
{
    ULONG dummy;
};

struct i2c_algo_bit_data
{
    void (*setsda)(void *data, int state);
    void (*setscl)(void *data, int state);
    int  (*getsda)(void *data);
    int  (*getscl)(void *data);

    APTR data;
    ULONG timeout; /* jiffies */

    int  (*pre_xfer)(struct i2c_adapter *);
    void (*post_xfer)(struct i2c_adapter *);
};

/* I2C handling */
int i2c_transfer(struct i2c_adapter *adap, struct i2c_msg *msgs, int num);
int i2c_del_adapter(struct i2c_adapter *);
int i2c_add_adapter(struct i2c_adapter *);
int i2c_bit_add_bus(struct i2c_adapter *);

#endif /* _DRM_COMPAT_I2C_ */
