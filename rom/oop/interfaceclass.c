/*
    Copyright 1995-1997 AROS - The Amiga Replacement OS
    $Id$

    Desc: Class for interface objects.
    Lang: english
*/

#include <proto/exec.h>
#include <exec/memory.h>
#include <proto/utility.h>
#include <proto/oop.h>
#include <oop/oop.h>
#include <oop/root.h>
#include <oop/meta.h>
#include <oop/interface.h>

#include "intern.h"

#undef DEBUG
#define DEBUG 1
#include <aros/debug.h>

#define OOPBase ((struct Library *)cl->UserData)
static struct IFMethod *FindIF(struct IntClass *cl, ULONG ifid);
static IPTR StdCallIF(Interface *iface, Msg msg);

struct InterfaceData
{
    Interface PublicPart;
    struct IFMethod	*MethodTable;
};    

struct IntIFObject
{
    Class *OClass;
    struct InterfaceData Inst;

};

static Object *_Root_New(Class *cl, Object *o, struct P_Root_New *msg)
{
     
    
    ULONG if_id = 0UL;
    struct IFMethod *if_mtab;
    Object *if_obj;
    struct IntIFObject *if_inst;
    
     
    if_obj = (Object *)GetTagData(A_Interface_TargetObject, NULL, msg->AttrList);
    if_id  = (ULONG)GetTagData(A_Interface_InterfaceID,  0UL,  msg->AttrList);

    EnterFunc(bug("Interface::New(if_obj=%p, if_id=%ld)\n", if_obj, if_id));
    
    if ( !(if_obj && if_id ) )
    	ReturnPtr("Interface::New", Object *, NULL);

	
    /* Try to find interface */
    D(bug("Trying to find interface: %ld\n", if_id));

    if_mtab = FindIF((struct IntClass *)OCLASS(if_obj), if_id);
    if (!if_mtab)
    	ReturnPtr("Interface::New", Object *, NULL);
	
    D(bug("mtab found: %p\n", if_mtab));
    
    /* Allocate mem for the interface object */
    if_inst = AllocMem( sizeof (struct IntIFObject), MEMF_ANY );
    if (if_inst)
    {

        D(bug("obj alloced\n"));
	
	if_inst->Inst.PublicPart.TargetObject = if_obj;
	

	if_inst->Inst.PublicPart.Call = StdCallIF;
	if_inst->Inst.MethodTable = if_mtab;
	
	/* Initialize OCLASS(interfaceobject) */
	if_inst->OClass	= cl;
	
	ReturnPtr ("Interface::New", Object *, (Object *)&(if_inst->Inst.PublicPart));
    }
    ReturnPtr ("Interface::New", Object *, NULL);
    
}

static VOID _Root_Dispose(Class *cl, Interface  *ifobj, Msg msg )
{
    EnterFunc(bug("Interface::Dispose()\n"));
    
    FreeMem(_OBJECT(ifobj), sizeof (struct IntIFObject));
    
    ReturnVoid("Interface::Dispose");
}

static struct IFMethod *FindIF(struct IntClass *cl, ULONG ifid)
{
    register struct IFBucket *b;

    b = ((struct IntClass *)cl)->IFTableDirectPtr[ifid & cl->HashMask];
loop:
    if (b)
    {
        if (b->InterfaceID == ifid)
	{
	    return(b->MethodTable);
    	}

        b = b->Next;
        goto loop;
    }
    return (NULL);
}

static IPTR StdCallIF(Interface *iface, Msg msg)
{
    register ULONG midx = msg->MethodID & METHOD_MASK;
    register struct IFMethod *method;
    
    method = &( ((struct InterfaceData *)iface)->MethodTable[midx] );
    
    return ( method->MethodFunc(method->mClass, iface->TargetObject, msg) );

}

#undef OOPBase


Class *InitInterfaceClass(struct Library *OOPBase)
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
	{A_Class_ID,			(IPTR)INTERFACECLASS},
	{A_Class_InstSize,		(IPTR)sizeof (struct InterfaceData)},
	{TAG_DONE, 0UL}
    };

    
    Class *cl;
    
    EnterFunc(bug("InitInterfaceClass()\n"));
    
    cl = (Class *)NewObjectA(NULL, METACLASS, tags);
    if (cl)
    {
        cl->UserData = OOPBase;
    	AddClass(cl);
    }
    
    ReturnPtr ("InitInterfaceClass", Class *, cl);
}
