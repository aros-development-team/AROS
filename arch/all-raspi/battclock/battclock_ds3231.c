/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: DS3231 RTC on the Pi i2c header bus (BSC1), for boards without
          the Pi 5 PMIC RTC. Only used when the device tree describes it,
          i.e. with dtoverlay=i2c-rtc,ds3231 in config.txt.
*/

#define DEBUG 0
#include <aros/debug.h>

#include <proto/exec.h>
#include <proto/oop.h>
#include <proto/utility.h>

#include <oop/oop.h>
#include <hidd/i2c.h>
#include <hidd/i2cbcm2708.h>
#include <utility/date.h>
#include <utility/tagitem.h>

#include "battclock_rtc.h"
#include "battclock_ds3231.h"

struct Library *OOPBase = NULL;
OOP_AttrBase HiddI2CDeviceAttrBase = 0;
OOP_AttrBase HiddI2CBCM2708AttrBase = 0;

#define DS3231_COMPATIBLE   "maxim,ds3231"
#define DS3231_ADDR         0x68
#define DS3231_REG_TIME     0x00
#define DS3231_REG_STATUS   0x0f
#define DS3231_STATUS_OSF   0x80    /* oscillator stopped: time invalid */

/* The i2c-rtc overlay puts the part on i2c_arm, which is BSC1. */
#define BSC1_PATH           "/soc/i2c@7e804000"
#define BSC1_UNIT           1

static UBYTE bcd2bin(UBYTE v)
{
    return (v >> 4) * 10 + (v & 0x0f);
}

static UBYTE bin2bcd(UBYTE v)
{
    return ((v / 10) << 4) | (v % 10);
}

/* 7-bit address of an enabled DS3231 on BSC1, or 0. */
static UBYTE ds3231_find(void)
{
    APTR OpenFirmwareBase = OpenResource("openfirmware.resource");
    void *bus, *node, *prop;

    if (!OpenFirmwareBase)
        return 0;

    bus = OF_OpenKey(BSC1_PATH);
    node = bus ? OF_FindNodeByCompatible(bus, DS3231_COMPATIBLE) : NULL;
    if (!node || !rpi_of_node_ok(OpenFirmwareBase, node))
    {
        D(bug("[battclock] DS3231: bus %p node %p, not in the device tree\n", bus, node));
        return 0;
    }

    prop = OF_FindProperty(node, "reg");
    if (prop && OF_GetPropLen(prop) >= 4)
        return (UBYTE)AROS_BE2LONG(*(ULONG *)OF_GetPropValue(prop));

    return DS3231_ADDR;
}

/*
 * One write-then-read. The bus objects are made per call: the clock is
 * read and written rarely, and hidd.i2c needs timer.device, which is not
 * up when this resource initialises.
 */
static BOOL ds3231_xfer(UBYTE addr, UBYTE *wbuf, ULONG wlen,
                        UBYTE *rbuf, ULONG rlen)
{
    struct pHidd_I2CDevice_WriteRead msg;
    struct Library *drv;
    OOP_Object *bus, *dev = NULL;
    BOOL ok = FALSE;

    if (!OOPBase && !(OOPBase = OpenLibrary("oop.library", 0)))
        return FALSE;

    /* Opening the driver registers its class */
    drv = OpenLibrary(I2CBCM2708_NAME, 0);
    if (!drv)
    {
        D(bug("[battclock] DS3231: cannot open %s\n", I2CBCM2708_NAME));
        return FALSE;
    }

    if (!HiddI2CDeviceAttrBase)
        HiddI2CDeviceAttrBase = OOP_ObtainAttrBase((STRPTR)IID_Hidd_I2CDevice);
    if (!HiddI2CBCM2708AttrBase)
        HiddI2CBCM2708AttrBase = OOP_ObtainAttrBase((STRPTR)IID_Hidd_I2C_BCM2708);

    if (HiddI2CDeviceAttrBase && HiddI2CBCM2708AttrBase)
    {
        struct TagItem busTags[] =
        {
            { aHidd_I2C_BCM2708_Unit, BSC1_UNIT },
            { TAG_DONE,               0         }
        };

        bus = OOP_NewObject(NULL, (STRPTR)CLID_Hidd_I2C_BCM2708, busTags);
        if (bus)
        {
            struct TagItem devTags[] =
            {
                { aHidd_I2CDevice_Driver,  (IPTR)bus         },
                { aHidd_I2CDevice_Address, (IPTR)(addr << 1) },
                { aHidd_I2CDevice_Name,    (IPTR)"DS3231"    },
                { TAG_DONE,                0                 }
            };

            dev = OOP_NewObject(NULL, (STRPTR)CLID_Hidd_I2CDevice, devTags);
            if (dev)
            {
                msg.mID         = OOP_GetMethodID((STRPTR)IID_Hidd_I2CDevice,
                                                  moHidd_I2CDevice_WriteRead);
                msg.writeBuffer = wbuf;
                msg.writeLength = wlen;
                msg.readBuffer  = rbuf;
                msg.readLength  = rlen;

                ok = (BOOL)OOP_DoMethod(dev, (OOP_Msg)&msg);
                OOP_DisposeObject(dev);
            }
            OOP_DisposeObject(bus);
        }
        D(bug("[battclock] DS3231: bus %p dev %p, transfer %d\n", bus, dev, ok));
    }

    CloseLibrary(drv);
    return ok;
}

