/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: Test dwi2c.hidd - the DesignWare i2c controllers.

          With no arguments every controller the driver found is scanned
          the way i2cdetect would. ADDR reads LEN bytes from REG. RTC
          decodes the ST M41T11 the Milk-V Titan carries on i2c@20430000
          and says what battclock.resource would make of it, SETRTC
          writes a time to it and CLEARCAL zeroes its control register.

          battclock.resource is not used, so that a wrong time can be
          pinned on the part, the bus or the resource.
*/

#include <exec/types.h>
#include <dos/dos.h>
#include <oop/oop.h>
#include <hidd/i2c.h>
#include <hidd/dwi2c.h>
#include <utility/date.h>
#include <utility/tagitem.h>

#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/oop.h>
#include <proto/utility.h>

#include <stdio.h>
#include <stdlib.h>

OOP_AttrBase HiddI2CDeviceAttrBase;
OOP_AttrBase HiddI2CDWAttrBase;

static const char version[] __attribute__((used)) =
    "$VER: I2C-DW 1.0 (13.10.2026)";

#define TEMPLATE "UNIT/N,ADDR/K,REG/K,LEN/N,RTC/S,SETRTC/K,CLEARCAL/S,ALL/S"

enum { ARG_UNIT, ARG_ADDR, ARG_REG, ARG_LEN, ARG_RTC, ARG_SETRTC,
       ARG_CLEARCAL, ARG_ALL, ARG_COUNT };

/* Table 10 of the M41T11 datasheet, kept in step with m41t11.h */
#define RTC_ADDR        0x68
#define RTC_REG_SECS    0x00    /* ST on top of the seconds             */
#define RTC_REG_MINS    0x01
#define RTC_REG_HOURS   0x02    /* CEB and CB on top of the hours       */
#define RTC_REG_WDAY    0x03
#define RTC_REG_MDAY    0x04
#define RTC_REG_MONTH   0x05
#define RTC_REG_YEAR    0x06
#define RTC_REG_CONTROL 0x07    /* OUT, FT, S, calibration              */
#define RTC_SECS_ST     0x80
#define RTC_CENTURY     2000

#define MAX_UNITS       8

static UBYTE fromBCD(UBYTE v) { return ((v >> 4) & 0x0f) * 10 + (v & 0x0f); }
static UBYTE toBCD(UBYTE v)   { return ((v / 10) << 4) | (v % 10); }

static OOP_Object *makedev(OOP_Object *bus, UBYTE addr)
{
    struct TagItem devTags[] =
    {
        { aHidd_I2CDevice_Driver,  (IPTR)bus         },
        { aHidd_I2CDevice_Address, (IPTR)(addr << 1) },
        { aHidd_I2CDevice_Name,    (IPTR)"I2C-DW"    },
        { TAG_DONE,                0                 }
    };

    return OOP_NewObject(NULL, (STRPTR)CLID_Hidd_I2CDevice, devTags);
}

static BOOL xfer(OOP_Object *dev, const UBYTE *wbuf, ULONG wlen,
                 UBYTE *rbuf, ULONG rlen)
{
    struct pHidd_I2CDevice_WriteRead msg;

    msg.mID = OOP_GetMethodID((STRPTR)IID_Hidd_I2CDevice,
                              moHidd_I2CDevice_WriteRead);
    msg.writeBuffer = (UBYTE *)wbuf;
    msg.writeLength = wlen;
    msg.readBuffer  = rbuf;
    msg.readLength  = rlen;

    return (BOOL)OOP_DoMethod(dev, (OOP_Msg)&msg);
}

/* Why the last transfer failed, which a FALSE return cannot express */
static void whyfail(OOP_Object *bus)
{
    IPTR fail = 0;

    OOP_GetAttr(bus, aHidd_I2C_DW_LastFail, &fail);

    printf("  last failure         : %08lx", (unsigned long)fail);

    if (fail == 0)
        printf(" - none recorded\n");
    else if (fail >= HIDD_I2C_DW_FAIL_FIRST)
        printf(" - THE BUS DID NOT TRANSFER (no clock, pins not muxed,\n"
               "                           no pull-ups, or SDA held low)\n");
    else if (fail & HIDD_I2C_DW_NOACK)
        printf(" - address NOACK: the bus works, nothing is there\n");
    else
        printf(" - controller aborted (IC_TX_ABRT_SOURCE)\n");
}

