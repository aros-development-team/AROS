/*
   (C) 1997-98 AROS - The Amiga Replacement OS
   $Id$

   Desc: Demo of new OOP system
   Lang: english
*/

#include "types.h"
#include "oop.h"

#define CallMethod(cl, o, msg) 				\
{							\
    return ( cl->MTable[msg->MethodID].MethodFunc(cl, o, msg)); \
}

IPTR CoerceMethodA(Class *cl, Object *o, Msg msg)
{
    CallMethod(cl, o, msg);
}

IPTR DoMethodA(Object *o, Msg msg)
{
    register Class *cl = OCLASS(o);
    CallMethod(cl, o, msg);
}

IPTR DoSuperMethodA(Class *cl, Object *o, Msg msg)
{
    Class *super = cl->SuperClass;
    CallMethod(super, o, msg);
}

