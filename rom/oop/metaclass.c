/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: OOP metaclass
    Lang: english
*/

#include <proto/exec.h>
#include <proto/utility.h>
#include <exec/memory.h>

#include <proto/oop.h>
#include <oop/oop.h>

#include "intern.h"
#include "hash.h"
#include "private.h"

#undef SDEBUG
#undef DEBUG
#define SDEBUG 0
#define DEBUG 0
#include <aros/debug.h>

#define MD(cl) ((struct metadata *)cl)
#define IFI(cl) ((struct ifmeta_inst *)cl)

#   define IntCallMethod(cl, o, msg) 					\
    {				    					\
    	register struct IFBucket *b;					\
    	register OOP_MethodID mid = *msg;				\
    	register ULONG ifid = mid & (~METHOD_MASK);			\
    	register struct IFMethod *method;				\
    									\
    	mid &= METHOD_MASK;						\
									\
	b = IFI(cl)->data.iftab_directptr[ifid & IFI(cl)->data.hashmask];	\
	while (b) 								\
   	{								\
       	    if (b->InterfaceID == ifid)					\
	    {								\
	    	method = &(b->MethodTable[mid]);			\
	    	return (method->MethodFunc(method->mClass, o, msg));	\
	    }    							\
            b = b->Next;						\
    	}								\
    	return (0UL);							\
     }



#define UB(x) ((UBYTE *)x)

/* Allocates and initializes the interface hashtable, and the methodtables */
static BOOL ifmeta_allocdisptabs(OOP_Class *cl, OOP_Object *o, struct P_meta_allocdisptabs *msg);
static VOID ifmeta_freedisptabs(OOP_Class *cl, OOP_Object *o, OOP_Msg msg);

static IPTR Meta_DoMethod(OOP_Object *o, OOP_Msg msg);
static IPTR Meta_CoerceMethod(OOP_Class *cl, OOP_Object *o, OOP_Msg msg);
static IPTR Meta_DoSuperMethod(OOP_Class *cl, OOP_Object *o, OOP_Msg msg);

/* Hooks */
VOID freebucket(struct Bucket *b, struct IntOOPBase *OOPBase);
struct Bucket *copybucket(struct Bucket *old_b, APTR data, struct IntOOPBase *OOPBase);

/* Internal */
static struct IFBucket *createbucket(
			 STRPTR			interface_id
			,ULONG			num_methods
			,struct IntOOPBase 	*OOPBase);
			
static ULONG calc_ht_entries(struct ifmeta_inst *cl
		,OOP_Class *super
		,struct OOP_InterfaceDescr *ifDescr
		,struct IntOOPBase *OOPBase);



/*
   The metaclass is used to create class. That means,
   classes are instances of the meta class.
   The meta class is itself both a class (you can
   create instances of it), and an object (you can invoke
   methods on it. 
*/   
   
   
#define OOPBase (GetOBase(cl->OOPBasePtr))

