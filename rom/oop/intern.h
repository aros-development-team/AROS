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

/* These are used to index an array in the library base
   from which we get the IDs for the used interfaces.
*/

enum { 
    IDX_IRoot = 0,
    IDX_IMeta,
    IDX_IMethod,
    IDX_IServer,
    IDX_IProxy,
    IDX_IInterface,
    NUM_Indexes
};

/* Used by the internal GetIDs() functions, to get several
IDs at a time */

struct IDDescr
{
    STRPTR ID;
    ULONG *Storage;
};

/* Definition of the entire class structure */

struct IntClass
{
    /* The number of methods that are new for this class */
    struct IClass PPart; /* Public part */
    
    ULONG SubClassCount;
    ULONG ObjectCount;
    
    struct IntClass *SuperClass;

    
    /* The hashtable containing the interfaces (method tables) */
    struct HashTable *IFTable;
    
    /* A direct pointer into the hashtable, for faster lookup */
    struct IFBucket **IFTableDirectPtr;
    
    /* Used by the hash function. As the above field, it is put
    ** here for speed
    */
    ULONG HashMask;
    
    /* Number of interfaces in the hashtable */
    ULONG NumInterfaces;
    
};


/* defines used to access the interface IDs stored in the library base */

#define __OOPI_Root		(GetOBase(OOPBase)->ob_InternIDs[IDX_IRoot])
#define __OOPI_Meta		(GetOBase(OOPBase)->ob_InternIDs[IDX_IMeta])
#define __OOPI_Method		(GetOBase(OOPBase)->ob_InternIDs[IDX_IMethod])
#define __OOPI_Server		(GetOBase(OOPBase)->ob_InternIDs[IDX_IServer])
#define __OOPI_Proxy		(GetOBase(OOPBase)->ob_InternIDs[IDX_IProxy])
#define __OOPI_Interface	(GetOBase(OOPBase)->ob_InternIDs[IDX_IInterface])

/* definition of oop.library's library base structure */
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
    
    /* rootclass */
    struct IntClass 		 ob_RootClass;
    
    /* Instance data of the meta object/class */
    struct MetaInst
    {
    	Class *Class; /* The meta object's class */
		/* Possible future addon: Root object's instance here */
	struct IntClass InstanceData; /* Meta object's instance data */
    }
    			 	 ob_MetaClass;

    /* Store pointers to some usefull classes here */				 
    Class			 *ob_MethodClass;
    Class			 *ob_ServerClass;
    Class			 *ob_ProxyClass;
    Class			 *ob_InterfaceClass;
    
    /* Hashtable containing string interface ID => Numeric interface
    ** ID mappings. Used by GetID()
    */
    struct HashTable	       * ob_IDTable;
    
    /* The currently lowest available numeric interfaceID that
    ** we can map a new interface string ID onto.
    ** Used by GetID()
    */
    
    ULONG			 ob_CurrentID;
    
    /* An array of the interface IDs used internally. */
    ULONG			 ob_InternIDs[NUM_Indexes];
};

#define GetOBase(lib)           ((struct IntOOPBase *)(lib))
#define SysBase 		(GetOBase(OOPBase)->ob_SysBase)
#define UtilityBase		(GetOBase(OOPBase)->ob_UtilityBase)

/*****************
**  Prototypes  **
*****************/

/* Functions from... */
/* support.c */
BOOL AllocDispatchTables(
		struct IntClass 	*cl,
		struct InterfaceDescr 	*ifDescr,
		struct IntOOPBase 	*OOPBase);
VOID FreeDispatchTables(struct IntClass *cl, struct IntOOPBase *OOPBase);



BOOL GetIDs(struct IDDescr *idDesr, struct IntOOPBase *OOPBase);

/* method.c */
IPTR LocalDoMethod(Object *, Msg);

/* rootclass.c */
BOOL InitRootClass(struct IntOOPBase *OOPBase);

/* metaclass.c */
BOOL InitMetaClass(struct IntOOPBase *OOPBase);

/* methodclass.c */
Class *InitMethodClass(struct IntOOPBase *OOPBase);

/* serverclass.c */
Class *InitServerClass(struct Library *OOPBase);

/* proxyclass.c */
Class *InitProxyClass(struct Library *OOPBase);

/* interfaceclass.c */
Class *InitInterfaceClass(struct Library *OOPBase);

/*****************
**  Structures  **
*****************/

/* Bucket for hashtable used to map string interface IDs to
** numeric interface IDs. Used in getid.c().		
*/

struct IDBucket
{
    struct IDBucket *Next;
    STRPTR StrID;
    ULONG  NumericID;
    
};

/* Instance data of the meta object */
struct MetaData
{
    Class ClassInstance;
};


/* Definition of bucket for hashtable used to store
** the interfaces (method tables)
*/

struct IFBucket
{
    struct IFBucket *Next;
    ULONG InterfaceID;
    struct IFMethod *MethodTable;
    ULONG NumMethods;
};


/* Definition of an entry in a method table.
** Ie. the method tables are arrays of these.
** One has to store the class for each method
** because we skip unimplemented class calls, and
** therefore a method can go directly to a parent
** class of OCLASS(o)
*/

struct IFMethod
{
    IPTR (*MethodFunc)();
    Class *mClass;
};


/* Message struct used for sending object message structs across
** to another task
*/
struct ProxyMsg
{
    struct Message pm_Message;
    Msg		pm_ObjMsg;
    IPTR	pm_RetVal;
    Object	*pm_Object;
};

/* Listnode that helps keeping track of servers. */
struct ServerNode
{
    struct Node sn_Node;
    Object	*sn_Server;
};

/* Macro that performs normal method invocations on an object.
   Used in DoMethod(), CoerceMethod() and DoSuperMethod().
   Basically, the macro below uses the interfaceID part of the
   methodID to look up the class' hashtable of supported interfaces.
   Then it gets the method array for the interface, and uses
   the method offset part of the methodID to call the correct
   method.
*/

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
