/*
    Copyright © 2004, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Stub functions for PCI subsystem
    Lang: English
*/

#ifndef AROS_USE_OOP
#   define AROS_USE_OOP
#endif

#include <exec/types.h>
#include <exec/libraries.h>

#include <utility/tagitem.h>
#include <utility/hooks.h>

#include <oop/oop.h>

#include <hidd/hidd.h>
#include <hidd/i2c.h>

#include <proto/oop.h>

#undef OOPBase
#define OOPBase	(OOP_OOPBASE(obj))

#ifdef AROS_CREATE_ROM
#error	Do not use stubs in ROM code!!!
#else
#define	STATIC_MID  static OOP_MethodID mid
#endif

/***************************************************************************/

BOOL HIDD_I2C_ProbeAddress(OOP_Object *obj, UWORD address)
{
    STATIC_MID;
    struct pHidd_I2C_ProbeAddress p, *msg=&p;

    if (!mid) mid = OOP_GetMethodID((STRPTR)IID_Hidd_I2C, moHidd_I2C_ProbeAddress);

    p.mID = mid;
    p.address = address;

    return OOP_DoMethod(obj, (OOP_Msg) msg);
}

BOOL HIDD_I2CDevice_ReadStatus(OOP_Object *obj, UBYTE *status)
{
    STATIC_MID;
    struct pHidd_I2CDevice_ReadStatus p, *msg=&p;

    if (!mid) mid = OOP_GetMethodID((STRPTR)IID_Hidd_I2CDevice, moHidd_I2CDevice_ReadStatus);

    p.mID = mid;
    p.status = status;

    return OOP_DoMethod(obj, (OOP_Msg) msg);
}

BOOL HIDD_I2CDevice_Read(OOP_Object *obj, APTR buffer, ULONG length)
{
    STATIC_MID;
    struct pHidd_I2CDevice_Read p, *msg=&p;

    if (!mid) mid = OOP_GetMethodID((STRPTR)IID_Hidd_I2CDevice, moHidd_I2CDevice_Read);

    p.mID = mid;
    p.readBuffer = buffer;
    p.readLength = length;

    return OOP_DoMethod(obj, (OOP_Msg) msg);
}

BOOL HIDD_I2CDevice_ReadByte(OOP_Object *obj, UBYTE subaddr, UBYTE *data)
{
    STATIC_MID;
    struct pHidd_I2CDevice_ReadByte p, *msg=&p;

    if (!mid) mid = OOP_GetMethodID((STRPTR)IID_Hidd_I2CDevice, moHidd_I2CDevice_ReadByte);

    p.mID = mid;
    p.subaddr = subaddr;
    p.data = data;

    return OOP_DoMethod(obj, (OOP_Msg) msg);
}

BOOL HIDD_I2CDevice_ReadBytes(OOP_Object *obj, UBYTE subaddr, UBYTE *data, ULONG length)
{
    STATIC_MID;
    struct pHidd_I2CDevice_ReadBytes p, *msg=&p;

    if (!mid) mid = OOP_GetMethodID((STRPTR)IID_Hidd_I2CDevice, moHidd_I2CDevice_ReadBytes);

    p.mID = mid;
    p.subaddr = subaddr;
    p.data = data;
    p.length = length;

    return OOP_DoMethod(obj, (OOP_Msg) msg);
}

BOOL HIDD_I2CDevice_ReadWord(OOP_Object *obj, UBYTE subaddr, UWORD *data)
{
    STATIC_MID;
    struct pHidd_I2CDevice_ReadWord p, *msg=&p;

    if (!mid) mid = OOP_GetMethodID((STRPTR)IID_Hidd_I2CDevice, moHidd_I2CDevice_ReadWord);

    p.mID = mid;
    p.subaddr = subaddr;
    p.data = data;

    return OOP_DoMethod(obj, (OOP_Msg) msg);
}

BOOL HIDD_I2CDevice_Write(OOP_Object *obj, APTR buffer, ULONG length)
{
    STATIC_MID;
    struct pHidd_I2CDevice_Write p, *msg=&p;

    if (!mid) mid = OOP_GetMethodID((STRPTR)IID_Hidd_I2CDevice, moHidd_I2CDevice_Write);

    p.mID = mid;
    p.writeBuffer = buffer;
    p.writeLength = length;

    return OOP_DoMethod(obj, (OOP_Msg) msg);
}

BOOL HIDD_I2CDevice_WriteByte(OOP_Object *obj, UBYTE subaddr, UBYTE data)
{
    STATIC_MID;
    struct pHidd_I2CDevice_WriteByte p, *msg=&p;

    if (!mid) mid = OOP_GetMethodID((STRPTR)IID_Hidd_I2CDevice, moHidd_I2CDevice_WriteByte);

    p.mID = mid;
    p.subaddr = subaddr;
    p.data = data;

    return OOP_DoMethod(obj, (OOP_Msg) msg);
}

BOOL HIDD_I2CDevice_WriteBytes(OOP_Object *obj, UBYTE subaddr, UBYTE *data, ULONG length)
{
    STATIC_MID;
    struct pHidd_I2CDevice_WriteBytes p, *msg=&p;

    if (!mid) mid = OOP_GetMethodID((STRPTR)IID_Hidd_I2CDevice, moHidd_I2CDevice_WriteBytes);

    p.mID = mid;
    p.subaddr = subaddr;
    p.data = data;
    p.length = length;

    return OOP_DoMethod(obj, (OOP_Msg) msg);
}

BOOL HIDD_I2CDevice_WriteWord(OOP_Object *obj, UBYTE subaddr, UWORD data)
{
    STATIC_MID;
    struct pHidd_I2CDevice_WriteWord p, *msg=&p;

    if (!mid) mid = OOP_GetMethodID((STRPTR)IID_Hidd_I2CDevice, moHidd_I2CDevice_WriteWord);

    p.mID = mid;
    p.subaddr = subaddr;
    p.data = data;

    return OOP_DoMethod(obj, (OOP_Msg) msg);
}

BOOL HIDD_I2CDevice_WriteVec(OOP_Object *obj, UBYTE *data, ULONG length)
{
    STATIC_MID;
    struct pHidd_I2CDevice_WriteVec p, *msg=&p;

    if (!mid) mid = OOP_GetMethodID((STRPTR)IID_Hidd_I2CDevice, moHidd_I2CDevice_WriteVec);

    p.mID = mid;
    p.data = data;
    p.length = length;

    return OOP_DoMethod(obj, (OOP_Msg) msg);
}
