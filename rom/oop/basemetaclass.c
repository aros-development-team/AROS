/*
    Copyright © 1995-2011, The AROS Development Team. All rights reserved.
    $Id$

    Desc: OOP base metaclass
    Lang: english
*/

#include <proto/exec.h>
#include <proto/oop.h>
#include <proto/utility.h>
#include <exec/alerts.h>
#include <exec/memory.h>
#include <oop/oop.h>
#include <string.h>

#include "intern.h"
#include "private.h"
#include "basemetaclass.h"

#undef SDEBUG
#undef DEBUG
#define SDEBUG 0
#define DEBUG 0
#include <aros/debug.h>

#define MD(x) ((struct metadata *)x)

/*****************************************************************************************

    NAME
	--background_meta--

    LOCATION
	Meta

    NOTES
	Classes are objects of metaclasses, so therefore
	classes are created with OOP_NewObject().

	As of now there are three different metaclasses available:

	  mimetaclass (CLID_MIMeta)
	  - Creates classes that supports multiple interfaces.

	  simetaclass (CLID_SIMeta)
	  - Creates classes that can only support single interfaces. Advantage is faster
	    method invocation (doesn't require hashing).

	How to create a class is best shown through an example.
	Here is a Timer class with two simple methods,
	Start and Stop.
	Note, this example doesn't show the New and Dispose
	methods and OOP_DoSuperMethod() usage, but it is exactly the same as in BOOPSI,
	so those familiar with BOOPSI, should find creating classes with this system simple.

	// In the classes' include file you have to define class ID, interface ID
	// method offsets and attribute offset
	#define CLID_Timer	"timerclass"
	#define IID_Timer	"I_timer"

	// Method offset for methods in the IID_Timer interface.
	enum
	{
	    moTimer_Start	= 0,
	    moTimer_Stop,

	    Num_Timer_Methods	// number of methods in the Timer interface
	};

	// Attribute offsets for attrs in the IID_Timer interface.
	enum
	{
	    aoTimer_Elapsed = 0,

	    Num_Timer_Attrs	// number of attrs in the timer interface
	};

	// private instance data
	struct timer_data
	{
	    struct timeval start_time;
	    struct timeval elapsed_time;
	};

	// The methods
	static VOID timer_start(Class *cl, Object *o, Msg msg)
	{
	    struct timer_data *data;

	    data = INST_DATA(tcl, o);

	    gettimeofday(&(data->start_time), NULL);

	    return;
	}

	static VOID timer_stop(Class *cl, Object *o, Msg msg)
	{
	    struct timer_data *data = INST_DATA(tcl, o);
	    gettimeofday(&(data->elapsed_time), NULL);

	    SubTime(&(data->elapsed_time), &(data->start_time));

	    return;
	}

	#define NUM_TIMER_METHODS 2
	Class *make_timerclass()
	{
	    struct MethodDescr methods[NUM_TIMER_METHODS + 1] =
	    {
		{(IPTR (*)())timer_start,		moTimer_Start},
		{(IPTR (*)())timer_stop,		moTimer_Stop},
		{NULL, 0UL} // must be null-terminated
	    };

	    struct InterfaceDescr ifdescr[] =
	    {
	    	{ methods, "Timer", NUM_TIMER_METHODS },
		{ NULL, 0UL, 0UL} // must be null-terminated
	    };

	    struct TagItem tags[] =
	    {
	        {aMeta_SuperID,		(IPTR)CLID_Root},
		{aMeta_InterfaceDescr,	(IPTR)ifdescr},
		{aMeta_ID,		(IPTR)CLID_Timer},
		{aMeta_InstSize,	(IPTR)sizeof (struct timer_data)},
		{TAG_DONE, 0UL}
	    };

	    Class *tcl;

	    // Make it a class of the SIMeta
	    tcl = (Class *)OOP_NewObject(NULL, CLID_SIMeta, tags);

	    if (tcl)
	    {
	    	// Make the class public
	    	OOP_AddClass(tcl);
	    }

	    return tcl;
	}

	VOID free_timerclass(Class *cl)
	{
	    OOP_DisposeObject((Object *)cl);

	    return;
	}

*****************************************************************************************/

