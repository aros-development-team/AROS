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
#define DEBUG 0
#include <aros/debug.h>

#define OOPBase ((struct Library *)cl->UserData)
static struct IFMethod *FindIF(struct IntClass *cl, ULONG ifid);
static IPTR StdCallIF(Interface *iface, Msg msg);

struct InterfaceData
{
    /* First part of the interface object's instance data is
       public, and may be accesesd directly.
    */
    Interface PublicPart;
    
    /* The pointer to the interface's methods should indeed not
       be public.
    */
    struct IFMethod	*MethodTable;
};    


struct IntIFObject
{
    /* All objects has a pointer to their class at ((VOID **)o)[-1] */
    Class *OClass;
    
    /* When getting a interface ojject, you will get a pointer to
       &(intifobject->Inst.PublicPart)
    */
    struct InterfaceData Inst;

};

/************
**  New()  **
************/
static Object *_Root_New(Class *cl, Object *o, struct P_Root_New *msg)
{
     
    
    ULONG if_id = 0UL;
    struct IFMethod *if_mtab;
    Object *if_obj;
    struct IntIFObject *if_inst;
    
    /* Parse parameters */
    
    /* What target object does the user want an interface object for ? */
    if_obj = (Object *)GetTagData(A_Interface_TargetObject, NULL, msg->AttrList);
    
    /* What interface does he want from the object ? */
    if_id  = (ULONG)GetTagData(A_Interface_InterfaceID,  0UL,  msg->AttrList);

    EnterFunc(bug("Interface::New(if_obj=%p, if_id=%ld)\n", if_obj, if_id));
    
    /* We MUST have those two parameters, to be able to
       create an interface object */
    if ( !(if_obj && if_id ) )
    	ReturnPtr("Interface::New", Object *, NULL);

	
    D(bug("Trying to find interface: %ld\n", if_id));

    /* Try to find interface in the target object's class*/
    if_mtab = FindIF((struct IntClass *)OCLASS(if_obj), if_id);
    
    if (!if_mtab) 
    	/* Not supported. Failed. */
    	ReturnPtr("Interface::New", Object *, NULL);
	
    D(bug("mtab found: %p\n", if_mtab));
    
    /* Allocate mem for the interface object */
    if_inst = AllocMem( sizeof (struct IntIFObject), MEMF_ANY );
    if (if_inst)
    {

        D(bug("obj alloced\n"));
	
	/* The interface object remebers it's target object.
	   Convenience for the user + he can store interface
	   objects instead of a pointer to the object itself.
	   (He doesn't have to store both)
	*/
	
	if_inst->Inst.PublicPart.TargetObject = if_obj;
	
	
	/* Function for calling a method on an interface obect.
	   Just use a standard one, but we could add support
	   for letting the user override the function with his own.
	*/
	if_inst->Inst.PublicPart.Call = StdCallIF;
	
	/* The interface object must have some methods to call :-) */
	if_inst->Inst.MethodTable = if_mtab;
	
	/* Initialize OCLASS(interfaceobject) */
	if_inst->OClass	= cl;
	
	ReturnPtr ("Interface::New", Object *, (Object *)&(if_inst->Inst.PublicPart));
    }
    ReturnPtr ("Interface::New", Object *, NULL);
    
}

/****************
**  Dispose()  **
****************/
static VOID _Root_Dispose(Class *cl, Interface  *ifobj, Msg msg )
{
    EnterFunc(bug("Interface::Dispose()\n"));
    
    /* Just free the thing */
    FreeMem(_OBJECT(ifobj), sizeof (struct IntIFObject));
    
    ReturnVoid("Interface::Dispose");
}


/************************
**  Support functions  **
************************/

/* FindIF() finds the method table of an interface inside a class.
   If the interface isn't supported by the class, it returns NULL
*/

static struct IFMethod *FindIF(struct IntClass *cl, ULONG ifid)
{
    register struct IFBucket *b;

    /* Get offset into hashtable (linked list of buckets) */
    b = ((struct IntClass *)cl)->IFTableDirectPtr[ifid & cl->HashMask];
    
    /* Look thriugh the linked list of buckets for the correct one */
loop:
    if (b)
    {
    	/* Correct bucket ? */
        if (b->InterfaceID == ifid)
	{
	    /* Yep. return methodtable */
	    return(b->MethodTable);
    	}

	/* Not correct ID. Try again with next bucket in linked list */
        b = b->Next;
        goto loop;
    }
    /* Interface not found in hash table */
    return (NULL);
}

/* Default way to call a interface objects' method.
   (Inserted into the Interface struct's CallIF() field
*/
static IPTR StdCallIF(Interface *iface, Msg msg)
{
    /* Mask off the method offset */
    register ULONG midx = msg->MethodID & METHOD_MASK;
    
    register struct IFMethod *method;
    
    /* Get the correct method from the correct offset */
    method = &( ((struct InterfaceData *)iface)->MethodTable[midx] );
    
    /* ... and call the method on the interface object's target object */
    return ( method->MethodFunc(method->mClass, iface->TargetObject, msg) );

}

#undef OOPBase

/* Well, initalize the interface class. Self explainatory */
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
