/*
    Copyright � 2004-2006, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <exec/types.h>
#include <hidd/hidd.h>
#include <hidd/i2c.h>
#include <oop/oop.h>

#include <utility/tagitem.h>
#include <utility/hooks.h>
#include <devices/timer.h>

#include <proto/exec.h>
#include <proto/utility.h>
#include <proto/oop.h>

#include <aros/symbolsets.h>

#include "i2c.h"

#define DEBUG 0
#include <aros/debug.h>
#include <aros/atomic.h>

/*
    There are no static AttrBases in this class. Therefore it might be placed
    directly in ROM without any harm
*/
#undef HiddI2CAttrBase
#undef HiddI2CDeviceAttrBase

#define HiddI2CAttrBase         (SD(cl)->hiddI2CAB)
#define HiddI2CDeviceAttrBase   (SD(cl)->hiddI2CDeviceAB)
#define HiddAttrBase            (SD(cl)->hiddAB)

static void I2C_UDelay(tDrvData *drv, ULONG delay)
{
    drv->mp.mp_SigTask = FindTask(NULL);
    drv->tr.tr_node.io_Command = TR_ADDREQUEST;
    drv->tr.tr_time.tv_secs = delay / 10000;
    drv->tr.tr_time.tv_micro = 10 * (delay % 10000);

    DoIO((struct IORequest *)&drv->tr);

    drv->mp.mp_SigTask = NULL;
}


static BOOL RaiseSCL(OOP_Class *cl, OOP_Object *o, BOOL sda, ULONG timeout)
{
    tDrvData *drv = (tDrvData *)OOP_INST_DATA(cl, o);

    int i;
    BOOL scl;

    I2C_PutBits(o, 1, sda);
    I2C_UDelay(drv, drv->RiseFallTime);

    for (i=timeout; i>0; i -= drv->RiseFallTime)
    {
        I2C_GetBits(o, &scl, &sda);
        if (scl) break;
        I2C_UDelay(drv, drv->RiseFallTime);
    }

    if (i <= 0)
    {
        bug("[I2C] RaiseSCL(<%s>,%d,%d) timeout\n", drv->name, sda, timeout);
        return FALSE;
    }

    return TRUE;
}

static BOOL WriteBit(OOP_Class *cl, OOP_Object *o, BOOL sda, ULONG timeout)
{
    tDrvData *drv = (tDrvData *)OOP_INST_DATA(cl, o);

    BOOL r;

    I2C_PutBits(o, 0, sda);
    I2C_UDelay(drv, drv->RiseFallTime);

    r = RaiseSCL(cl, o, sda, timeout);
    I2C_UDelay(drv, drv->HoldTime);

    I2C_PutBits(o, 0, sda);
    I2C_UDelay(drv, drv->HoldTime);

    return r;
}

static BOOL ReadBit(OOP_Class *cl, OOP_Object *o, BOOL *sda, ULONG timeout)
{
    tDrvData *drv = (tDrvData *)OOP_INST_DATA(cl, o);

    BOOL scl, r;

    r = RaiseSCL(cl, o, 1, timeout);
    I2C_UDelay(drv, drv->HoldTime);

    I2C_GetBits(o, &scl, sda);

    I2C_PutBits(o, 0, 1);
    I2C_UDelay(drv, drv->HoldTime);

    return r;
}

/*** Hidd::I2C */

void METHOD(I2C, Hidd_I2C, PutBits)
{
    bug("[I2C] Pure virtual method I2C::PutBits() called!!!\n");
}

void METHOD(I2C, Hidd_I2C, GetBits)
{
    bug("[I2C] Pure virtual method I2C::GetBits() called!!!\n");
}

BOOL METHOD(I2C, Hidd_I2C, Start)
{
    tDrvData *drv = (tDrvData *)OOP_INST_DATA(cl, o);

    LOCK_HW

    if (!RaiseSCL(cl, o, 1, msg->timeout)) {
        UNLOCK_HW
        return FALSE;
    }

    I2C_PutBits(o, 1, 0);
    I2C_UDelay(drv, drv->HoldTime);
    I2C_PutBits(o, 0, 0);
    I2C_UDelay(drv, drv->HoldTime);

    D(bug("\n[I2C]: <"));

    UNLOCK_HW

    return TRUE;
}

