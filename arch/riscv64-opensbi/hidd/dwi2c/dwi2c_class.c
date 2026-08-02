#define DEBUG 1
/*
    Copyright (c) 2026, The AROS Development Team. All rights reserved.
    $Id$

    Desc: The bus class - a hidd.i2c driven by a DesignWare controller.

    hidd.i2c was written around a bus made of two wires that the driver
    wiggles itself: its Start, Address, PutByte and GetByte are built on
    PutBits/GetBits, and its WriteRead is built on those in turn. A
    controller works the other way round - it is handed a whole
    transaction and produces the signalling - so what this class
    overrides is the top of that stack rather than the bottom.

    WriteRead is the important one. Every method of hidd.i2c.device
    (ReadByte, WriteBytes, ReadWord and the rest) turns into a WriteRead
    on the bus object, so overriding it once gives a client the entire
    device interface.

    The byte-at-a-time methods below the transaction level are overridden
    only to fail. The controller cannot express them: it needs to know
    where the STOP goes when the transaction is queued, and by the time
    a caller asks for a byte that decision has been made.
*/

#include <aros/debug.h>

#include <proto/exec.h>
#include <proto/oop.h>
#include <proto/utility.h>

#include <aros/symbolsets.h>

#include <exec/types.h>
#include <hidd/hidd.h>
#include <hidd/i2c.h>
#include <utility/tagitem.h>

#include LC_LIBDEFS_FILE
#include "dwi2c_intern.h"

static CONST_STRPTR dwi2cHWName = "DesignWare i2c controller";

/*
 * A bus object stands for one of the controllers found at init, and
 * which one is settled here and never changes. Nothing is instantiated
 * before a client asks: the base class opens timer.device in its New(),
 * and a kickstart module's init can run before there is one.
 */
OOP_Object *DWI2C__Root__New(OOP_Class *cl, OOP_Object *o,
                             struct pRoot_New *msg)
{
    struct dwi2c_staticdata *psd = PSD(cl);
    struct pRoot_New superMsg;
    struct TagItem superTags[2];
    OOP_Object *busObj;
    IPTR unit;

    D(bug("[DWI2C:Bus] %s()\n", __func__);)

    unit = GetTagData(aHidd_I2C_DW_Unit, (IPTR)-1, msg->attrList);

    /*
     * Selecting by register base instead: look up which controller
     * that is, so the client does not have to know the order they were
     * discovered in.
     */
    if (unit == (IPTR)-1)
    {
        IPTR base = GetTagData(aHidd_I2C_DW_MMIOBase, 0, msg->attrList);
        ULONG i;

        if (base)
        {
            for (i = 0; i < psd->ctrlCount; i++)
            {
                if ((IPTR)psd->ctrl[i].base == base)
                {
                    unit = (IPTR)i;
                    break;
                }
            }
        }
    }

    if (unit >= (IPTR)psd->ctrlCount)
    {
        D(bug("[DWI2C:Bus] %s: no unit %ld to be a bus for\n", __func__,
              (LONG)unit);)
        return NULL;
    }

    /*
     * The base class keeps the name it is handed rather than copying
     * it, so give it the driver's own - which lives as long as the
     * driver does - instead of leaving the lifetime to the client.
     * Everything else the client asked for follows unchanged.
     */
    superTags[0].ti_Tag  = aHidd_I2C_Name;
    superTags[0].ti_Data = (IPTR)psd->ctrl[unit].name;
    superTags[1].ti_Tag  = TAG_MORE;
    superTags[1].ti_Data = (IPTR)msg->attrList;

    superMsg.mID      = msg->mID;
    superMsg.attrList = superTags;

    busObj = (OOP_Object *)OOP_DoSuperMethod(cl, o, (OOP_Msg)&superMsg);
    if (busObj)
    {
        struct dwi2c_busdata *data = OOP_INST_DATA(cl, busObj);

        data->ctrl = &psd->ctrl[unit];

        D(bug("[DWI2C:Bus] %s: object @ 0x%p for %s\n", __func__, busObj,
              data->ctrl->name);)
    }

    return busObj;
}

