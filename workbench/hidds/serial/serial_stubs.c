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

//#include "serial_intern.h"

#undef  SDEBUG
#undef  DEBUG
#define DEBUG 0
#include <aros/debug.h>

#warning FIXME: Extremely ugly hack to get OOPBase.

#undef OOPBase
#define OOPBase ((struct Library *)OOP_OCLASS(OOP_OCLASS(OOP_OCLASS(obj)))->UserData)


/* A small utility function for using varargs when setting attrs */

#warning OOP_SetAttrsTags is defined in inline/oop.h

#ifndef OOP_SetAttrsTags
IPTR OOP_SetAttrsTags(OOP_Object *obj, IPTR tag1, ...)
{
    AROS_SLOWSTACKTAGS_PRE(tag1)
    retval = OOP_SetAttrs(obj, AROS_SLOWSTACKTAGS_ARG(tag1));
    AROS_SLOWSTACKTAGS_POST

}
#endif

/***************************************************************/

OOP_Object * HIDD_Serial_NewUnit(OOP_Object *obj, ULONG unitnum)
{
    static OOP_MethodID mid = 0;
    struct pHidd_Serial_NewUnit p;
    
    if(!mid) mid = OOP_GetMethodID(IID_Hidd_Serial, moHidd_Serial_NewUnit);
        
    p.mID      = mid;
    p.unitnum  = unitnum;

    return((OOP_Object *) OOP_DoMethod(obj, (OOP_Msg) &p));
}
/***************************************************************/

VOID HIDD_Serial_DisposeUnit(OOP_Object *obj, OOP_Object *unit)
{
    static OOP_MethodID mid = 0;
    struct pHidd_Serial_DisposeUnit p;
    
    if(!mid) mid = OOP_GetMethodID(IID_Hidd_Serial, moHidd_Serial_DisposeUnit);
        
    p.mID    = mid;
    p.unit   = unit;

    OOP_DoMethod(obj, (OOP_Msg) &p);
}




/********************** Stubs for serial unit **********************/

BOOL HIDD_SerialUnit_Init(OOP_Object *obj, VOID * DataReceived, VOID * DataReceivedUserData, VOID * WriteData, VOID * WriteDataUserData)
{
    static OOP_MethodID mid = 0;
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
    static OOP_MethodID mid = 0;
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
    static OOP_MethodID mid = 0;
    struct pHidd_SerialUnit_SetBaudrate p;
    
    if(!mid) mid = OOP_GetMethodID(IID_Hidd_SerialUnit, moHidd_SerialUnit_SetBaudrate);
        
    p.mID	= mid;
    p.baudrate  = baudrate;

    return ((BOOL) OOP_DoMethod(obj, (OOP_Msg) &p));
}

/***************************************************************/

BOOL HIDD_SerialUnit_SetParameters(OOP_Object *obj, struct TagItem * tags)
{
    static OOP_MethodID mid = 0;
    struct pHidd_SerialUnit_SetParameters p;
    
    if(!mid) mid = OOP_GetMethodID(IID_Hidd_SerialUnit, moHidd_SerialUnit_SetParameters);
        
    p.mID	= mid;
    p.tags      = tags;

    return ((BOOL) OOP_DoMethod(obj, (OOP_Msg) &p));
}

/***************************************************************/

BYTE HIDD_SerialUnit_SendBreak(OOP_Object *obj, int duration)
{
    static OOP_MethodID mid = 0;
    struct pHidd_SerialUnit_SendBreak p;
    
    if(!mid) mid = OOP_GetMethodID(IID_Hidd_SerialUnit, moHidd_SerialUnit_SendBreak);
        
    p.mID	= mid;
    p.duration  = duration;

    return ((BYTE)OOP_DoMethod(obj, (OOP_Msg) &p));
}

/***************************************************************/

VOID HIDD_SerialUnit_GetCapabilities(OOP_Object *obj, struct TagItem * tags)
{
  static OOP_MethodID mid = 0;
  struct pHidd_SerialUnit_GetCapabilities p;
  
  if (!mid) mid = OOP_GetMethodID(IID_Hidd_SerialUnit, moHidd_SerialUnit_GetCapabilities);

  p.mID         = mid;
  p.taglist     = tags;
  
  OOP_DoMethod(obj, (OOP_Msg) &p);
}
