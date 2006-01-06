/*
    Copyright � 1995-2001, The AROS Development Team. All rights reserved.
    $Id: tap_stubs.c 12692 2001-11-14 21:02:01Z chodorowski $

    Desc: Stubs for Tap and TapUnit class
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
#include <hidd/tap.h>

#undef  SDEBUG
#undef  DEBUG
#define DEBUG 0
#include <aros/debug.h>

#warning FIXME: Extremely ugly hack to get OOPBase.

#undef OOPBase
#define OOPBase ((struct Library *)OOP_OCLASS(OOP_OCLASS(OOP_OCLASS(obj)))->UserData)

#define STATIC_MID static OOP_MethodID mid

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

OOP_Object * HIDD_Tap_NewUnit(OOP_Object *obj, ULONG unitnum)
{
    STATIC_MID;
    struct pHidd_Tap_NewUnit p;
    
    if(!mid) mid = OOP_GetMethodID(IID_Hidd_Tap, moHidd_Tap_NewUnit);
        
    p.mID      = mid;
    p.unitnum  = unitnum;

    return((OOP_Object *) OOP_DoMethod(obj, (OOP_Msg) &p));
}
/***************************************************************/

VOID HIDD_Tap_DisposeUnit(OOP_Object *obj, OOP_Object *unit)
{
    STATIC_MID;
    struct pHidd_Tap_DisposeUnit p;
    
    if(!mid) mid = OOP_GetMethodID(IID_Hidd_Tap, moHidd_Tap_DisposeUnit);
        
    p.mID    = mid;
    p.unit   = unit;

    OOP_DoMethod(obj, (OOP_Msg) &p);
}




/********************** Stubs for tap unit **********************/

BOOL HIDD_TapUnit_Init(OOP_Object *obj, VOID * DataReceived, VOID * DataReceivedUserData, VOID * WriteData, VOID * WriteDataUserData)
{
    STATIC_MID;
    struct pHidd_TapUnit_Init p;
    
    if(!mid) mid = OOP_GetMethodID(IID_Hidd_TapUnit, moHidd_TapUnit_Init);
        
    p.mID      		   = mid;
    p.DataReceived 	   = DataReceived;
    p.DataReceivedUserData = DataReceivedUserData;
    p.WriteData    	   = WriteData;
    p.WriteDataUserData	   = WriteDataUserData;

    return((BOOL) OOP_DoMethod(obj, (OOP_Msg) &p));
}
/***************************************************************/

ULONG HIDD_TapUnit_Write (OOP_Object *obj, UBYTE * data, ULONG length)
{
    STATIC_MID;
    struct pHidd_TapUnit_Write p;
    
    if(!mid) mid = OOP_GetMethodID(IID_Hidd_TapUnit, moHidd_TapUnit_Write);
        
    p.mID	= mid;
    p.Length	= length;
    p.Outbuffer	= data; 

    return ((ULONG) OOP_DoMethod(obj, (OOP_Msg) &p));
}

/***************************************************************/

VOID HIDD_TapUnit_Start (OOP_Object *obj)
{
    STATIC_MID;
    struct pHidd_TapUnit_Start p;
    
    if(!mid) mid = OOP_GetMethodID(IID_Hidd_TapUnit, moHidd_TapUnit_Start);
        
    p.mID	= mid;

    ((VOID)OOP_DoMethod(obj, (OOP_Msg) &p));
}

/***************************************************************/

VOID HIDD_TapUnit_Stop (OOP_Object *obj)
{
    STATIC_MID;
    struct pHidd_TapUnit_Stop p;
    
    if(!mid) mid = OOP_GetMethodID(IID_Hidd_TapUnit, moHidd_TapUnit_Stop);
        
    p.mID	= mid;

    ((VOID) OOP_DoMethod(obj, (OOP_Msg) &p));
}

/***************************************************************/

UWORD HIDD_TapUnit_GetStatus(OOP_Object *obj)
{
    STATIC_MID;
    struct pHidd_TapUnit_GetStatus p;

    if (!mid) mid = OOP_GetMethodID(IID_Hidd_TapUnit, moHidd_TapUnit_GetStatus);

    p.mID         = mid;

    return ((UWORD)OOP_DoMethod(obj, (OOP_Msg) &p));
}
