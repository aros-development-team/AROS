/*
    Copyright © 2010, The AROS Development Team. All rights reserved.
    $Id$
*/

/* 
    This is ment to be (in future) a generic class that will be capable of
    compositing bitmaps on screen to get effects like screen dragging.
    The code is generic where possible, using abstract OOP_Objects instead of
    concrete driver structures. There are places where nouveau specific calls 
    are performed, however there are only few and can be generilized to 
    (private) methods that should be reimplemented by child classes in each of 
    the drivers.
*/

#include "compositing_intern.h"

// FIXME: temp forwarding hack
#include "nouveau_compositing.h"

/* METHODS */
OOP_Object *METHOD(Compositing, Root, New)
{
    o = (OOP_Object *)OOP_DoSuperMethod(cl, o, (OOP_Msg) msg);

    if(o)
    {
    //TODO: specific object creation
/*        {
            OOP_MethodID dispose_mid;
            dispose_mid = OOP_GetMethodID(IID_Root, moRoot_Dispose);
            OOP_CoerceMethod(cl, o, (OOP_Msg) & dispose_mid);        
            o = NULL;
        }*/
    }

    return o;
}

VOID METHOD(Compositing, Hidd_Compositing, BitMapStackChanged)
{
    // FIXME: temp forwarding hack
    Compositing_BitMapStackChanged(msg->data);
}

VOID METHOD(Compositing, Hidd_Compositing, BitMapRectChanged)
{
    // FIXME: temp forwarding hack
    Compositing_BitMapRectChanged(msg->bm, msg->x, msg->y, msg->width, msg->height);
}

VOID METHOD(Compositing, Hidd_Compositing, BitMapPositionChanged)
{
    // FIXME: temp forwarding hack
    Compositing_BitMapPositionChanged(msg->bm);
}


