/*
   Copyright © 1997-98, The AROS Development Team. All rights reserved.
   $Id$

   Desc: Demo of new OOP system
   Lang: english
*/

#include "types.h"
#include "oop.h"
#include "intern.h"


#define CallMethod(cl, o, msg) 				\
{				    			\
    register struct Bucket *b;				\
    register ULONG mid = msg->MethodID;			\
							\
    for (b = cl->HashTable[CalcHash(mid, cl->HashTableSize)]; \
              b; b = b->Next)						\
    {							\
       	if (b->MethodID == mid)		\
	{						\
	    IPTR (*method)(Class *, Object *, Msg);	\
	    method = b->MethodFunc;			\
	    return (method(b->mClass, o, msg));		\
	}    						\
    }							\
    return (NULL);					\
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
    CallMethod(super, o, msg)
}

BOOL GetMethod(Object *o, ULONG methodID, APTR *methodPtrPtr, Class **classPtrPtr)
{
    ULONG idx;
    struct Bucket *b;
    Class *cl = OCLASS(o);
    idx = CalcHash(methodID, cl->HashTableSize);
    b = cl->HashTable[idx];
    while (b)
    {
       	if (b->MethodID == methodID)
	{
	    *methodPtrPtr = b->MethodFunc;
	    *classPtrPtr  = b->mClass;
	    return (TRUE);
	}
	b = b->Next;
    }
    return (FALSE);
}