/*****************************************************************************************

    NAME
	--naming_conventions--

    LOCATION
	Meta

    NOTES
    	This section describes the recommented convention for naming attributes and methods.

	Method and attribute offsets are constructed like this:

	method offset:
	  mo<interface>_<method name>  (eg. moTimer_Start)

	attribute offset:
	  ao<interface>_<attrname>  (eg. aoTimer_Elapsed)

	or moHidd_GC_SetPixel and aoHidd_GC_FgPen

	Macro specifying class ID is defined like this:
	CLID_<system>_<class name> (eg. CLID_Hidd_Gfx )

	And interface IDs like this.
	IID_<system>_<interface name> (eg. IID_Hidd_Gfx )

	ID themselves are strings.

*****************************************************************************************/

/*****************************************************************************************

    NAME
	aoMeta_ID

    SYNOPSIS
        [I..], CONST_STRPTR

    LOCATION
	Meta

    FUNCTION
	Specifies the class ID for the class.

*****************************************************************************************/

/*****************************************************************************************

    NAME
	aoMeta_SuperID

    SYNOPSIS
    	[I..], CONST_STRPTR

    LOCATION
	Meta

    FUNCTION
	ID of public class that will be superclass of class to be created.

*****************************************************************************************/

/*****************************************************************************************

    NAME
	aoMeta_SuperPtr

    SYNOPSIS
    	[I..], OOP_Class *

    LOCATION
	Meta

    FUNCTION
	Pointer to private class that will be superclass to
	class created.

*****************************************************************************************/

/*****************************************************************************************
	
    NAME
	aoMeta_InstSize

    SYNOPSIS
    	[I..], ULONG

    FUNCTION
	Size of the instance data for this class.
	Note, this is not necessarily the same as the size of the whole
	object of this class.

*****************************************************************************************/

/*****************************************************************************************

    NAME
	aoMeta_InterfaceDescr

    SYNOPSIS
	[I..],	struct InterfaceDescr *

    LOCATION
	Meta

    FUNCTION
	Pointer to an array of interface descriptors (struct InterfaceDescr).
	This array has to be null-terminated.

	Each

	struct InterfaceDescr
	{
    		struct MethodDescr *MethodTable;
    		CONST_STRPTR InterfaceID;
    		ULONG NumMethods;
	};

	describes an interface of the class.
	The MethodTable is an array of 

	struct MethodDescr
	{
   		IPTR (*MethodFunc)();
		ULONG MethodIdx;
	};
	
	which describes each method's implementation.

    EXAMPLE
	struct MethodDescr root_mdescr[NUM_ROOT_METHODS + 1] =
	{
    	    { (IPTR (*)())unixio_new,	  moRoot_New		},
    	    { (IPTR (*)())unixio_dispose, moRoot_Dispose	},
    	    { NULL, 0UL }
        };

	struct MethodDescr unixio_mdescr[NUM_UNIXIO_METHODS + 1] =
    	{
    	    { (IPTR (*)())unixio_wait,	moHidd_UnixIO_Wait	},
    	    { NULL, 0UL }
        };

        struct InterfaceDescr ifdescr[] =
	{
    	    {root_mdescr, IID_Root, NUM_ROOT_METHODS},
	    {unixio_mdescr, IID_UnixIO, NUM_UNIXIO_METHODS},
	    {NULL, NULL, 0UL}
        };
	
        struct TagItem tags[] =
    	{
            {aMeta_SuperID,			(IPTR)CLID_Hidd},
	    {aMeta_InterfaceDescr,		(IPTR)ifdescr},
	    {aMeta_ID,			(IPTR)CLID_UnixIO_Hidd},
	    {aMeta_InstSize,		(IPTR)sizeof (struct UnixIOData) },
	    {TAG_DONE, 0UL}
    	};
    
	...

	cl = NewObjectA(NULL, CLID_HIDDMeta, tags);

    BUGS
    	InterfaceDescr->NumMethods field was originally intended to specify
    	size of internal method table. When creating a new interface (i. e.
    	if this is your own interface), you need to be sure that the value
    	you set there is equal to highest possible method number + 1.
    	
    	Since v42.1 oop.library always ensures that methods table has enough
    	entries to accomodate all defined methods. NumMethods field in interface
    	descriptor is effectively ignored and is present only for backwards
    	compatibility.

*****************************************************************************************/

