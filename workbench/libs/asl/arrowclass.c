/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc:
    Lang: english
*/


#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/utility.h>
#include <proto/intuition.h>
#include <proto/graphics.h>
#include <proto/cybergraphics.h>
#include <exec/memory.h>
#include <intuition/screens.h>
#include <intuition/icclass.h>
#include <intuition/cghooks.h>
#include <intuition/imageclass.h>
#include <intuition/gadgetclass.h>
#include <graphics/gfx.h>
#include <cybergraphx/cybergraphics.h>

#include <string.h>

#include "asl_intern.h"
#include "layout.h"

#define SDEBUG 0
#define DEBUG 0

#include <aros/debug.h>

#define G(x) ((struct Gadget *)(x))
#define EG(x) ((struct ExtGadget *)(x))

#define CLASS_ASLBASE ((struct AslBase_intern *)cl->cl_UserData)
#define HOOK_ASLBASE  ((struct AslBase_intern *)hook->h_Data)

#define AslBase CLASS_ASLBASE

/********************** ASL ARROW CLASS **************************************************/

struct AslArrowData
{
    WORD scrollticker;
};

/***********************************************************************************/

static IPTR aslarrow_notify(Class * cl, Object * o, struct opUpdate *msg)
{
    struct AslArrowData *data;
    IPTR retval = 0;
    
    data = INST_DATA(cl, o);

    if (!data->scrollticker || (data->scrollticker == SCROLLTICKER))
    {
        retval = DoSuperMethodA(cl, o, (Msg)msg);
    }
    
    if (data->scrollticker) data->scrollticker--;

    return retval;
}

/***********************************************************************************/

static IPTR aslarrow_goactive(Class * cl, Object * o, struct gpInput *msg)
{
    struct AslArrowData *data;
    IPTR retval;
    
    data = INST_DATA(cl, o);
    data->scrollticker = SCROLLTICKER;
    
    retval = DoSuperMethodA(cl, o, (Msg)msg);
    
    return retval;
}

/***********************************************************************************/

AROS_UFH3S(IPTR, dispatch_aslarrowclass,
	  AROS_UFHA(Class *, cl, A0),
	  AROS_UFHA(Object *, obj, A2),
	  AROS_UFHA(Msg, msg, A1)
)
{
    AROS_USERFUNC_INIT

    IPTR retval = 0UL;

    switch (msg->MethodID)
    {
	case OM_NOTIFY:
	    retval = aslarrow_notify(cl, obj, (struct opUpdate *)msg);
	    break;
	    
	case GM_GOACTIVE:
	    retval = aslarrow_goactive(cl, obj, (struct gpInput *)msg);
	    break;
	        
	default:
	    retval = DoSuperMethodA(cl, obj, msg);
	    break;

    } /* switch (msg->MethodID) */
    
    return retval;

    AROS_USERFUNC_EXIT
}

/***********************************************************************************/

#undef AslBase

Class *makeaslarrowclass(struct AslBase_intern * AslBase)
{
    Class *cl = NULL;

    if (AslBase->aslarrowclass)
	return AslBase->aslarrowclass;

    cl = MakeClass(NULL, BUTTONGCLASS, NULL, sizeof(struct AslArrowData), 0UL);
    if (!cl)
	return NULL;
	
    cl->cl_Dispatcher.h_Entry = (APTR) AROS_ASMSYMNAME(dispatch_aslarrowclass);
    cl->cl_Dispatcher.h_SubEntry = NULL;
    cl->cl_UserData = (IPTR) AslBase;

    AslBase->aslarrowclass = cl;

    return cl;
}

/***********************************************************************************/
