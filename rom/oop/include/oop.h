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

#ifndef EXEC_NODES_H
#   include <exec/nodes.h>
#endif

#ifndef UTILITY_TAGITEM_H
#   include <utility/tagitem.h>
#endif

typedef ULONG Object;

typedef ULONG MethodID;

typedef struct
{
    MethodID MID;
} *Msg;


typedef struct IClass Class;

struct IClass
{

    /* Array of pointers to methodtables for this class */
    struct Node 	ClassNode;
    
    
    ULONG InstOffset;
    APTR UserData;
    IPTR (*DoMethod)(Object *, Msg);
    IPTR (*CoerceMethod)(Class *, Object *, Msg);
    IPTR (*DoSuperMethod)(Class *, Object *, Msg);

};



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
	(((VOID *)(obj)) + (cl)->InstOffset)

#define OCLASS(obj) \
	(_OBJECT(obj)->o_Class)


#define DoMethod(o, msg) ( (OCLASS(o))->DoMethod((o), (msg)) )
#define DoSuperMethod(cl, o, msg) ((cl)->DoSuperMethod(cl, o, msg))
#define CoerceMethod(cl, o, msg) ((cl)->CoerceMethod(cl, o, msg))

#define TagIdx(tag) ((tag) & METHOD_MASK)

struct InterfaceDescr
{
    struct MethodDescr *MethodTable;
    STRPTR InterfaceID;
    ULONG NumMethods; /* Number of methods in the methodtable */
};

struct MethodDescr
{
    IPTR (*MethodFunc)();
    ULONG MethodIdx;
};


#endif /* OOP_OOP_H */
