#ifndef OOP_H
#define OOP_H

#include "types.h"
#include "support.h"

#define InitSemaphore(x)
#define ObtainSemaphore(x)
#define ObtainSemaphoreShared(x)
#define ReleaseSemaphore(x)



typedef struct IClass
{

    /* Array of pointers to methodtables for this class */
    struct Node 	ClassNode;
    
    
    ULONG InstOffset;
    ULONG InstSize;

    /* The number of methods that are new for this class */
    
    ULONG SubClassCount;
    ULONG ObjectCount;
    
    /* Can also be gotten with indexing the ClassTable */
    struct IClass *SuperClass;


#if (HASHED_METHODS || HASHED_STRINGS)
    struct MethodBucket **HashTable;
    ULONG HashMask;
    ULONG NumMethods; /* Number of buckets in the hashtable */
#endif

#ifdef HASHED_IFS
    struct InterfaceBucket **HashTable;
    ULONG HashMask;
    ULONG NumInterfaces;
#endif

/*
#ifdef DIRECT_LOOKUP
    IPTR (**MethodTab)();
#endif
*/  

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


/** Root class defs */	
#define ROOTCLASS "rootclass"


#if (HASHED_METHODS || HASHED_IFS)

#   define MIDX_New	0
#   define MIDX_Dispose	1

#   define I_Root (0)
#   define Root_Base (I_Root << NUM_METHOD_BITS)

#   define M_New	(I_Root + MIDX_New)
#   define M_Dispose	(I_Root + MIDX_Dispose)
#endif

#if (HASHED_STRINGS)
#   define M_New	"New"
#   define M_Dispose	"Dispose"
#endif

struct P_New
{
    ULONG MethodID;
    APTR ParamPtr;
};


struct OOPBase
{
    struct List ClassList;
    struct Bucket **IDHashTable;
};
#endif /* OOP_H */