BOOL METHOD(I2C, Hidd_I2C, Address)
{
    tDrvData *drv = (tDrvData *)OOP_INST_DATA(cl, o);
    tDevData *dev = (tDevData *)OOP_INST_DATA(SD(cl)->i2cDeviceClass, msg->device);

    LOCK_HW

    if (I2C_Start(o, dev->StartTimeout)) {
        if (I2C_PutByte(o, msg->device, msg->address & 0xFF)) {
            if ((msg->address & 0xF8) != 0xF0 &&
                (msg->address & 0xFE) != 0x00) {
                UNLOCK_HW;
                return TRUE;
                }

            if (I2C_PutByte(o, msg->device, (msg->address >> 8) & 0xFF)) {
                UNLOCK_HW;
                return TRUE;
            }
        }

        I2C_Stop(o, msg->device);
    }

    UNLOCK_HW

    return FALSE;
}

BOOL METHOD(I2C, Hidd_I2C, ProbeAddress)
{
    tDrvData *drv = (tDrvData *)OOP_INST_DATA(cl, o);

    struct TagItem attrs[] = {
        { aHidd_I2CDevice_Driver,   (IPTR)o         },
        { aHidd_I2CDevice_Address,  msg->address    },
        { aHidd_I2CDevice_Name,     (IPTR)"Probing" },
        { TAG_DONE, 0UL }
    };

    BOOL r = FALSE;

    D(bug("[I2C] I2C::ProbeAddress(%04x)\n", msg->address));

    LOCK_HW

    OOP_Object *probing = OOP_NewObject(SD(cl)->i2cDeviceClass, NULL, attrs);

    if (probing)
    {
        r = I2C_Address(o, probing, msg->address);
        if (r)
          I2C_Stop(o, probing);

        OOP_DisposeObject(probing);
    }

    UNLOCK_HW

    return r;
}


void METHOD(I2C, Hidd_I2C, Stop)
{
    tDrvData *drv = (tDrvData *)OOP_INST_DATA(cl, o);

    LOCK_HW

    I2C_PutBits(o, 0, 0);
    I2C_UDelay(drv, drv->RiseFallTime);

    I2C_PutBits(o, 1, 0);
    I2C_UDelay(drv, drv->HoldTime);
    I2C_PutBits(o, 1, 1);
    I2C_UDelay(drv, drv->HoldTime);

    D(bug(">\n"));

    UNLOCK_HW
}

BOOL METHOD(I2C, Hidd_I2C, PutByte)
{
    tDrvData *drv = (tDrvData *)OOP_INST_DATA(cl, o);
    tDevData *dev = (tDevData *)OOP_INST_DATA(SD(cl)->i2cDeviceClass, msg->device);

    BOOL r, scl, sda;
    int i;

    LOCK_HW

    if (!WriteBit(cl, o, (msg->data >> 7) & 1, dev->ByteTimeout))
    {
        UNLOCK_HW
        return FALSE;
    }

    for (i = 6; i >= 0; i--)
        if (!WriteBit(cl, o, (msg->data >> i) & 1, dev->BitTimeout))
        {
            UNLOCK_HW
            return FALSE;
        }

    I2C_PutBits(o, 0, 1);
    I2C_UDelay(drv, drv->RiseFallTime);

    r = RaiseSCL(cl, o, 1, drv->HoldTime);

    if (r)
    {
        for (i = dev->AcknTimeout; i > 0; i -= drv->HoldTime)
        {
            I2C_UDelay(drv, drv->HoldTime);
            I2C_GetBits(o, &scl, &sda);
            if (sda == 0) break;
        }

        if (i <= 0) {
            D(bug("[I2C] PutByte(<%s>, 0x%02x, %d, %d, %d) timeout",
                                       drv->name, msg->data, dev->BitTimeout,
                                       dev->ByteTimeout, dev->AcknTimeout));
            r = FALSE;
        }

        D(bug("W%02x%c ", (int) msg->data, sda ? '-' : '+'));
    }

    I2C_PutBits(o, 0, 1);
    I2C_UDelay(drv, drv->HoldTime);

    UNLOCK_HW

    return r;
}

