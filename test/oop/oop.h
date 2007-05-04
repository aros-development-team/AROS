#ifndef OOP_H
#define OOP_H
/*
   Copyright © 1997-98, The AROS Development Team. All rights reserved.
   $Id$

   Desc: Demo of new OOP system
   Lang: english
*/

#ifndef TYPES_H
#   include "types.h"
#endif
#ifndef SUPPORT_H
#   include "support.h"
#endif

typedef struct IClass Class;

struct IClass
{

    /* Array of pointers to methodtables for this class */
    struct Node 	ClassNode;

    ULONG InstOffset;
    ULONG InstSize;

    /* The number of methods in the hashtable */
    ULONG NumMethods;

    ULONG SubClassCount;
    ULONG ObjectCount;

    Class *SuperClass;
    struct Bucket **HashTable;
    ULONG HashTableSize; /* Hashtable size counted in number of *entries* (not bytes) */
};


typedef ULONG Object;
typedef ULONG Method;

struct _Object
{
    Class *oClass;
};

typedef struct
{
    ULONG MethodID;
} *Msg;

/* Used when initializing a class */
struct MethodDescr
{
    APTR MethodFunc;
    ULONG MethodID;
};




/* Macros */

#define NUM_METHOD_BITS 10

#define BASEOBJECT(obj) ((Object *)(_OBJ(obj) + 1))
#define _OBJECT(obj) (_OBJ(obj) - 1)
#define _OBJ(obj) ((struct _Object *)(obj))

#define INST_DATA(cl, obj) \
	(((VOID *)(obj)) + cl->InstOffset)


#define OCLASS(o) (_OBJECT(o)->oClass)


#define ROOTCLASS "rootclass"

#define RootInterface (0 << NUM_METHOD_BITS)
#define M_New		(RootInterface + 0)
#define M_Dispose	(RootInterface + 1)

struct P_New
{
    ULONG MethodID;
    APTR ParamPtr;
};


#include "intern.h"


#define CallMethodFast(o, m, msg)                         \
({                                                        \
	IPTR (*m_func)(Class *, Object *, Msg);   \
	m_func = ((struct Bucket *)m)->MethodFunc;        \
	m_func(((struct Bucket *)m)->Class, o, (Msg)msg); \
})

#endif /* OOP_H */