static BOOL probe(OOP_Object *bus, UBYTE addr)
{
    struct pHidd_I2C_ProbeAddress msg;

    msg.mID = OOP_GetMethodID((STRPTR)IID_Hidd_I2C,
                              moHidd_I2C_ProbeAddress);
    msg.address = addr << 1;

    return (BOOL)OOP_DoMethod(bus, (OOP_Msg)&msg);
}

static int scan(OOP_Object *bus)
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
            else if (probe(bus, (UBYTE)a))
            {
                printf(" %02x", a);
                found++;
            }
            else
                printf(" --");
        }
        printf("\n");
    }

    return found;
}

static int dump(OOP_Object *bus, UBYTE addr, UBYTE reg, ULONG len)
{
    OOP_Object *dev = makedev(bus, addr);
    UBYTE buf[256];
    ULONG i;
    BOOL ok;

    if (!dev)
    {
        printf("could not create a device object for %02x\n", addr);
        return RETURN_FAIL;
    }

    ok = xfer(dev, &reg, 1, buf, len);
    OOP_DisposeObject(dev);

    if (!ok)
    {
        printf("%02x: read of %u byte(s) at reg %02x failed\n", addr,
               (unsigned)len, reg);
        whyfail(bus);
        return RETURN_ERROR;
    }

    for (i = 0; i < len; i++)
        printf("%s%02x", (i % 16) ? " " : (i ? "\n" : ""), buf[i]);
    printf("\n");

    return RETURN_OK;
}

/* The clock registers as the part holds them, then as decoded */
static int showrtc(OOP_Object *bus)
{
    OOP_Object *dev = makedev(bus, RTC_ADDR);
    UBYTE reg = RTC_REG_SECS;
    UBYTE r[9];
    struct ClockData cd;
    BOOL sane, stopped;
    ULONG i;

    if (!dev)
    {
        printf("could not create a device object for the RTC\n");
        return RETURN_FAIL;
    }

    if (!xfer(dev, &reg, 1, r, sizeof(r)))
    {
        printf("the RTC at %02x did not answer\n", RTC_ADDR);
        whyfail(bus);
        OOP_DisposeObject(dev);
        return RETURN_ERROR;
    }

    OOP_DisposeObject(dev);

    printf("raw 00-08:");
    for (i = 0; i < sizeof(r); i++)
        printf(" %02x", r[i]);
    printf("   (00-06 clock, 07 control, 08 first RAM byte)\n");

    stopped = (r[RTC_REG_SECS] & RTC_SECS_ST) ? TRUE : FALSE;

    printf("ST                     : %u - oscillator %s\n", stopped ? 1 : 0,
           stopped ? "STOPPED" : "running");
    printf("century bits (reg 02)  : CEB %u CB %u\n",
           (r[RTC_REG_HOURS] >> 7) & 1, (r[RTC_REG_HOURS] >> 6) & 1);
    printf("control (reg 07)       : %02x - OUT %u FT %u S %u calibration %u\n",
           r[RTC_REG_CONTROL], (r[RTC_REG_CONTROL] >> 7) & 1,
           (r[RTC_REG_CONTROL] >> 6) & 1, (r[RTC_REG_CONTROL] >> 5) & 1,
           r[RTC_REG_CONTROL] & 0x1f);

    if (r[RTC_REG_CONTROL] & 0x3f)
        printf("  *** calibration is not zero: the clock is being pulled off\n"
               "      rate. CLEARCAL zeroes it.\n");

    cd.sec   = fromBCD(r[RTC_REG_SECS]  & 0x7f);
    cd.min   = fromBCD(r[RTC_REG_MINS]  & 0x7f);
    cd.hour  = fromBCD(r[RTC_REG_HOURS] & 0x3f);
    cd.wday  = 0;
    cd.mday  = fromBCD(r[RTC_REG_MDAY]  & 0x3f);
    cd.month = fromBCD(r[RTC_REG_MONTH] & 0x1f);
    cd.year  = RTC_CENTURY + fromBCD(r[RTC_REG_YEAR]);

    printf("decoded                : %04u-%02u-%02u %02u:%02u:%02u "
           "(weekday reg %u)\n", cd.year, cd.month, cd.mday, cd.hour,
           cd.min, cd.sec, r[RTC_REG_WDAY] & 0x07);

    sane = !(cd.sec > 59 || cd.min > 59 || cd.hour > 23 ||
             cd.mday < 1 || cd.mday > 31 ||
             cd.month < 1 || cd.month > 12);

    printf("battclock would        : %s\n",
           stopped ? "REJECT it - ST set, so the oscillator is stopped"
                   : (!sane ? "REJECT it - the registers are not a date"
                            : "accept it"));

    if (!stopped && sane)
        printf("  Date2Amiga()         : %lu seconds since 1978-01-01\n",
               (unsigned long)Date2Amiga(&cd));

    return RETURN_OK;
}

