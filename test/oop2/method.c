/*
   Copyright © 1997-98, The AROS Development Team. All rights reserved.
   $Id$

   Desc: Demo of new OOP system
   Lang: english
*/

#include "types.h"
#include "oop.h"

#define CallMethod(cl, o, msg) 				\
{							\
    if (!(msg->MethodID >> NUM_METHOD_BITS))		\
    {							\
    	register struct Method *m = &(cl->MTable[msg->MethodID & METHOD_MASK]);\
    	return ( m->MethodFunc(m->m_Class, o, msg)); \
    }	\
    return (NULL);\
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