/***************************
**  RootClass' metaclass  **
***************************/
/* The root class' meta class is not really needed, but 
   it makes code of other metaclasses more consistent
*/

#define IS_META_ATTR(attr, idx) ( (idx = attr - MetaAttrBase) < num_Meta_Attrs )
/**********************
**  BaseMeta::New()  **
**********************/
static OOP_Object *basemeta_new(OOP_Class *cl, OOP_Object *o, struct pRoot_New *msg)
{
    struct IntOOPBase *OOPBase = (struct IntOOPBase *)cl->OOPBasePtr;
    struct Library *UtilityBase = OOPBase->ob_UtilityBase;
    struct metadata *data;

    struct OOP_InterfaceDescr *ifdescr = NULL;
    CONST_STRPTR superid = NULL, clid = "-- private class -- ";
    struct metadata *superptr = NULL;
    struct TagItem *tag, *tstate;
    ULONG instsize = (ULONG)-1L;
    
    ULONG idx;

    EnterFunc(bug("BaseMeta::New(cl=%s, msg = %p)\n",
    	cl->ClassNode.ln_Name, msg));

    /* Analyze the taglist before object is allocated,
    ** so we can easily exit cleanly if some info is missing
    */

    tstate = msg->attrList;

    while ((tag = NextTagItem(&tstate)))
    {    
        if (IS_META_ATTR(tag->ti_Tag, idx))
	{
	    D(bug("Got meta attr %lx with TagIdx %ld\n",
	    	tag->ti_Tag, idx ));
	    
	    switch (idx)
	    {
	    
	    case aoMeta_SuperID:
		/* ID of superclass */
		superid = (CONST_STRPTR)tag->ti_Data;
	        D(bug("Got superID: %s\n", superid));
		break;
		    
	    case aoMeta_InterfaceDescr:
	        D(bug("Got ifdescr\n"));
		/* What interfaces does the class support ? */
		ifdescr = (struct OOP_InterfaceDescr *)tag->ti_Data;
		break;
		    
	    case aoMeta_ID:
		/* The new class' ID */
		clid = (CONST_STRPTR)tag->ti_Data;
	        D(bug("Got classID: %s\n", clid));
		break;
		    
	    case aoMeta_SuperPtr:
	        D(bug("Got superPtr\n"));
		/* If the super class is private, than we must have
		   a pointer to it.
		*/
		superptr = (struct metadata *)tag->ti_Data;
		break;
		
	    case aoMeta_InstSize:
	        /* Instance data size for the new class */
	        instsize = (ULONG)tag->ti_Data;
		break;
		
	    }
	    
	}
	
    }
    
    /* The user must supply instance size */
    if (instsize == (ULONG)-1)
    	ReturnPtr ("Meta::New, no instsize", OOP_Object *, NULL);
    
    /* The new class must have interfaces */
    if (!ifdescr)
    	ReturnPtr ("Meta::New, no ifdescr", OOP_Object *, NULL);

    /* The new class must have a superclass */
    if (!superptr)
    {
	if (superid)
	{
	    superptr = (struct metadata *)FindName((struct List *)&(GetOBase(OOPBase)->ob_ClassList), superid);
	    if (!superptr)
	    	ReturnPtr ("Meta::New, no superptr/id", OOP_Object *, NULL);
	}
    }
    
    /* We are sure we have enough args, and can let rootclass alloc the instance data */
    o = (OOP_Object *)OOP_DoSuperMethod((OOP_Class *)cl, o, (OOP_Msg)msg);
    if (o)
    {

	ULONG dispose_mid = OOP_GetMethodID(IID_Root, moRoot_Dispose);

        D(bug("Instance allocated\n"));
	
	data = OOP_INST_DATA(cl, o);
	
	D(bug("o=%p,data=%p\n", o, data));
	D(bug("instoffset: %ld\n", cl->InstOffset));
	
	/* Clear instdata, so we in Dispose() can see what's been allocated */
	memset(data, 0, sizeof (struct metadata));
	
	D(bug("superptr=%p\n", superptr));

    	data->public.OOPBasePtr = (struct IntOOPBase *)OOPBase;

	
	/* Let subclass create an initialize dispatch tables for the new class object*/
	if (meta_allocdisptabs(o, (OOP_Class *)superptr, ifdescr))
	{
	    data->disptabs_inited = TRUE;
	
	    /* Copy the class' ID */
	    D(bug("Allocating class ID\n"));
	    data->public.ClassNode.ln_Name = AllocVec(strlen (clid) + 1, MEMF_ANY);
	    if (data->public.ClassNode.ln_Name)
	    {
	    	D(bug("class ID allocated\n"));
	    
	    	/* Initialize class fields */
		D(bug("Setting instoffset\n"));
		/* Instoffset */
		if (superptr)
	    	    data->public.InstOffset = superptr->public.InstOffset + superptr->instsize;
		else
		    data->public.InstOffset = 0UL;
		D(bug("Setting other stuff\n"));
				
	    	data->subclasscount 	= 0UL;
	    	data->objectcount	= 0UL;
	    	data->public.superclass	= (OOP_Class *)superptr;
		data->instsize 	= instsize;
		
		D(bug("Copying class ID\n"));
		/* Copy class ID */
		strcpy(data->public.ClassNode.ln_Name, clid);
		

    	    	ReturnPtr ("Meta::New", OOP_Object *, o);
	    }
	}

	D(bug("Failed to allocate dispatch tables\n"));

	OOP_CoerceMethod((OOP_Class *)cl, o, (OOP_Msg)&dispose_mid);
    }
    
    ReturnPtr ("Meta::New", OOP_Object *, NULL);   
    
}