/********************
**  IFMeta::New()  **
********************/
static OOP_Object *ifmeta_new(OOP_Class *cl, OOP_Object *o, struct pRoot_New *msg)
{

    IPTR (*domethod)(OOP_Object *, OOP_Msg) = NULL;
    IPTR (*coercemethod)(OOP_Class *, OOP_Object *, OOP_Msg) 	= NULL;
    IPTR (*dosupermethod)(OOP_Class *, OOP_Object *, OOP_Msg) 	= NULL;

    EnterFunc(bug("IFMeta::New(cl=%s, msg = %p)\n",
    	cl->ClassNode.ln_Name, msg));

    /* Let the BaseMeta class initialize some stuff for us */
    o = (OOP_Object *)OOP_DoSuperMethod((OOP_Class *)cl, o, (OOP_Msg)msg);
    if (o)
    {
       
        struct ifmeta_inst *inst;
	
	inst = (struct ifmeta_inst *)o;
	
	domethod 	= (IPTR (*)())GetTagData(aMeta_DoMethod, 	NULL, msg->attrList);
	coercemethod 	= (IPTR (*)())GetTagData(aMeta_CoerceMethod, 	NULL, msg->attrList);
	dosupermethod	= (IPTR (*)())GetTagData(aMeta_DoSuperMethod, 	NULL, msg->attrList);
	

        D(bug("Instance allocated %p\n", inst));
	
		    
	if (!domethod)
	    domethod = Meta_DoMethod;
		    
	if (!coercemethod)
	    coercemethod = Meta_CoerceMethod;
		    
	if (!dosupermethod)
	{
	    
	    OOP_Class *superptr = inst->base.superclass;
	    if (superptr)
	    {
	    	D(bug("Got superptr: %p\n", superptr));
	    	/* Use superclass' DoSupermethod call if superclass isn't
	      	    an instance of the HIDDMetaClass
	    	*/
	    	if (OOP_OCLASS(superptr) != (OOP_Class *)cl)
	    	{
	    	    D(bug("superptr has different meta\n"));

	    	    dosupermethod = superptr->cl_DoSuperMethod;
	    	}
	    	else
	    	{
	    	    D(bug("superptr has same meta\n"));
	    
	    	    dosupermethod = Meta_DoSuperMethod;
	    	}
		
	    }
	    else /* if (class has no superclass) */
	    {
	    	dosupermethod = NULL;
	    }
	        
	}
	
	inst->base.public.OOPBasePtr	= (struct IntOOPBase *)OOPBase;
	
	inst->base.public.cl_DoMethod	= domethod;
	inst->base.public.cl_CoerceMethod	= coercemethod;
	inst->base.public.cl_DoSuperMethod	= dosupermethod;
	    
	D(bug("Classes' functions set\n"));
    }
    
    ReturnPtr ("IFMeta::New", OOP_Object *, o);   
}

/******************************
**  IFMeta::allocdisptabs()  **
******************************/

