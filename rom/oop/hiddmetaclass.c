/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: OOP HIDD metaclass
    Lang: english
*/

#include <string.h>

#include <proto/exec.h>
#include <proto/utility.h>
#include <exec/memory.h>

#include <proto/oop.h>

#include "intern.h"

#undef SDEBUG
#undef DEBUG
#define SDEBUG 0
#define DEBUG 0
#include <aros/debug.h>

#define UB(x) ((UBYTE *)x)

#define MD(cl) ((struct metadata *)cl)

#define IntCallMethod(cl, o, msg)					\
{									\
    register struct IFMethod *ifm;					\
    D(bug("mid=%ld\n", *msg)); ifm = &((struct hiddmeta_inst *)cl)->data.methodtable[*msg];	\
    D(bug("ifm: func %p, cl %p\n", ifm->MethodFunc, ifm->mClass)); return (ifm->MethodFunc(ifm->mClass, o, msg));			\
}


static VOID get_info_on_ifs(OOP_Class *super
			,struct OOP_InterfaceDescr *ifdescr
			,ULONG 	*total_num_methods_ptr
			,ULONG 	*total_num_ifs_ptr
			,struct IntOOPBase *OOPBase);

static IPTR HIDD_DoMethod(OOP_Object *o, OOP_Msg msg);
static IPTR HIDD_CoerceMethod(OOP_Class *cl, OOP_Object *o, OOP_Msg msg);
static IPTR HIDD_DoSuperMethod(OOP_Class *cl, OOP_Object *o, OOP_Msg msg);
static inline ULONG get_max_midx(struct OOP_MethodDescr *md);

/*
   The metaclass is used to create class. That means,
   classes are instances of the meta class.
   The meta class is itself both a class (you can
   create instances of it), and an object (you can invoke
   methods on it. 
*/   

struct hiddmeta_inst
{
    struct metadata base;
    struct hiddmeta_data
    {
	struct if_info
	{
	    /* The globally unique ID of the interface */
	    STRPTR interface_id;
	    ULONG num_methods;
	    ULONG mtab_offset;
	    
	} *ifinfo;
	struct IFMethod *methodtable;
    } data;
};

#define OOPBase	(cl->OOPBasePtr)
   
/**********************
**  HIDDMeta::New()  **
**********************/
static OOP_Object *hiddmeta_new(OOP_Class *cl, OOP_Object *o, struct pRoot_New *msg)
{

    IPTR (*domethod)(OOP_Object *, OOP_Msg) 			= NULL;
    IPTR (*coercemethod)(OOP_Class *, OOP_Object *, OOP_Msg) 	= NULL;
    IPTR (*dosupermethod)(OOP_Class *, OOP_Object *, OOP_Msg) 	= NULL;

    
    EnterFunc(bug("HIDDMeta::New(cl=%s, msg = %p)\n",
    	cl->ClassNode.ln_Name, msg));

    /* Analyze the taglist before object is allocated,
    ** so we can easily exit cleanly if some info is missing
    */
    
    domethod	  = (IPTR (*)())GetTagData(aMeta_DoMethod,	NULL, msg->attrList);
    coercemethod  = (IPTR (*)())GetTagData(aMeta_CoerceMethod,  NULL, msg->attrList);
    dosupermethod = (IPTR (*)())GetTagData(aMeta_DoSuperMethod, NULL, msg->attrList);

    
    /* We are sure we have enough args, and can let rootclass alloc the instance data */
    o = (OOP_Object *)OOP_DoSuperMethod((OOP_Class *)cl, o, (OOP_Msg)msg);
    if (o)
    {

        D(bug("Instance allocated\n"));
	
	    
	/* Initialize class fields */
		    
	if (!domethod)
	    domethod = HIDD_DoMethod;
		    
	if (!coercemethod)
	    coercemethod = HIDD_CoerceMethod;
		    
	if (!dosupermethod)
	{
		    
	    /* Use superclass' DoSupermethod call if superclass isn't
	       an instance of the HIDDMetaClass
	    */
	    if (OOP_OCLASS(MD(o)->superclass) != (OOP_Class *)cl)
	    	dosupermethod = MD(o)->superclass->cl_DoSuperMethod;
	    else
	    	dosupermethod = HIDD_DoSuperMethod;
	}
	
	((OOP_Class *)o)->cl_DoMethod	= domethod;
	((OOP_Class *)o)->cl_CoerceMethod	= coercemethod;
	((OOP_Class *)o)->cl_DoSuperMethod	= dosupermethod;
		  
    }
    
    ReturnPtr ("HIDDMeta::New", Object *, o);
}