/**************************
**  BaseMeta::Dispose()  **
**************************/
static VOID basemeta_dispose(OOP_Class *cl, OOP_Object *o, OOP_Msg msg)
{
    struct IntOOPBase *OOPBase = (struct IntOOPBase *)cl->OOPBasePtr;
    struct metadata *data = OOP_INST_DATA(cl, o);
    IPTR iterval = 0UL;
    CONST_STRPTR interface_id = NULL;
    ULONG num_methods = 0UL;
    
    if (data->public.ClassNode.ln_Name)
    	FreeVec(data->public.ClassNode.ln_Name);
	
    /* Release interfaces from global interface table */
    while (meta_iterateifs(o, &iterval, &interface_id, &num_methods))
    {
    	/* Only release the interfaces that were new for the class */
	if (!meta_getifinfo((OOP_Object *)MD(o)->public.superclass, interface_id, &num_methods))
    	     release_idbucket(interface_id, GetOBase(OOPBase));
    }
    

    if (data->disptabs_inited)
    	meta_freedisptabs(o);

    OOP_DoSuperMethod(cl, o, msg);
    
    return;
}


/****************************
**  BaseMeta::getifinfo()  **
****************************/
static struct IFMethod *basemeta_getifinfo(OOP_Class *cl, OOP_Object *o, struct P_meta_getifinfo *msg)
{
    struct IntOOPBase *OOPBase = (struct IntOOPBase *)cl->OOPBasePtr;
    struct IFMethod *mtab = NULL;
    EnterFunc(bug("BaseMeta::hasinterface(cl=%p, o=%p, iid=%s\n",
    	cl, o, msg->interface_id));
	
    /* The object passed might be one of two classes: Root class or basemetaclass */
    if (0 == strcmp(msg->interface_id, IID_Root))
    {
        /* Both classes support the root interface */
	D(bug("Root interface\n"));
	*(msg->num_methods_ptr) = num_Root_Methods;
	if ( ((OOP_Class *)o) == BASEMETAPTR)
	{
	    mtab = OOPBase->ob_BaseMetaObject.inst.rootif;
	}
	else
	{
	    mtab = OOPBase->ob_RootClassObject.inst.rootif;
	}

    }
    else if (0 == strcmp(msg->interface_id, IID_Meta))
    {
    	D(bug("Meta interface. BASEMETAPTR: %p\n", BASEMETAPTR));
    	if ( ((OOP_Class *)o) == BASEMETAPTR )
	{
	
	    /* Only BaseMeta has Meta interface */
	    mtab = OOPBase->ob_BaseMetaObject.inst.metaif;
	    *(msg->num_methods_ptr) = NUMTOTAL_M_Meta;
	}
	
    }
    ReturnPtr ("BaseMeta::hasinterface", struct IFMethod *, mtab);
    
}

