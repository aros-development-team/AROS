/*
    (C) 2000 AROS - The Amiga Research OS
    $Id$

    Desc: colorwheel class implementation.
    Lang: english
*/


#include <exec/libraries.h>
#include <graphics/rastport.h>
#include <graphics/text.h>
#include <intuition/classes.h>
#include <intuition/classusr.h>
#include <intuition/imageclass.h>
#include <intuition/intuition.h>
#include <utility/tagitem.h>
#include <devices/inputevent.h>
#include <gadgets/colorwheel.h>

#include <proto/exec.h>
#include <proto/graphics.h>
#include <proto/intuition.h>
#include <proto/utility.h>
#include <proto/alib.h>

#include <aros/debug.h>

#include "colorwheel_intern.h"


#define ColorWheelBase ((LIBBASETYPEPTR)(cl->cl_UserData))

/* Insert colorwheel.gadget implementation here */


#undef ColorWheelBase

/*************************** Classes *****************************/

struct IClass *InitColorWheelClass (LIBBASETYPEPTR ColorWheelBase)
{
    Class *cl = NULL;
#if 0
    cl = MakeClass (COLORWHEELCLASS, GADGETCLASS, NULL, sizeof (struct ColorWheelData), 0);
    if (cl) 
    {
	cl->cl_Dispatcher.h_Entry    = (APTR) AROS_ASMSYMNAME (dispatch_checkclass);
	cl->cl_Dispatcher.h_SubEntry = NULL;
	cl->cl_UserData 	     = (IPTR) ColorWheelBase;

	AddClass (cl);
    }
#endif
    return (cl);
}
