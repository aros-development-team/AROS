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

#define UtilityBase             (SD(cl)->utilitybase)

/*** Hidd::I2C */

void METHOD(I2C, Hidd_I2C, UDelay)
{
}

void METHOD(I2C, Hidd_I2C, PutBits)
{
}

void METHOD(I2C, Hidd_I2C, GetBits)
{
}

BOOL METHOD(I2C, Hidd_I2C, Start)
{
}

BOOL METHOD(I2C, Hidd_I2C, Address)
{
}

void METHOD(I2C, Hidd_I2C, Stop)
{
}

BOOL METHOD(I2C, Hidd_I2C, PutByte)
{
}

BOOL METHOD(I2C, Hidd_I2C, GetByte)
{
}

BOOL METHOD(I2C, Hidd_I2C, WriteRead)
{
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
    struct TagItem *tags, *tag;

    tags = msg->attrList;

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

/* Class initialization and destruction */

#undef UtilityBase
#define UtilityBase (sd->utilitybase)

AROS_SET_LIBFUNC(I2C_ExpungeClass, LIBBASETYPE, LIBBASE)
{
    AROS_SET_LIBFUNC_INIT

    D(bug("[I2C] Base Class destruction\n"));

    OOP_ReleaseAttrBase(IID_Hidd_I2C);
    OOP_ReleaseAttrBase(IID_Hidd_I2C);
    OOP_ReleaseAttrBase(IID_Hidd);

    return TRUE;
                                
    AROS_SET_LIBFUNC_EXIT
}

AROS_SET_LIBFUNC(I2C_InitClass, LIBBASETYPE, LIBBASE)
{
    AROS_SET_LIBFUNC_INIT
    
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
    
    AROS_SET_LIBFUNC_EXIT
}

ADD2INITLIB(I2C_InitClass, 0)
ADD2EXPUNGELIB(I2C_ExpungeClass, 0)