void DWI2C__Root__Get(OOP_Class *cl, OOP_Object *o, struct pRoot_Get *msg)
{
    struct dwi2c_busdata *data = OOP_INST_DATA(cl, o);
    ULONG idx;

    if (IS_HIDD_ATTR(msg->attrID, idx) && idx == aoHidd_HardwareName)
    {
        *msg->storage = (IPTR)dwi2cHWName;
        return;
    }

    if (IS_I2CDW_ATTR(msg->attrID, idx) && idx == aoHidd_I2C_DW_Unit)
    {
        *msg->storage = (IPTR)(data->ctrl - PSD(cl)->ctrl);
        return;
    }

    OOP_DoSuperMethod(cl, o, (OOP_Msg)msg);
}

/*
 * The write-then-read transaction, and with either half left out, the
 * plain write and the plain read.
 *
 * The address comes from the device object the caller passes, exactly
 * as the base class takes it, except that the controller wants it
 * unshifted: the base class ORs in the read/write bit by hand because
 * it puts the address byte on the wire itself, while here the
 * controller does that.
 */
BOOL DWI2C__Hidd_I2C__WriteRead(OOP_Class *cl, OOP_Object *o,
                                struct pHidd_I2C_WriteRead *msg)
{
    struct dwi2c_busdata *data = OOP_INST_DATA(cl, o);
    IPTR address = 0;
    BOOL ok;

    if (!data->ctrl)
        return FALSE;

    if (msg->device)
        OOP_GetAttr(msg->device, aHidd_I2CDevice_Address, &address);

    ObtainSemaphore(&data->ctrl->lock);

    ok = DWI2C_HWTransfer(data->ctrl, (UWORD)((address >> 1) & 0x7f),
                          msg->writeBuffer, msg->writeLength,
                          msg->readBuffer, msg->readLength);

    ReleaseSemaphore(&data->ctrl->lock);

    return ok;
}

/*
 * Is anything at this address? A one byte read is the least intrusive
 * question that still makes the controller put the address on the bus
 * and wait for an acknowledge.
 */
BOOL DWI2C__Hidd_I2C__ProbeAddress(OOP_Class *cl, OOP_Object *o,
                                   struct pHidd_I2C_ProbeAddress *msg)
{
    struct dwi2c_busdata *data = OOP_INST_DATA(cl, o);
    UBYTE dummy = 0;
    BOOL ok;

    if (!data->ctrl)
        return FALSE;

    ObtainSemaphore(&data->ctrl->lock);

    ok = DWI2C_HWTransfer(data->ctrl, (UWORD)((msg->address >> 1) & 0x7f),
                          NULL, 0, &dummy, 1);

    ReleaseSemaphore(&data->ctrl->lock);

    D(bug("[DWI2C:Bus] %s: %04x %s\n", __func__, msg->address,
          ok ? "answered" : "silent");)

    return ok;
}

/*
 * Below this line are the parts of the interface a controller cannot
 * provide. They exist so that a caller which reaches for them gets a
 * refusal and a line in the log, instead of falling through to the base
 * class and finding its way to the unimplemented PutBits/GetBits.
 */

BOOL DWI2C__Hidd_I2C__Start(OOP_Class *cl, OOP_Object *o,
                            struct pHidd_I2C_Start *msg)
{
    D(bug("[DWI2C:Bus] %s: not possible on a controller, use WriteRead\n",
          __func__);)

    return FALSE;
}

void DWI2C__Hidd_I2C__Stop(OOP_Class *cl, OOP_Object *o,
                           struct pHidd_I2C_Stop *msg)
{
    D(bug("[DWI2C:Bus] %s: not possible on a controller, use WriteRead\n",
          __func__);)
}

BOOL DWI2C__Hidd_I2C__Address(OOP_Class *cl, OOP_Object *o,
                              struct pHidd_I2C_Address *msg)
{
    D(bug("[DWI2C:Bus] %s: not possible on a controller, use WriteRead\n",
          __func__);)

    return FALSE;
}

BOOL DWI2C__Hidd_I2C__PutByte(OOP_Class *cl, OOP_Object *o,
                              struct pHidd_I2C_PutByte *msg)
{
    D(bug("[DWI2C:Bus] %s: not possible on a controller, use WriteRead\n",
          __func__);)

    return FALSE;
}

BOOL DWI2C__Hidd_I2C__GetByte(OOP_Class *cl, OOP_Object *o,
                              struct pHidd_I2C_GetByte *msg)
{
    D(bug("[DWI2C:Bus] %s: not possible on a controller, use WriteRead\n",
          __func__);)

    return FALSE;
}
