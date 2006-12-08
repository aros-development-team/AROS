/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: OOP rootclass
    Lang: english
*/

#include <proto/exec.h>
#include <proto/oop.h>
#include <proto/utility.h>
#include <exec/memory.h>
#include <oop/oop.h>
#include <string.h>

#include "intern.h"
#include "private.h"

#undef SDEBUG
#undef DEBUG
#define SDEBUG 0
#define DEBUG 0
#include <aros/debug.h>

#define MD(x) ((struct metadata *)x)

/***************************
**  RootClass' metaclass  **
***************************/
/* The root class' meta class is not really needed, but 
   it makes code of other metaclasses more consistent
*/

#define OOPBase ((struct IntOOPBase *)(cl->OOPBasePtr))
#define IS_META_ATTR(attr, idx) ( (idx = attr - MetaAttrBase) < num_Meta_Attrs )
/**********************
**  BaseMeta::New()  **
**********************/
static OOP_Object *basemeta_new(OOP_Class *cl, OOP_Object *o, struct pRoot_New *msg)
{
    struct metadata *data;

    struct OOP_InterfaceDescr *ifdescr = NULL;
    STRPTR superid = NULL, clid = "-- private class -- ";
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
    while ((tag = NextTagItem((const struct TagItem **)&tstate)))
    {
        if (IS_META_ATTR(tag->ti_Tag, idx))
	{
	    D(bug("Got meta attr %lx with TagIdx %ld\n",
	    	tag->ti_Tag, idx ));
	    
	    switch (idx)
	    {
	    
	    case aoMeta_SuperID:
		/* ID of superclass */
		superid = (STRPTR)tag->ti_Data;
	        D(bug("Got superID: %s\n", superid));
		break;
		    
	    case aoMeta_InterfaceDescr:
	        D(bug("Got ifdescr\n"));
		/* What interfaces does the class support ? */
		ifdescr = (struct OOP_InterfaceDescr *)tag->ti_Data;
		break;
		    
	    case aoMeta_ID:
		/* The new class' ID */
		clid = (STRPTR)tag->ti_Data;
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
	    	data->superclass	= (OOP_Class *)superptr;
		data->instsize 	= instsize;
		
		D(bug("Copying class ID\n"));
		/* Copy class ID */
		strcpy(data->public.ClassNode.ln_Name, clid);
		

    	    	ReturnPtr ("Meta::New", OOP_Object *, o);
	    }
	}

	OOP_CoerceMethod((OOP_Class *)cl, o, (OOP_Msg)&dispose_mid);
    }
    
    ReturnPtr ("Meta::New", OOP_Object *, NULL);   
    
}

