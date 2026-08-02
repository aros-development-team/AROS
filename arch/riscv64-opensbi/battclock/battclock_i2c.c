/*
    Copyright (c) 2026, The AROS Development Team. All rights reserved.

    Desc: Reaching the RTC through the i2c HIDD.

    See battclock_i2c.h: this file is the whole of the resource's
    dependency on i2c. Nothing that knows about the RTC knows there is
    a bus under it, and nothing here knows what the registers mean.
*/

#define DEBUG 0
#include <aros/debug.h>

#include <proto/exec.h>
#include <proto/oop.h>

#include <oop/oop.h>
#include <hidd/i2c.h>
#include <hidd/dwi2c.h>

#include "battclock_i2c.h"

/* proto/oop.h reaches oop.library through a variable of this name */
struct Library *OOPBase = NULL;

/* and the attribute macros of the two interfaces through these */
OOP_AttrBase HiddI2CDeviceAttrBase = 0;
OOP_AttrBase HiddI2CDWAttrBase = 0;

/*
 * Give up on the RTC for good. Used when the bus turns out not to lead
 * anywhere after all, which leaves the resource in the state a machine
 * with no RTC at all is in - answering with what was last written to it.
 */
static void BattClock_I2CForget(struct BattClockBase *BattClockBase)
{
    if (BattClockBase->bb_I2CBus)
    {
        OOP_DisposeObject((OOP_Object *)BattClockBase->bb_I2CBus);
        BattClockBase->bb_I2CBus = NULL;
    }

    if (BattClockBase->bb_Bus)
    {
        CloseLibrary((struct Library *)BattClockBase->bb_Bus);
        BattClockBase->bb_Bus = NULL;
    }
}

/*
 * The RTC as an i2c device object, made the first time one is wanted.
 *
 * It cannot be made when the resource initialises: hidd.i2c opens
 * timer.device in its New(), and timer.device is a lower priority
 * resident than this one, so at init there is no such device yet.
 * The first read or write of the clock happens long after that.
 */
static OOP_Object *BattClock_I2CDevice(struct BattClockBase *BattClockBase)
{
    struct pHidd_I2C_ProbeAddress probe;
    struct TagItem devTags[4];
    UWORD address;

    if (BattClockBase->bb_I2CDev)
        return (OOP_Object *)BattClockBase->bb_I2CDev;

    if (!BattClockBase->bb_Bus)
        return NULL;

    if (!BattClockBase->bb_I2CBus)
    {
        struct TagItem busTags[] =
        {
            /*
             * Selected by the controller's register base rather than
             * by index: the driver enumerates controllers in its own
             * order, and reproducing that here would mean two walks of
             * the tree that have to agree forever.
             */
            { aHidd_I2C_DW_MMIOBase, (IPTR)BattClockBase->bb_BusBase },
            { TAG_DONE, 0                                     }
        };

        BattClockBase->bb_I2CBus =
            OOP_NewObject(NULL, (STRPTR)CLID_Hidd_I2C_DW, busTags);

        if (!BattClockBase->bb_I2CBus)
        {
            D(bug("[BattClock] no i2c bus at %p to talk on\n",
                  (APTR)(IPTR)BattClockBase->bb_BusBase));
            BattClock_I2CForget(BattClockBase);
            return NULL;
        }
    }

    /*
     * hidd.i2c carries a slave address in the form it takes on the
     * wire: its WriteRead sends "address & ~1" to write and
     * "address | 1" to read, so the bottom bit is the read/write bit
     * and the part's 7 bit address sits one place up.
     */
    address = (UWORD)(BattClockBase->bb_Addr << 1);

    probe.mID = OOP_GetMethodID((STRPTR)IID_Hidd_I2C, moHidd_I2C_ProbeAddress);
    probe.address = address;

    if (!OOP_DoMethod((OOP_Object *)BattClockBase->bb_I2CBus, (OOP_Msg)&probe))
    {
        D(bug("[BattClock] nothing answers at %02x on the bus at %p\n",
              BattClockBase->bb_Addr,
              (APTR)(IPTR)BattClockBase->bb_BusBase));
        BattClock_I2CForget(BattClockBase);
        return NULL;
    }

    devTags[0].ti_Tag  = aHidd_I2CDevice_Driver;
    devTags[0].ti_Data = (IPTR)BattClockBase->bb_I2CBus;
    devTags[1].ti_Tag  = aHidd_I2CDevice_Address;
    devTags[1].ti_Data = (IPTR)address;
    devTags[2].ti_Tag  = aHidd_I2CDevice_Name;
    devTags[2].ti_Data = (IPTR)"RTC";
    devTags[3].ti_Tag  = TAG_DONE;
    devTags[3].ti_Data = 0;

    BattClockBase->bb_I2CDev =
        OOP_NewObject(NULL, (STRPTR)CLID_Hidd_I2CDevice, devTags);

    if (!BattClockBase->bb_I2CDev)
    {
        D(bug("[BattClock] could not make a device object for the RTC\n"));
        BattClock_I2CForget(BattClockBase);
        return NULL;
    }

    D(bug("[BattClock] RTC at %02x on the bus at %p\n",
          BattClockBase->bb_Addr, (APTR)(IPTR)BattClockBase->bb_BusBase));

    return (OOP_Object *)BattClockBase->bb_I2CDev;
}

