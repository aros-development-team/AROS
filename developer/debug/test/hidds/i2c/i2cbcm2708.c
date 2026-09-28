/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: Test i2c-bcm2708.hidd. Scans the bus like i2cdetect, or with
          ADDR reads LEN bytes from REG (ADDR 0x68 LEN 7 = DS3231 time).
*/

#include <exec/types.h>
#include <dos/dos.h>
#include <oop/oop.h>
#include <hidd/i2c.h>
#include <hidd/i2cbcm2708.h>
#include <utility/tagitem.h>

#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/oop.h>

#include <stdio.h>
#include <stdlib.h>

OOP_AttrBase HiddI2CDeviceAttrBase;
OOP_AttrBase HiddI2CBCM2708AttrBase;

static const char version[] __attribute__((used)) = "$VER: I2C-BCM2708 1.0 (23.09.2026)";

#define TEMPLATE "UNIT/N,ADDR/K,REG/K,LEN/N"

enum { ARG_UNIT, ARG_ADDR, ARG_REG, ARG_LEN, ARG_COUNT };

static BOOL probe(OOP_Object *bus, UBYTE addr)
{
    struct pHidd_I2C_ProbeAddress msg;

    msg.mID = OOP_GetMethodID((STRPTR)IID_Hidd_I2C, moHidd_I2C_ProbeAddress);
    msg.address = addr << 1;

    return (BOOL)OOP_DoMethod(bus, (OOP_Msg)&msg);
}

static void scan(OOP_Object *bus)
{
    int row, col, found = 0;

    printf("     0  1  2  3  4  5  6  7  8  9  a  b  c  d  e  f\n");
    for (row = 0; row < 0x80; row += 16)
    {
        printf("%02x:", row);
        for (col = 0; col < 16; col++)
        {
            int a = row + col;

            /* Reserved addresses, as i2cdetect skips them */
            if (a < 0x08 || a > 0x77)
                printf("   ");
            else if (probe(bus, a))
            {
                printf(" %02x", a);
                found++;
            }
            else
                printf(" --");
        }
        printf("\n");
    }
    printf("%d device(s)\n", found);
}

static int dump(OOP_Object *bus, UBYTE addr, UBYTE reg, ULONG len)
{
    struct TagItem devTags[] =
    {
        { aHidd_I2CDevice_Driver,  (IPTR)bus         },
        { aHidd_I2CDevice_Address, (IPTR)(addr << 1) },
        { aHidd_I2CDevice_Name,    (IPTR)"I2CTest"   },
        { TAG_DONE,                0                 }
    };
    struct pHidd_I2CDevice_WriteRead msg;
    OOP_Object *dev;
    UBYTE buf[256];
    ULONG i;
    BOOL ok;

    dev = OOP_NewObject(NULL, (STRPTR)CLID_Hidd_I2CDevice, devTags);
    if (!dev)
    {
        printf("could not create device object\n");
        return RETURN_FAIL;
    }

    msg.mID         = OOP_GetMethodID((STRPTR)IID_Hidd_I2CDevice, moHidd_I2CDevice_WriteRead);
    msg.writeBuffer = &reg;
    msg.writeLength = 1;
    msg.readBuffer  = buf;
    msg.readLength  = len;

    ok = (BOOL)OOP_DoMethod(dev, (OOP_Msg)&msg);
    OOP_DisposeObject(dev);

    if (!ok)
    {
        printf("%02x: read of %u byte(s) at reg %02x failed\n",
               addr, (unsigned)len, reg);
        return RETURN_ERROR;
    }

    for (i = 0; i < len; i++)
        printf("%s%02x", (i % 16) ? " " : (i ? "\n" : ""), buf[i]);
    printf("\n");

    return RETURN_OK;
}

int main(void)
{
    IPTR args[ARG_COUNT] = { 0 };
    struct RDArgs *rda;
    struct Library *drv;
    OOP_Object *bus;
    ULONG unit = 1, len = 1;
    int rc = RETURN_FAIL;

    rda = ReadArgs(TEMPLATE, args, NULL);
    if (!rda)
    {
        PrintFault(IoErr(), "I2C-BCM2708");
        return RETURN_FAIL;
    }

    if (args[ARG_UNIT])
        unit = *(LONG *)args[ARG_UNIT];
    if (args[ARG_LEN])
        len = *(LONG *)args[ARG_LEN];
    if (len < 1 || len > 256)
    {
        printf("LEN must be 1..256\n");
        goto out_args;
    }

    /* Opening the driver registers its class */
    drv = OpenLibrary(I2CBCM2708_NAME, 0);
    if (!drv)
    {
        printf("cannot open %s (not a BCM283x/2711?)\n", I2CBCM2708_NAME);
        goto out_args;
    }

    HiddI2CDeviceAttrBase  = OOP_ObtainAttrBase((STRPTR)IID_Hidd_I2CDevice);
    HiddI2CBCM2708AttrBase = OOP_ObtainAttrBase((STRPTR)IID_Hidd_I2C_BCM2708);

    if (HiddI2CDeviceAttrBase && HiddI2CBCM2708AttrBase)
    {
        struct TagItem busTags[] =
        {
            { aHidd_I2C_BCM2708_Unit, unit },
            { TAG_DONE,               0    }
        };

        bus = OOP_NewObject(NULL, (STRPTR)CLID_Hidd_I2C_BCM2708, busTags);
        if (bus)
        {
            if (args[ARG_ADDR])
                rc = dump(bus, (UBYTE)strtoul((char *)args[ARG_ADDR], NULL, 0),
                          args[ARG_REG] ? (UBYTE)strtoul((char *)args[ARG_REG], NULL, 0) : 0,
                          len);
            else
            {
                printf("BSC%u:\n", (unsigned)unit);
                scan(bus);
                rc = RETURN_OK;
            }
            OOP_DisposeObject(bus);
        }
        else
            printf("no bus object for unit %u\n", (unsigned)unit);
    }
    else
        printf("cannot obtain attribute bases\n");

    if (HiddI2CBCM2708AttrBase)
        OOP_ReleaseAttrBase((STRPTR)IID_Hidd_I2C_BCM2708);
    if (HiddI2CDeviceAttrBase)
        OOP_ReleaseAttrBase((STRPTR)IID_Hidd_I2CDevice);
    CloseLibrary(drv);

out_args:
    FreeArgs(rda);
    return rc;
}
