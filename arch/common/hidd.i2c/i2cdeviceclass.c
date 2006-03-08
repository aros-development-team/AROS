/*
    Copyright © 2004, The AROS Development Team. All rights reserved.
    $Id$

    Desc: PCI device class
    Lang: English
*/

#include <exec/types.h>
#include <hidd/i2c.h>
#include <oop/oop.h>

#include <utility/tagitem.h>

#include <proto/exec.h>
#include <proto/utility.h>
#include <proto/oop.h>

#include "i2c.h"

#define DEBUG 1
#include <aros/debug.h>

#ifdef HiddI2CDeviceAttrBase
#undef HiddI2CDeviceAttrBase
#endif // HiddPCIDeviceAttrBase

#define	HiddI2CDeviceAttrBase	(SD(cl)->hiddI2CDeviceAB)
#define UtilityBase             (SD(cl)->utilitybase)

BOOL METHOD(I2CDev, Hidd_I2CDevice, Read)
{
    
}

BOOL METHOD(I2CDev, Hidd_I2CDevice, ReadByte)
{
    
}

BOOL METHOD(I2CDev, Hidd_I2CDevice, ReadBytes)
{
    
}

BOOL METHOD(I2CDev, Hidd_I2CDevice, ReadStatus)
{
    
}

BOOL METHOD(I2CDev, Hidd_I2CDevice, ReadWord)
{
    
}

BOOL METHOD(I2CDev, Hidd_I2CDevice, Write)
{
    
}

BOOL METHOD(I2CDev, Hidd_I2CDevice, WriteByte)
{
    
}

BOOL METHOD(I2CDev, Hidd_I2CDevice, WriteBytes)
{
    
}

BOOL METHOD(I2CDev, Hidd_I2CDevice, WriteWord)
{
    
}

BOOL METHOD(I2CDev, Hidd_I2CDevice, WriteVec)
{
    
}

/*** Root */
OOP_Object *METHOD(I2CDev, Root, New)
{
    o = (OOP_Object *)OOP_DoSuperMethod(cl, o, (OOP_Msg) msg);
    if (o)
    {
        struct TagItem *tags, *tag;
        tDevData *dev = (tDevData *)OOP_INST_DATA(cl, o);
        OOP_Object *driver = NULL;
        UWORD address;
                
        tags=msg->attrList;
    
        while((tag = NextTagItem((struct TagItem **)&tags)))
        {
            ULONG idx;
        
            if (IS_I2CDEV_ATTR(tag->ti_Tag, idx))
            {
                switch(idx)
                {
                    case aoHidd_I2CDevice_Driver:
                        dev->driver = (OOP_Object*)tag->ti_Data;
                        driver = dev->driver;
                        break;
            
                    case aoHidd_I2CDevice_Address:
                        dev->address = tag->ti_Data;
                        address = dev->address;
                        break;
        
                    case aoHidd_I2CDevice_HoldTime:
                        dev->HoldTime = tag->ti_Data;
                        break;
        
                    case aoHidd_I2CDevice_BitTimeout:
                        dev->BitTimeout = tag->ti_Data;
                        break;
        
                    case aoHidd_I2CDevice_ByteTimeout:
                        dev->ByteTimeout = tag->ti_Data;
                        break;
                    
                    case aoHidd_I2CDevice_AcknTimeout:
                        dev->AcknTimeout = tag->ti_Data;
                        break;
        
                    case aoHidd_I2CDevice_StartTimeout:
                        dev->StartTimeout = tag->ti_Data;
                        break;
        
                    case aoHidd_I2CDevice_RiseFallTime:
                        dev->RiseFallTime = tag->ti_Data;
                        break;
                }
            }
        }
        
        if (driver)
        {
        }
        else
        {
             OOP_MethodID disp_mid = OOP_GetMethodID(IID_Root, moRoot_Dispose);
             OOP_CoerceMethod(cl, o, (OOP_Msg) &disp_mid);
                            
        }
    }

    return o;
}

void METHOD(I2CDev, Root, Get)
{
    tDevData *dev = (tDevData *)OOP_INST_DATA(cl, o);
    ULONG idx;
    
    if (IS_I2CDEV_ATTR(msg->attrID, idx))
    {
        switch (idx)
        {
            case aoHidd_I2CDevice_Driver:
                *msg->storage = (IPTR)dev->driver;
                break;
        
            case aoHidd_I2CDevice_Address:
                *msg->storage = dev->address;
        
            case aoHidd_I2CDevice_HoldTime:
                *msg->storage = dev->HoldTime;
                break;
                    
            case aoHidd_I2CDevice_BitTimeout:
                *msg->storage = dev->BitTimeout;
                break;
        
            case aoHidd_I2CDevice_ByteTimeout:
                *msg->storage = dev->ByteTimeout;
                break;
        
            case aoHidd_I2CDevice_AcknTimeout:
                *msg->storage = dev->AcknTimeout;
                break;
        
            case aoHidd_I2CDevice_StartTimeout:
                *msg->storage = dev->StartTimeout;
                break;
        
            case aoHidd_I2CDevice_RiseFallTime:
                *msg->storage = dev->RiseFallTime;
                break;
        }
    }
    else
    {
        OOP_DoSuperMethod(cl, o, (OOP_Msg)msg);
    }
}

void METHOD(I2CDev, Root, Set)
{
    tDevData *dev = (tDevData *)OOP_INST_DATA(cl, o);
    ULONG idx;
    struct TagItem *tags, *tag;
    
    tags = msg->attrList;
    
    while ((tag = NextTagItem(&tags)))
    {
        if (IS_I2CDEV_ATTR(tag->ti_Tag, idx))
        {
            switch (idx)
            {
                case aoHidd_I2CDevice_HoldTime:
                    dev->HoldTime = tag->ti_Data;
                    break;
                    
                case aoHidd_I2CDevice_BitTimeout:
                    dev->BitTimeout = tag->ti_Data;
                    break;
                
                case aoHidd_I2CDevice_ByteTimeout:
                    dev->ByteTimeout = tag->ti_Data;
                    break;
                
                case aoHidd_I2CDevice_AcknTimeout:
                    dev->AcknTimeout = tag->ti_Data;
                    break;
                
                case aoHidd_I2CDevice_StartTimeout:
                    dev->StartTimeout = tag->ti_Data;
                    break;
                
                case aoHidd_I2CDevice_RiseFallTime:
                    dev->RiseFallTime = tag->ti_Data;
                    break;
            }
        }
    }
}