/********************************
**  HIDDMeta::allocdisptabs()  **
********************************/
static BOOL hiddmeta_allocdisptabs(OOP_Class *cl, OOP_Object *o, struct P_meta_allocdisptabs *msg)
{
    ULONG disptab_size, total_num_methods = 0UL, total_num_ifs = 0UL;
    
    struct hiddmeta_data *data = OOP_INST_DATA(cl, o);
    struct IFMethod *mtab = NULL;
    
    EnterFunc(bug("HIDDMeta::allocdisptabs(cl=%p, ifDescr=%p)\n", cl, msg->ifdescr));
    
    /* Find the total number of methods in the superclass */
    get_info_on_ifs(msg->superclass
    				,msg->ifdescr
				,&total_num_methods
				,&total_num_ifs
				,(struct IntOOPBase *)OOPBase);

    /* Set number of interfaces */
    MD(o)->numinterfaces = total_num_ifs;
    
    D(bug("Number of  interfaces: %ld, methods %ld\n",
    	total_num_ifs, total_num_methods));
    
    /* Allocate the dispatch table */
    disptab_size = UB(&mtab[total_num_methods]) - UB(&mtab[0]);

    data->methodtable = AllocVec(disptab_size, MEMF_ANY|MEMF_CLEAR);
    if (data->methodtable)
    {
	struct if_info *ifinfo = NULL;
	ULONG ifinfo_size;
	
	mtab = data->methodtable;

	/* Allocate memory for interface info table */
	ifinfo_size = UB(&ifinfo[total_num_ifs]) - UB(&ifinfo[0]);
	data->ifinfo = AllocVec(ifinfo_size, MEMF_ANY|MEMF_CLEAR);
	if (data->ifinfo)
	{
	    /* Iterate through all parent interfaces, copying relevant info
	       into our own dispatch tables.
	    */
	    STRPTR interface_id;
	    ULONG num_methods;
	    IPTR iterval = 0UL;
	    struct IFMethod *ifm;
	    ULONG mtab_offset = 0;
	    struct OOP_InterfaceDescr *ifdescr = msg->ifdescr;

	    ifinfo = data->ifinfo;
	    
	    while ((ifm = meta_iterateifs((OOP_Object *)msg->superclass, &iterval, &interface_id, &num_methods)))
	    {
	        ULONG copy_size;
		struct OOP_InterfaceDescr *ifd;
		
	    	/* Copy interface into dispatch tables */
    		copy_size = UB(&mtab[num_methods]) - UB(&mtab[0]);
		
		D(bug("Copying mtab (%p to %p) , size: %ld\n", ifm, mtab, copy_size));
		CopyMem(ifm, mtab, copy_size);
		
		D(bug("mtab copied, mtab=%p\n", mtab));
		/* store interface info */
		
		/* allready copied by superclass, no need to recopy it */
		ifinfo->interface_id	= interface_id;
		D(bug("interfaceID for ifinfo %p set to %s\n", ifinfo, ifinfo->interface_id));
		
		/* See if this class supplies more methods for the interace */
		for (ifd = msg->ifdescr; ifd->MethodTable != 0; ifd ++)
		{
		    if (0 == strcmp(ifd->InterfaceID, interface_id))
		    {
		    	ULONG max_midx;
			
			max_midx = get_max_midx(ifd->MethodTable);
			if (max_midx >= num_methods)
				num_methods = max_midx + 1;
		    }
		}
		ifinfo->num_methods	= num_methods;
		D(bug("numemthods set to %ld\n", num_methods));
		ifinfo->mtab_offset	= mtab_offset;
		D(bug("mtab_offset set to %ld\n", mtab_offset));
		
		mtab_offset += num_methods;
		mtab += num_methods;
	    {
	    	ULONG idx;
		D(bug("\n"));
		for (idx = 0; idx < total_num_ifs; idx ++)
		{
		    D(bug("ifinfo: (%p, %s), idx: %ld\n"
		     	,&(data->ifinfo[idx])
			,data->ifinfo[idx].interface_id
			,idx ));
		
		}
		D(bug("\n"));
	    }

		ifinfo ++;
	    }
	    
	    D(bug("Finished copying super IFs\n\n"));
	    
	    {
	    	ULONG idx;
		for (idx = 0; idx < total_num_ifs; idx ++)
		{
		    D(bug(" ifinfo: (%p, %s), idx: %ld\n"
		     	,&(data->ifinfo[idx])
			,data->ifinfo[idx].interface_id
			,idx ));
		
		}
	    }
	    	
	    
	    
	    /* Now find the interface (max one) that is new for this class,
	       and at the same time override all methods for all interfaces
	    */
	    
	    D(bug("Find new interface\n"));
	    for (; ifdescr->MethodTable != NULL; ifdescr ++)
	    {
	        struct OOP_MethodDescr *mdescr;
	    	ULONG current_idx;
		
	    	ifm = meta_getifinfo((OOP_Object *)msg->superclass, ifdescr->InterfaceID, &num_methods);
		if (!ifm)
		{
		    ULONG mbase = 0UL;
		    
	    	    D(bug("Found new interface %s\n", ifdescr->InterfaceID));
		    /* Interface is new to this class */

		    
		    /* Do NOT copy interfaceID */
		    ifinfo->interface_id = ifdescr->InterfaceID;
		    ifinfo->num_methods	 = get_max_midx(ifdescr->MethodTable) + 1;
		    ifinfo->mtab_offset	 = mtab_offset;
		    
		    /* Install methodbase for this interface */
		    if (!init_methodbase( ifdescr->InterfaceID
		    			 ,mtab_offset
					 ,&mbase
					 ,(struct IntOOPBase *)OOPBase))
		    {
		    	goto init_err;
		    }
					 
		    
		    
		    /* The below is not necessary, since we never have more than one
		       new interface 
		    
		    ifinfo ++;
		    mtab_offset += ifdescr->NumMethods;
		    mtab = &mtab[mtab_offset];
		    		    
	       	    */
		}
		
		/* Find the index into the ifinfo table for the current entry */
		D(bug("Finding current idx\n"));
		for (current_idx = 0; ; current_idx ++)
		{
		    D(bug("ifdecr: %s, ifinfo: (%p, %s), idx: %ld\n", ifdescr->InterfaceID, &(data->ifinfo[current_idx]), data->ifinfo[current_idx].interface_id, current_idx ));
		    
		    if (!strcmp(ifdescr->InterfaceID, data->ifinfo[current_idx].interface_id))
		    	break;
		}
		
		D(bug("Overriding methods\n"));


	    	for (mdescr = ifdescr->MethodTable; mdescr->MethodFunc != NULL; mdescr ++)
	    	{
		    ULONG mtab_idx;
		    
		    mtab_idx = mdescr->MethodIdx + data->ifinfo[current_idx].mtab_offset;
		    D(bug("Initing of if %s methods at %ld\n", ifdescr->InterfaceID, mtab_idx));
		    
		    data->methodtable[mtab_idx].MethodFunc = mdescr->MethodFunc;
		    data->methodtable[mtab_idx].mClass = (OOP_Class *)o;
		
	    	} /* for (each method in current interface) */
	    
		
	    } /* for (each interface in this class' interface description) */
	    
	    ReturnBool("HIIDMeta::allocdisptabs", TRUE);
init_err:
	    FreeVec(data->ifinfo);

	} /* if (interface info table allocated) */
	
	FreeVec(data->methodtable);
    
    } /* if (methodtable allocated) */
    
    ReturnBool("HIDDMeta::allocdisptabs", FALSE);
}