/*****************************
**  BaseMeta::iterateifs()  **
*****************************/
static struct IFMethod *basemeta_iterateifs(
			OOP_Class *cl, OOP_Object *o, struct P_meta_iterateifs *msg)
{
    struct IntOOPBase *OOPBase = (struct IntOOPBase *)cl->OOPBasePtr;
    struct IFMethod *current_if = NULL;
    
    EnterFunc(bug("BaseMeta::iterateifs(o=%p)\n", o));
    
    /* As in has_interface() the object here can only be the basemetaclass, or rootclass */
    if (((OOP_Class *)o) == ROOTCLASSPTR)
    {
    	/* Rootclass have only one interface */
        if ( *(msg->iterval_ptr) )
	{
	    current_if = NULL;
	}
	else
	{
	    current_if = OOPBase->ob_RootClassObject.inst.rootif;
	    *(msg->num_methods_ptr)	= num_Root_Methods;
	    *(msg->interface_id_ptr)	= IID_Root;
	    *(msg->iterval_ptr) = 1UL; /* We're through iterating */
	}
    }
    else if (((OOP_Class *)o) == BASEMETAPTR)
    {
    	struct basemeta_inst *inst = (struct basemeta_inst *)o;
    	switch (*(msg->iterval_ptr))
	{
	    case 0:
	    	current_if  = inst->rootif;
		*(msg->num_methods_ptr)  = num_Root_Methods;
		*(msg->interface_id_ptr) = IID_Root;
		break;
	    
	    case 1:
	    	current_if  = inst->metaif;
		*(msg->num_methods_ptr)  = NUMTOTAL_M_Meta;
		*(msg->interface_id_ptr) = IID_Meta;
		break;
	    
	    default:
  	    	current_if = NULL;
		break;
		
	}
	(*(msg->iterval_ptr)) ++;

    }
    else
    {
    	/* Should never get here, unless someone has created an instance
	   of the BaseMeta class (which is meaningless)
	*/
	current_if = NULL;
    }
#if DEBUG
    if (current_if)
    {
    	D(bug("Current IF: %s, num_methods %ld\n",
    		*(msg->interface_id_ptr), *(msg->num_methods_ptr)));
    }
#endif
    ReturnPtr ("BaseMeta::iterate_ifs", struct IFMethod *, current_if);
    
}			

/*******************************
**  BaseMeta DoSuperMethod()  **
*******************************/
/* cl->USerData passed to DoSuperMethodA might be
   a subclass of rootclass, which does not have
   the OOPBase in cl->UserData, so instead we use the
   meta's UserData (IFMeta or HIDDMeta class
*/   
   
IPTR basemeta_dosupermethod(OOP_Class *cl, OOP_Object *o, OOP_Msg msg)
{
    struct IntOOPBase *OOPBase = (struct IntOOPBase *)cl->OOPBasePtr;
    struct IFMethod *ifm;
    IPTR ret;
    
    EnterFunc(bug("basemeta_dosupermethod(cl=%p, o=%p, msg=%p)\n",
    	cl, o, msg));

    if (MD(cl)->public.superclass == ROOTCLASSPTR)
    {
    	ifm = &(OOPBase->ob_RootClassObject.inst.rootif[*msg]);

    	ret = ifm->MethodFunc(ifm->mClass, o, msg);
    }
    else /* superclass is the BaseMeta class */
    	ret = basemeta_coercemethod(cl, o, msg);

    ReturnPtr ("basemeta_dosupermethod", IPTR, ret);
}