/* Allocates and initializes the interface hashtable, and the methodtables */
static BOOL ifmeta_allocdisptabs(OOP_Class *cl, OOP_Object *o, struct P_meta_allocdisptabs *msg)
{
    ULONG num_if;
    
    struct ifmeta_inst *inst = (struct ifmeta_inst *)o;
    
    EnterFunc(bug("IFMeta::allocdisptabs(cl=%p, o=%p,ifDescr=%p)\n",
    	cl, o, msg->ifdescr));
    
    /* Get number of needed hash entries */
    num_if = calc_ht_entries(inst, msg->superclass, msg->ifdescr, OOPBase);
    
    inst->base.numinterfaces = num_if;

    D(bug("numinterfaces set to %ld\n", num_if));
    
    /* Create a new integer hashtable, with a reasonable size */
    inst->data.iftable = NewHash(num_if, HT_INTEGER, OOPBase);
    if (inst->data.iftable)
    {
    	struct OOP_InterfaceDescr *ifdescr;
    	D(bug("Got iftable\n"));
    	/* Save hashmask for use in method lookup */
	inst->data.hashmask = HashMask(inst->data.iftable);
	
	if (msg->superclass) /* This test makes it work for initializing root classes */
	{

	    /* Copy parent interfaces into the new class */
	    struct IFMethod *superif;
	    struct P_meta_iterateifs ii_msg;
	    STRPTR interface_id;
	    ULONG num_methods;

	    /* must be initialized to zero */
	    IPTR iterval = 0UL;
	    
	    D(bug("Adding superclass' methods\n"));
	    
	    ii_msg.mid = OOP_GetMethodID(IID_Meta, MO_meta_iterateifs);
	    
	    ii_msg.iterval_ptr		= &iterval;
	    ii_msg.interface_id_ptr	= &interface_id;
	    ii_msg.num_methods_ptr	= &num_methods;
	    
	    
	    for (;;)
	    {
	    	struct IFBucket *ifb;
    		struct IFMethod *ifm = NULL;
    		ULONG mtab_size;

	    	superif = (struct IFMethod *)OOP_CoerceMethod(OOP_OCLASS(msg->superclass)
							     ,(OOP_Object *)msg->superclass
							     ,(OOP_Msg)&ii_msg);
		if (!superif)
		    break;
		    
		/* Allocate and insert the interface into the new class */
		ifb = createbucket(interface_id, num_methods, OOPBase);
		D(bug("Created bucket: %p\n", ifb));
		if (!ifb)
		    goto failure;
		    
		/* Copy the interface */
		mtab_size = UB (&ifm[num_methods]) - UB( &ifm[0]);
		D(bug("Copying from superclass methods for if %s, mtab_size=%d,basmetaroot %p, superif %p\n",
			ifb->GlobalInterfaceID, mtab_size, OOPBase->ob_BaseMetaObject.inst.rootif, superif));
			
		CopyMem(superif, ifb->MethodTable, mtab_size);
		InsertBucket(inst->data.iftable, (struct Bucket *)ifb, OOPBase);
		    
	    } /* for (;;) */
	
	} /* if (we inherit interfaces from some superclass) */
	
	/* Insert our own interfaces */
	D(bug("Inserting own methods\n"));
	for ( ifdescr = msg->ifdescr; ifdescr->MethodTable; ifdescr ++)
	{
	    struct IFBucket *ifb;
	    ULONG i;
	    
	    ULONG iid;
	    /* Get variable interface ID */
	    
	    D(bug("Getting Local ifID for global ID %s\n", ifdescr->InterfaceID));
	    if (!init_mi_methodbase(ifdescr->InterfaceID, &iid, OOPBase))
	    	goto failure;
	    
	    D(bug("Got local ifID %ld\n", iid));

	    /* Lookup hashtable to see if interface has been copied from superclass */
	    ifb = (struct IFBucket *)inst->data.iftable->Lookup(
	    		  inst->data.iftable
			, iid
			, OOPBase);

	    D(bug("tried to find bucket in hashtable: %p\n", ifb));
	    if (!ifb)
	    {
	    	D(bug("Bucket doesn't exist, creating..\n"));
	    	/* Bucket doesn't exist, allocate it */
		ifb = createbucket(ifdescr->InterfaceID, ifdescr->NumMethods, OOPBase);
	    	if (!ifb)
	    	    goto failure;
		else
		{
	    	    D(bug("Inserting bucket for IF %s\n", ifdescr->InterfaceID));
		    InsertBucket(inst->data.iftable, (struct Bucket *)ifb, OOPBase);
		}
	    }
		
	    
	    D(bug("overriding methods\n"));

	    /* Ovveride the superclass methods with our new ones */
	    for (i = 0; ifdescr->MethodTable[i].MethodFunc; i ++)
	    {
   	    	if (ifdescr->MethodTable[i].MethodFunc)
	    	{
	   	    ifb->MethodTable[ ifdescr->MethodTable[i].MethodIdx ].MethodFunc = ifdescr->MethodTable[i].MethodFunc;
	    	    ifb->MethodTable[ ifdescr->MethodTable[i].MethodIdx ].mClass     = (OOP_Class *)o;
	    	}
	    } /* for (each method in the interface) */
	    
	} /* for (each interface to add to class) */
	
	/* For speedup in method lookup */
	inst->data.iftab_directptr = (struct IFBucket **)inst->data.iftable->Table;
    	
	ReturnBool ("IFMeta::allocdisptabs", TRUE);
	
    } /* if (interface hash table allocated) */

failure:
    D(bug("FAILURE\n"));
    if (inst->data.iftable)
    	FreeHash(inst->data.iftable, freebucket, OOPBase);
    ReturnBool ("IFMeta::allocdisptabs", FALSE);
}

/*****************************
**  IFMeta::freedisptabs()  **
*****************************/
static VOID ifmeta_freedisptabs(OOP_Class *cl, OOP_Object *o, OOP_Msg msg)

