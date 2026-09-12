/*
    Copyright 2009-2026, The AROS Development Team. All rights reserved.
*/

#ifndef _LINUX_I2C_ALGO_BIT_H_
#define _LINUX_I2C_ALGO_BIT_H_

#include <linux/i2c.h>

struct i2c_algo_bit_data {
    void *data;
    void (*setsda)(void *data, int state);
    void (*setscl)(void *data, int state);
    int  (*getsda)(void *data);
    int  (*getscl)(void *data);
    int  (*pre_xfer)(struct i2c_adapter *);
    void (*post_xfer)(struct i2c_adapter *);
    int udelay;
    int timeout;
    bool can_do_atomic;
};
extern const struct i2c_algorithm i2c_bit_algo;
int i2c_bit_add_bus(struct i2c_adapter *adap);
int i2c_bit_add_numbered_bus(struct i2c_adapter *adap);

#endif /* _LINUX_I2C_ALGO_BIT_H_ */