BOOL BattClock_I2COpen(struct BattClockBase *BattClockBase)
{
    struct Library *drv;

    if (!BattClockBase->bb_BusBase)
        return FALSE;

    if (!OOPBase)
        OOPBase = OpenLibrary("oop.library", 0);

    if (!OOPBase)
        return FALSE;

    /*
     * Opening the controller driver is what registers its class:
     * OOP_NewObject() by name only finds classes some module has
     * already added. Holding it open is also what keeps that class
     * there for as long as we might instantiate it.
     */
    drv = OpenLibrary(DWI2C_NAME, 0);
    if (!drv)
    {
        D(bug("[BattClock] no i2c controller driver\n"));
        return FALSE;
    }

    if (!HiddI2CDeviceAttrBase)
        HiddI2CDeviceAttrBase = OOP_ObtainAttrBase(IID_Hidd_I2CDevice);
    if (!HiddI2CDWAttrBase)
        HiddI2CDWAttrBase = OOP_ObtainAttrBase(IID_Hidd_I2C_DW);

    if (!HiddI2CDeviceAttrBase || !HiddI2CDWAttrBase)
    {
        D(bug("[BattClock] ObtainAttrBase failed\n"));
        CloseLibrary(drv);
        return FALSE;
    }

    /*
     * A route to the part exists. Whether the part is really on the
     * other end of it is only settled the first time the clock is
     * read, when the bus can actually be driven.
     */
    BattClockBase->bb_Bus = drv;

    return TRUE;
}

BOOL BattClock_I2CWriteRead(struct BattClockBase *BattClockBase,
                            const UBYTE *wbuf, ULONG wlen,
                            UBYTE *rbuf, ULONG rlen)
{
    struct pHidd_I2CDevice_WriteRead msg;
    OOP_Object *dev = BattClock_I2CDevice(BattClockBase);

    if (!dev)
        return FALSE;

    msg.mID = OOP_GetMethodID((STRPTR)IID_Hidd_I2CDevice,
                              moHidd_I2CDevice_WriteRead);
    msg.writeBuffer = (UBYTE *)wbuf;
    msg.writeLength = wlen;
    msg.readBuffer  = rbuf;
    msg.readLength  = rlen;

    return (BOOL)OOP_DoMethod(dev, (OOP_Msg)&msg);
}

BOOL BattClock_I2CWrite(struct BattClockBase *BattClockBase,
                        const UBYTE *wbuf, ULONG wlen)
{
    return BattClock_I2CWriteRead(BattClockBase, wbuf, wlen, NULL, 0);
}
