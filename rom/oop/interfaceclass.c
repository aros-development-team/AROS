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

#include "private.h"

#include "intern.h"

#undef SDEBUG
#undef DEBUG
#define DEBUG 0
#define SDEBUG 0
#include <aros/debug.h>

#define OOPBase ((struct Library *)cl->UserData)

static IPTR StdCallIF(Interface *iface, Msg msg);

struct interface_data
{
    /* First part of the interface object's instance data is
       public, and may be accesesd directly.
    */
    Interface public;
    
    /* The pointer to the interface's methods should indeed not
       be public.
    */
    struct IFMethod	*methodtable;
};    


struct interface_object
{
    /* All objects has a pointer to their class at ((VOID **)o)[-1] */
    Class *oclass;
    
    /* When getting a interface ojject, you will get a pointer to
       &(intifobject->Inst.PublicPart)
    */
    struct interface_data data;

};

/***********************
**  Interface::New()  **
***********************/
static Object *interface_new(Class *cl, Object *o, struct pRoot_New *msg)
{
     
    
    STRPTR if_id = NULL;
    struct IFMethod *if_mtab;
    Object *if_obj;
    struct interface_object *ifo;
    
    /* Parse parameters */
    
    /* What target object does the user want an interface object for ? */
    if_obj = (Object *)GetTagData(aInterface_TargetObject, NULL, msg->attrList);
    
    /* What interface does he want from the object ? */
    if_id  = (STRPTR)GetTagData(aInterface_InterfaceID,  NULL,  msg->attrList);

    EnterFunc(bug("Interface::New(if_obj=%p, if_id=%s)\n", if_obj, if_id));
    
    /* We MUST have those two parameters, to be able to
       create an interface object */
    if ( !(if_obj && if_id ) )
    	ReturnPtr("Interface::New", Object *, NULL);

	
    D(bug("Trying to find interface: %s\n", if_id));

    /* Try to find interface in the target object's class*/
    if_mtab = findinterface(OCLASS(if_obj), if_id);
    
    if (!if_mtab) 
    	/* Not supported. Failed. */
    	ReturnPtr("Interface::New", Object *, NULL);
	
    D(bug("mtab found: %p\n", if_mtab));
    
    /* Allocate mem for the interface object */
    ifo = AllocMem( sizeof (struct interface_object), MEMF_ANY );
    if (ifo)
    {

        D(bug("obj alloced\n"));
	
	/* The interface object remebers it's target object.
	   Convenience for the user + he can store interface
	   objects instead of a pointer to the object itself.
	   (He doesn't have to store both)
	*/
	
	ifo->data.public.targetObject = if_obj;
	
	
	/* Function for calling a method on an interface obect.
	   Just use a standard one, but we could add support
	   for letting the user override the function with his own.
	*/
	ifo->data.public.callMethod = StdCallIF;
	
	/* The interface object must have some methods to call :-) */
	ifo->data.methodtable = if_mtab;
	
	/* Initialize OCLASS(interfaceobject) */
	ifo->oclass	= cl;
	
	ReturnPtr ("Interface::New", Object *, (Object *)&(ifo->data.public));
    }
    ReturnPtr ("Interface::New", Object *, NULL);
    
}

/****************
**  Dispose()  **
****************/
static VOID interface_dispose(Class *cl, Interface  *ifobj, Msg msg )
{
    EnterFunc(bug("Interface::Dispose()\n"));
    
    /* Just free the thing */
    FreeMem(_OBJECT(ifobj), sizeof (struct interface_object));
    
    ReturnVoid("Interface::Dispose");
}


/************************
**  Support functions  **
************************/


/* Default way to call a interface objects' method.
   (Inserted into the Interface struct's CallIF() field
*/
static IPTR StdCallIF(Interface *iface, Msg msg)
{
    /* Mask off the method offset */
    register ULONG midx = msg->MID & METHOD_MASK;
    
    register struct IFMethod *method;
    
    /* Get the correct method from the correct offset */
    method = &( ((struct interface_data *)iface)->methodtable[midx] );
    
    /* ... and call the method on the interface object's target object */
    return ( method->MethodFunc(method->mClass, iface->targetObject, msg) );

}

#undef OOPBase

/* Well, initalize the interface class. Self explainatory */
Class *init_interfaceclass(struct Library *OOPBase)
{

    struct MethodDescr methods[] =
    {
	{(IPTR (*)())interface_new,		moRoot_New},
	{(IPTR (*)())interface_dispose,		moRoot_Dispose},
	{ NULL, 0UL }
    };
    
    struct InterfaceDescr ifdescr[] =
    {
    	{ methods, IID_Root, 2},
	{ NULL, 0UL, 0UL}
    };
    
    struct TagItem tags[] =
    {
        {aMeta_SuperID,		(IPTR)NULL},
	{aMeta_InterfaceDescr,		(IPTR)ifdescr},
	{aMeta_ID,			(IPTR)CLID_Interface},
	{aMeta_InstSize,		(IPTR)sizeof (struct interface_data)},
	{TAG_DONE, 0UL}
    };

    
    Class *cl;
    
    EnterFunc(bug("init_interfaceclass()\n"));
    
    cl = (Class *)NewObject(NULL, CLID_MIMeta, tags);
    if (cl)
    {
        cl->UserData = OOPBase;
    	AddClass(cl);
    }
    
    ReturnPtr ("init_interfaceclass", Class *, cl);
}
