/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Stubs for IRQ class
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
#include <hidd/irq.h>

//#include "irq.h"

#undef  SDEBUG
#undef  DEBUG
#define DEBUG 0
#include <aros/debug.h>

#undef OOPBase
#define OOPBase (OOP_OOPBASE(obj))

#ifdef AROS_CREATE_ROM
# define STATIC_MID OOP_MethodID mid = 0
#else
# define STATIC_MID static OOP_MethodID mid
#endif
/***************************************************************/

BOOL HIDD_IRQ_AddHandler(OOP_Object *obj, HIDDT_IRQ_Handler *handler, HIDDT_IRQ_Id id)
{
    STATIC_MID;
    struct pHidd_IRQ_AddHandler p, *msg = &p;
    
    if(!mid) mid = OOP_GetMethodID(IID_Hidd_IRQ, moHidd_IRQ_AddHandler);
        
    p.mID           = mid;
    p.handlerinfo   = handler;
    p.id            = id;

    return((BOOL) OOP_DoMethod(obj, (OOP_Msg) msg));
}

/***************************************************************/

VOID HIDD_IRQ_RemHandler(OOP_Object *obj, HIDDT_IRQ_Handler *handler)
{
    STATIC_MID;
    struct pHidd_IRQ_RemHandler p, *msg = &p;

    if (!mid) mid = OOP_GetMethodID(IID_Hidd_IRQ, moHidd_IRQ_RemHandler);

    p.mID           = mid;
    p.handlerinfo   = handler;

    OOP_DoMethod(obj, (OOP_Msg) msg);
}

/*****************************************************************/

VOID HIDD_IRQ_CauseIRQ(OOP_Object *obj, HIDDT_IRQ_Id id, HIDDT_IRQ_HwInfo *hwinfo)
{
    STATIC_MID;
    struct pHidd_IRQ_CauseIRQ p, *msg = &p;

    if (!mid) mid = OOP_GetMethodID(IID_Hidd_IRQ, moHidd_IRQ_CauseIRQ);

    p.mID           = mid;
    p.id            = id;
    p.hardwareinfo  = hwinfo;

    OOP_DoMethod(obj, (OOP_Msg) msg);
}