/*
 * Zero the control register. A driver with the register map shifted by
 * one writes the year over this byte, leaving a calibration value that
 * pulls the clock off rate until it is cleared.
 */
static int clearcal(OOP_Object *bus)
{
    OOP_Object *dev = makedev(bus, RTC_ADDR);
    UBYTE reg = RTC_REG_CONTROL;
    UBYTE buf[2];
    UBYTE before;

    if (!dev)
    {
        printf("could not create a device object for the RTC\n");
        return RETURN_FAIL;
    }

    if (!xfer(dev, &reg, 1, &before, 1))
    {
        printf("could not read the control register\n");
        whyfail(bus);
        OOP_DisposeObject(dev);
        return RETURN_ERROR;
    }

    buf[0] = RTC_REG_CONTROL;
    buf[1] = 0;

    if (!xfer(dev, buf, sizeof(buf), NULL, 0))
    {
        printf("could not write the control register\n");
        OOP_DisposeObject(dev);
        return RETURN_ERROR;
    }

    OOP_DisposeObject(dev);

    printf("control register %02x -> 00\n", before);

    return RETURN_OK;
}

/* Write a time the way writebattclock.c does, to exercise the same path */
static int setrtc(OOP_Object *bus, const char *spec)
{
    OOP_Object *dev;
    struct ClockData cd;
    UBYTE buf[8];       /* one register address, then the seven clock regs */
    unsigned Y, M, D, h, m, s;

    if (sscanf(spec, "%u-%u-%u/%u:%u:%u", &Y, &M, &D, &h, &m, &s) != 6)
    {
        printf("SETRTC wants YYYY-MM-DD/HH:MM:SS\n");
        return RETURN_ERROR;
    }

    if (Y < RTC_CENTURY || Y >= RTC_CENTURY + 100)
    {
        printf("%u is outside the %u-%u the part can hold\n", Y,
               RTC_CENTURY, RTC_CENTURY + 99);
        return RETURN_ERROR;
    }

    cd.year = Y; cd.month = M; cd.mday = D;
    cd.hour = h; cd.min = m; cd.sec = s;
    cd.wday = 0;

    /* The part does not interpret the weekday, but it should still agree */
    {
        struct ClockData back;
        Amiga2Date(Date2Amiga(&cd), &back);
        cd.wday = back.wday;
    }

    dev = makedev(bus, RTC_ADDR);
    if (!dev)
    {
        printf("could not create a device object for the RTC\n");
        return RETURN_FAIL;
    }

    buf[0] = RTC_REG_SECS;
    buf[1] = toBCD((UBYTE)cd.sec);      /* ST clear: the oscillator runs */
    buf[2] = toBCD((UBYTE)cd.min);
    buf[3] = toBCD((UBYTE)cd.hour);     /* CEB and CB left clear         */
    buf[4] = (UBYTE)((cd.wday + 1) & 0x07);
    buf[5] = toBCD((UBYTE)cd.mday);
    buf[6] = toBCD((UBYTE)cd.month);
    buf[7] = toBCD((UBYTE)(cd.year - RTC_CENTURY));

    /* The same kick-start the resource does */
    {
        UBYTE reg = RTC_REG_SECS, cur;

        if (xfer(dev, &reg, 1, &cur, 1) && (cur & RTC_SECS_ST))
        {
            UBYTE kick[2];

            printf("the oscillator is stopped - kick-starting it\n");
            kick[0] = RTC_REG_SECS;
            kick[1] = RTC_SECS_ST;
            xfer(dev, kick, sizeof(kick), NULL, 0);
        }
    }

    printf("writing   :");
    {
        ULONG i;
        for (i = 1; i < sizeof(buf); i++)
            printf(" %02x", buf[i]);
    }
    printf("\n");

    if (!xfer(dev, buf, sizeof(buf), NULL, 0))
    {
        printf("the RTC would not take the time\n");
        OOP_DisposeObject(dev);
        return RETURN_ERROR;
    }

    OOP_DisposeObject(dev);

    printf("written, reading it back:\n");

    return showrtc(bus);
}

