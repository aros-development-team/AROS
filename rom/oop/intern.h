#ifndef INTERN_H
#define INTERN_H

/*
    Copyright 1995-1997 AROS - The Amiga Replacement OS
    $Id$

    Desc:
    Lang: english
*/

#ifndef EXEC_LISTS_H
#   include <exec/lists.h>
#endif

#ifndef EXEC_LIBRARIES_H
#   include <exec/libraries.h>
#endif

#ifndef EXEC_SEMAPHORES_H
#   include <exec/semaphores.h>
#endif

#ifndef DOS_BPTR_H
#   include <dos/bptr.h>
#endif

#ifndef UTILITY_TAGITEM_H
#   include <utility/tagitem.h>
#endif

#ifndef OOP_OOP_H
#   include <oop/oop.h>
#endif


#define DEBUG 0

#define IntCl(cl) ((struct IntClass *)cl)
enum { 
    IDX_IRoot = 0,
    IDX_IMeta,
    IDX_IMethod,
    IDX_IServer,
    IDX_IProxy,
    NUM_Indexes
};

struct IDDescr
{
    STRPTR ID;
    ULONG *Storage;
};

struct IntClass
{
    /* The number of methods that are new for this class */
    struct IClass PPart; /* Public part */
    
    ULONG SubClassCount;
    ULONG ObjectCount;
    
    /* Can also be gotten with indexing the ClassTable */
    struct IntClass *SuperClass;


    struct HashTable *IFTable;
    struct IFBucket **IFTableDirectPtr;
    ULONG HashMask;
    ULONG NumInterfaces;
    
};



#define __OOPI_Root	(GetOBase(OOPBase)->ob_InternIDs[IDX_IRoot])
#define __OOPI_Meta	(GetOBase(OOPBase)->ob_InternIDs[IDX_IMeta])
#define __OOPI_Method	(GetOBase(OOPBase)->ob_InternIDs[IDX_IMethod])
#define __OOPI_Server	(GetOBase(OOPBase)->ob_InternIDs[IDX_IServer])
#define __OOPI_Proxy	(GetOBase(OOPBase)->ob_InternIDs[IDX_IProxy])

struct IntOOPBase
{
    struct Library		 ob_LibNode;
    struct ExecBase	       * ob_SysBase;
    struct Library	       * ob_UtilityBase;
    BPTR			 ob_SegList;

    struct SignalSemaphore	 ob_ClassListLock;
    struct MinList		 ob_ClassList;
    
    struct SignalSemaphore	 ob_ServerListLock;
    struct MinList		 ob_ServerList;
    
    struct IntClass 		 ob_RootClass;
    struct MetaInst
    {
    	Class *Class; /* The meta object's class */
		/* Possible future addon: Root object's instance here */
	struct IntClass InstanceData; /* Meta object's instance data */
    }
    			 	 ob_MetaClass;
				 
    Class			 *ob_MethodClass;
    Class			 *ob_ServerClass;
    Class			 *ob_ProxyClass;
    
    struct HashTable	       * ob_IDTable;
    ULONG			 ob_CurrentID;
    ULONG			 ob_InternIDs[NUM_Indexes];
};

#define GetOBase(lib)           ((struct IntOOPBase *)(lib))
#define SysBase 		(GetOBase(OOPBase)->ob_SysBase)
#define UtilityBase		(GetOBase(OOPBase)->ob_UtilityBase)

/*****************
**  Prototypes  **
*****************/
BOOL AllocDispatchTables(
		struct IntClass 	*cl,
		struct InterfaceDescr 	*ifDescr,
		struct IntOOPBase 	*OOPBase);
VOID FreeDispatchTables(struct IntClass *cl, struct IntOOPBase *OOPBase);


BOOL GetIDs(struct IDDescr *idDesr, struct IntOOPBase *OOPBase);

IPTR LocalDoMethod(Object *, Msg);

BOOL InitRootClass(struct IntOOPBase *OOPBase);
BOOL InitMetaClass(struct IntOOPBase *OOPBase);

Class *InitMethodClass(struct IntOOPBase *OOPBase);
Class *InitServerClass(struct Library *OOPBase);
Class *InitProxyClass(struct Library *OOPBase);

/*****************
**  Structures  **
*****************/
		
struct IDBucket
{
    struct IDBucket *Next;
    STRPTR StrID;
    ULONG  NumericID;
    
};

struct MetaData
{
    Class ClassInstance;
};


struct IFBucket
{
    struct IFBucket *Next;
    ULONG InterfaceID;
    struct IFMethod *MethodTable;
    ULONG NumMethods;
};

struct IFMethod
{
    IPTR (*MethodFunc)();
    Class *mClass;
};


/* Message struct used for sending object message structs across
to another task */
struct ProxyMsg
{
    struct Message pm_Message;
    Msg		pm_ObjMsg;
    IPTR	pm_RetVal;
    Object	*pm_Object;
};

struct ServerNode
{
    struct Node sn_Node;
    Object	*sn_Server;
};

#   define IntCallMethod(cl, o, msg) 					\
    {				    					\
    	register struct IFBucket *b;					\
    	register ULONG mid = msg->MethodID;				\
    	register ULONG ifid = mid & (~METHOD_MASK);			\
    	register struct IFMethod *method;				\
    									\
    	mid &= METHOD_MASK;						\
									\
	b = cl->IFTableDirectPtr[ifid & cl->HashMask];			\
loop:   if (b) 								\
   	{								\
       	    if (b->InterfaceID == ifid)					\
	    {								\
	    	method = &(b->MethodTable[mid]);			\
	    	return (method->MethodFunc(method->mClass, o, msg));	\
	    }    							\
            b = b->Next;						\
	    goto loop;							\
    	}								\
    	return (0UL);							\
     }


#endif /* INTERN_H */
