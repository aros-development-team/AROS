#ifndef OOP_OOP_H
#define OOP_OOP_H
/*
    Copyright 1995-1997 AROS - The Amiga Replacement OS
    $Id$

    Desc:
    Lang: english
*/

#ifndef EXEC_TYPES_H
#   include <exec/types.h>
#endif

#ifndef UTILITY_TAGITEM_H
#include <utility/tagitem.h>
#endif

typedef struct IClass
{

    /* Array of pointers to methodtables for this class */
    struct Node 	ClassNode;
    
    
    ULONG InstOffset;
    ULONG InstSize;
    APTR UserData;

} Class;



typedef ULONG Object;
typedef struct
{
    IPTR MethodID;
} *Msg;

struct _Object
{
    Class *o_Class;
};



/* Macros */

#define NUM_METHOD_BITS 10
#define METHOD_MASK ((1 << NUM_METHOD_BITS) - 1)

#define BASEOBJECT(obj) ((Object *)(_OBJ(obj) + 1))
#define _OBJECT(obj) (_OBJ(obj) - 1)
#define _OBJ(obj) ((struct _Object *)(obj))

#define INST_DATA(cl, obj) \
	(((VOID *)(obj)) + _OBJECT(obj)->o_Class->InstOffset)

#define OCLASS(obj) \
	(_OBJECT(obj)->o_Class)

#define TagIdx(tag) ((tag) & METHOD_MASK)

struct InterfaceDescr
{
    struct MethodDescr *MethodTable;
    STRPTR InterfaceID;
    ULONG NumMethods; /* Max method idx */
};

struct MethodDescr
{
    IPTR (*MethodFunc)();
    ULONG MethodIdx;
};


#endif /* OOP_OOP_H */