{
    struct ifmeta_inst *inst = (struct ifmeta_inst *)o;
    /* This frees the hashtable + all buckets */
    
    FreeHash(inst->data.iftable, freebucket, OOPBase);
    
    return;
}

/**************************
**  IFMeta::getifinfo()  **
**************************/
static struct IFMethod *ifmeta_getifinfo(OOP_Class *cl, OOP_Object *o, struct P_meta_getifinfo *msg)
{
    ULONG iid;
    struct ifmeta_inst *inst = (struct ifmeta_inst *)o;
    struct IFMethod *mtab = NULL;
    struct IFBucket *b;
        
    /* Get the ULONG variable interface id */
    iid = OOP_GetMethodID(msg->interface_id, 0);
    
    /* Try looking it up in the class' table */
    b = (struct IFBucket *)inst->data.iftable->Lookup(inst->data.iftable, iid, OOPBase);
    {
    	*(msg->num_methods_ptr) = b->NumMethods;
	
    	mtab = b->MethodTable;
    }
    
    return mtab;
}

/***************************
**  IFMeta::iterateifs()  **
***************************/
static struct IFMethod *ifmeta_iterateifs(OOP_Class *cl, OOP_Object *o, struct P_meta_iterateifs *msg)
{
    struct HashTable *ht = ((struct ifmeta_inst *)o)->data.iftable;
    struct IFBucket *found_bucket = NULL; /* MUST default to NULL */
   
    struct IFBucket *ifb = NULL; /* keep compiler happy */
    

    struct IFMethod *current_if = NULL;     /* MUST default to NULL */

    UWORD last_idx = (*(msg->iterval_ptr)) >> 16;
    UWORD last_bucket_no = (*(msg->iterval_ptr)) & 0x0000FFFF;
    
    UWORD idx;
    
    UWORD bucket_no = 0; /* keep compiler happy */
    EnterFunc(bug("IFMeta::iterateifs(cl=%s, o=%s)\n",
    	cl->ClassNode.ln_Name, ((Class *)o)->ClassNode.ln_Name ));
	
    D(bug("last_idx: %ld, last_bucket_no=%ld\n", last_idx, last_bucket_no));
    

    for (idx = last_idx ;idx < HashSize(ht); idx ++)
    {
    	D(bug("idx=%ld\n", idx));
	
	bucket_no = 0;
	
    	for (ifb = (struct IFBucket *)ht->Table[idx]; ifb; )
	{
	    D(bug("ifb=%s, netx=%p, bucket_no=%ld\n",
	    	ifb->GlobalInterfaceID, ifb->Next, bucket_no));
	    /* Is this a new bucket in the iteration ? */
	    
	    if ((idx > last_idx) || (bucket_no >= last_bucket_no))
	    {
	    	found_bucket = ifb;
		
		/* Yes, it's a goto, but it really simplifies things here */
		goto after_loop;
	    }
	    else
	    {
	    	ifb = ifb->Next;
		bucket_no ++;
	    }
	    
	} /* for (all buckets at each idx) */
	
    } /* for (each entry at east index from last idx) */

after_loop:
    
    /* OK, found a bucket ? */
    if (found_bucket)
    {
    	D(bug("bucket found: %s at idx %ld, b_no %ld\n",
		found_bucket->GlobalInterfaceID, idx, bucket_no));
        *(msg->iterval_ptr)	 = (idx << 16) + bucket_no + 1;
	*(msg->interface_id_ptr) = ifb->GlobalInterfaceID;
	*(msg->num_methods_ptr)  = ifb->NumMethods;
	
	current_if = ifb->MethodTable;
    }

    ReturnPtr ("IFMeta::iterateifs", struct IFMethod *, current_if);
}

/***************************
**  IFMeta::findmethod()  **
***************************/

/* Used for finding a method for method objects */
static struct IFMethod *ifmeta_findmethod(OOP_Class *cl, OOP_Object *o, struct P_meta_findmethod *msg)
{
    register struct IFBucket *b;
    register ULONG method_offset;
    struct ifmeta_inst *inst = (struct ifmeta_inst *)o;
    