/*******************************
**  HIDDMeta::freedisptabs()  **
********************************/
static VOID hiddmeta_freedisptabs(OOP_Class *cl, OOP_Object *o, OOP_Msg msg)
{
    struct hiddmeta_inst *inst = (struct hiddmeta_inst *)o;
    
    FreeVec(inst->data.methodtable);
    FreeVec(inst->data.ifinfo);
    return;
}



/*****************************
**  HIDDMeta::iterateifs()  **
*****************************/
static struct IFMethod *hiddmeta_iterateifs(OOP_Class *cl, OOP_Object *o, struct P_meta_iterateifs *msg)
{
    struct hiddmeta_inst *inst = (struct hiddmeta_inst *)o;
    ULONG idx = *msg->iterval_ptr;
    struct IFMethod *mtab;
    
    EnterFunc(bug("HIDDMeta::iterateifs(cl=%p, o=%p, iterval=%p)\n",
    	cl, o, *msg->iterval_ptr));
    if (idx >= inst->base.numinterfaces)
    	mtab = NULL;
    else
    {
        /* First iteration */
	struct if_info *ifinfo;
	ifinfo = &(inst->data.ifinfo[idx]);
	
	*msg->interface_id_ptr	= ifinfo->interface_id;
	*msg->num_methods_ptr	= ifinfo->num_methods;

	mtab = &(inst->data.methodtable[ifinfo->mtab_offset]);
	
	(*msg->iterval_ptr) ++;
	
    }
    ReturnPtr("HIDDMeta::iterateifs", struct IFMethod *, mtab);
}

