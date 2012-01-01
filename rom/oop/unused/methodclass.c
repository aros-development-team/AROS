/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: OOP rootclass
    Lang: english
*/

#include <proto/exec.h>
#include <exec/memory.h>

#include <proto/utility.h>
#include <proto/oop.h>
#include <oop/oop.h>

#include "intern.h"
#include "private.h"

#undef DEBUG
#undef SDEBUG

#define DEBUG 0
#include <aros/debug.h>

#define OOPBase GetOBase(((OOP_Class *)cl)->UserData)

/* This class creates method objects. Method objects are objects
   you can obtain for a single method of some object,
   and lets you invoke that particular method on
   that particular object almost as fast as
   a function call. Nice if you do many invocations of a method on
   the same object many times, and need it to be fast.
*/


struct intmethod
{
    /* Every object has its class at ((VOID **)obj)[-1] */
    
    OOP_Class *oclass;
    
    /* The app gets a pointer to &(intmethod->PublicPart).
       The public part is a readonly public struct.
     */
    OOP_Method public;
};

struct method_data
{
    OOP_Method public;
};

#define IS_METHOD_ATTR(attr, idx) ((idx = attr - MethodAttrBase) < num_Method_Attrs)
/********************
**  Method::New()  **
********************/
static OOP_Object *method_new(OOP_Class *cl, OOP_Object *o, struct pRoot_New *msg)
{
    OOP_Msg m_msg     = NULL;
    OOP_Object *m_obj = NULL;
     
    struct TagItem *tag, *tstate;
    
    ULONG mid = 0UL;
    struct intmethod *m;
    struct IFMethod *ifm;
    
    ULONG idx;
    
    EnterFunc(bug("Method::New()\n"));
    
    
    /* Parse the createion-time attributes passed to the object */ 
    tstate = msg->attrList;
    
    while ((tag = NextTagItem(&tstate)))
    {
     	if (IS_METHOD_ATTR(tag->ti_Tag, idx))
	{
	    switch (idx)
	    {
	    	case aoMethod_TargetObject:
		    /* The object from which we get the method */
		    m_obj = (OOP_Object *)tag->ti_Data;
		    break;
		
		
		case aoMethod_Message:
		     /* The message to pass with the method */
		     m_msg = (OOP_Msg)tag->ti_Data;
		     break;
		
		case aoMethod_MethodID:
		    /* The ID of the method to pass */
		    mid = (ULONG)tag->ti_Data;
		    break;
		
	    }
	    
	}
	
    }
    
    /* User MUST supply methodID, message and target object to get a method object */
    if ( !(mid && m_msg && m_obj) )
    	ReturnPtr("Method::New", OOP_Object *, NULL);
	
    /* Try to find methodfunc */
    D(bug("trying to find method, oclass=%p, oclass(oclass)=%p\n",
    	OOP_OCLASS(m_obj), OOP_OCLASS(OOP_OCLASS(m_obj)) ));
    D(bug("oclass(oclass)=%s\n", OOP_OCLASS(OOP_OCLASS(m_obj))->ClassNode.ln_Name));
    
    ifm = meta_findmethod((OOP_Object *)OOP_OCLASS(m_obj), mid, (struct Library *)OOPBase);
    D(bug("found method: %p\n", ifm));
    if (!ifm)
    	/* If method isn't supported by target object, exit gracefully */
    	ReturnPtr("Method::New", OOP_Object *, NULL);
    
    /* Allocate mem for the method object */
    m = AllocMem( sizeof (struct intmethod), MEMF_ANY );
    if (m)
    {
    	D(bug("Method object allocated\n"));
	
	/* Initialize message's method ID. (User convenience) */
	m_msg->MID = mid;

	/* Target object is stored for user convenience */
    	m->public.targetObject  = m_obj;
	
	/* The message is stored for user convenience */
	m->public.message       = m_msg;
	
	/* Store method implemetation func and the class that will
	   receive the method call. (We skip unimplemented class calls)
	*/
	m->public.methodFunc    = ifm->MethodFunc;
	m->public.methodClass	= ifm->mClass;
	
	/* Initialize OCLASS(methodobject) */
	m->oclass	= cl;
	
	/* Return pointer to the public part */
	ReturnPtr ("Method::New", OOP_Object *, (OOP_Object *)&(m->public));
    }
    ReturnPtr ("Method::New", OOP_Object *, NULL);
    
}

/************************
**  Method::Dispose()  **
************************/
static VOID method_dispose(OOP_Class *cl, OOP_Method  *m, OOP_Msg msg )
{
    EnterFunc(bug("Method::Dispose()\n"));
    
    /* Well, free the method object */
    FreeMem(_OOP_OBJECT(m), sizeof (struct intmethod));
    
    ReturnVoid("Method::Dispose");
}

/************************
**  Support functions  **
************************/

#undef OOPBase

/* Self-explainatory */
OOP_Class *init_methodclass(struct IntOOPBase *OOPBase)
{

    struct OOP_MethodDescr methods[] =
    {
	{(IPTR (*)())method_new,	moRoot_New},
	{(IPTR (*)())method_dispose,	moRoot_Dispose},
	{ NULL, 0UL }
    };
    
    struct OOP_InterfaceDescr ifdescr[] =
    {
    	{ methods, IID_Root, 2},
	{ NULL, 0UL, 0UL}
    };
    
    struct TagItem tags[] =
    {
        {aMeta_SuperID,			(IPTR)NULL},
	{aMeta_InterfaceDescr,		(IPTR)ifdescr},
	{aMeta_ID,			(IPTR)CLID_Method},
	{aMeta_InstSize,		(IPTR)sizeof (struct method_data)},
	{TAG_DONE,  0UL}
    };

    
    OOP_Class *cl;
    
    EnterFunc(bug("init_methodclass()\n"));
    
    cl = (OOP_Class *)OOP_NewObject(NULL, CLID_MIMeta, tags);
    if (cl)
    {
    	D(bug("Method class successfully created\n"));
        cl->UserData = OOPBase;
    	OOP_AddClass(cl);
    }
    
    ReturnPtr ("init_methodclass", OOP_Class *, cl);
}
