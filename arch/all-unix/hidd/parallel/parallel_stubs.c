/*
    (C) 1998 AROS - The Amiga Research OS
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

#include "parallel_intern.h"

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

Object * HIDD_Parallel_NewUnit(Object *obj, ULONG unitnum)
{
    static MethodID mid = 0;
    struct pHidd_Parallel_NewUnit p;
    
    if(!mid) mid = GetMethodID(IID_Hidd_Parallel, moHidd_Parallel_NewUnit);
        
    p.mID      = mid;
    p.unitnum  = unitnum;

    return((Object *) DoMethod(obj, (Msg) &p));
}
/***************************************************************/

VOID HIDD_Parallel_DisposeUnit(Object *obj, Object *unit)
{
    static MethodID mid = 0;
    struct pHidd_Parallel_DisposeUnit p;
    
    if(!mid) mid = GetMethodID(IID_Hidd_Parallel, moHidd_Parallel_DisposeUnit);
        
    p.mID    = mid;
    p.unit   = unit;

    DoMethod(obj, (Msg) &p);
}




/********************** Stubs for parallel unit **********************/

BOOL HIDD_ParallelUnit_Init(Object *obj, VOID * DataReceived, VOID * WriteData)
{
    static MethodID mid = 0;
    struct pHidd_ParallelUnit_Init p;
    
    if(!mid) mid = GetMethodID(IID_Hidd_ParallelUnit, moHidd_ParallelUnit_Init);
        
    p.mID      = mid;
    p.DataReceived = DataReceived;
    p.WriteData    = WriteData;

    return((BOOL) DoMethod(obj, (Msg) &p));
}
/***************************************************************/

ULONG HIDD_ParallelUnit_Write (Object *obj, UBYTE * data, ULONG length)
{
    static MethodID mid = 0;
    struct pHidd_ParallelUnit_Write p;
    
    if(!mid) mid = GetMethodID(IID_Hidd_ParallelUnit, moHidd_ParallelUnit_Write);
        
    p.mID	= mid;
    p.Length	= length;
    p.Outbuffer	= data; 

    return ((ULONG) DoMethod(obj, (Msg) &p));
}
