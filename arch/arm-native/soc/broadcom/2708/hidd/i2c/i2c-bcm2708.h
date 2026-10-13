/*
    Copyright (C) 2010-2026, The AROS Development Team. All rights reserved.

    Desc: Private data of the BCM283x/BCM2711 BSC i2c driver.
*/

#ifndef I2C_BCM2708_H
#define I2C_BCM2708_H

#include <aros/macros.h>
#include <exec/types.h>
#include <exec/libraries.h>
#include <exec/semaphores.h>
#include <oop/oop.h>

#include "i2cbcm2708.h"

#define BSC_C                   0x00
#define BSC_S                   0x04
#define BSC_DLEN                0x08
#define BSC_A                   0x0c
#define BSC_FIFO                0x10
#define BSC_DIV                 0x14

#define BSC_BUS_HZ              100000
#define BSC_SPIN_LIMIT          1000000UL

#define BSC_UNITS               2

struct bsc_unit
{
    struct SignalSemaphore  lock;       /* one transaction at a time */
    IPTR                    base;
    UBYTE                   sda, scl;   /* GPIO pins */
    BOOL                    ready;      /* pins muxed, divider set */
};

struct bsc_staticdata
{
    struct Library      *OOPBase;
    struct Library      *utilityBase;
    OOP_AttrBase         hiddI2CAB;
    OOP_AttrBase         hiddI2CDeviceAB;
    OOP_AttrBase         hiddI2CBCM2708AB;
    OOP_Class           *i2cDrvClass;
    IPTR                 periiobase;
    struct bsc_unit      unit[BSC_UNITS];
};

struct i2cbcm2708base
{
    struct Library          i2c_LibNode;
    struct bsc_staticdata   psd;
};

struct bsc_busdata
{
    struct bsc_unit     *unit;
};

#define PSD(cl) (&((struct i2cbcm2708base *)cl->UserData)->psd)

#undef HiddI2CAttrBase
#undef HiddI2CDeviceAttrBase
#undef HiddI2CBCM2708AttrBase
#define HiddI2CAttrBase         (PSD(cl)->hiddI2CAB)
#define HiddI2CDeviceAttrBase   (PSD(cl)->hiddI2CDeviceAB)
#define HiddI2CBCM2708AttrBase  (PSD(cl)->hiddI2CBCM2708AB)
#define UtilityBase             (PSD(cl)->utilityBase)
#define OOPBase                 (PSD(cl)->OOPBase)

static inline ULONG bsc_rd(IPTR base, ULONG off)
{
    return AROS_LE2LONG(*(volatile ULONG *)(base + off));
}

static inline void bsc_wr(IPTR base, ULONG off, ULONG val)
{
    *(volatile ULONG *)(base + off) = AROS_LONG2LE(val);
}

#endif /* I2C_BCM2708_H */