/****************************
**  HIDDMeta::getifinfo()  **
****************************/
static struct IFMethod *hiddmeta_getifinfo(OOP_Class *cl, OOP_Object *o, struct P_meta_getifinfo *msg)
{

    struct hiddmeta_inst *inst = (struct hiddmeta_inst *)o;
    ULONG current_idx;
    struct IFMethod *mtab = NULL;
    
    EnterFunc(bug("HIDDMeta::getifinfo(cl=%p, o=%p, ifID=%s)\n",
    	cl, o, msg->interface_id));
	
    /* Look up the interface */

    for (current_idx = 0; current_idx < inst->base.numinterfaces; current_idx ++)
    {
    	struct if_info *ifinfo;
	
	ifinfo = &(inst->data.ifinfo[current_idx]);
	if (!strcmp(msg->interface_id, ifinfo->interface_id))
	{
	    *(msg->num_methods_ptr) = ifinfo->num_methods;
	    mtab = &(inst->data.methodtable[ifinfo->mtab_offset]);
	    break;
	}
    }
    ReturnPtr("HIDDMeta::getifinfo", struct IFMethod *, mtab);
}


/*****************************
**  HIDDMeta::findmethod()  **
*****************************/
struct IFMethod *hiddmeta_findmethod(OOP_Class *cl, OOP_Object *o, struct P_meta_findmethod *msg)
{
    struct hiddmeta_inst *inst = (struct hiddmeta_inst *)o;
    struct IFMethod *m;
    
    EnterFunc(bug("HIDDMeta::findmethod(cl=%p, o=%p, mID=%ld)\n",
    	cl, o, msg->method_to_find));
	
    m = &(inst->data.methodtable[msg->method_to_find]);
	

    ReturnPtr("HIDDMeta::findmethod", struct IFMethod *, m);
    
}

#undef OOPBase

#define NUM_ROOT_METHODS 1
#define NUM_META_METHODS 5

/************************
**  Support functions  **
************************/
OOP_Class *init_hiddmetaclass(struct IntOOPBase *OOPBase)
{
    struct OOP_MethodDescr root_mdescr[NUM_ROOT_METHODS + 1] =
    {
    	{ (IPTR (*)())hiddmeta_new,		moRoot_New		},
	{  NULL, 0UL }
    };
    
    struct OOP_MethodDescr meta_mdescr[NUM_META_METHODS + 1] =
    {
    	{ (IPTR (*)())hiddmeta_allocdisptabs,	MO_meta_allocdisptabs	},
    	{ (IPTR (*)())hiddmeta_freedisptabs,	MO_meta_freedisptabs	},
    	{ (IPTR (*)())hiddmeta_getifinfo,	MO_meta_getifinfo	},
    	{ (IPTR (*)())hiddmeta_iterateifs,	MO_meta_iterateifs	},
    	{ (IPTR (*)())hiddmeta_findmethod,	MO_meta_findmethod	},
	{  NULL, 0UL }
    };
    

    struct OOP_InterfaceDescr ifdescr[] =
    {
    	{root_mdescr, IID_Root, NUM_ROOT_METHODS},
	{meta_mdescr, IID_Meta, NUM_META_METHODS},
	{NULL, NULL, 0UL}
    };
    
    struct TagItem tags[] =
    {
        {aMeta_SuperPtr,		(IPTR)BASEMETAPTR},
	{aMeta_InterfaceDescr,		(IPTR)ifdescr},
	{aMeta_ID,			(IPTR)CLID_SIMeta},
	{aMeta_InstSize,		(IPTR)sizeof (struct hiddmeta_data) },
	{TAG_DONE, 0UL}
    };
    
    OOP_Class *cl;
    EnterFunc(bug("init_hiddmetaclass()\n"));

    cl = (OOP_Class *)OOP_NewObject(NULL, CLID_MIMeta, tags);
    if (cl)
    {
        cl->UserData = OOPBase;
    	OOP_AddClass(cl);
    }
    D(bug("HIDD_CoerceMethod=%p\n", HIDD_CoerceMethod));
    
    ReturnPtr ("init_hiddmetaclass()", OOP_Class *, cl);

}