    /* Get interfaceID part of methodID */
    register ULONG ifid = msg->method_to_find & (~METHOD_MASK);	
    
    EnterFunc(bug("IFMeta::findmethod(o=%p, mid=%ld)\n", o, msg->method_to_find));
    

    /* Get method offset part of methdoID */
    method_offset =  msg->method_to_find & METHOD_MASK;
    
    /* Look up ID in hashtable and get linked list of buckets,
       storing interfaces
    */
    b = inst->data.iftab_directptr[ifid & inst->data.hashmask];
loop:
    if (b)
    {
    	/* Found correct interface ? */
        if (b->InterfaceID == ifid)
	{
	    /* Yep. Return method at supplied method offset */
	    ReturnPtr ("IFMeta::findmethod", struct IFMethod *, &(b->MethodTable[method_offset]));
    	}

        b = b->Next;
        goto loop;
    }
    /* Method not found, return NULL */
    ReturnPtr ("IFMeta::findmethod", struct IFMethod *, NULL);

}

#undef OOPBase

/**********
  Support
**********/

/*************************
**  init_ifmetaclass()  **
*************************/

#define NUM_META_METHODS 5
#define NUM_ROOT_METHODS 1
BOOL init_ifmetaclass(struct IntOOPBase *OOPBase)
{
    struct OOP_MethodDescr root_mdescr[NUM_ROOT_METHODS + 1]=
    {
    	{ (IPTR (*)())ifmeta_new,	moRoot_New		},
	{  NULL, 0UL }
    };

    struct OOP_MethodDescr meta_mdescr[NUM_META_METHODS + 1]=
    {
    	{ (IPTR (*)())ifmeta_allocdisptabs,	MO_meta_allocdisptabs	},
    	{ (IPTR (*)())ifmeta_freedisptabs,	MO_meta_freedisptabs	},
    	{ (IPTR (*)())ifmeta_getifinfo,		MO_meta_getifinfo	},
    	{ (IPTR (*)())ifmeta_iterateifs,	MO_meta_iterateifs	},
    	{ (IPTR (*)())ifmeta_findmethod,	MO_meta_findmethod	},
	{  NULL, 0UL }
    };
    

    struct OOP_InterfaceDescr meta_descr[] =
    {
    	{root_mdescr, IID_Root, 1},
    	{meta_mdescr, IID_Meta, NUM_META_METHODS},
	{NULL, NULL, 0UL}
    };
    
    struct ifmetaobject *imo = &(OOPBase->ob_IFMetaObject);
    struct P_meta_allocdisptabs adt_msg;
    OOP_Class *ifmeta_cl;
    
    EnterFunc(bug("init_ifmetaclass()\n"));

    ifmeta_cl = &(imo->inst.base.public);
    
    D(bug("Got ifmeta classptr\n"));
       
    imo->inst.base.superclass = BASEMETAPTR;
    imo->inst.base.public.OOPBasePtr = (struct OOPBase *)OOPBase;
    
    D(bug("Initialized ifmeta superclass\n"));
    
    adt_msg.superclass = imo->inst.base.superclass;
    adt_msg.ifdescr = meta_descr;
    
    /* allocdisptabs() must know the OOPBase */
    imo->inst.base.public.UserData		= (APTR)OOPBase;
    /* It must also have a valid DoSuperMethod(), more exatly
       the DoSuperMethod() of the BaseMeta class
    */
    imo->inst.base.public.cl_DoSuperMethod = BASEMETAPTR->cl_DoSuperMethod;
    
    
    D(bug("Allocating ifmeta disptabs\n"));

    
    if (ifmeta_allocdisptabs(ifmeta_cl, (OOP_Object *)ifmeta_cl, &adt_msg))
    {
    	D(bug("ifmeta disptabs allocated\n"));
    	/* initialize Class ID */

	imo->inst.base.public.ClassNode.ln_Name = CLID_MIMeta;
	imo->inst.base.public.InstOffset 	= sizeof (struct metadata);
	
	D(bug("IFMeta DoMethod=%p\n", Meta_DoMethod));
	imo->inst.base.public.cl_DoMethod		= Meta_DoMethod;
	imo->inst.base.public.cl_CoerceMethod	= Meta_CoerceMethod;

	imo->inst.base.instsize   = sizeof (struct ifmeta_data);
	imo->inst.base.subclasscount	= 0UL;
	imo->inst.base.objectcount	= 0UL;

	imo->inst.data.numinterfaces 	= 2UL;
	
	/* This class' class is itself */
    	imo->oclass = &(imo->inst.base.public);
	
/*	{
	   ULONG i;
	   D(bug("Trying to call get_if_info on ifmeta many times\n"));
	   for (i = 0; i < 10; i ++)
	   {
	   	ULONG num_methods;
		meta_getifinfo((OOP_Object *)imo->oclass, IID_Meta, &num_methods);
		
		D(bug("IF has %ld methods\n", num_methods));
	   }
	   
	}
*/	/* Make it public */
	OOP_AddClass(ifmeta_cl);
	ReturnBool ("init_metaclass", TRUE);
    }
    ReturnBool ("init_ifmetaclass", FALSE);

}


