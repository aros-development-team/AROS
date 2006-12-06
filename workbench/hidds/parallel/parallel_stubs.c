/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Stubs for Parallel and ParallelUnit class
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
#include <hidd/parallel.h>

//#include "parallel_intern.h"

#undef  SDEBUG
#undef  DEBUG
#define DEBUG 0
#include <aros/debug.h>

#undef OOPBase
#define OOPBase ((struct Library *)OOP_OCLASS(obj)->OOPBasePtr)

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

OOP_Object * HIDD_Parallel_NewUnit(OOP_Object *obj, ULONG unitnum)
{
    STATIC_MID;
    struct pHidd_Parallel_NewUnit p, *msg = &p;
    
    if(!mid) mid = OOP_GetMethodID(IID_Hidd_Parallel, moHidd_Parallel_NewUnit);
        
    p.mID      = mid;
    p.unitnum  = unitnum;

    return((OOP_Object *) OOP_DoMethod(obj, (OOP_Msg) msg));
}
/***************************************************************/

VOID HIDD_Parallel_DisposeUnit(OOP_Object *obj, OOP_Object *unit)
{
    STATIC_MID;
    struct pHidd_Parallel_DisposeUnit p, *msg = &p;
    
    if(!mid) mid = OOP_GetMethodID(IID_Hidd_Parallel, moHidd_Parallel_DisposeUnit);
        
    p.mID    = mid;
    p.unit   = unit;

    OOP_DoMethod(obj, (OOP_Msg) msg);
}




/********************** Stubs for parallel unit **********************/

BOOL HIDD_ParallelUnit_Init(OOP_Object *obj, VOID * DataReceived, VOID * DataReceivedUserData, VOID * WriteData, VOID * WriteDataUserData)
{
    STATIC_MID;
    struct pHidd_ParallelUnit_Init p, *msg = &p;
    
    if(!mid) mid = OOP_GetMethodID(IID_Hidd_ParallelUnit, moHidd_ParallelUnit_Init);
        
    p.mID      		   = mid;
    p.DataReceived 	   = DataReceived;
    p.DataReceivedUserData = DataReceivedUserData;
    p.WriteData    	   = WriteData;
    p.WriteDataUserData	   = WriteDataUserData;

    return((BOOL) OOP_DoMethod(obj, (OOP_Msg) msg));
}
/***************************************************************/

ULONG HIDD_ParallelUnit_Write (OOP_Object *obj, UBYTE * data, ULONG length)
{
    STATIC_MID;
    struct pHidd_ParallelUnit_Write p, *msg = &p;
    
    if(!mid) mid = OOP_GetMethodID(IID_Hidd_ParallelUnit, moHidd_ParallelUnit_Write);
        
    p.mID	= mid;
    p.Length	= length;
    p.Outbuffer	= data; 

    return ((ULONG) OOP_DoMethod(obj, (OOP_Msg) msg));
}

/***************************************************************/

VOID HIDD_ParallelUnit_Start (OOP_Object *obj)
{
    STATIC_MID;
    struct pHidd_ParallelUnit_Start p, *msg = &p;
    
    if(!mid) mid = OOP_GetMethodID(IID_Hidd_ParallelUnit, moHidd_ParallelUnit_Start);
        
    p.mID	= mid;

    ((VOID)OOP_DoMethod(obj, (OOP_Msg) msg));
}

/***************************************************************/

VOID HIDD_ParallelUnit_Stop (OOP_Object *obj)
{
    STATIC_MID;
    struct pHidd_ParallelUnit_Stop p, *msg = &p;
    
    if(!mid) mid = OOP_GetMethodID(IID_Hidd_ParallelUnit, moHidd_ParallelUnit_Stop);
        
    p.mID	= mid;

    ((VOID) OOP_DoMethod(obj, (OOP_Msg) msg));
}

/***************************************************************/

UWORD HIDD_ParallelUnit_GetStatus(OOP_Object *obj)
{
    STATIC_MID;
    struct pHidd_ParallelUnit_GetStatus p, *msg = &p;

    if (!mid) mid = OOP_GetMethodID(IID_Hidd_ParallelUnit, moHidd_ParallelUnit_GetStatus);

    p.mID         = mid;

    return ((UWORD)OOP_DoMethod(obj, (OOP_Msg) msg));
}
