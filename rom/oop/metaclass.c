/*
    Copyright (C) 1995-1997 AROS - The Amiga Replacement OS
    $Id$

    Desc: OOP metaclass
    Lang: english
*/

#include <proto/exec.h>
#include <proto/utility.h>
#include <exec/memory.h>

#include <proto/oop.h>
#include <oop/meta.h>
#include <oop/root.h>
#include <oop/oop.h>

#include "intern.h"
#include "hash.h"


#undef SDEBUG
#undef DEBUG
#define SDEBUG 0
#define DEBUG 0
#include <aros/debug.h>

#define OOPBase	(GetOBase(((Class *)cl)->UserData))

/*
   The metaclass is used to create class. That means,
   classes are instances of the meta class.
   The meta class is itself both a class (you can
   create instances of it), and an object (you can invoke
   methods on it. 
*/   
   
   

/************
**  New()  **
************/
static Object *_Root_New(struct IntClass *cl, Object *o, struct P_Root_New *msg)
{
    struct IntClass *data;

    struct InterfaceDescr *ifdescr = NULL;
    STRPTR superid = NULL, clid = NULL;
    struct IntClass *superptr = NULL;
    struct TagItem *tag, *tstate;
    ULONG instsize = (ULONG)-1L;
    IPTR (*domethod)(Object *, Msg) = NULL;

    EnterFunc(bug("Meta::New(cl=%s, msg = %p)\n",
    	cl->PPart.ClassNode.ln_Name, msg));

    /* Analyze the taglist before object is allocated,
    ** so we can easily exit cleanly if some info is missing
    */

    tstate = msg->AttrList;
    while ((tag = NextTagItem(&tstate)))
    {
        if (IsMetaAttr(tag->ti_Tag))
	{
	    D(bug("Got meta attr %lx with TagIdx %ld\n",
	    	tag->ti_Tag, TagIdx(tag->ti_Tag) ));
	    
	    switch (TagIdx(tag->ti_Tag))
	    {
	    
	    case AIDX_Class_SuperID:
	        D(bug("Got superID\n"));
		/* ID of superclass */
		superid = (STRPTR)tag->ti_Data;
		break;
		    
	    case AIDX_Class_InterfaceDescr:
	        D(bug("Got ifdescr\n"));
		/* What interfaces does the class support ? */
		ifdescr = (struct InterfaceDescr *)tag->ti_Data;
		break;
		    
	    case AIDX_Class_ID:
	        D(bug("Got classID\n"));
		/* The new class' ID */
		clid = (STRPTR)tag->ti_Data;
		break;
		    
	    case AIDX_Class_SuperPtr:
	        D(bug("Got superPtr\n"));
		/* If the super class is private, than we must have
		   a pointer to it.
		*/
		superptr = (struct IntClass *)tag->ti_Data;
		break;
		
	    case AIDX_Class_InstSize:
	        /* Instance data size for the new class */
	        instsize = (ULONG)tag->ti_Data;
		break;
		
	    case AIDX_Class_DoMethod:
	        /* The user can override the class' default DoMethod() call */
	        domethod = (IPTR(*)(Object *, Msg))tag->ti_Data;
		break;
	    }
	    
	}
	
    }
    
    /* The user must supply instance size */
    if (instsize == (ULONG)-1)
    	ReturnPtr ("Meta::New, no instsize", Object *, NULL);
    
    /* The new class must have interfaces */
    if (!ifdescr)
    	ReturnPtr ("Meta::New, no ifdescr", Object *, NULL);

    /* The new class must have a superclass */
    if (!superptr)
    {
	if (superid)
	{
	    superptr = (struct IntClass *)FindName((struct List *)&(GetOBase(OOPBase)->ob_ClassList), superid);
	    if (!superptr)
	    	ReturnPtr ("Meta::New, no superptr/id", Object *, NULL);
	}
    }
    
    /* We are sure we have enough args, and can let rootclass alloc the instance data */
    o = (Object *)DoSuperMethodA((Class *)cl, o, (Msg)msg);
    if (o)
    {
	ULONG dispose_mid = M_Root_Dispose;

        D(bug("Instance allocated\n"));
	
	data = (struct IntClass *)INST_DATA(cl, o);
	
	D(bug("superptr=%p\n", superptr));
	
	/* Set new class' superclass. This MUST be done
	   before calling AllocDispatchTables(), cause this
	   function has to copy the dispatch tables of the superclass
	 */
	data->SuperClass = superptr;
	
	/* Allocate dispatch tables for the new class (see support.c) */
	if (AllocDispatchTables(data, ifdescr, OOPBase))
	{
	    /* Copy the class' ID */
	    data->PPart.ClassNode.ln_Name = AllocVec(strlen (clid) + 1, MEMF_ANY);
	    if (data->PPart.ClassNode.ln_Name)
	    {
	    
	    	/* Initialize class fields */
		
		/* Instoffset */
		if (superptr)
	    	    data->PPart.InstOffset = superptr->PPart.InstOffset + superptr->PPart.InstSize;
		else
		    data->PPart.InstOffset = 0UL;
		    
		if (!domethod)
		    domethod = LocalDoMethod;
		    
		data->PPart.InstSize 	= instsize;
		data->PPart.DoMethod	= domethod;
	    	data->SubClassCount 	= 0UL;
	    	data->ObjectCount	= 0UL;
	    
	    	data->SuperClass	= superptr;
		
		/* Copy class ID */
		strcpy(data->PPart.ClassNode.ln_Name, clid);
		
	    	/* NumInterfaces, HashTable, IFTableDirectPtr and HashMask allready set
	   	** by AllocDispatchTables()
		*/
	    
	    
    	    	ReturnPtr ("Meta::New", Object *, o);
	    }
	    FreeDispatchTables(data, OOPBase);
	}

	CoerceMethodA((Class *)cl, o, (Msg)&dispose_mid);
    }
    
    ReturnPtr ("Meta::New", Object *, NULL);   
}

