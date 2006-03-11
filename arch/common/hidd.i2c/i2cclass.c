/*
    Copyright © 2004, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <exec/types.h>
#include <hidd/hidd.h>
#include <hidd/i2c.h>
#include <oop/oop.h>

#include <utility/tagitem.h>
#include <utility/hooks.h>

#include <proto/exec.h>
#include <proto/utility.h>
#include <proto/oop.h>

#include <aros/symbolsets.h>

#include "i2c.h"

#define DEBUG 1
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

static void I2C_UDelay(ULONG delay)
{
    volatile long cnt;
    if (delay > 0)
        for (cnt = delay * 500; cnt > 0; cnt--);
}


static BOOL RaiseSCL(OOP_Class *cl, OOP_Object *o, BOOL sda, ULONG timeout)
{
    int i;
    BOOL scl;
    
    I2C_PutBits(o, 1, sda);
    I2C_UDelay(SD(cl)->RiseFallTime);
    
    for (i=timeout; i>0; i -= SD(cl)->RiseFallTime)
    {
        I2C_GetBits(o, &scl, &sda);
        if (scl) break;
        I2C_UDelay(SD(cl)->RiseFallTime);
    }
    
    if (i <= 0)
    {
        bug("[I2C] RaiseSCL(<%s>,%d,%d) timeout\n", SD(cl)->name, sda, timeout);
        return FALSE;
    }
    
    return TRUE;
}

static BOOL WriteBit(OOP_Class *cl, OOP_Object *o, BOOL sda, ULONG timeout)
{
    BOOL r;
    
    I2C_PutBits(o, 0, sda);
    I2C_UDelay(SD(cl)->RiseFallTime);
    
    r = RaiseSCL(cl, o, sda, timeout);
    I2C_UDelay(SD(cl)->HoldTime);

    I2C_PutBits(o, 0, sda);
    I2C_UDelay(SD(cl)->HoldTime);

    return r;
}

static BOOL ReadBit(OOP_Class *cl, OOP_Object *o, BOOL *sda, ULONG timeout)
{
    BOOL scl, r;
    
    r = RaiseSCL(cl, o, 1, timeout);
    I2C_UDelay(SD(cl)->HoldTime);
    
    I2C_GetBits(o, &scl, sda);

    I2C_PutBits(o, 0, 1);
    I2C_UDelay(SD(cl)->HoldTime);
    
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
    LOCK_HW
    
    if (!RaiseSCL(cl, o, 1, msg->timeout))
        return FALSE;
    
    I2C_PutBits(o, 1, 0);
    I2C_UDelay(SD(cl)->HoldTime);
    I2C_PutBits(o, 0, 0);
    I2C_UDelay(SD(cl)->HoldTime);

    D(bug("\n[I2C]: <"));

    return TRUE;    
    
    UNLOCK_HW    
}

BOOL METHOD(I2C, Hidd_I2C, Address)
{
    tDevData *dev = (tDevData *)OOP_INST_DATA(SD(cl)->i2cDeviceClass, msg->device);

    LOCK_HW

    if (I2C_Start(o, dev->StartTimeout)) {
        if (I2C_PutByte(o, msg->device, msg->address & 0xFF)) {
            if ((msg->address & 0xF8) != 0xF0 &&
                (msg->address & 0xFE) != 0x00)
                return TRUE;

            if (I2C_PutByte(o, msg->device, (msg->address >> 8) & 0xFF))
                return TRUE;
        }

        I2C_Stop(o, msg->device);
    }

    UNLOCK_HW    

    return FALSE;
}

BOOL METHOD(I2C, Hidd_I2C, ProbeAddress)
{
    struct TagItem attrs[] = {
        { aHidd_I2CDevice_Driver,   (IPTR)o         },
        { aHidd_I2CDevice_Address,  msg->address    },
        { aHidd_I2CDevice_Name,     (IPTR)"Probing" },
        { TAG_DONE, 0UL }
    };
    
    BOOL r = FALSE;

    D(bug("[I2C] I2C::ProbeAddress(%04x)\n", msg->address));
        
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
    LOCK_HW

    I2C_PutBits(o, 0, 0);
    I2C_UDelay(SD(cl)->RiseFallTime);
    
    I2C_PutBits(o, 1, 0);
    I2C_UDelay(SD(cl)->HoldTime);
    I2C_PutBits(o, 1, 1);
    I2C_UDelay(SD(cl)->HoldTime);

    D(bug(">\n"));
    
    UNLOCK_HW    
}

BOOL METHOD(I2C, Hidd_I2C, PutByte)
{
    BOOL r, scl, sda;
    int i;
    tDevData *dev = (tDevData *)OOP_INST_DATA(SD(cl)->i2cDeviceClass, msg->device);

    LOCK_HW
    
    if (!WriteBit(cl, o, (msg->data >> 7) & 1, dev->ByteTimeout))
        return FALSE;

    for (i = 6; i >= 0; i--)
        if (!WriteBit(cl, o, (msg->data >> i) & 1, dev->BitTimeout))
            return FALSE;

    I2C_PutBits(o, 0, 1);
    I2C_UDelay(SD(cl)->RiseFallTime);

    r = RaiseSCL(cl, o, 1, SD(cl)->HoldTime);

    if (r)
    {
        for (i = dev->AcknTimeout; i > 0; i -= SD(cl)->HoldTime)
        {
            I2C_UDelay(SD(cl)->HoldTime);
            I2C_GetBits(o, &scl, &sda);
            if (sda == 0) break;
        }

        if (i <= 0) {
            bug("[I2C] PutByte(<%s>, 0x%02x, %d, %d, %d) timeout",
                                       SD(cl)->name, msg->data, dev->BitTimeout,
                                       dev->ByteTimeout, dev->AcknTimeout);
            r = FALSE;
        }
        
        D(bug("W%02x%c ", (int) msg->data, sda ? '-' : '+'));
    }

    I2C_PutBits(o, 0, 1);
    I2C_UDelay(SD(cl)->HoldTime);
    
    UNLOCK_HW
    
    return r;
}

BOOL METHOD(I2C, Hidd_I2C, GetByte)
{
    int i;
    BOOL sda;
    tDevData *dev = (tDevData *)OOP_INST_DATA(SD(cl)->i2cDeviceClass, msg->device);
    
    LOCK_HW

    I2C_PutBits(o, 0, 1);
    I2C_UDelay(SD(cl)->RiseFallTime);

    if (!ReadBit(cl, o, &sda, dev->ByteTimeout))
        return FALSE;

    *msg->data = (sda? 1:0) << 7;

    for (i = 6; i >= 0; i--)
        if (!ReadBit(cl, o, &sda, dev->BitTimeout))
            return FALSE;
        else
            *msg->data |= (sda? 1:0) << i;

    if (!WriteBit(cl, o, msg->last ? 1 : 0, dev->BitTimeout))
        return FALSE;

    D(bug("R%02x%c ", (int) *msg->data, msg->last ? '+' : '-'));
    
    UNLOCK_HW    

    return TRUE;
}

BOOL METHOD(I2C, Hidd_I2C, WriteRead)
{
    BOOL r = TRUE;
    int s = 0;
    tDevData *dev = (tDevData *)OOP_INST_DATA(SD(cl)->i2cDeviceClass, msg->device);

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

    return r;
    
    UNLOCK_HW    
}

/*** Root */

