/*
    (C) 1998 AROS - The Amiga Research OS
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

#include "serial_intern.h"

#undef  SDEBUG
#undef  DEBUG
#define DEBUG 0
#include <aros/debug.h>

#undef OOPBase
#define OOPBase ((struct Library *)OCLASS(OCLASS(OCLASS(obj)))->UserData)


/* A small utility function for using varargs when setting attrs */

#warning SetAttrsTags is defined in inline/oop.h

#ifndef SetAttrsTags
IPTR SetAttrsTags(Object *obj, IPTR tag1, ...)
{
    AROS_SLOWSTACKTAGS_PRE(tag1)
    retval = SetAttrs(obj, AROS_SLOWSTACKTAGS_ARG(tag1));
    AROS_SLOWSTACKTAGS_POST

}
#endif

/***************************************************************/

Object * HIDD_Serial_NewUnit(Object *obj, ULONG unitnum)
{
    static MethodID mid = 0;
    struct pHidd_Serial_NewUnit p;
    
    if(!mid) mid = GetMethodID(IID_Hidd_Serial, moHidd_Serial_NewUnit);
        
    p.mID      = mid;
    p.unitnum  = unitnum;

    return((Object *) DoMethod(obj, (Msg) &p));
}
/***************************************************************/

VOID HIDD_Serial_DisposeUnit(Object *obj, Object *unit)
{
    static MethodID mid = 0;
    struct pHidd_Serial_DisposeUnit p;
    
    if(!mid) mid = GetMethodID(IID_Hidd_Serial, moHidd_Serial_DisposeUnit);
        
    p.mID    = mid;
    p.unit   = unit;

    DoMethod(obj, (Msg) &p);
}




/********************** Stubs for serial unit **********************/

BOOL HIDD_SerialUnit_Init(Object *obj, VOID * DataReceived, VOID * WriteData)
{
    static MethodID mid = 0;
    struct pHidd_SerialUnit_Init p;
    
    if(!mid) mid = GetMethodID(IID_Hidd_SerialUnit, moHidd_SerialUnit_Init);
        
    p.mID      = mid;
    p.DataReceived = DataReceived;
    p.WriteData    = WriteData;

    return((BOOL) DoMethod(obj, (Msg) &p));
}
/***************************************************************/

ULONG HIDD_SerialUnit_Write (Object *obj, UBYTE * data, ULONG length)
{
    static MethodID mid = 0;
    struct pHidd_SerialUnit_Write p;
    
    if(!mid) mid = GetMethodID(IID_Hidd_SerialUnit, moHidd_SerialUnit_Write);
        
    p.mID	= mid;
    p.Length	= length;
    p.Outbuffer	= data; 

    return ((ULONG) DoMethod(obj, (Msg) &p));
}

/***************************************************************/

BOOL HIDD_SerialUnit_SetBaudrate(Object *obj, ULONG baudrate)
{
    static MethodID mid = 0;
    struct pHidd_SerialUnit_SetBaudrate p;
    
    if(!mid) mid = GetMethodID(IID_Hidd_SerialUnit, moHidd_SerialUnit_SetBaudrate);
        
    p.mID	= mid;
    p.baudrate  = baudrate;

    return ((BOOL) DoMethod(obj, (Msg) &p));
}

/***************************************************************/

BYTE HIDD_SerialUnit_SendBreak(Object *obj, int duration)
{
    static MethodID mid = 0;
    struct pHidd_SerialUnit_SendBreak p;
    
    if(!mid) mid = GetMethodID(IID_Hidd_SerialUnit, moHidd_SerialUnit_SendBreak);
        
    p.mID	= mid;
    p.duration  = duration;

    return ((BYTE)DoMethod(obj, (Msg) &p));
}

