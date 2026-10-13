/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: Public interface to the BCM283x/BCM2711 BSC i2c bus driver.

    Open I2CBCM2708_NAME, then OOP_NewObject(NULL, CLID_Hidd_I2C_BCM2708,
    tags) gives a hidd.i2c bus. aHidd_I2CDevice_Address is 7-bit << 1.
    Needs timer.device up. Transfers are polled; not for interrupts.
*/

#ifndef HIDD_I2CBCM2708_H
#define HIDD_I2CBCM2708_H

#ifndef EXEC_TYPES_H
#include <exec/types.h>
#endif

#ifndef OOP_OOP_H
#include <oop/oop.h>
#endif

#define CLID_Hidd_I2C_BCM2708   "hidd.i2c.bcm2708"
#define IID_Hidd_I2C_BCM2708    "hidd.i2c.bcm2708"

#define HiddI2CBCM2708AttrBase  __IHidd_I2C_BCM2708

#ifndef __OOP_NOATTRBASES__
extern OOP_AttrBase HiddI2CBCM2708AttrBase;
#endif

enum
{
    /* 0 = GPIO 0/1 (HAT EEPROM), 1 = GPIO 2/3 (header, default) [I.G] */
    aoHidd_I2C_BCM2708_Unit,

    num_Hidd_I2C_BCM2708_Attrs
};

#define aHidd_I2C_BCM2708_Unit  (HiddI2CBCM2708AttrBase + aoHidd_I2C_BCM2708_Unit)

#define IS_I2CBCM2708_ATTR(attr, idx) \
    (((idx) = (attr) - HiddI2CBCM2708AttrBase) < num_Hidd_I2C_BCM2708_Attrs)

#define I2CBCM2708_NAME         "i2c-bcm2708.hidd"

#endif /* HIDD_I2CBCM2708_H */