BOOL DS3231_Read(ULONG *secs)
{
    struct UtilityBase *UtilityBase;
    struct ClockData cd;
    UBYTE addr, reg, status, r[7];

    addr = ds3231_find();
    if (!addr)
        return FALSE;

    /* OSF stays set until the time is written after a power loss */
    reg = DS3231_REG_STATUS;
    if (!ds3231_xfer(addr, &reg, 1, &status, 1) || (status & DS3231_STATUS_OSF))
    {
        D(bug("[battclock] DS3231 @ %02x: no valid time\n", addr));
        return FALSE;
    }

    reg = DS3231_REG_TIME;
    if (!ds3231_xfer(addr, &reg, 1, r, sizeof(r)))
        return FALSE;

    cd.sec   = bcd2bin(r[0] & 0x7f);
    cd.min   = bcd2bin(r[1] & 0x7f);
    if (r[2] & 0x40)    /* 12-hour mode, bit 5 = PM */
        cd.hour = bcd2bin(r[2] & 0x1f) % 12 + ((r[2] & 0x20) ? 12 : 0);
    else
        cd.hour = bcd2bin(r[2] & 0x3f);
    cd.mday  = bcd2bin(r[4] & 0x3f);
    cd.month = bcd2bin(r[5] & 0x1f);
    cd.year  = 2000 + bcd2bin(r[6]) + ((r[5] & 0x80) ? 100 : 0);
    cd.wday  = 0;

    if (cd.sec > 59 || cd.min > 59 || cd.hour > 23 ||
        cd.mday < 1 || cd.mday > 31 || cd.month < 1 || cd.month > 12)
    {
        D(bug("[battclock] DS3231: bad time %02x %02x %02x %02x %02x %02x\n",
              r[0], r[1], r[2], r[4], r[5], r[6]));
        return FALSE;
    }

    UtilityBase = (struct UtilityBase *)TaggedOpenLibrary(TAGGEDOPEN_UTILITY);
    if (!UtilityBase)
        return FALSE;
    *secs = Date2Amiga(&cd);
    CloseLibrary((struct Library *)UtilityBase);

    D(bug("[battclock] DS3231 @ %02x: %04u-%02u-%02u %02u:%02u:%02u\n", addr,
          cd.year, cd.month, cd.mday, cd.hour, cd.min, cd.sec));
    return TRUE;
}

BOOL DS3231_Write(ULONG secs)
{
    struct UtilityBase *UtilityBase;
    struct ClockData cd;
    UBYTE addr, reg, status, w[8];

    addr = ds3231_find();
    if (!addr)
        return FALSE;

    UtilityBase = (struct UtilityBase *)TaggedOpenLibrary(TAGGEDOPEN_UTILITY);
    if (!UtilityBase)
        return FALSE;
    Amiga2Date(secs, &cd);
    CloseLibrary((struct Library *)UtilityBase);

    /* The part counts 2000..2199 */
    if (cd.year < 2000 || cd.year > 2199)
        return FALSE;

    w[0] = DS3231_REG_TIME;
    w[1] = bin2bcd(cd.sec);
    w[2] = bin2bcd(cd.min);
    w[3] = bin2bcd(cd.hour);            /* 24-hour mode */
    w[4] = cd.wday + 1;                 /* 1..7 */
    w[5] = bin2bcd(cd.mday);
    w[6] = bin2bcd(cd.month) | (cd.year >= 2100 ? 0x80 : 0);
    w[7] = bin2bcd(cd.year % 100);

    if (!ds3231_xfer(addr, w, sizeof(w), NULL, 0))
        return FALSE;

    /* The time is valid again */
    reg = DS3231_REG_STATUS;
    if (ds3231_xfer(addr, &reg, 1, &status, 1) && (status & DS3231_STATUS_OSF))
    {
        w[0] = DS3231_REG_STATUS;
        w[1] = status & ~DS3231_STATUS_OSF;
        ds3231_xfer(addr, w, 2, NULL, 0);
    }

    return TRUE;
}
