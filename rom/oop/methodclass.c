/*
    Copyright (C) 1995-1997 AROS - The Amiga Research OS
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

#define OOPBase GetOBase(((Class *)cl)->UserData)

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
    
    Class *oclass;
    
    /* The app gets a pointer to &(intmethod->PublicPart).
       The public part is a readonly public struct.
     */
    Method public;
};

struct method_data
{
    Method public;
};

#define IS_METHOD_ATTR(attr, idx) ((idx = attr - MethodAttrBase) < num_Method_Attrs)
/********************
**  Method::New()  **
********************/
static Object *method_new(Class *cl, Object *o, struct pRoot_New *msg)
{
    Msg m_msg     = NULL;
    Object *m_obj = NULL;
     
    struct TagItem *tag, *tstate;
    
    ULONG mid = 0UL;
    struct intmethod *m;
    struct IFMethod *ifm;
    
    ULONG idx;
    
    EnterFunc(bug("Method::New()\n"));
    
    
    /* Parse the createion-time attributes passed to the object */ 
    tstate = msg->attrList;
    
    while ((tag = NextTagItem((const struct TagItem **)&tstate)))
    {
     	if (IS_METHOD_ATTR(tag->ti_Tag, idx))
	{
	    switch (idx)
	    {
	    	case aoMethod_TargetObject:
		    /* The object from which we get the method */
		    m_obj = (Object *)tag->ti_Data;
		    break;
		
		
		case aoMethod_Message:
		     /* The message to pass with the method */
		     m_msg = (Msg)tag->ti_Data;
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
    	ReturnPtr("Method::New", Object *, NULL);
	
    /* Try to find methodfunc */
    D(bug("trying to find method, oclass=%p, oclass(oclass)=%p\n",
    	OCLASS(m_obj), OCLASS(OCLASS(m_obj)) ));
    D(bug("oclass(oclass)=%s\n", OCLASS(OCLASS(m_obj))->ClassNode.ln_Name));
    
    ifm = meta_findmethod((Object *)OCLASS(m_obj), mid, (struct Library *)OOPBase);
    D(bug("found method: %p\n", ifm));
    if (!ifm)
    	/* If method isn't supported by target object, exit gracefully */
    	ReturnPtr("Method::New", Object *, NULL);
    
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
	ReturnPtr ("Method::New", Object *, (Object *)&(m->public));
    }
    ReturnPtr ("Method::New", Object *, NULL);
    
}

/************************
**  Method::Dispose()  **
************************/
static VOID method_dispose(Class *cl, Method  *m, Msg msg )
{
    EnterFunc(bug("Method::Dispose()\n"));
    
    /* Well, free the method object */
    FreeMem(_OBJECT(m), sizeof (struct intmethod));
    
    ReturnVoid("Method::Dispose");
}

/************************
**  Support functions  **
************************/

#undef OOPBase

/* Self-explainatory */
Class *init_methodclass(struct IntOOPBase *OOPBase)
{

    struct MethodDescr methods[] =
    {
	{(IPTR (*)())method_new,	moRoot_New},
	{(IPTR (*)())method_dispose,	moRoot_Dispose},
	{ NULL, 0UL }
    };
    
    struct InterfaceDescr ifdescr[] =
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

    
    Class *cl;
    
    EnterFunc(bug("init_methodclass()\n"));
    
    cl = (Class *)NewObject(NULL, CLID_MIMeta, tags);
    if (cl)
    {
    	D(bug("Method class successfully created\n"));
        cl->UserData = OOPBase;
    	AddClass(cl);
    }
    
    ReturnPtr ("init_methodclass", Class *, cl);
}