BOOL METHOD(I2C, Hidd_I2C, GetByte)
{
    tDrvData *drv = (tDrvData *)OOP_INST_DATA(cl, o);
    tDevData *dev = (tDevData *)OOP_INST_DATA(SD(cl)->i2cDeviceClass, msg->device);

    int i;
    BOOL sda;

    LOCK_HW

    I2C_PutBits(o, 0, 1);
    I2C_UDelay(drv, drv->RiseFallTime);

    if (!ReadBit(cl, o, &sda, dev->ByteTimeout))
    {
        UNLOCK_HW
        return FALSE;
    }


    *msg->data = (sda? 1:0) << 7;

    for (i = 6; i >= 0; i--)
        if (!ReadBit(cl, o, &sda, dev->BitTimeout))
        {
            UNLOCK_HW
            return FALSE;
        }
        else
            *msg->data |= (sda? 1:0) << i;

    if (!WriteBit(cl, o, msg->last ? 1 : 0, dev->BitTimeout))
    {
        UNLOCK_HW
        return FALSE;
    }

    D(bug("R%02x%c ", (int) *msg->data, msg->last ? '+' : '-'));

    UNLOCK_HW

    return TRUE;
}

BOOL METHOD(I2C, Hidd_I2C, WriteRead)
{
    tDrvData *drv = (tDrvData *)OOP_INST_DATA(cl, o);
    tDevData *dev = (tDevData *)OOP_INST_DATA(SD(cl)->i2cDeviceClass, msg->device);

    BOOL r = TRUE;
    int s = 0;

    ULONG nWrite = msg->writeLength;
    UBYTE *WriteBuffer = msg->writeBuffer;
    ULONG nRead = msg->readLength;
    UBYTE *ReadBuffer = msg->readBuffer;

    LOCK_HW

    if (r && nWrite > 0) {
        r = I2C_Address(o, msg->device, dev->address & ~1);
        if (r) {
            for (; nWrite > 0; WriteBuffer++, nWrite--)
                if (!(r = I2C_PutByte(o, msg->device, *WriteBuffer)))
                    break;
            s++;
        }
    }

    if (r && nRead > 0) {
        r = I2C_Address(o, msg->device, dev->address | 1);
        if (r) {
            for (; nRead > 0; ReadBuffer++, nRead--)
                if (!(r = I2C_GetByte(o, msg->device, ReadBuffer, nRead == 1)))
                    break;
            s++;
        }
    }

    if (s) I2C_Stop(o, msg->device);

    UNLOCK_HW

    return r;
}

/*** Root */

void METHOD(I2C, Root, Get)
{
    tDrvData *drv = (tDrvData *)OOP_INST_DATA(cl, o);

    ULONG idx;

    if (IS_I2C_ATTR(msg->attrID, idx))
    {
        switch (idx)
        {
            case aoHidd_I2C_HoldTime:
                *msg->storage = drv->HoldTime;
                break;

            case aoHidd_I2C_BitTimeout:
                *msg->storage = drv->BitTimeout;
                break;

            case aoHidd_I2C_ByteTimeout:
                *msg->storage = drv->ByteTimeout;
                break;

            case aoHidd_I2C_AcknTimeout:
                *msg->storage = drv->AcknTimeout;
                break;

            case aoHidd_I2C_StartTimeout:
                *msg->storage = drv->StartTimeout;
                break;

            case aoHidd_I2C_RiseFallTime:
                *msg->storage = drv->RiseFallTime;
                break;

            case aoHidd_I2C_Name:
                *msg->storage = (IPTR)drv->name;
                break;
        }
    }
    else
    {
        OOP_DoSuperMethod(cl, o, (OOP_Msg)msg);
    }
}

void METHOD(I2C, Root, Set)
{
    tDrvData *drv = (tDrvData *)OOP_INST_DATA(cl, o);

    ULONG idx;
    struct TagItem *tag;
    struct TagItem *tags = msg->attrList;

    while ((tag = NextTagItem(&tags)))
    {
        if (IS_I2C_ATTR(tag->ti_Tag, idx))
        {
            switch (idx)
            {
                case aoHidd_I2C_HoldTime:
                    drv->HoldTime = tag->ti_Data;
                    break;

                case aoHidd_I2C_BitTimeout:
                    drv->BitTimeout = tag->ti_Data;
                    break;

                case aoHidd_I2C_ByteTimeout:
                    drv->ByteTimeout = tag->ti_Data;
                    break;

                case aoHidd_I2C_AcknTimeout:
                    drv->AcknTimeout = tag->ti_Data;
                    break;

                case aoHidd_I2C_StartTimeout:
                    drv->StartTimeout = tag->ti_Data;
                    break;

                case aoHidd_I2C_RiseFallTime:
                    drv->RiseFallTime = tag->ti_Data;
                    break;
            }
        }
    }
}

