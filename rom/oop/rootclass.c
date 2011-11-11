/*
    Copyright © 1995-2011, The AROS Development Team. All rights reserved.
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
#include "basemetaclass.h"

#undef SDEBUG
#undef DEBUG
#define SDEBUG 0
#define DEBUG 0
#include <aros/debug.h>

/*****************************************************************************************

    NAME
	--background_root--

    LOCATION
	Root

    NOTES
	Root class is the base class of all classes.
	One can create new baseclasses, but all classes must implement the root interface.

*****************************************************************************************/

/*****************************************************************************************

    NAME
	moRoot_New
	
    SYNOPSIS
	See OOP_NewObject() doc.

    FUNCTION
	Creates a new object of some class. Class users should use OOP_NewObject() to
	create an object.

*****************************************************************************************/

/*****************************************************************************************

    NAME
	moRoot_Dispose
	
    SYNOPSIS
	See OOP_DisposeObject() doc.

    FUNCTION
	Used internally to dispose of an object previously
	created using the moRoot_New method.

*****************************************************************************************/

/*****************************************************************************************

    NAME
	moRoot_Set

    SYNOPSIS
	OOP_SetAttrs() (OOP_Object *object, struct TagItem *attrs);

    FUNCTION
	Set an attribute of an object.

*****************************************************************************************/

/*****************************************************************************************

    NAME
	moRoot_Get

    SYNOPSIS
	OOP_GetAttr(OOP_Object *object, ULONG attrID, IPTR *storage);

    FUNCTION
	Get the value for an object attribute.
	The attribute value will be stored in *storage.

    EXAMPLE
	..
	ULONG num_members;
	
	OOP_GetAttr(list, aList_NumMembers, &num_members);

*****************************************************************************************/

/************************
**  Rootclass methods  **
************************/

/************
**  New()  **
************/
OOP_Object *root_new(OOP_Class *root_cl, OOP_Class *cl, struct pRoot_New *param)
{
    struct IntOOPBase *OOPBase = (struct IntOOPBase *)root_cl->OOPBasePtr;
    struct _OOP_Object *o;

    EnterFunc(bug("Root::New(cl=%s, param = %p)\n",
    	cl->ClassNode.ln_Name, param));
    
    /* Allocate memory for the object */
    D(bug("Object size: %ld\n", MD(cl)->public.InstOffset + MD(cl)->instsize + sizeof (struct _OOP_Object)));
    o = AllocVec(MD(cl)->public.InstOffset + MD(cl)->instsize + sizeof (struct _OOP_Object), MEMF_ANY|MEMF_CLEAR);
    if (o)
    {
    	D(bug("Mem allocated: %p\n", o));
    	o->o_Class = (OOP_Class *)cl;
    	
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
    rco->inst.data.public.OOPBasePtr	= OOPBase;
    rco->inst.data.public.InstOffset	= 0;
    rco->inst.data.public.UserData	= (APTR)OOPBase;
    
    rco->inst.data.public.cl_DoSuperMethod = basemeta_dosupermethod;
    rco->inst.data.public.cl_CoerceMethod  = basemeta_coercemethod;
    rco->inst.data.public.cl_DoMethod	   = basemeta_domethod;

    D(bug("Root stuff: dosupermethod %p, coeremethod %p, domethod %p\n",
	basemeta_dosupermethod, basemeta_coercemethod, basemeta_domethod));
    
    rco->inst.data.public.superclass	= NULL;
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