int main(void)
{
    IPTR args[ARG_COUNT] = { 0 };
    struct RDArgs *rda;
    struct Library *drv;
    ULONG len = 1;
    LONG only = -1;
    int rc = RETURN_FAIL;
    ULONG unit;

    rda = ReadArgs(TEMPLATE, args, NULL);
    if (!rda)
    {
        PrintFault(IoErr(), "I2C-DW");
        return RETURN_FAIL;
    }

    if (args[ARG_UNIT])
        only = *(LONG *)args[ARG_UNIT];
    if (args[ARG_LEN])
        len = *(LONG *)args[ARG_LEN];
    if (len < 1 || len > 256)
    {
        printf("LEN must be 1..256\n");
        goto out_args;
    }

    /* Opening the driver is what registers its class */
    drv = OpenLibrary(DWI2C_NAME, 0);
    if (!drv)
    {
        printf("cannot open %s - no DesignWare i2c controller described?\n",
               DWI2C_NAME);
        goto out_args;
    }

    HiddI2CDeviceAttrBase = OOP_ObtainAttrBase((STRPTR)IID_Hidd_I2CDevice);
    HiddI2CDWAttrBase     = OOP_ObtainAttrBase((STRPTR)IID_Hidd_I2C_DW);

    if (!HiddI2CDeviceAttrBase || !HiddI2CDWAttrBase)
    {
        printf("cannot obtain the attribute bases\n");
        goto out_drv;
    }

    /* With no UNIT given every one is tried; a bus object for one that
       does not exist fails to be made, which ends the list */
    for (unit = 0; unit < MAX_UNITS; unit++)
    {
        struct TagItem busTags[] =
        {
            { aHidd_I2C_DW_Unit, (IPTR)unit },
            { TAG_DONE,          0          }
        };
        OOP_Object *bus;

        if (only >= 0 && (LONG)unit != only)
            continue;

        bus = OOP_NewObject(NULL, (STRPTR)CLID_Hidd_I2C_DW, busTags);
        if (!bus)
        {
            if (only >= 0)
                printf("there is no unit %ld\n", only);
            continue;
        }

        {
            IPTR base = 0;

            OOP_GetAttr(bus, aHidd_I2C_DW_MMIOBase, &base);
            printf("\n=== i2c unit %u, controller at %p ===\n",
                   (unsigned)unit, (APTR)base);
        }

        if (args[ARG_CLEARCAL])
            rc = clearcal(bus);
        else if (args[ARG_SETRTC])
            rc = setrtc(bus, (const char *)args[ARG_SETRTC]);
        else if (args[ARG_RTC])
            rc = showrtc(bus);
        else if (args[ARG_ADDR])
            rc = dump(bus,
                      (UBYTE)strtoul((char *)args[ARG_ADDR], NULL, 0),
                      args[ARG_REG]
                          ? (UBYTE)strtoul((char *)args[ARG_REG], NULL, 0)
                          : 0,
                      len);
        else
        {
            int found = scan(bus);

            printf("%d device(s)\n", found);
            if (!found)
                whyfail(bus);
            rc = RETURN_OK;
        }

        OOP_DisposeObject(bus);

        /* Walking on would write to whatever answers at 68 on the next
           bus; ALL says to carry on regardless */
        if (rc == RETURN_OK && !args[ARG_ALL] &&
            (args[ARG_RTC] || args[ARG_SETRTC] || args[ARG_CLEARCAL] ||
             args[ARG_ADDR]))
            break;
    }

out_drv:
    if (HiddI2CDWAttrBase)
        OOP_ReleaseAttrBase((STRPTR)IID_Hidd_I2C_DW);
    if (HiddI2CDeviceAttrBase)
        OOP_ReleaseAttrBase((STRPTR)IID_Hidd_I2CDevice);
    CloseLibrary(drv);

out_args:
    FreeArgs(rda);
    return rc;
}
