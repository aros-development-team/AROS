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

#undef HiddI2CAttrBase
#undef HiddI2CDeviceAttrBase

#define HiddI2CAttrBase         (SD(cl)->hiddI2CAB)
#define HiddI2CDeviceAttrBase   (SD(cl)->hiddI2CDeviceAB)
#define HiddAttrBase            (SD(cl)->hiddAB)

#define UtilityBase             (SD(cl)->utilitybase)

BOOL METHOD(I2CDev, Hidd_I2CDevice, Read)
{
    tDevData *dev = (tDevData *)OOP_INST_DATA(cl, o);
    BOOL r;
    LOCK_DEV

    r = I2C_WriteRead(dev->driver, o, NULL, 0, msg->readBuffer, msg->readLength);

    UNLOCK_DEV
    
    return r;
}

BOOL METHOD(I2CDev, Hidd_I2CDevice, ReadByte)
{
    tDevData *dev = (tDevData *)OOP_INST_DATA(cl, o);
    BOOL r;
    
    LOCK_DEV

    r = I2C_WriteRead(dev->driver, o, &msg->subaddr, 1, msg->data, 1);

    UNLOCK_DEV   
    
    return r;
}

BOOL METHOD(I2CDev, Hidd_I2CDevice, ReadBytes)
{
    tDevData *dev = (tDevData *)OOP_INST_DATA(cl, o);
    BOOL r;
    
    LOCK_DEV

    r = I2C_WriteRead(dev->driver, o, &msg->subaddr, 1, msg->data, msg->length);

    UNLOCK_DEV    
    
    return r;
}

BOOL METHOD(I2CDev, Hidd_I2CDevice, ReadStatus)
{
    tDevData *dev = (tDevData *)OOP_INST_DATA(cl, o);
    BOOL r;
    
    LOCK_DEV

    r = I2C_WriteRead(dev->driver, o, NULL, 0, msg->status, 1);

    UNLOCK_DEV
    
    return r;
}

BOOL METHOD(I2CDev, Hidd_I2CDevice, ReadWord)
{
    tDevData *dev = (tDevData *)OOP_INST_DATA(cl, o);
    BOOL r;
    UBYTE buff[2];
    
    LOCK_DEV

    if ((r = I2C_WriteRead(dev->driver, o, &msg->subaddr, 1, buff, 2)))
    {
        *msg->data = (buff[0] << 8) | buff[1];
    }

    UNLOCK_DEV
    
    return r;
}

BOOL METHOD(I2CDev, Hidd_I2CDevice, Write)
{
    tDevData *dev = (tDevData *)OOP_INST_DATA(cl, o);
    BOOL r;
    LOCK_DEV

    r = I2C_WriteRead(dev->driver, o, msg->writeBuffer, msg->writeLength, NULL, 0);

    UNLOCK_DEV
    
    return r;
}

BOOL METHOD(I2CDev, Hidd_I2CDevice, WriteByte)
{
    tDevData *dev = (tDevData *)OOP_INST_DATA(cl, o);
    BOOL r;
    UBYTE buff[2];
    
    buff[0] = msg->subaddr;
    buff[1] = msg->data;
    
    LOCK_DEV

    r = I2C_WriteRead(dev->driver, o, buff, 2, NULL, 0);

    UNLOCK_DEV
    
    return r;
}

BOOL METHOD(I2CDev, Hidd_I2CDevice, WriteBytes)
{
    tDevData *dev = (tDevData *)OOP_INST_DATA(cl, o);
    BOOL r = TRUE;
    ULONG nWrite = msg->length;
    UBYTE *WriteBuffer = msg->data;
    
    LOCK_DEV

    if (nWrite > 0)
    {
        if ((r = I2C_Address(dev->driver, o, dev->address & ~1)))
        {
            if ((r = I2C_PutByte(dev->driver, o, msg->subaddr)))
            {
                for (; nWrite > 0; WriteBuffer++, nWrite--)
                    if (!(r = I2C_PutByte(dev->driver, o, *WriteBuffer)))
                        break;
            }
            
            I2C_Stop(dev->driver, o);
        }
    }

    UNLOCK_DEV
    
    return r;
}