static inline ULONG get_max_midx(struct OOP_MethodDescr *md)
{

    ULONG max_midx = 0;

	    
    for (; md->MethodFunc != NULL; md ++)
    {
    	if (md->MethodIdx > max_midx)
	{
	    max_midx = md->MethodIdx;
	}
    }
    return max_midx;
}

/************************
**  get_info_on_ifs()  **
************************/
static VOID get_info_on_ifs(OOP_Class *super
			,struct OOP_InterfaceDescr *ifdescr
			,ULONG *total_num_methods_ptr
			,ULONG *total_num_ifs_ptr
			,struct IntOOPBase *OOPBase)
{
    ULONG num_methods;
    STRPTR interface_id;
    IPTR iterval = 0UL;
    EnterFunc(bug("get_info_on_ifs(super=%s, ifdescr=%p, OOPBase=%p\n",
    	super->ClassNode.ln_Name, ifdescr, OOPBase));
    
    *total_num_methods_ptr = 0UL;
    *total_num_ifs_ptr = 0UL;
    
    /* Iterate all parent interfaces, counting methods and interfaces */
    while (meta_iterateifs((OOP_Object *)super, &iterval, &interface_id, &num_methods))
    {
	struct OOP_InterfaceDescr *ifd;
    	D(bug("if %s has %ld methods\n", interface_id, num_methods));
	/* Check whether current class also supplies methods for this interface */
	
	for (ifd = ifdescr; ifd->MethodTable != NULL; ifd ++)
	{
	    if (0 == strcmp(ifd->InterfaceID, interface_id))
	    {
	    	/* Interface also supplied here */
		ULONG max_midx;
		
		max_midx = get_max_midx(ifd->MethodTable);
		if (max_midx >= num_methods)
			num_methods = max_midx + 1;
		
	    }
	}
    	*total_num_methods_ptr += num_methods;
	(*total_num_ifs_ptr) ++;
    }
    D(bug("Finished counting methods\n"));

    /* Go through all interface descrs for this class, and find evt. new ones */   
    for (; ifdescr->MethodTable != NULL; ifdescr ++)
    {
        if (!meta_getifinfo((OOP_Object *)super, ifdescr->InterfaceID, &num_methods))
	{
	    /* The interface is new for this class.
	       For HIDDMeta class max. one interface can be new for a class
	    */
	    ULONG max_midx = 0;
	    max_midx = get_max_midx(ifdescr->MethodTable);
	    
	    /* Get the largest method idx */
	    
	    (*total_num_methods_ptr) += (max_midx + 1);
	    (*total_num_ifs_ptr) ++;
	}
    }
    
    ReturnVoid("get_info_on_ifs");
}

#define OOPBase (cl->OOPBasePtr)

/**********************
**  HIDD_DoMethod()  **
**********************/
static IPTR HIDD_DoMethod(OOP_Object *o, OOP_Msg msg)
{
    register struct hiddmeta_inst *cl = (struct hiddmeta_inst *)OOP_OCLASS(o);
    D(bug("HIDD_DoMethod(o=%p. msg=%p)\n", o, msg));
    
    IntCallMethod(cl, o, msg);
    
}

/************************
**  HIDD_CoerceMethod  **
************************/
static IPTR HIDD_CoerceMethod(OOP_Class *cl, OOP_Object *o, OOP_Msg msg)
{
    register struct IFMethod *ifm;
    D(bug("HIDD_CoerceMethod()\n"));
    D(bug("cl=%s, mid=%ld\n", cl->ClassNode.ln_Name, *msg)); ifm = &((struct hiddmeta_inst *)cl)->data.methodtable[*msg];
    D(bug("ifm %p func %p, cl %p\n", ifm, ifm->MethodFunc, ifm->mClass)); return (ifm->MethodFunc(ifm->mClass, o, msg));
}

/************************
**  HIDD_DoSuperMethod  **
************************/
static IPTR HIDD_DoSuperMethod(OOP_Class *cl, OOP_Object *o, OOP_Msg msg)
{
    D(bug("HIDD_DoSuperMethod()\n"));
    cl = MD(cl)->superclass;
    IntCallMethod(cl, o, msg);
}
