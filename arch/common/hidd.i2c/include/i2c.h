#ifndef HIDD_I2C_H
#define HIDD_I2C_H

/*
    Copyright © 2003, The AROS Development Team. All rights reserved.
    $Id$
*/

#ifndef EXEC_TYPES_H
#include <exec/types.h>
#endif

#ifndef HIDD_HIDD_H
#include <hidd/hidd.h>
#endif

#ifndef OOP_OOP_H
#include <oop/oop.h>
#endif

/* Base I2C class */

#define CLID_Hidd_I2C	"hidd.i2c"
#define IID_Hidd_I2C	"hidd.i2c"

#define HiddI2CAttrBase	__IHidd_I2C

#ifndef __OOP_NOATTRBASES__
extern OOP_AttrBase HiddI2CAttrBase;
#endif

/* I2C Class methods */
enum
{
    moHidd_I2C_UDelay = 0,
    moHidd_I2C_PutBits,
    moHidd_I2C_GetBits,
    moHidd_I2C_Start,
    moHidd_I2C_Address,
    moHidd_I2C_Stop,
    moHidd_I2C_PutByte,
    moHidd_I2C_GetByte,
    moHidd_I2C_WriteRead,
    moHidd_I2C_ProbeAddress,

    NUM_I2C_METHODS
};

enum
{
    aoHidd_I2C_HoldTime,
    aoHidd_I2C_BitTimeout,
    aoHidd_I2C_ByteTimeout,
    aoHidd_I2C_AcknTimeout,
    aoHidd_I2C_StartTimeout,
    aoHidd_I2C_RiseFallTime,
    aoHidd_I2C_Name,
    
    num_Hidd_I2C_Attrs
};

#define aHidd_I2C_HoldTime      (HiddI2CAttrBase + aoHidd_I2C_HoldTime)
#define aHidd_I2C_BitTimeout    (HiddI2CAttrBase + aoHidd_I2C_BitTimeout)
#define aHidd_I2C_ByteTimeout   (HiddI2CAttrBase + aoHidd_I2C_ByteTimeout)
#define aHidd_I2C_AcknTimeout   (HiddI2CAttrBase + aoHidd_I2C_AcknTimeout)
#define aHidd_I2C_StartTimeout  (HiddI2CAttrBase + aoHidd_I2C_StartTimeout)
#define aHidd_I2C_RiseFallTime  (HiddI2CAttrBase + aoHidd_I2C_RiseFallTime)
#define aHidd_I2C_Name          (HiddI2CAttrBase + aoHidd_I2C_Name)

#define IS_I2C_ATTR(attr, idx) \
    (((idx) = (attr) - HiddI2CAttrBase) < num_Hidd_I2C_Attrs)

struct pHidd_I2C_UDelay
{
    OOP_MethodID    mID;
    ULONG   	    delay;
};

struct pHidd_I2C_PutBits
{
    OOP_MethodID    mID;
    BOOL       	    scl;
    BOOL            sda;
};

struct pHidd_I2C_GetBits
{
    OOP_MethodID    mID;
    BOOL       	    *scl;
    BOOL            *sda;
};

struct pHidd_I2C_Start
{
    OOP_MethodID    mID;
    ULONG           timeout;
};

struct pHidd_I2C_Address
{
    OOP_MethodID    mID;
    OOP_Object      *device;
    UWORD           address;
};

struct pHidd_I2C_Stop
{
    OOP_MethodID    mID;
    OOP_Object      *device;
};

struct pHidd_I2C_PutByte
{
    OOP_MethodID    mID;
    OOP_Object      *device;
    UBYTE           data;
};

struct pHidd_I2C_GetByte
{
    OOP_MethodID    mID;
    OOP_Object      *device;
    UBYTE           *data;
    BOOL            last;
};

struct pHidd_I2C_WriteRead
{
    OOP_MethodID    mID;
    OOP_Object      *device;
    UBYTE           *writeBuffer;
    ULONG           writeLength;
    UBYTE           *readBuffer;
    ULONG           readLength;
};

struct pHidd_I2C_ProbeAddress
{
    OOP_MethodID    mID;
    UWORD           address;
};

/* I2C device class */

#define CLID_Hidd_I2CDevice	"hidd.i2c.device"
#define IID_Hidd_I2CDevice 	"hidd.i2c.device"

#define HiddI2CDeviceAttrBase	__IHidd_I2CDevice

#ifndef __OOP_NOATTRBASES__
extern OOP_AttrBase HiddI2CDeviceAttrBase;
#endif

