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

#define OOPBase GetOBase(((struct IntClass *)cl)->UserData)

static struct IFMethod *FindMethod(struct IntClass *cl, ULONG mid);

struct IntMethod
{
    Class *OClass;
    Method PublicPart;
};

struct MethodData
{
    Method PublicPart;
};    
static Object *_Root_New(Class *cl, Object *o, struct P_Root_New *msg)
{
    Msg m_msg     = NULL;
    Object *m_obj = NULL;
     
    struct TagItem *tag, *tstate;
    
    ULONG mid = 0UL;
    struct IntMethod *m;
    struct IFMethod *ifm;
    
    EnterFunc(bug("Method::New()\n"));
     
    tstate = msg->AttrList;
     
    while ((tag = NextTagItem(&tstate)))
    {
     	if (IsMethodAttr(tag->ti_Tag))
	{
	    switch (TagIdx(tag->ti_Tag))
	    {
	    	case AIDX_Method_TargetObject:
		    m_obj = (Object *)tag->ti_Data;
		    break;
		
		
		case AIDX_Method_Message:
		     m_msg = (Msg)tag->ti_Data;
		     break;
		
		case AIDX_Method_MethodID:
		    mid = (ULONG)tag->ti_Data;
		    break;
		
	    }
	    
	}
	
    }
    
    if ( !(mid && m_msg && m_obj) )
    	ReturnPtr("Method::New", Object *, NULL);
	
    /* Try to find methodfunc */
    ifm = FindMethod((struct IntClass *)OCLASS(m_obj), mid);
    if (!ifm)
    	ReturnPtr("Method::New", Object *, NULL);
    
    /* Allocate mem for the method object */
    m = AllocMem( sizeof (struct IntMethod), MEMF_ANY );
    if (m);
    {

	m_msg->MethodID = mid;

    	m->PublicPart.TargetObject  = m_obj;
	m->PublicPart.Message       = m_msg;
	
	
	m->PublicPart.MethodFunc    = ifm->MethodFunc;
	m->PublicPart.MClass	    = ifm->mClass;
	
	/* Initialize OCLASS(methodobject) */
	m->OClass	= cl;
	
	ReturnPtr ("Method::New", Object *, (Object *)&(m->PublicPart));
    }
    ReturnPtr ("Method::New", Object *, NULL);
    
}

static VOID _Root_Dispose(Class *cl, Method  *m, Msg msg )
{
    EnterFunc(bug("Method::Dispose()\n"));
    
    FreeMem(_OBJECT(m), sizeof (struct IntMethod));
    
    ReturnVoid("Method::Dispose");
}

static struct IFMethod *FindMethod(struct IntClass *cl, ULONG mid)
{
    register struct IFBucket *b;
    register ULONG ifid = mid & (~METHOD_MASK);	

    mid &= METHOD_MASK;

    b = ((struct IntClass *)cl)->IFTableDirectPtr[ifid & cl->HashMask];
loop:
    if (b)
    {
        if (b->InterfaceID == ifid)
	{
	    return(&(b->MethodTable[mid]));
    	}

        b = b->Next;
        goto loop;
    }
    return (NULL);
}


#undef OOPBase

Class *InitMethodClass(struct IntOOPBase *OOPBase)
{

    struct MethodDescr methods[] =
    {
	{(IPTR (*)())_Root_New,			MIDX_Root_New},
	{(IPTR (*)())_Root_Dispose,		MIDX_Root_Dispose},
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
        ((struct  IntClass *)cl)->UserData = OOPBase;
        AddHead((struct List *)&OOPBase->ob_ClassList,
		(struct Node *)cl);
//    	AddClass(cl);
    }
    
    ReturnPtr ("InitMethodClass", Class *, cl);
}
