/*
    Copyright (C) 1995-1997 AROS - The Amiga Replacement OS
    $Id$

    Desc: OOP rootclass
    Lang: english
*/

#include <proto/exec.h>
#include <exec/memory.h>

#include <proto/utility.h>
#include <proto/oop.h>
#include <oop/oop.h>
#include <oop/method.h>
#include <oop/root.h>
#include <oop/meta.h>

#include "intern.h"

#undef DEBUG
#undef SDEBUG

#define DEBUG 0
#include <aros/debug.h>

#define OOPBase GetOBase(((Class *)cl)->UserData)

/* This class creates method objects. Method objects are objects
   you can obtain for a single method of some object,
   and lets you invoke that particular method on
   that particular object almost as fast as
   a function call. Nice if you do many invocations of a method on
   the same object many times, and need it to be fast.
*/

static struct IFMethod *FindMethod(struct IntClass *cl, ULONG mid);

struct IntMethod
{
    /* Every object has its class at ((VOID **)obj)[-1] */
    
    Class *OClass;
    
    /* The app gets a pointer to &(intmethod->PublicPart).
       The public part is a readonly public struct.
     */
    Method PublicPart;
};

struct MethodData
{
    Method PublicPart;
};

/************
**  New()  **
************/
static Object *_Root_New(Class *cl, Object *o, struct P_Root_New *msg)
{
    Msg m_msg     = NULL;
    Object *m_obj = NULL;
     
    struct TagItem *tag, *tstate;
    
    ULONG mid = 0UL;
    struct IntMethod *m;
    struct IFMethod *ifm;
    
    EnterFunc(bug("Method::New()\n"));
    
    
    /* Parse the createion-time attributes passed to the object */ 
    tstate = msg->AttrList;
    
    while ((tag = NextTagItem(&tstate)))
    {
     	if (IsMethodAttr(tag->ti_Tag))
	{
	    switch (TagIdx(tag->ti_Tag))
	    {
	    	case AIDX_Method_TargetObject:
		    /* The object from which we get the method */
		    m_obj = (Object *)tag->ti_Data;
		    break;
		
		
		case AIDX_Method_Message:
		     /* The message to pass with the method */
		     m_msg = (Msg)tag->ti_Data;
		     break;
		
		case AIDX_Method_MethodID:
		    /* The ID of the method to pass */
		    mid = (ULONG)tag->ti_Data;
		    break;
		
	    }
	    
	}
	
    }
    
    /* User MUST supply methodID, message and target object to get a method object */
    if ( !(mid && m_msg && m_obj) )
    	ReturnPtr("Method::New", Object *, NULL);
	
    /* Try to find methodfunc */
    ifm = FindMethod((struct IntClass *)OCLASS(m_obj), mid);
    if (!ifm)
    	/* If method isn't supported by target object, exit gracefully */
    	ReturnPtr("Method::New", Object *, NULL);
    
    /* Allocate mem for the method object */
    m = AllocMem( sizeof (struct IntMethod), MEMF_ANY );
    if (m);
    {
	
	/* Initialize message's method ID. (User convenience) */
	m_msg->MethodID = mid;

	/* Target object is stored for user convenience */
    	m->PublicPart.TargetObject  = m_obj;
	
	/* The message is stored for user convenience */
	m->PublicPart.Message       = m_msg;
	
	/* Store method implemetation func and the class that will
	   receive the method call. (We skip unimplemented class calls)
	*/
	m->PublicPart.MethodFunc    = ifm->MethodFunc;
	m->PublicPart.MClass	    = ifm->mClass;
	
	/* Initialize OCLASS(methodobject) */
	m->OClass	= cl;
	
	/* Return pointer to the public part */
	ReturnPtr ("Method::New", Object *, (Object *)&(m->PublicPart));
    }
    ReturnPtr ("Method::New", Object *, NULL);
    
}

/****************
**  Dispose()  **
****************/
static VOID _Root_Dispose(Class *cl, Method  *m, Msg msg )
{
    EnterFunc(bug("Method::Dispose()\n"));
    
    /* Well, free the method object */
    FreeMem(_OBJECT(m), sizeof (struct IntMethod));
    
    ReturnVoid("Method::Dispose");
}

/************************
**  Support functions  **
************************/

/* FindMethod()
   Find the method in the target object's class.
   If method isn't supported by class, NULL is supplied.
*/

static struct IFMethod *FindMethod(struct IntClass *cl, ULONG mid)
{
    register struct IFBucket *b;
    
    /* Get interfaceID part of methodID */
    register ULONG ifid = mid & (~METHOD_MASK);	

    /* Get method offset part of methdoID */
    mid &= METHOD_MASK;
    
    /* Look up ID in hashtable and get linked list of buckets,
       storing interfaces
     */
    b = ((struct IntClass *)cl)->IFTableDirectPtr[ifid & cl->HashMask];
loop:
    if (b)
    {
    	/* Founc correct interface ? */
        if (b->InterfaceID == ifid)
	{
	    /* Yep. Return method at supplied method offset */
	    return(&(b->MethodTable[mid]));
    	}

        b = b->Next;
        goto loop;
    }
    /* Method not found, return NULL */
    return (NULL);
}


#undef OOPBase

/* Self-exapainatory */
Class *InitMethodClass(struct IntOOPBase *OOPBase)
{

    struct MethodDescr methods[] =
    {
	{(IPTR (*)())_Root_New,			MIDX_Root_New},
	{(IPTR (*)())_Root_Dispose,		MIDX_Root_Dispose},
	{ NULL, 0UL }
    };
    
    struct InterfaceDescr ifdescr[] =
    {
    	{ methods, GUID_Root, 2},
	{ NULL, 0UL, 0UL}
    };
    
    struct TagItem tags[] =
    {
        {A_Class_SuperID,		(IPTR)NULL},
	{A_Class_InterfaceDescr,	(IPTR)ifdescr},
	{A_Class_ID,			(IPTR)METHODCLASS},
	{A_Class_InstSize,		(IPTR)sizeof (struct MethodData)},
	{TAG_DONE, 0UL}
    };

    
    Class *cl;
    
    EnterFunc(bug("InitMethodClass()\n"));
    
    cl = (Class *)NewObjectA(NULL, METACLASS, tags);
    if (cl)
    {
        cl->UserData = OOPBase;
    	AddClass(cl);
    }
    
    ReturnPtr ("InitMethodClass", Class *, cl);
}
