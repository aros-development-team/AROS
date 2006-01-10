/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Stubs for Serial and SerialUnit class
    Lang: english
*/

#ifndef AROS_USE_OOP
#   define AROS_USE_OOP
#endif

#include <aros/config.h>
#include <exec/types.h>
#include <exec/libraries.h>

#include <proto/oop.h>

#include <utility/tagitem.h>

#include <oop/oop.h>
#include <hidd/serial.h>

//#include "serial_intern.h"

#undef  SDEBUG
#undef  DEBUG
#define DEBUG 0
#include <aros/debug.h>

#undef OOPBase
#define OOPBase ((struct Library *)OOP_OCLASS(obj)->OOPBasePtr)

#ifdef AROS_CREATE_ROM
# define STATIC_MID OOP_MethodID mid = 0
#else
# define STATIC_MID static OOP_MethodID mid
#endif

/***************************************************************/

OOP_Object * HIDD_Serial_NewUnit(OOP_Object *obj, ULONG unitnum)
{
    STATIC_MID;
    struct pHidd_Serial_NewUnit p;
    
    if(!mid) mid = OOP_GetMethodID(IID_Hidd_Serial, moHidd_Serial_NewUnit);
        
    p.mID      = mid;
    p.unitnum  = unitnum;

    return((OOP_Object *) OOP_DoMethod(obj, (OOP_Msg) &p));
}
/***************************************************************/

VOID HIDD_Serial_DisposeUnit(OOP_Object *obj, OOP_Object *unit)
{
    STATIC_MID;
    struct pHidd_Serial_DisposeUnit p;
    
    if(!mid) mid = OOP_GetMethodID(IID_Hidd_Serial, moHidd_Serial_DisposeUnit);
        
    p.mID    = mid;
    p.unit   = unit;

    OOP_DoMethod(obj, (OOP_Msg) &p);
}




/********************** Stubs for serial unit **********************/

BOOL HIDD_SerialUnit_Init(OOP_Object *obj, VOID * DataReceived, VOID * DataReceivedUserData, VOID * WriteData, VOID * WriteDataUserData)
{
    STATIC_MID;
    struct pHidd_SerialUnit_Init p;
    
    if(!mid) mid = OOP_GetMethodID(IID_Hidd_SerialUnit, moHidd_SerialUnit_Init);
        
    p.mID      			= mid;
    p.DataReceived 		= DataReceived;
    p.DataReceivedUserData 	= DataReceivedUserData;
    p.WriteData    		= WriteData;
    p.WriteDataUserData 	= WriteDataUserData;
    
    return((BOOL) OOP_DoMethod(obj, (OOP_Msg) &p));
}
/***************************************************************/

ULONG HIDD_SerialUnit_Write (OOP_Object *obj, UBYTE * data, ULONG length)
{
    STATIC_MID;
    struct pHidd_SerialUnit_Write p;
    
    if(!mid) mid = OOP_GetMethodID(IID_Hidd_SerialUnit, moHidd_SerialUnit_Write);
        
    p.mID	= mid;
    p.Length	= length;
    p.Outbuffer	= data; 

    return ((ULONG) OOP_DoMethod(obj, (OOP_Msg) &p));
}

/***************************************************************/

BOOL HIDD_SerialUnit_SetBaudrate(OOP_Object *obj, ULONG baudrate)
{
    STATIC_MID;
    struct pHidd_SerialUnit_SetBaudrate p;
    
    if(!mid) mid = OOP_GetMethodID(IID_Hidd_SerialUnit, moHidd_SerialUnit_SetBaudrate);
        
    p.mID	= mid;
    p.baudrate  = baudrate;

    return ((BOOL) OOP_DoMethod(obj, (OOP_Msg) &p));
}

/***************************************************************/

BOOL HIDD_SerialUnit_SetParameters(OOP_Object *obj, struct TagItem * tags)
{
    STATIC_MID;
    struct pHidd_SerialUnit_SetParameters p;
    
    if(!mid) mid = OOP_GetMethodID(IID_Hidd_SerialUnit, moHidd_SerialUnit_SetParameters);
        
    p.mID	= mid;
    p.tags      = tags;

    return ((BOOL) OOP_DoMethod(obj, (OOP_Msg) &p));
}

/***************************************************************/

BYTE HIDD_SerialUnit_SendBreak(OOP_Object *obj, int duration)
{
    STATIC_MID;
    struct pHidd_SerialUnit_SendBreak p;
    
    if(!mid) mid = OOP_GetMethodID(IID_Hidd_SerialUnit, moHidd_SerialUnit_SendBreak);
        
    p.mID	= mid;
    p.duration  = duration;

    return ((BYTE)OOP_DoMethod(obj, (OOP_Msg) &p));
}

/***************************************************************/

VOID HIDD_SerialUnit_GetCapabilities(OOP_Object *obj, struct TagItem * tags)
{
    STATIC_MID;
    struct pHidd_SerialUnit_GetCapabilities p;

    if (!mid) mid = OOP_GetMethodID(IID_Hidd_SerialUnit, moHidd_SerialUnit_GetCapabilities);

    p.mID         = mid;
    p.taglist     = tags;

    OOP_DoMethod(obj, (OOP_Msg) &p);
}

/***************************************************************/

VOID HIDD_SerialUnit_Start(OOP_Object *obj)
{
    STATIC_MID;
    struct pHidd_SerialUnit_Start p;

    if (!mid) mid = OOP_GetMethodID(IID_Hidd_SerialUnit, moHidd_SerialUnit_Start);

    p.mID         = mid;

    OOP_DoMethod(obj, (OOP_Msg) &p);
}


/***************************************************************/

VOID HIDD_SerialUnit_Stop(OOP_Object *obj)
{
    STATIC_MID;
    struct pHidd_SerialUnit_Stop p;

    if (!mid) mid = OOP_GetMethodID(IID_Hidd_SerialUnit, moHidd_SerialUnit_Stop);

    p.mID         = mid;

    OOP_DoMethod(obj, (OOP_Msg) &p);
}

/***************************************************************/

UWORD HIDD_SerialUnit_GetStatus(OOP_Object *obj)
{
    STATIC_MID;
    struct pHidd_SerialUnit_GetStatus p;

    if (!mid) mid = OOP_GetMethodID(IID_Hidd_SerialUnit, moHidd_SerialUnit_GetStatus);

    p.mID         = mid;

    return ((UWORD)OOP_DoMethod(obj, (OOP_Msg) &p));
}
