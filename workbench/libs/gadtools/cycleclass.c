/*
   (C) 1997 AROS - The Amiga Research OS
   $Id$

   Desc: Internal GadTools cycle class.
   Lang: English
 */
 
#undef AROS_ALMOST_COMPATIBLE
#define AROS_ALMOST_COMPATIBLE 1

#include <proto/exec.h>
#include <exec/libraries.h>
#include <exec/memory.h>
#include <proto/dos.h>
#include <intuition/classes.h>
#include <intuition/classusr.h>
#include <intuition/imageclass.h>
#include <intuition/intuition.h>
#include <intuition/cghooks.h>
#include <graphics/rastport.h>
#include <graphics/text.h>
#include <utility/tagitem.h>
#include <devices/inputevent.h>
#include <proto/alib.h>
#include <proto/utility.h>
#include <gadgets/aroscycle.h>

#include <string.h> /* memset() */

#define SDEBUG 0
#define DEBUG 1
#include <aros/debug.h>

#include "gadtools_intern.h"

/**********************************************************************************************/

#define G(x) ((struct Gadget *)(x))
#define EG(X) ((struct ExtGadget *)(x))

#define GadToolsBase ((struct GadToolsBase_intern *)cl->cl_UserData)

/**********************************************************************************************/

struct CycleData
{
    UBYTE labelplace;
};

/**********************************************************************************************/

STATIC IPTR cycle_new(Class *cl, Object *o, struct opSet *msg)
{ 
    o = (Object *)DoSuperMethodA(cl, o, (Msg)msg);   
    if (o)
    {
	struct CycleData *data = INST_DATA(cl, o);
		    
	data->labelplace = GetTagData(GA_LabelPlace, GV_LabelPlace_Left, msg->ops_AttrList);
    }
    
    return (IPTR)o;
}

/**********************************************************************************************/

STATIC IPTR cycle_render(Class *cl, Object *o, struct gpRender *msg)
{
    struct CycleData *data;
    IPTR 	     retval;
    
    data = INST_DATA(cl, o);
    
    if (msg->gpr_Redraw == GREDRAW_REDRAW)
    {
       renderlabel(GadToolsBase, (struct Gadget *)o, msg->gpr_RPort, data->labelplace);
    }
    
    retval = DoSuperMethodA(cl, o, (Msg)msg);
    
    ReturnInt("Cycle::Render", IPTR, retval);
}

/**********************************************************************************************/

STATIC IPTR cycle_get(Class *cl, Object *o, struct opGet *msg)
{
    IPTR retval;
    
    switch (msg->opg_AttrID)
    {
	case GTA_GadgetKind:
	case GTA_ChildGadgetKind:
	    *(msg->opg_Storage) = CYCLE_KIND;
	    retval = 1UL;
	    break;

	default:
	    retval = DoSuperMethodA(cl, o, (Msg)msg);
	    break;
    }
    
    return retval;
}

/**********************************************************************************************/

AROS_UFH3S(IPTR, dispatch_cycleclass,
	  AROS_UFHA(Class *, cl, A0),
	  AROS_UFHA(Object *, obj, A2),
	  AROS_UFHA(Msg, msg, A1)
)
{
    IPTR retval;
	    
    switch (msg->MethodID)
    {
	case OM_NEW:
 	    retval = cycle_new(cl, obj, (struct opSet *)msg);
	    break;
	    
	case OM_GET:
	    retval = cycle_get(cl, obj, (struct opGet *)msg);
	    break;

	case GM_RENDER:
    	    retval = cycle_render(cl, obj, (struct gpRender *)msg);
	    break;

	default:
	    retval = DoSuperMethodA(cl, obj, msg);
	    break;
    }

    return retval;
}

/**********************************************************************************************/

#undef GadToolsBase

Class *makecycleclass(struct GadToolsBase_intern * GadToolsBase)
{
    Class *cl;

    ObtainSemaphore(&GadToolsBase->classsema);

    cl = GadToolsBase->cycleclass;
    if (!cl)
    {
	if (!GadToolsBase->aroscybase)
            GadToolsBase->aroscybase = OpenLibrary(AROSCYCLENAME, 0);
	
	if (GadToolsBase->aroscybase)
	{
	    cl = MakeClass(NULL, AROSCYCLECLASS, NULL, sizeof(struct CycleData), 0UL);
	    if (cl)
	    {
		cl->cl_Dispatcher.h_Entry = (APTR) AROS_ASMSYMNAME(dispatch_cycleclass);
		cl->cl_Dispatcher.h_SubEntry = NULL;
		cl->cl_UserData = (IPTR) GadToolsBase;

		GadToolsBase->cycleclass = cl;
	    }
	}
    }
    
    ReleaseSemaphore(&GadToolsBase->classsema);
  
    return cl;
}

/**********************************************************************************************/
