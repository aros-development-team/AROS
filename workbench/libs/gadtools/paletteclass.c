/*
   (C) 1997 AROS - The Amiga Research OS
   $Id$

   Desc: Internal GadTools palette class.
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
#include <gadgets/arospalette.h>

#include <string.h> /* memset() */

#define SDEBUG 0
#define DEBUG 0
#include <aros/debug.h>

#include "gadtools_intern.h"

/**********************************************************************************************/

#define G(x) ((struct Gadget *)(x))
#define EG(X) ((struct ExtGadget *)(x))

#define GadToolsBase ((struct GadToolsBase_intern *)cl->cl_UserData)

/**********************************************************************************************/

struct PaletteData
{
    UBYTE dummy;
};

/**********************************************************************************************/

STATIC IPTR palette_get(Class *cl, Object *o, struct opGet *msg)
{
    IPTR retval;
    
    switch (msg->opg_AttrID)
    {
	case GTA_GadgetKind:
	case GTA_ChildGadgetKind:
	    *(msg->opg_Storage) = PALETTE_KIND;
	    retval = 1UL;
	    break;

	default:
	    retval = DoSuperMethodA(cl, o, (Msg)msg);
	    break;
    }
    
    return retval;
}

/**********************************************************************************************/

AROS_UFH3S(IPTR, dispatch_paletteclass,
	  AROS_UFHA(Class *, cl, A0),
	  AROS_UFHA(Object *, obj, A2),
	  AROS_UFHA(Msg, msg, A1)
)
{
    IPTR retval;

    switch (msg->MethodID)
    {
	case OM_GET:
	    retval = palette_get(cl, obj, (struct opGet *)msg);
	    break;

	default:
	    retval = DoSuperMethodA(cl, obj, msg);
	    break;
    }

    return retval;
}

/**********************************************************************************************/

#undef GadToolsBase

Class *makepaletteclass(struct GadToolsBase_intern * GadToolsBase)
{
    Class *cl;

    ObtainSemaphore(&GadToolsBase->classsema);

    cl = GadToolsBase->paletteclass;
    if (!cl)
    {
	if (!GadToolsBase->arospabase)
            GadToolsBase->arospabase = OpenLibrary(AROSPALETTENAME, 0);

	if (GadToolsBase->arospabase)
	{
	    cl = MakeClass(NULL, AROSPALETTECLASS, NULL, sizeof(struct PaletteData), 0UL);
	    if (cl)
	    {
		cl->cl_Dispatcher.h_Entry = (APTR) AROS_ASMSYMNAME(dispatch_paletteclass);
		cl->cl_Dispatcher.h_SubEntry = NULL;
		cl->cl_UserData = (IPTR) GadToolsBase;

		GadToolsBase->paletteclass = cl;
	    }
	}
    }
    
    ReleaseSemaphore(&GadToolsBase->classsema);
    
    return cl;
}

/**********************************************************************************************/
