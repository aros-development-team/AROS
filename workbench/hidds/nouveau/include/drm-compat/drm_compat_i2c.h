/*
    Copyright 2009-2026, The AROS Development Team. All rights reserved.
*/

#ifndef _DRM_COMPAT_I2C_
#define _DRM_COMPAT_I2C_

#include <exec/types.h>

/* I2C handling */
struct i2c_adapter
{
    IPTR i2cdriver;     /* OOP_Object * */
    APTR algo_data;
};

struct i2c_client;
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
};

#define I2C_M_RD    0x0001

struct i2c_msg
{
    UWORD addr;
    UWORD flags;
    UWORD len;
    UBYTE *buf;
};

/* I2C handling */
int i2c_transfer(struct i2c_adapter *adap, struct i2c_msg *msgs, int num);
int i2c_del_adapter(struct i2c_adapter *);

#endif /* _DRM_COMPAT_I2C_ */
