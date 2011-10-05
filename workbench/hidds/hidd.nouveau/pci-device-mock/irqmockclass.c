/*
    Copyright 2011, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <aros/debug.h>
#include <proto/oop.h>

#include "pcimock_intern.h"

OOP_Object * METHOD(IRQMock, Root, New)
{
    o = (OOP_Object *)OOP_DoSuperMethod(cl, o, (OOP_Msg)msg);

    return o;
}

BOOL METHOD(IRQMock, Hidd_IRQ, AddHandler)
{
    return TRUE;
}

BOOL METHOD(IRQMock, Hidd_IRQ, RemHandler)
{
    return TRUE;
}