/* I2C Device Class methods */
enum
{
    moHidd_I2CDevice_Read,
    moHidd_I2CDevice_ReadStatus,
    moHidd_I2CDevice_ReadByte,
    moHidd_I2CDevice_ReadBytes,
    moHidd_I2CDevice_ReadWord,
    moHidd_I2CDevice_Write,
    moHidd_I2CDevice_WriteByte,
    moHidd_I2CDevice_WriteBytes,
    moHidd_I2CDevice_WriteWord,
    moHidd_I2CDevice_WriteVec,
    moHidd_I2CDevice_WriteRead,

    NUM_I2CDEV_METHODS
};

enum
{
    aoHidd_I2CDevice_Driver,
    aoHidd_I2CDevice_Address,
    aoHidd_I2CDevice_HoldTime,
    aoHidd_I2CDevice_BitTimeout,
    aoHidd_I2CDevice_ByteTimeout,
    aoHidd_I2CDevice_AcknTimeout,
    aoHidd_I2CDevice_StartTimeout,
    aoHidd_I2CDevice_RiseFallTime,
    aoHidd_I2CDevice_Name,
    
    num_Hidd_I2CDevice_Attrs
};

#define aHidd_I2CDevice_Driver        (HiddI2CDeviceAttrBase + aoHidd_I2CDevice_Driver)
#define aHidd_I2CDevice_Address       (HiddI2CDeviceAttrBase + aoHidd_I2CDevice_Address)
#define aHidd_I2CDevice_HoldTime      (HiddI2CDeviceAttrBase + aoHidd_I2CDevice_HoldTime)
#define aHidd_I2CDevice_BitTimeout    (HiddI2CDeviceAttrBase + aoHidd_I2CDevice_BitTimeout)
#define aHidd_I2CDevice_ByteTimeout   (HiddI2CDeviceAttrBase + aoHidd_I2CDevice_ByteTimeout)
#define aHidd_I2CDevice_AcknTimeout   (HiddI2CDeviceAttrBase + aoHidd_I2CDevice_AcknTimeout)
#define aHidd_I2CDevice_StartTimeout  (HiddI2CDeviceAttrBase + aoHidd_I2CDevice_StartTimeout)
#define aHidd_I2CDevice_RiseFallTime  (HiddI2CDeviceAttrBase + aoHidd_I2CDevice_RiseFallTime)
#define aHidd_I2CDevice_Name          (HiddI2CDeviceAttrBase + aoHidd_I2CDevice_Name)

#define IS_I2CDEV_ATTR(attr, idx) \
    (((idx) = (attr) - HiddI2CDeviceAttrBase) < num_Hidd_I2CDevice_Attrs)

struct pHidd_I2CDevice_Read
{
    OOP_MethodID    mID;
    UBYTE           *readBuffer;
    ULONG           readLength;
};

struct pHidd_I2CDevice_ReadStatus
{
    OOP_MethodID    mID;
    UBYTE           *status;
};

struct pHidd_I2CDevice_ReadByte
{
    OOP_MethodID    mID;
    UBYTE           subaddr;
    UBYTE           *data;
};

struct pHidd_I2CDevice_ReadBytes
{
    OOP_MethodID    mID;
    UBYTE           subaddr;
    UBYTE           *data;
    ULONG           length;
};

struct pHidd_I2CDevice_ReadWord
{
    OOP_MethodID    mID;
    UBYTE           subaddr;
    UWORD           *data;
};

struct pHidd_I2CDevice_Write
{
    OOP_MethodID    mID;
    UBYTE           *writeBuffer;
    ULONG           writeLength;
};

struct pHidd_I2CDevice_WriteByte
{
    OOP_MethodID    mID;
    UBYTE           subaddr;
    UBYTE           data;
};

struct pHidd_I2CDevice_WriteBytes
{
    OOP_MethodID    mID;
    UBYTE           subaddr;
    UBYTE           *data;
    ULONG           length;
};

struct pHidd_I2CDevice_WriteWord
{
    OOP_MethodID    mID;
    UBYTE           subaddr;
    UWORD           data;
};

struct pHidd_I2CDevice_WriteVec
{
    OOP_MethodID    mID;
    UBYTE           *data;
    ULONG           length;
};

struct pHidd_I2CDevice_WriteRead
{
    OOP_MethodID    mID;
    UBYTE           *writeBuffer;
    ULONG           writeLength;
    UBYTE           *readBuffer;
    ULONG           readLength;
};

#endif
