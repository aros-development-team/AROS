/*
    Copyright © 1995-2005, The AROS Development Team. All rights reserved.
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

#define CLASS_ASLBASE ((struct AslBase_intern *)cl->cl_UserData)
#define HOOK_ASLBASE  ((struct AslBase_intern *)hook->h_Data)

#define AslBase CLASS_ASLBASE

/********************** ASL ARROW CLASS **************************************************/

IPTR AslArrow__OM_NOTIFY(Class * cl, Object * o, struct opUpdate *msg)
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

IPTR AslArrow__GM_GOACTIVE(Class * cl, Object * o, struct gpInput *msg)
{
    struct AslArrowData *data;
    IPTR retval;
    
    data = INST_DATA(cl, o);
    data->scrollticker = SCROLLTICKER;
    
    retval = DoSuperMethodA(cl, o, (Msg)msg);
    
    return retval;
}

/***********************************************************************************/