OOP_Object *METHOD(I2C, Root, New)
{
    D(bug("[I2C] new()\n"));

    o = (OOP_Object *)OOP_DoSuperMethod(cl, o, (OOP_Msg) msg);
    if (o)
    {
        tDrvData *drv = (tDrvData *)OOP_INST_DATA(cl, o);
        struct TagItem *tag, *tags = msg->attrList;

        drv->HoldTime = 5;
        drv->AcknTimeout = 5;
        drv->BitTimeout = 5;
        drv->ByteTimeout = 5;
        drv->RiseFallTime = 2;
        drv->StartTimeout = 5;

        drv->name = (STRPTR)"bus?";

        InitSemaphore(&drv->driver_lock);

        D(bug("[I2C] Initializing MsgPort\n"));

        /* Initialize MsgPort */
        drv->mp.mp_SigBit = SIGB_SINGLE;
        drv->mp.mp_Flags = PA_SIGNAL;
        drv->mp.mp_SigTask = FindTask(NULL);
        drv->mp.mp_Node.ln_Type = NT_MSGPORT;
        NEWLIST(&drv->mp.mp_MsgList);

        drv->tr.tr_node.io_Message.mn_ReplyPort = &drv->mp;
        drv->tr.tr_node.io_Message.mn_Length = sizeof(drv->tr);

        D(bug("[I2C] Trying to open UNIT_MICROHZ of timer.device\n"));

        if (!OpenDevice((STRPTR)"timer.device", UNIT_MICROHZ, (struct IORequest *)&drv->tr, 0))
        {
            D(bug("[I2C] Everything OK\n"));

            tags=msg->attrList;

            while((tag = NextTagItem(&tags)))
            {
                ULONG idx;

                if (IS_I2C_ATTR(tag->ti_Tag, idx))
                {
                    switch(idx)
                    {
                        case aoHidd_I2C_Name:
                            drv->name = (STRPTR)tag->ti_Data;
                            break;

                        case aoHidd_I2C_HoldTime:
                            drv->HoldTime = tag->ti_Data;
                            break;

                        case aoHidd_I2C_BitTimeout:
                            drv->BitTimeout = tag->ti_Data;
                            break;

                        case aoHidd_I2C_ByteTimeout:
                            drv->ByteTimeout = tag->ti_Data;
                            break;

                        case aoHidd_I2C_AcknTimeout:
                            drv->AcknTimeout = tag->ti_Data;
                            break;

                        case aoHidd_I2C_StartTimeout:
                            drv->StartTimeout = tag->ti_Data;
                            break;

                        case aoHidd_I2C_RiseFallTime:
                            drv->RiseFallTime = tag->ti_Data;
                            break;
                    }
                }
            }
        }
        else
        {
            D(bug("[I2C] Opening of timer.device failed\n"));
            OOP_MethodID disp_mid = OOP_GetMethodID((STRPTR)IID_Root, moRoot_Dispose);
            OOP_CoerceMethod(cl, o, (OOP_Msg) &disp_mid);
            o = NULL;
        }

    }

    return o;
}


/* Class initialization and destruction */

//#undef UtilityBase
//#define UtilityBase (sd->utilitybase)

static int I2C_ExpungeClass(LIBBASETYPEPTR LIBBASE)
{
    D(bug("[I2C] Base Class destruction\n"));

    OOP_ReleaseAttrBase(IID_Hidd_I2CDevice);
    OOP_ReleaseAttrBase(IID_Hidd_I2C);
    OOP_ReleaseAttrBase(IID_Hidd);

    return TRUE;
}

static int I2C_InitClass(LIBBASETYPEPTR LIBBASE)
{
    D(bug("[I2C] base class initialization\n"));

    LIBBASE->sd.hiddI2CAB = OOP_ObtainAttrBase(IID_Hidd_I2C);
    LIBBASE->sd.hiddI2CDeviceAB = OOP_ObtainAttrBase(IID_Hidd_I2CDevice);
    LIBBASE->sd.hiddAB = OOP_ObtainAttrBase(IID_Hidd);

    if (LIBBASE->sd.hiddI2CAB && LIBBASE->sd.hiddI2CDeviceAB && LIBBASE->sd.hiddAB)
    {
        D(bug("[I2C] Everything OK\n"));
        return TRUE;
    }

    return FALSE;
}

ADD2INITLIB(I2C_InitClass, 0)
ADD2EXPUNGELIB(I2C_ExpungeClass, 0)