/**************************
**  BaseMeta::Dispose()  **
**************************/
static VOID basemeta_dispose(OOP_Class *cl, OOP_Object *o, OOP_Msg msg)
{
    struct metadata *data = OOP_INST_DATA(cl, o);
    IPTR iterval = 0UL;
    STRPTR interface_id = NULL;
    ULONG num_methods = 0UL;
    
    if (data->public.ClassNode.ln_Name)
    	FreeVec(data->public.ClassNode.ln_Name);
	
    /* Release interfaces from global interface table */
    while (meta_iterateifs(o, &iterval, &interface_id, &num_methods))
    {
    	/* Only release the interfaces that were new for the class */
	if (!meta_getifinfo((OOP_Object *)MD(o)->superclass, interface_id, &num_methods))
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

#undef OOPBase

/*******************************
**  BaseMeta DoSuperMethod()  **
*******************************/
/* cl->USerData passed to DoSuperMethodA might be
   a subclass of rootclass, which does not have
   the OOPBase in cl->UserData, so instead we use the
   meta's UserData (IFMeta or HIDDMeta class
*/   
   
#define OOPBase ((struct IntOOPBase *)cl->OOPBasePtr)

static IPTR basemeta_dosupermethod(OOP_Class *cl, OOP_Object *o, OOP_Msg msg)
{
    ULONG method_offset = *msg & METHOD_MASK;
    struct IFMethod *ifm;
    
    EnterFunc(bug("basemeta_dosupermethod(cl=%p, o=%p, msg=%p)\n",
    	cl, o, msg));

    if (MD(cl)->superclass == ROOTCLASSPTR)
    {
    	ifm = &(OOPBase->ob_RootClassObject.inst.rootif[*msg]);
    }
    else /* superclass is the BaseMeta class */
    {
    	switch (*msg >> NUM_METHOD_BITS)
    	{
    
        case 0:
	    ifm = &(OOPBase->ob_BaseMetaObject.inst.rootif[method_offset]);
	    break;
	    
	case 1:
	    ifm = &(OOPBase->ob_BaseMetaObject.inst.metaif[method_offset]);
	    break;
	    
	
	default:
	    D(bug("Error: basemeta_dosupermethod got method call to unknown interface %d\n",
	    	*msg >> NUM_METHOD_BITS));
	    ifm = NULL;
	    break;
    	}
    }
    ReturnPtr ("basemeta_dosupermethod", IPTR, ifm->MethodFunc(ifm->mClass, o, msg));
}

#undef OOPBase

#define OOPBase ((struct IntOOPBase *)(cl->OOPBasePtr))
/*******************************
**  BaseMeta CoerceMethod()  **
*******************************/
static IPTR basemeta_coercemethod(OOP_Class *cl, OOP_Object *o, OOP_Msg msg)
{
    ULONG method_offset = *msg & METHOD_MASK;
    struct IFMethod *ifm;
    
    EnterFunc(bug("basemeta_coercemethod(cl=%p, o=%p, msg=%p)\n",
    	cl, o, msg));
	
    
    switch (*msg >> NUM_METHOD_BITS)
    {
    
        case 0:
	    ifm = &(OOPBase->ob_BaseMetaObject.inst.rootif[method_offset]);
	    break;
	    
	case 1:
	    ifm = &(OOPBase->ob_BaseMetaObject.inst.metaif[method_offset]);
	    break;
	    
	
	default:
	    D(bug("Error: basemeta_coercemethod got method call to unknown interface %d\n",
	    	*msg >> NUM_METHOD_BITS));
	    ifm = NULL;
	    break;
    }
    ReturnPtr ("basemeta_coercemethod", IPTR, ifm->MethodFunc(ifm->mClass, o, msg));
}

/************************
**  BaseMeta DoMethod  **
************************/

static IPTR basemeta_domethod(OOP_Object *o, OOP_Msg msg)
{
    return basemeta_coercemethod(OOP_OCLASS(o), o, msg);
}
#undef OOPBase

/**********************
**  init_basemeta()  **
**********************/

BOOL init_basemeta(struct IntOOPBase *OOPBase)
{
    struct basemetaobject *bmo;
    BOOL success;
    ULONG mbase = NULL;
    
    EnterFunc(bug("init_basemeta()\n"));
    
    bmo = &(OOPBase->ob_BaseMetaObject);
    bmo->oclass = BASEMETAPTR;
    
    bmo->inst.data.public.ClassNode.ln_Name = "private base metaclass";
    bmo->inst.data.public.OOPBasePtr	= (struct Library *)OOPBase;
    bmo->inst.data.public.InstOffset 	= 0UL;
    bmo->inst.data.public.UserData 	= OOPBase;
    bmo->inst.data.public.cl_DoSuperMethod = basemeta_dosupermethod;
    bmo->inst.data.public.cl_CoerceMethod 	= basemeta_coercemethod;
    bmo->inst.data.public.cl_DoMethod 	= basemeta_domethod;
    	
    bmo->inst.data.superclass  		= ROOTCLASSPTR;
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

/* Root class is the base class of all classes.
   (Well, one can create new baseclasses, but all classes must
   implement the root interface).
*/    

/************************
**  Rootclass methods  **
************************/

/************
**  New()  **
************/
#define OOPBase ((struct IntOOPBase*)(root_cl->OOPBasePtr))


OOP_Object *root_new(OOP_Class *root_cl, OOP_Class *cl, struct pRoot_New *param)
{
    struct _OOP_Object *o;
    struct RootData *data;

    EnterFunc(bug("Root::New(cl=%s, param = %p)\n",
    	cl->ClassNode.ln_Name, param));
    
    /* Allocate memory for the object */
    D(bug("Object size: %ld\n", MD(cl)->public.InstOffset + MD(cl)->instsize + sizeof (struct _OOP_Object)));
    o = AllocVec(MD(cl)->public.InstOffset + MD(cl)->instsize + sizeof (struct _OOP_Object), MEMF_ANY|MEMF_CLEAR);
    if (o)
    {
    	D(bug("Mem allocated: %p\n", o));
    	o->o_Class = (OOP_Class *)cl;
    	
    	data = (struct RootData *)OOP_BASEOBJECT(o);
    	
	/* Class has one more object */
    	MD(cl)->objectcount ++;
    	
    	ReturnPtr ("Root::New", OOP_Object *, OOP_BASEOBJECT(o) );
    }
    
    ReturnPtr ("Root::New", OOP_Object *, NULL);
}

/****************
**  Dispose()  **
****************/
static VOID root_dispose(OOP_Class *root_cl, OOP_Object *o, OOP_Msg msg)
{
    EnterFunc(bug("Root::Dispose(o=%p, oclass=%s)\n", o, _OOP_OBJECT(o)->o_Class->ClassNode.ln_Name));

    MD(OOP_OCLASS(o))->objectcount --;
    D(bug("Object mem: %p, size: %ld\n", _OOP_OBJECT(o), ((ULONG *)_OOP_OBJECT(o))[-1] ));
    
    /* Free object's memory */
    FreeVec(_OOP_OBJECT(o));
    
    ReturnVoid("Root::Dispose");
}

static VOID root_get(OOP_Class *root_cl, OOP_Object *p, struct pRoot_Get *msg)
{
    *msg->storage = 0UL;
    D(bug("!!! Get() METHOD REACHED ROOTCLASS !!!\n"));
    return;
}

#undef OOPBase

/**********************
**  init_rootclass() **
**********************/
BOOL init_rootclass(struct IntOOPBase *OOPBase)
{
    
    struct rootclassobject *rco;
    OOP_Class *rootclass;
    
    BOOL success;
    ULONG mbase = 0UL;
    
    EnterFunc(bug("init_rootclass()\n"));
    
    rco = &(OOPBase->ob_RootClassObject);
    rootclass = &(rco->inst.data.public);
    
    /* Its class is the metaobject */
    rco->oclass = &(OOPBase->ob_BaseMetaObject.inst.data.public);
    
    rco->inst.data.public.ClassNode.ln_Name = CLID_Root;
    rco->inst.data.public.OOPBasePtr	= (struct Library *)OOPBase;
    rco->inst.data.public.InstOffset	= NULL;
    rco->inst.data.public.UserData	= (APTR)OOPBase;
    
    rco->inst.data.public.cl_DoMethod	= NULL;
    rco->inst.data.public.cl_CoerceMethod	= NULL;
    rco->inst.data.public.cl_DoSuperMethod	= basemeta_dosupermethod;
    rco->inst.data.public.cl_CoerceMethod	= basemeta_coercemethod;
    rco->inst.data.public.cl_DoMethod	= basemeta_domethod;
    
    D(bug("Root stuff: dosupermethod %p, coeremethod %p, domethod %p\n",
	basemeta_dosupermethod, basemeta_coercemethod, basemeta_domethod));
    
    rco->inst.data.superclass		= NULL;
    rco->inst.data.subclasscount	= 0UL;
    rco->inst.data.objectcount		= 0UL;
    rco->inst.data.instsize		= 0UL;
    rco->inst.data.numinterfaces	= 1UL;
    
    /* Initialize methodtable */
    
    rco->inst.rootif[moRoot_New].MethodFunc	= (IPTR (*)())root_new;
    rco->inst.rootif[moRoot_New].mClass		= rootclass;
    
    rco->inst.rootif[moRoot_Dispose].MethodFunc	= (IPTR (*)())root_dispose;
    rco->inst.rootif[moRoot_Dispose].mClass	= rootclass;

    rco->inst.rootif[moRoot_Get].MethodFunc	= (IPTR (*)())root_get;
    rco->inst.rootif[moRoot_Get].mClass 	= rootclass;

    /* Important: IID_Root interface ID MUST be the first one
       initialized, so that it gets the value 0UL. This is
       because it's used as rootclass both for IFMeta and HIDDMeta classes
    */
    
    success = init_mi_methodbase(IID_Root, &mbase, OOPBase);
    if (success)
    {	
    	/* Make it public */
    	OOP_AddClass(rootclass);
    }
	
    ReturnBool ("init_rootclass", success);
}

/* Below is rootclass DoMethod and CoerceMethod. They are hardly useful,
   cause you would never create an object of rootclass
   
   

#define ROOT_CALLMETHOD(cl, o, m)			\
{							\
    register struct IFMethod *ifm;			\
    ifm = &(RI(cl)->rootif[msg->MethodID]);		\
    return ifm->MethodFunc(ifm->mClass, o, msg);	\
}

#define RI(cl) ((struct rootinst *)cl)

static IPTR root_domethod(OOP_Object *o, OOP_Msg msg)
{
    register Class *cl;
    cl = OCLASS(o);
    
    ROOT_CALLMETHOD(cl, o, msg);
}

static IPTR root_coercemethod(OOP_Class *cl, OOP_Object *o, OOP_Msg msg)
{
    ROOT_CALLMETHOD(cl, o, msg);
}

*/


