#ifndef OOPSYS_CLASS_H
#define OOPSYS_CLASS_H

#ifndef EXEC_TYPES_H
#   include <exec/types.h>
#endif

typedef struct IClass
{

    /* Array of pointers to methodtables for this class */
    struct Node 	ClassNode;

    IPTR  		**InterfaceTable;
    struct IClass 	**ClassTable;

    ULONG InstOffset;
    ULONG InstSize;

    /* The number of methods that are new for this class */
    ULONG NumMethods;

    ULONG SubClassCount;
    ULONG ObjectCount;

    /* Can also be gotten with indexing the ClassTable */
    struct IClass *SuperClass;

    /* What level in the hierarchy are we ? */
    ULONG SuperCount;

} Class;

typedef APTR Object;
struct _Object
{
    Class *Class;
};

/* Used when initializing a class */
struct MTabDescr
{
    APTR 	*Table;
    ULONG 	NumMethods;
};


/* Macros */
#define BASEOBJECT(obj) ((Object *)(_OBJ(obj) + 1))
#define _OBJECT(obj) (_OBJ(obj) - 1)
#define _OBJ(obj) ((struct _Object *)(obj))

#define INST_DATA(obj, cl_level) \
	(((VOID *)(obj)) + _OBJECT(obj)->Class->ClassTable[(cl_level)]->InstOffset)

#define CL_INTERFACE(cl, cl_level, if_level) \
	((cl)->ClassTable[(cl_level)]->InterfaceTable[(if_level)])

#define OBJ_INTERFACE(o, cl_level, if_level) \
	(_OBJECT(o)->Class->ClassTable[(cl_level)]->InterfaceTable[(if_level)])

#define ROOTCLASSNAME "rootclass"

#define CL_Level_Root 0UL /* Root is on top of hierachy */
#define IF_Level_Root 0UL /* Root is on top of hierachy */

/* Root interface */
typedef struct RootInterface
{
    Object (*New)(Class *, APTR);
    VOID (*Dispose)(Object);
} IRoot;

typedef struct RootIFStorage
{
    IRoot *IRoot;
} IRootTable;

#endif /* OOPSYS_CLASS_H */