/************************
**  calc_ht_entries()  **
************************/

/* Calculates the number of interfaces the new class has
   ( == number of buckets in the hashtable)
*/
static ULONG calc_ht_entries(struct ifmeta_inst *cl
		,OOP_Class *super
		,struct OOP_InterfaceDescr *ifDescr
		,struct IntOOPBase *OOPBase)
{
    ULONG num_if = 0;
    
    EnterFunc(bug("calc_ht_entries(cl=%p, ifDescr=%p, super=%p)\n", cl, ifDescr, super));

  
 
    if (super)
    {
    	/* Get number of interfaces (method tables) in superclass */
	

    	num_if = MD(super)->numinterfaces;
	
	D(bug("Super-interfaces: %ld\n", num_if));
    
    	/* Check if there are any new interfaces in this class */
	
    	for (; ifDescr->MethodTable; ifDescr ++)
    	{
	    struct P_meta_getifinfo gii_msg;
	    ULONG num_methods;
	    
	    D(bug("Checking for interface %s\n", ifDescr->InterfaceID));
	    
	    gii_msg.mid = OOP_GetMethodID(IID_Meta, MO_meta_getifinfo);
	    gii_msg.interface_id = ifDescr->InterfaceID;
	    gii_msg.num_methods_ptr = &num_methods;
	    
	    
	    /* Does super support interface ? */
	    D(bug("Calling CoerceMethod on class %s\n", OOP_OCLASS(super)->ClassNode.ln_Name));
	    if (!OOP_CoerceMethod(OOP_OCLASS(super), (OOP_Object *)super, (OOP_Msg)&gii_msg))
	    {
	        D(bug("Found new interface: %s\n", ifDescr->InterfaceID));
		
		/* If it didn't then we have a new interface for this class */
	    	num_if ++;
	    }
	
    	} /* for (each interface in the description for the class) */
    
    }
    else
    {
    	/* This is a baseclass, count the interfaces */
    	for (; ifDescr->MethodTable; ifDescr ++)
    	{
	    num_if ++;
	
    	} /* for (each interface in the description for the class) */
    }
    ReturnInt ("calc_ht_entries", ULONG, num_if);
}






