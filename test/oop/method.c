/*
   (C) 1997-98 AROS - The Amiga Replacement OS
   $Id$

   Desc: Demo of new OOP system
   Lang: english
*/

#include "types.h"
#include "oop.h"
#include "intern.h"


#define CallMethod(cl, o, msg) 				\
{				    			\
    ULONG idx;						\
    struct Bucket *b;					\
    idx = CalcHash(msg->MethodID, cl->HashTableSize);	\
    b = cl->HashTable[idx];				\
    while (b)						\
    {							\
       	if (b->MethodID == msg->MethodID)		\
	{						\
	    IPTR (*method)(Class *, Object *, Msg);	\
	    method = b->MethodFunc;			\
	    return (method(cl, o, msg));		\
	}    						\
	b = b->Next;					\
    }							\
    return (NULL);					\
}

IPTR CoerceMethodA(Class *cl, Object *o, Msg msg)
{
    CallMethod(cl, o, msg);
}

IPTR DoMethodA(Object *o, Msg msg)
{
    Class *cl = OCLASS(o);
    CallMethod(cl, o, msg);
}

IPTR DoSuperMethodA(Class *cl, Object *o, Msg msg)
{
    Class *super = cl->SuperClass;
    CallMethod(super, o, msg)
}

Method *GetMethod(Object *o, ULONG methodID)
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
	    return ((Method *)b);
	}
	b = b->Next;
    }
    return (NULL);
}