BOOL METHOD(I2CDev, Hidd_I2CDevice, WriteWord)
{
    tDevData *dev = (tDevData *)OOP_INST_DATA(cl, o);
    BOOL r;
    UBYTE buff[3];
    
    buff[0] = msg->subaddr;
    buff[1] = msg->data >> 8;
    buff[2] = msg->data & 0xff;
    
    LOCK_DEV

    r = I2C_WriteRead(dev->driver, o, buff, 2, NULL, 0);

    UNLOCK_DEV
    
    return r;
}

BOOL METHOD(I2CDev, Hidd_I2CDevice, WriteVec)
{
    tDevData *dev = (tDevData *)OOP_INST_DATA(cl, o);
    BOOL r = TRUE;
    int s = 0;
    ULONG nValues = msg->length;
    UBYTE *vec = msg->data;
    
    LOCK_DEV

    if (nValues > 0)
    {
        for (; nValues > 0; nValues--, vec+=2)
        {
            if (!(r = I2C_Address(dev->driver, o, dev->address & ~1)))
                break;
            
            s++;
            
            if (!(r = I2C_PutByte(dev->driver, o, vec[0])))
                break;
            
            if (!(r = I2C_PutByte(dev->driver, o, vec[1])))
                break;
        }
        
        if (s > 0) I2C_Stop(dev->driver, o);
    }

    UNLOCK_DEV
    
    return r;
}

/*** Root */

OOP_Object *METHOD(I2CDev, Root, New)
{
    o = (OOP_Object *)OOP_DoSuperMethod(cl, o, (OOP_Msg) msg);
    if (o)
    {
        struct TagItem *tag;
        const struct TagItem *tags = msg->attrList;
        tDevData *dev = (tDevData *)OOP_INST_DATA(cl, o);
        OOP_Object *driver = NULL;
        UWORD address = 0;

        InitSemaphore(&dev->lock);

        dev->HoldTime = -1;
        dev->AcknTimeout = -1;
        dev->BitTimeout = -1;
        dev->ByteTimeout = -1;
        dev->RiseFallTime = -1;
        dev->StartTimeout = -1;

        dev->name = (STRPTR)"dev?";

        tags=msg->attrList;
    
        while((tag = NextTagItem(&tags)))
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

                    case aoHidd_I2CDevice_Name:
                        dev->name = (STRPTR)tag->ti_Data;
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
        
        if (driver && address)
        {
            IPTR val;
            
            if (dev->AcknTimeout == -1)
            {
                OOP_GetAttr(driver, aHidd_I2C_AcknTimeout, &val);
                dev->AcknTimeout = val;
            }
            
            if (dev->BitTimeout == -1)
            {
                OOP_GetAttr(driver, aHidd_I2C_BitTimeout, &val);
                dev->BitTimeout = val;
            }
            
            if (dev->ByteTimeout == -1)
            {
                OOP_GetAttr(driver, aHidd_I2C_ByteTimeout, &val);
                dev->ByteTimeout = val;
            }
            
            if (dev->HoldTime == -1)
            {
                OOP_GetAttr(driver, aHidd_I2C_HoldTime, &val);
                dev->HoldTime = val;
            }
            
            if (dev->RiseFallTime == -1)
            {
                OOP_GetAttr(driver, aHidd_I2C_RiseFallTime, &val);
                dev->RiseFallTime = val;
            }
            
            if (dev->StartTimeout == -1)
            {
                OOP_GetAttr(driver, aHidd_I2C_StartTimeout, &val);
                dev->StartTimeout = val;
            }          
        }
        else
        {
             OOP_MethodID disp_mid = OOP_GetMethodID((STRPTR)IID_Root, moRoot_Dispose);
             OOP_CoerceMethod(cl, o, (OOP_Msg) &disp_mid);
             o = NULL;
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
    struct TagItem *tag;
    const struct TagItem *tags = msg->attrList;
    
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