void METHOD(I2C, Root, Get)
{
    ULONG idx;

    if (IS_I2C_ATTR(msg->attrID, idx))
    {
        switch (idx)
        {
            case aoHidd_I2C_HoldTime:
                *msg->storage = SD(cl)->HoldTime;
                break;
                    
            case aoHidd_I2C_BitTimeout:
                *msg->storage = SD(cl)->BitTimeout;
                break;

            case aoHidd_I2C_ByteTimeout:
                *msg->storage = SD(cl)->ByteTimeout;
                break;

            case aoHidd_I2C_AcknTimeout:
                *msg->storage = SD(cl)->AcknTimeout;
                break;

            case aoHidd_I2C_StartTimeout:
                *msg->storage = SD(cl)->StartTimeout;
                break;

            case aoHidd_I2C_RiseFallTime:
                *msg->storage = SD(cl)->RiseFallTime;
                break;
            
            case aoHidd_I2C_Name:
                *msg->storage = (IPTR)SD(cl)->name;
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
    ULONG idx;
    struct TagItem *tag;
    const struct TagItem *tags = msg->attrList;

    while ((tag = NextTagItem(&tags)))
    {
        if (IS_I2C_ATTR(tag->ti_Tag, idx))
        {
            switch (idx)
            {
                case aoHidd_I2C_HoldTime:
                    SD(cl)->HoldTime = tag->ti_Data;
                    break;
                    
                case aoHidd_I2C_BitTimeout:
                    SD(cl)->BitTimeout = tag->ti_Data;
                    break;

                case aoHidd_I2C_ByteTimeout:
                    SD(cl)->ByteTimeout = tag->ti_Data;
                    break;

                case aoHidd_I2C_AcknTimeout:
                    SD(cl)->AcknTimeout = tag->ti_Data;
                    break;

                case aoHidd_I2C_StartTimeout:
                    SD(cl)->StartTimeout = tag->ti_Data;
                    break;

                case aoHidd_I2C_RiseFallTime:
                    SD(cl)->RiseFallTime = tag->ti_Data;
                    break;
            }
        }
    }
}

OOP_Object *METHOD(I2C, Root, New)
{
    o = (OOP_Object *)OOP_DoSuperMethod(cl, o, (OOP_Msg) msg);
    if (o)
    {
        struct TagItem *tag;
        const struct TagItem *tags = msg->attrList;

        SD(cl)->HoldTime = 5;
        SD(cl)->AcknTimeout = 5;
        SD(cl)->BitTimeout = 5;
        SD(cl)->ByteTimeout = 5;
        SD(cl)->RiseFallTime = 2;
        SD(cl)->StartTimeout = 5;
        
        SD(cl)->name = (STRPTR)"bus?";

        tags=msg->attrList;

        while((tag = NextTagItem(&tags)))
        {
            ULONG idx;
        
            if (IS_I2C_ATTR(tag->ti_Tag, idx))
            {
                switch(idx)
                {
                    case aoHidd_I2C_Name:
                        SD(cl)->name = (STRPTR)tag->ti_Data;
                        break;
            
                    case aoHidd_I2C_HoldTime:
                        SD(cl)->HoldTime = tag->ti_Data;
                        break;
        
                    case aoHidd_I2C_BitTimeout:
                        SD(cl)->BitTimeout = tag->ti_Data;
                        break;
        
                    case aoHidd_I2C_ByteTimeout:
                        SD(cl)->ByteTimeout = tag->ti_Data;
                        break;
                    
                    case aoHidd_I2C_AcknTimeout:
                        SD(cl)->AcknTimeout = tag->ti_Data;
                        break;
        
                    case aoHidd_I2C_StartTimeout:
                        SD(cl)->StartTimeout = tag->ti_Data;
                        break;
        
                    case aoHidd_I2C_RiseFallTime:
                        SD(cl)->RiseFallTime = tag->ti_Data;
                        break;
                }
            }
        }
    }

    return o;
}


/* Class initialization and destruction */

//#undef UtilityBase
//#define UtilityBase (sd->utilitybase)

AROS_SET_LIBFUNC(I2C_ExpungeClass, LIBBASETYPE, LIBBASE)
{
    AROS_SET_LIBFUNC_INIT

    D(bug("[I2C] Base Class destruction\n"));

    OOP_ReleaseAttrBase((STRPTR)IID_Hidd_I2C);
    OOP_ReleaseAttrBase((STRPTR)IID_Hidd_I2C);
    OOP_ReleaseAttrBase((STRPTR)IID_Hidd);

    return TRUE;
                                
    AROS_SET_LIBFUNC_EXIT
}

AROS_SET_LIBFUNC(I2C_InitClass, LIBBASETYPE, LIBBASE)
{
    AROS_SET_LIBFUNC_INIT
    
    D(bug("[I2C] base class initialization\n"));
    
    LIBBASE->sd.hiddI2CAB = OOP_ObtainAttrBase((STRPTR)IID_Hidd_I2C);
    LIBBASE->sd.hiddI2CDeviceAB = OOP_ObtainAttrBase((STRPTR)IID_Hidd_I2CDevice);
    LIBBASE->sd.hiddAB = OOP_ObtainAttrBase((STRPTR)IID_Hidd);
    
    if (LIBBASE->sd.hiddI2CAB && LIBBASE->sd.hiddI2CDeviceAB && LIBBASE->sd.hiddAB)
    {
        D(bug("[I2C] Everything OK\n"));
        return TRUE;
    }
    
    return FALSE;
    
    AROS_SET_LIBFUNC_EXIT
}

ADD2INITLIB(I2C_InitClass, 0)
ADD2EXPUNGELIB(I2C_ExpungeClass, 0)
