#ifndef INTERN_H
#define INTERN_H

/*
    Copyright © 1995-2006, The AROS Development Team. All rights reserved.
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


/* Predeclaration */
struct IntOOPBase;

#ifndef PRIVATE_H
#   include "private.h"
#endif

#define SDEBUG 0
#define DEBUG 0

#define NUM_METHOD_BITS 10
#define METHOD_MASK ((1 << NUM_METHOD_BITS) - 1)


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




/* defines used to access the interface IDs stored in the library base */

#define __IRoot		(GetOBase(OOPBase)->ob_InternIDs[IDX_IRoot])
#define __IMeta		(GetOBase(OOPBase)->ob_InternIDs[IDX_IMeta])
#define __IMethod	(GetOBase(OOPBase)->ob_InternIDs[IDX_IMethod])
#define __IServer	(GetOBase(OOPBase)->ob_InternIDs[IDX_IServer])
#define __IProxy	(GetOBase(OOPBase)->ob_InternIDs[IDX_IProxy])
#define __IInterface	(GetOBase(OOPBase)->ob_InternIDs[IDX_IInterface])

#define GetOBase(lib)           ((struct IntOOPBase *)(lib))

/*****************
**  Prototypes  **
*****************/

BOOL GetIDs(struct IDDescr *idDesr, struct IntOOPBase *OOPBase);


/* rootclass.c */
BOOL init_rootclass(struct IntOOPBase *OOPBase);

/* metaclass.c */
BOOL init_basemeta(struct IntOOPBase *OOPBase);
BOOL init_ifmetaclass(struct IntOOPBase *OOPBase);

/* methodclass.c */
OOP_Class *init_methodclass(struct IntOOPBase *OOPBase);

/* serverclass.c */
OOP_Class *init_serverclass(struct Library *OOPBase);

/* proxyclass.c */
OOP_Class *init_proxyclass(struct Library *OOPBase);

/* interfaceclass.c */
OOP_Class *init_interfaceclass(struct Library *OOPBase);


OOP_Class *init_hiddmetaclass(struct IntOOPBase *OOPBase);

/* support.c */
BOOL hasinterface(OOP_Class *cl, STRPTR interface_id);
struct IFMethod *findinterface(OOP_Class *cl, STRPTR interface_id);
struct IFMethod *findmethod(OOP_Class *cl, STRPTR interface_id, ULONG method_offset);
BOOL init_methodbase(STRPTR interface_id, ULONG methodbase, ULONG *methodbase_ptr, struct IntOOPBase *OOPBase);
BOOL init_mi_methodbase(STRPTR interface_id, ULONG *methodbase_ptr, struct IntOOPBase *OOPBase);
VOID release_idbucket(STRPTR interface_id, struct IntOOPBase *OOPBase);
VOID obtain_idbucket(STRPTR interface_id, struct IntOOPBase *OOPBase);
/*****************
**  Structures  **
*****************/

/* Bucket for hashtable used to map string interface IDs to
** numeric interface IDs and attrbases.
*/

struct iid_bucket
{
    struct iid_bucket *next;
    STRPTR interface_id;
    ULONG  methodbase;
    ULONG  attrbase;
    ULONG refcount;
};



/* Definition of bucket for hashtable used to store
** the interfaces (method tables)
*/

struct IFBucket
{
    struct IFBucket *Next;
    ULONG InterfaceID;
    STRPTR GlobalInterfaceID;
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
    OOP_Class *mClass;
};


/* Message struct used for sending object message structs across
** to another task
*/
struct ProxyMsg
{
    struct Message 	pm_Message;
    OOP_Msg		pm_ObjMsg;
    IPTR		pm_RetVal;
    OOP_Object		*pm_Object;
};

/* Listnode that helps keeping track of servers. */
struct ServerNode
{
    struct Node sn_Node;
    OOP_Object	*sn_Server;
};


/* Instance data of base metaclass and rootclass */

struct metadata
{
    OOP_Class public; /* public part of class objects */
    OOP_Class *superclass;
    ULONG subclasscount;
    ULONG objectcount;
    ULONG instsize;
    BOOL  disptabs_inited;
    ULONG numinterfaces;
    
};

#define NUM_BASEMETA_IFS 2
struct basemetaobject /* Real instance data of the base meta class */
{
    OOP_Class  *oclass; /* The meta object's class */
    struct basemeta_inst
    {
    	struct metadata data;
    	struct IFMethod *iftable[NUM_BASEMETA_IFS]; /* has two interfaces: root and meta */
    
    	/* The interface tables */
    	struct IFMethod	rootif[num_Root_Methods];
    	struct IFMethod	metaif[NUMTOTAL_M_Meta];
    } inst;
};

struct rootclassobject
{
    OOP_Class *oclass; /* The rootclass object's class */
    struct rootinst
    {
    	struct metadata data;
    	struct IFMethod rootif[num_Root_Methods];
    } inst;
};


/* IFMeta class */

struct ifmeta_data
{
    
    /* The hashtable containing the interfaces (method tables) */
    struct HashTable *iftable;
    
    /* A direct pointer into the hashtable, for faster lookup */
    struct IFBucket **iftab_directptr;
    
    /* Used by the hash function. As the above field, it is put
    ** here for speed
    */
    ULONG hashmask;
    
    /* Number of interfaces in the hashtable */
    ULONG numinterfaces;

};


struct ifmetaobject
{
    OOP_Class *oclass; /* The interface metaclass' class (itself) */
    struct ifmeta_inst
    {
    	struct metadata base;
	struct ifmeta_data data;
	
    } inst;
};



/*************************
** Library Base struct  **
*************************/
struct IntOOPBase
{
    struct Library		 ob_LibNode;

    struct SignalSemaphore	 ob_ClassListLock;
    struct MinList		 ob_ClassList;
    
    struct SignalSemaphore	 ob_ServerListLock;
    struct MinList		 ob_ServerList;
    
    /* rootclass object */
    struct rootclassobject	 ob_RootClassObject;
    
    /* base metaclass object */
    struct basemetaobject	 ob_BaseMetaObject;
    
    /* interface metaclass object */
    struct ifmetaobject		 ob_IFMetaObject;
    
    /* HIDD metaclass */				
    OOP_Class 			 *ob_HIDDMetaClass;

    /* Store pointers to some usefull classes here */				 
    OOP_Class			 *ob_MethodClass;
    OOP_Class			 *ob_ServerClass;
    OOP_Class			 *ob_ProxyClass;
    OOP_Class			 *ob_InterfaceClass;
    
    /* Hashtable containing string interface ID => Numeric interface
    ** ID mappings. Used by GetMethodID() and GetAttrBase()
    */
    struct HashTable	       * ob_IIDTable;
    struct SignalSemaphore       ob_IIDTableLock;
    
    /* The currently lowest available numeric interfaceID that
    ** we can map a new interface string ID onto.
    ** Used by GetID()
    */
    
    ULONG			 ob_CurrentMethodBase;
    ULONG			 ob_CurrentAttrBase;
    
    /* An array of the interface IDs used internally. */
    ULONG			 ob_InternIDs[NUM_Indexes];
};

#define ROOTCLASSPTR (& (OOPBase)->ob_RootClassObject.inst.data.public)
#define BASEMETAPTR (& (OOPBase)->ob_BaseMetaObject.inst.data.public)

#endif /* INTERN_H */