/****************
**  Dispose()  **
****************/
static VOID _Root_Dispose(struct IntClass *cl, Object *o, Msg msg)
{
    struct IntClass *data = (struct IntClass *)INST_DATA(cl, o);

    EnterFunc(bug("Meta::Dispose(o=%p, oclass=%s)\n", o, _OBJECT(o)->o_Class->ClassNode.ln_Name));
    
    D(bug("Freeing dispatch table\n"));
    
    /* Free the dispatch tables */
    FreeDispatchTables(data, OOPBase);

    D(bug("Freeing class id\n"));
    
    /* Free the class ID */
    FreeVec(data->PPart.ClassNode.ln_Name);
    
    D(bug("Calling superclass\n"));
    
    /* And let the rootclass free the object */
    DoSuperMethodA((Class *)cl, o, msg);
    ReturnVoid("Meta::Dispose");
}

#undef OOPBase

BOOL InitMetaClass(struct IntOOPBase *OOPBase)
{
    struct MethodDescr root_mdescr[]=
    {
    	{ (IPTR (*)())_Root_New,		MIDX_Root_New		},
    	{ (IPTR (*)())_Root_Dispose,		MIDX_Root_Dispose	},
	{  NULL, 0UL }
    };
    

    struct InterfaceDescr _Meta_Descr[] =
    {
    	{root_mdescr, GUID_Root, 2},
	{NULL, NULL, 0UL}
    };
    
    struct IntClass *MetaClass = &(OOPBase->ob_MetaClass.InstanceData);
    
    EnterFunc(bug("InitMetaClass()\n"));

    /* The metaclass needs some special handling when being initialized */

    /* Superclass must be initialized before calling AllocDispatchTables() */
    MetaClass->SuperClass = &OOPBase->ob_RootClass;
    
    if (AllocDispatchTables(MetaClass, _Meta_Descr, OOPBase))
    {
    	/* initialize Class ID */
    	MetaClass->PPart.ClassNode.ln_Name   	= METACLASS;
	MetaClass->PPart.InstOffset 	= 0UL;
	MetaClass->PPart.InstSize   	= sizeof (struct IntClass);
	MetaClass->PPart.DoMethod	= LocalDoMethod;
	MetaClass->SubClassCount 	= 0UL;
	MetaClass->ObjectCount	 	= 0UL;

	MetaClass->NumInterfaces 	= 2UL;
	MetaClass->PPart.UserData	= (APTR)OOPBase;
	
	/* The class of the meta object is the metaclass,
	** so it is an instance of itself :-)
	*/
	OOPBase->ob_MetaClass.Class = (Class *)&(OOPBase->ob_MetaClass.InstanceData);
	
	/* Make it public */
	AddClass((Class *)MetaClass);
	ReturnBool ("InitMetaClass", TRUE);
    }
    ReturnBool ("InitMetaClass", FALSE);

}