/*********************
**  createbucket()  **
*********************/
/* Creates a new interface bucket */
static struct IFBucket *createbucket(
			 STRPTR			interface_id
			,ULONG			num_methods
			,struct IntOOPBase 	*OOPBase)
{
    struct IFMethod *ifm = NULL;
    ULONG mtab_size = UB (&ifm[num_methods]) - UB( &ifm[0]);
    
    /* Allocate bucket */
    struct IFBucket *ifb;
    
    ifb = (struct IFBucket *)AllocMem( sizeof (struct IFBucket), MEMF_ANY );
    if (ifb)
    {
    	/* Allocate method table for this interface */
    	ifb->MethodTable = (struct IFMethod *)AllocVec(mtab_size, MEMF_ANY);
	if (ifb->MethodTable)
	{
	    /* Get correct ID for the interface (string ID => interface ID mapping) */

	    if (init_mi_methodbase(interface_id, &(ifb->InterfaceID), OOPBase))
	    {
	    	/* Save number of methods in the interface */
	    	ifb->NumMethods  = num_methods;
		
		/* Save the global string representations of the ID */
		ifb->GlobalInterfaceID = interface_id;
		return (ifb);

	    }
	}
	FreeMem (ifb, sizeof (struct IFBucket));
    }
    return (NULL);
}    
    

/***********************
**  Hash table hooks  **
***********************/
#define IB(x) ((struct IFBucket *)x)

VOID freebucket(struct Bucket *b, struct IntOOPBase *OOPBase)
{

    /* Free methodtable */
    FreeVec(IB(b)->MethodTable);

    /* Free the bucket itself */
    FreeMem(b, sizeof (struct IFBucket));
    

    return;
}

/* Copies a hashtable bucket */
struct Bucket *copyBucket(struct Bucket *old_b, APTR data, struct IntOOPBase *OOPBase)
{
    struct IFBucket *new_b;
    
    EnterFunc(bug("CopyBucket(old_b=%p)\n", old_b));
    
    /* Allocate memory for the new interface bucket */
    new_b = (struct IFBucket *)AllocMem(sizeof (struct IFBucket), MEMF_ANY );
    if (new_b)
    {
    	struct IFMethod *ifm = NULL;
        ULONG mtab_size;
	
	/* Get number of methods in source methodtable */
	ULONG numentries = IB(old_b)->NumMethods;
	
	mtab_size = UB(&ifm[numentries]) - UB(&ifm[0]);
	
	/* Allocate memory for methodtable of same size as source one */
    	new_b->MethodTable = (struct IFMethod *)AllocVec(mtab_size, MEMF_ANY);
	if (new_b->MethodTable)
	{
	    /* Copy methodtable to destination */
	    CopyMem(IB(old_b)->MethodTable, new_b->MethodTable, mtab_size);
	    
    	    /* Initialize bucket */
	    new_b->InterfaceID  	= IB(old_b)->InterfaceID;
	    new_b->NumMethods 		= IB(old_b)->NumMethods;
	    new_b->GlobalInterfaceID 	= IB(old_b)->GlobalInterfaceID;
	    	    
	    ReturnPtr ("CopyBucket", struct Bucket *, (struct Bucket *)new_b );
	}
	FreeMem (new_b, sizeof (struct IFBucket));
    }
    
    ReturnPtr ("CopyBucket", struct Bucket *, NULL);
}



/* Default function for calling DoMethod() on a local object */
/*****************
**  DoMethod()  **
*****************/

#define OOPBase ((struct IntOOPBase *)OOP_OOPBASE(object))

static IPTR Meta_DoMethod(OOP_Object *object, OOP_Msg msg)
{
    struct metadata *cl = (struct metadata *)OOP_OCLASS(object);
    
    
    /* Macro below defined in intern.h */
    IntCallMethod(cl, object, msg);

}


/*******************
**  CoerceMethod  **
*******************/
static IPTR Meta_CoerceMethod(OOP_Class *cl, OOP_Object *object, OOP_Msg msg)
{
    IntCallMethod(cl, object, msg);
}

/********************
**  DoSuperMethod  **
********************/
static IPTR Meta_DoSuperMethod(OOP_Class *cl, OOP_Object *object, OOP_Msg msg)
{
    cl = IFI(cl)->base.superclass;
    IntCallMethod(cl, object, msg);
}

#undef OOPBase
