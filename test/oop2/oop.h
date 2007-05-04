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

struct Method;

typedef struct IClass
{

    /* Array of pointers to methodtables for this class */
    struct Node 	ClassNode;
    
    ULONG InstOffset;
    ULONG InstSize;
    
    /* The number of methods introduced by this class */
    ULONG NumMethods;
    
    ULONG SubClassCount;
    ULONG ObjectCount;
    
    struct IClass *SuperClass;
    struct Method *MTable;
    ULONG MTableSize; /* Counted in # of entries, not in bytes */
    
} Class;

struct Method
{
     IPTR (*MethodFunc)();
     Class *m_Class;
};


typedef ULONG Object;
typedef ULONG Method;

struct _Object
{
    Class *o_Class;
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
#define METHOD_MASK ((1 << (NUM_METHOD_BITS)) - 1)

#define BASEOBJECT(obj) ((Object *)(_OBJ(obj) + 1))
#define _OBJECT(obj) (_OBJ(obj) - 1)
#define _OBJ(obj) ((struct _Object *)(obj))

#define INST_DATA(cl, obj) \
	(((VOID *)(obj)) + cl->InstOffset)

	
#define OCLASS(o) (_OBJECT(o)->o_Class)


#define ROOTCLASS "rootclass"

#define NUM_ROOT_METHODS 2

#define M_New 		0
#define M_Dispose	1

struct P_New
{
    ULONG MethodID;
    APTR ParamPtr;
};


#endif /* OOP_H */
