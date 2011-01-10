/*
    Copyright � 1995-2001, The AROS Development Team. All rights reserved.
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
#include <oop/static_mid.h>
#include <hidd/serial.h>

//#include "serial_intern.h"

#undef  SDEBUG
#undef  DEBUG
#define DEBUG 0
#include <aros/debug.h>

#undef OOPBase
#define OOPBase ((struct Library *)OOP_OCLASS(obj)->OOPBasePtr)

/***************************************************************/

OOP_Object * HIDD_Serial_NewUnit(OOP_Object *obj, ULONG unitnum)
{
    STATIC_MID;
    struct pHidd_Serial_NewUnit p, *msg = &p;
    
    if(!static_mid) static_mid = OOP_GetMethodID(IID_Hidd_Serial, moHidd_Serial_NewUnit);
        
    p.mID      = static_mid;
    p.unitnum  = unitnum;

    return((OOP_Object *) OOP_DoMethod(obj, (OOP_Msg) msg));
}
/***************************************************************/

VOID HIDD_Serial_DisposeUnit(OOP_Object *obj, OOP_Object *unit)
{
    STATIC_MID;
    struct pHidd_Serial_DisposeUnit p, *msg = &p;
    
    if(!static_mid) static_mid = OOP_GetMethodID(IID_Hidd_Serial, moHidd_Serial_DisposeUnit);
        
    p.mID    = static_mid;
    p.unit   = unit;

    OOP_DoMethod(obj, (OOP_Msg) msg);
}




/********************** Stubs for serial unit **********************/

BOOL HIDD_SerialUnit_Init(OOP_Object *obj, VOID * DataReceived, VOID * DataReceivedUserData, VOID * WriteData, VOID * WriteDataUserData)
{
    STATIC_MID;
    struct pHidd_SerialUnit_Init p, *msg = &p;
    
    if(!static_mid) static_mid = OOP_GetMethodID(IID_Hidd_SerialUnit, moHidd_SerialUnit_Init);
        
    p.mID      			= static_mid;
    p.DataReceived 		= DataReceived;
    p.DataReceivedUserData 	= DataReceivedUserData;
    p.WriteData    		= WriteData;
    p.WriteDataUserData 	= WriteDataUserData;
    
    return((BOOL) OOP_DoMethod(obj, (OOP_Msg) msg));
}
/***************************************************************/

ULONG HIDD_SerialUnit_Write (OOP_Object *obj, UBYTE * data, ULONG length)
{
    STATIC_MID;
    struct pHidd_SerialUnit_Write p, *msg = &p;
    
    if(!static_mid) static_mid = OOP_GetMethodID(IID_Hidd_SerialUnit, moHidd_SerialUnit_Write);
        
    p.mID	= static_mid;
    p.Length	= length;
    p.Outbuffer	= data; 

    return ((ULONG) OOP_DoMethod(obj, (OOP_Msg) msg));
}

/***************************************************************/

BOOL HIDD_SerialUnit_SetBaudrate(OOP_Object *obj, ULONG baudrate)
{
    STATIC_MID;
    struct pHidd_SerialUnit_SetBaudrate p, *msg = &p;
    
    if(!static_mid) static_mid = OOP_GetMethodID(IID_Hidd_SerialUnit, moHidd_SerialUnit_SetBaudrate);
        
    p.mID	= static_mid;
    p.baudrate  = baudrate;

    return ((BOOL) OOP_DoMethod(obj, (OOP_Msg) msg));
}

/***************************************************************/

BOOL HIDD_SerialUnit_SetParameters(OOP_Object *obj, struct TagItem * tags)
{
    STATIC_MID;
    struct pHidd_SerialUnit_SetParameters p, *msg = &p;
    
    if(!static_mid) static_mid = OOP_GetMethodID(IID_Hidd_SerialUnit, moHidd_SerialUnit_SetParameters);
        
    p.mID	= static_mid;
    p.tags      = tags;

    return ((BOOL) OOP_DoMethod(obj, (OOP_Msg) msg));
}

/***************************************************************/

BYTE HIDD_SerialUnit_SendBreak(OOP_Object *obj, int duration)
{
    STATIC_MID;
    struct pHidd_SerialUnit_SendBreak p, *msg = &p;
    
    if(!static_mid) static_mid = OOP_GetMethodID(IID_Hidd_SerialUnit, moHidd_SerialUnit_SendBreak);
        
    p.mID	= static_mid;
    p.duration  = duration;

    return ((BYTE)OOP_DoMethod(obj, (OOP_Msg) msg));
}

/***************************************************************/

VOID HIDD_SerialUnit_GetCapabilities(OOP_Object *obj, struct TagItem * tags)
{
    STATIC_MID;
    struct pHidd_SerialUnit_GetCapabilities p, *msg = &p;

    if (!static_mid) static_mid = OOP_GetMethodID(IID_Hidd_SerialUnit, moHidd_SerialUnit_GetCapabilities);

    p.mID         = static_mid;
    p.taglist     = tags;

    OOP_DoMethod(obj, (OOP_Msg) msg);
}

/***************************************************************/

VOID HIDD_SerialUnit_Start(OOP_Object *obj)
{
    STATIC_MID;
    struct pHidd_SerialUnit_Start p, *msg = &p;

    if (!static_mid) static_mid = OOP_GetMethodID(IID_Hidd_SerialUnit, moHidd_SerialUnit_Start);

    p.mID         = static_mid;

    OOP_DoMethod(obj, (OOP_Msg) msg);
}


/***************************************************************/

VOID HIDD_SerialUnit_Stop(OOP_Object *obj)
{
    STATIC_MID;
    struct pHidd_SerialUnit_Stop p, *msg = &p;

    if (!static_mid) static_mid = OOP_GetMethodID(IID_Hidd_SerialUnit, moHidd_SerialUnit_Stop);

    p.mID         = static_mid;

    OOP_DoMethod(obj, (OOP_Msg) msg);
}

/***************************************************************/

UWORD HIDD_SerialUnit_GetStatus(OOP_Object *obj)
{
    STATIC_MID;
    struct pHidd_SerialUnit_GetStatus p, *msg = &p;

    if (!static_mid) static_mid = OOP_GetMethodID(IID_Hidd_SerialUnit, moHidd_SerialUnit_GetStatus);

    p.mID         = static_mid;

    return ((UWORD)OOP_DoMethod(obj, (OOP_Msg) msg));
}
