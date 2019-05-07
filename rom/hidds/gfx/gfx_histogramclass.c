/*
    Copyright (C) 2019, The AROS Development Team. All rights reserved.
    $Id$
*/

#include "gfx_debug.h"

#include <oop/oop.h>
#include <utility/tagitem.h>

#include "gfx_intern.h"

OOP_Object *GFXHist__Root__New(OOP_Class *cl, OOP_Object *o, struct pRoot_New *msg)
{
    D(bug("[Gfx:Hist] %s()\n", __func__);)

    return  (OOP_Object *)OOP_DoSuperMethod(cl, o, (OOP_Msg)msg);
}

VOID GFXHist__Root__Dispose(OOP_Class *cl, OOP_Object *o, OOP_Msg msg)
{
}

VOID GFXHist__Root__Get(OOP_Class *cl, OOP_Object *o, OOP_Msg msg)
{
}