/*******************************
**  BaseMeta CoerceMethod()  **
*******************************/
IPTR basemeta_coercemethod(OOP_Class *cl, OOP_Object *o, OOP_Msg msg)
{
    struct IntOOPBase *OOPBase = (struct IntOOPBase *)cl->OOPBasePtr;
    ULONG method_offset = *msg & METHOD_MASK;
    struct IFMethod *ifm;
    
    EnterFunc(bug("basemeta_coercemethod(cl=%p, o=%p, msg=%p)\n", cl, o, msg));

    switch (*msg >> NUM_METHOD_BITS)
    {
    
        case 0:
	    ifm = &(OOPBase->ob_BaseMetaObject.inst.rootif[method_offset]);
	    break;
	    
	case 1:
	    ifm = &(OOPBase->ob_BaseMetaObject.inst.metaif[method_offset]);
	    break;
	    
	
	default:
	    bug("Error: basemeta_coercemethod got method call to unknown interface %d\n", *msg >> NUM_METHOD_BITS);

	    /* Throw an alert to be able to see a stacktrace */
	    Alert(AN_OOP);
	    return 0;
    }
    ReturnPtr ("basemeta_coercemethod", IPTR, ifm->MethodFunc(ifm->mClass, o, msg));
}

/************************
**  BaseMeta DoMethod  **
************************/

IPTR basemeta_domethod(OOP_Object *o, OOP_Msg msg)
{
    return basemeta_coercemethod(OOP_OCLASS(o), o, msg);
}

/**********************
**  init_basemeta()  **
**********************/

BOOL init_basemeta(struct IntOOPBase *OOPBase)
{
    struct basemetaobject *bmo;
    BOOL success;
    ULONG mbase = 0;
    
    EnterFunc(bug("init_basemeta()\n"));
    
    bmo = &(OOPBase->ob_BaseMetaObject);
    bmo->oclass = BASEMETAPTR;
    
    bmo->inst.data.public.ClassNode.ln_Name = "private base metaclass";
    bmo->inst.data.public.OOPBasePtr	= OOPBase;
    bmo->inst.data.public.InstOffset 	= 0UL;
    bmo->inst.data.public.UserData 	= OOPBase;
    bmo->inst.data.public.cl_DoSuperMethod = basemeta_dosupermethod;
    bmo->inst.data.public.cl_CoerceMethod 	= basemeta_coercemethod;
    bmo->inst.data.public.cl_DoMethod 	= basemeta_domethod;
    	
    bmo->inst.data.public.superclass  	= ROOTCLASSPTR;
    bmo->inst.data.subclasscount 	= 0UL;
    bmo->inst.data.objectcount 		= 0UL;
    bmo->inst.data.instsize		= sizeof (struct metadata);
    bmo->inst.data.numinterfaces	= NUM_BASEMETA_IFS;
    
    /* Initialize interface table */
    bmo->inst.iftable[0] = bmo->inst.rootif;
    bmo->inst.iftable[1] = bmo->inst.metaif;
    
    /* initialize interfaces */
    bmo->inst.rootif[moRoot_New].MethodFunc     = (IPTR (*)())basemeta_new;
    bmo->inst.rootif[moRoot_Dispose].MethodFunc = (IPTR (*)())basemeta_dispose;

    bmo->inst.rootif[moRoot_New].mClass     	= BASEMETAPTR;
    bmo->inst.rootif[moRoot_Dispose].mClass  = BASEMETAPTR;
    
    /* Initialize meta interface */
    bmo->inst.metaif[MO_meta_allocdisptabs].MethodFunc 	= (IPTR (*)())NULL;
    bmo->inst.metaif[MO_meta_freedisptabs].MethodFunc		= (IPTR (*)())NULL;
    bmo->inst.metaif[MO_meta_getifinfo].MethodFunc		= (IPTR (*)())basemeta_getifinfo;
    bmo->inst.metaif[MO_meta_iterateifs].MethodFunc 		= (IPTR (*)())basemeta_iterateifs;
    
    bmo->inst.metaif[MO_meta_allocdisptabs].mClass 	= BASEMETAPTR;
    bmo->inst.metaif[MO_meta_freedisptabs].mClass 	= BASEMETAPTR;
    bmo->inst.metaif[MO_meta_getifinfo].mClass 		= BASEMETAPTR;
    bmo->inst.metaif[MO_meta_iterateifs].mClass 	= BASEMETAPTR;
    
    /* Meta interface ID gets initialized to 1 */
    success = init_mi_methodbase(IID_Meta, &mbase, OOPBase);
    
    ReturnBool ("init_basemeta", success);    
}
