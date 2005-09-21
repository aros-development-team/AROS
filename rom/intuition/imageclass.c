/*
    Copyright © 1995-2005, The AROS Development Team. All rights reserved.
    Copyright © 2001-2003, The MorphOS Development Team. All Rights Reserved.
    $Id$
*/

#ifdef _SASC

#define USE_SYSBASE
#define USE_BUILTIN_MATH
#define INTUI_V36_NAMES_ONLY

#endif

#include <exec/types.h>

#include <dos/dos.h>
#include <dos/dosextens.h>

#include <intuition/intuition.h>
#include <intuition/intuitionbase.h>
#include <intuition/classes.h>
#include <intuition/classusr.h>
#include <intuition/imageclass.h>
#include <intuition/cghooks.h>
#include <intuition/icclass.h>

#include <graphics/gfxbase.h>
#include <graphics/gfxmacros.h>

#include <utility/tagitem.h>
#include <utility/hooks.h>

#include <clib/macros.h>

#include <string.h>

#include <proto/exec.h>
#include <proto/intuition.h>
#include <proto/graphics.h>
#include <proto/utility.h>

#ifndef __MORPHOS__
#include "intuition_intern.h"
#include <aros/asmcall.h>
#include <proto/alib.h>
#endif /* !__MORPHOS__ */

/***********************************************************************************/

/* Set if to 1 to enable kprintf debugging
 */
#if 0
#define D(x) x
#else
#define D(x)
#endif

#define DEBUG_HIT(x)    ;

/***********************************************************************************/

#undef IntuitionBase
#define IntuitionBase   ((struct IntuitionBase *)(cl->cl_UserData))

/***********************************************************************************/

IPTR _om_set(Class *cl, struct Image *im, struct TagItem *tags)
{
    struct TagItem *tstate = tags;
    struct TagItem *tag;
    IPTR   tidata;
    BOOL   unsupported = FALSE;

    while ((tag = NextTagItem((const struct TagItem **)&tstate)))
    {
	tidata = tag->ti_Data;

	switch (tag->ti_Tag)
	{
	case IA_Left:
	    im->LeftEdge = (WORD) tidata;
	    break;

	case IA_Top:
	    im->TopEdge = (WORD) tidata;
	    break;

	case IA_Width:
	    im->Width = (WORD) tidata;
	    break;

	case IA_Height:
	    im->Height = (WORD) tidata;
	    break;

	case IA_FGPen:
	    im->PlanePick = (WORD) tidata;
	    break;

	case IA_BGPen:
	    im->PlaneOnOff = (WORD) tidata;
	    break;
	    
	case IA_Data:
	    im->ImageData = (UWORD *) tidata;
	    break;

	default:
	    unsupported = TRUE;
	    break;

	} /* switch (Tag) */

    } /* while (Tag) */

    /*
     * If all attributes were supported and there is no retval yet,
     * set retval to 1.
     */
    if (!unsupported)
	return (IPTR)1;
    else
	return (IPTR)0;
}

IPTR ImageClass__OM_NEW(Class *cl, Object *o, struct opSet *msg)
{
    struct Image *im;
    
    D(kprintf("ImageClass OM_NEW\n"));

    if (cl)
    {
	im = (struct Image *)DoSuperMethodA(cl, o, (Msg)msg);

	if(im)
	{
	    /*
	     * This is how Intuition knows an image is a boopsi
	     * object!
	     */
	    /*
	     * The instance object is contains cleared memory!
	     * memset ((void *)retval, 0, (cl->cl_InstOffset + cl->cl_InstSize));
	     */
	    im->Width = 80;
	    im->Height = 40;
	    im->Depth = CUSTOMIMAGEDEPTH;
	    
	    _om_set(cl, im, msg->ops_AttrList);
	    
	    return (IPTR)im;
	}
	else
	    return (IPTR)NULL;
    }
    else
	return (IPTR)NULL;
}

IPTR ImageClass__OM_SET(Class *cl, struct Image *im, struct opSet *msg)
{
    D( kprintf("ImageClass OM_SET\n") );

    /*
     * Because we are a direct subclass of rootclass
     * which has no settable/gettable attributes we
     * we will NOT pass this method to our superclass!
     */
    return _om_set(cl, im, msg->ops_AttrList);
}

IPTR ImageClass__OM_GET(Class *cl, struct Image *im, struct opGet *msg)
{
    D( kprintf("ImageClass OM_GET\n") );

    switch (msg->opg_AttrID)
    {
    case IA_Left:
	*msg->opg_Storage = (IPTR) im->LeftEdge;
	break;

    case IA_Top:
	*msg->opg_Storage = (IPTR) im->TopEdge;
	break;

    case IA_Width:
	*msg->opg_Storage = (IPTR) im->Width;
	break;

    case IA_Height:
	*msg->opg_Storage = (IPTR) im->Height;
	break;

    case IA_FGPen:
	*msg->opg_Storage = (IPTR) im->PlanePick;
	break;

    case IA_BGPen:
	*msg->opg_Storage = (IPTR) im->PlaneOnOff;
	break;

    case IA_Data:
	*msg->opg_Storage = (IPTR) im->ImageData;
	break;

    default:
	return (IPTR)0;
    } /* switch */

    /*
     * Because we are a direct subclass of rootclass
     * which has no settable/gettable attributes we
     * we will NOT pass this method to our superclass!
     */
    return (IPTR)1;
}

IPTR ImageClass__IM_ERASE(Class *cl, struct Image *im, struct impErase *msg)
{
    /*
     * Both erase methods are documented as handled the same
     * at this level, so we will fall thru...
     */
    WORD left, top, width, height;

    D(kprintf("ImageClass IM_ERASE(FRAME)\n") );

    left   = im->LeftEdge + msg->imp_Offset.X;
    top    = im->TopEdge + msg->imp_Offset.Y;
    width  = im->Width - 1;
    height = im->Height - 1;

    if (msg->imp_RPort)
    {
	EraseRect(msg->imp_RPort,
		  left, top,
		  left + width, top + height
	);
    }
    
    return (IPTR)0;
}

IPTR ImageClass__IM_HITTEST(Class *cl, struct Image *im, struct impHitTest *imp)
{
    int hit;
    
    /*
     * Loosing my sanity, better check that I do not
     * have my X/Y mixed up here. :)
     */
    hit = (imp->imp_Point.X >= im->LeftEdge && imp->imp_Point.X < im->LeftEdge + im->Width)
	  && (imp->imp_Point.Y >= im->TopEdge  && imp->imp_Point.Y < im->TopEdge + im->Height)
    ;
    
    DEBUG_HIT(dprintf("image: HITTEST %d %d (%d,%d) %d×%d = %d\n", imp->imp_Point.X, imp->imp_Point.Y,
		      im->LeftEdge, im->TopEdge, im->Width, im->Height, hit));

    return (IPTR)hit;
}

/***********************************************************************************/
