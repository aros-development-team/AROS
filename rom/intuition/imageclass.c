/* This is an native Amiga SAS/C implementation of the Amiga Rom
 * 'imageclass' for AROS.  I do not yet have GCC set up, nor access
 * to Linux for testing. Hopefully someone will adapt it to the AROS
 * coding style.
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

#include <clib/alib_protos.h>
#include <proto/exec.h>
#include <proto/intuition.h>
#include <proto/graphics.h>
#include <proto/utility.h>

#include "intuition_intern.h"

/****************************************************************************/

/* Set if to 1 to enable kprintf debugging
 */
#if 0
#define D(x) x
#else
#define D(x)
#endif

/* Some handy transparent base class object casting defines.
 */
#define G(o)  ((struct Gadget *)o)
#define EG(o) ((struct ExtGadget *)o)
#define IM(o) ((struct Image *)o)

/****************************************************************************/

/****************************************************************************/


/* Our imageclass dispatcher.
 */
__RA3(static IPTR, dispatch_imageclass,
    Class *,  cl,  A0,
    Object *, o,   A2,
    Msg,      msg, A1)
{
    ULONG retval = 0UL;

    switch (msg->MethodID)
    {
    case OM_NEW:
    {
	D(kprintf("ImageClass OM_NEW\n"));

	/*
	    We are paranoid, and will check validity of objects.
	    I beleive the AmigaOS, atleast at some level, allows NULL.
	*/

	if (o)
	{
	    if (!cl)
		cl = OCLASS(o);

	    if (cl)
	    {
		D( kprintf("ImageClass Instance Size; %ld bytes\n", SIZEOF_INSTANCE(cl)) );

		retval = (ULONG)DoSuperMethodA(cl, o, msg);

		if(retval)
		{
		    /*
			This is how Intuition knows an image is a boopsi
			object!
		    */
		    IM(retval)->Depth = CUSTOMIMAGEDEPTH;
		}
	    }
	    D( else { kprintf("Class pointer is NULL\n") } );
	}
	D( else { kprintf("Carrier Object pointer is NULL\n") } );
	break;
    }

    case OM_GET:
	D( kprintf("ImageClass OM_GET\n") );

	if (o)
	{
	    switch (((struct opGet *)msg)->opg_AttrID)
	    {
	    case IA_Left:
		*((struct opGet *)msg)->opg_Storage = (ULONG) IM(o)->LeftEdge;
		break;

	    case IA_Top:
		*((struct opGet *)msg)->opg_Storage = (ULONG) IM(o)->TopEdge;
		break;

	    case IA_Width:
		*((struct opGet *)msg)->opg_Storage = (ULONG) IM(o)->Width;
		break;

	    case IA_Height:
		*((struct opGet *)msg)->opg_Storage = (ULONG) IM(o)->Height;
		break;

	    case IA_FGPen:
		*((struct opGet *)msg)->opg_Storage = (ULONG) IM(o)->PlanePick;
		break;

	    case IA_BGPen:
		*((struct opGet *)msg)->opg_Storage = (ULONG) IM(o)->PlaneOnOff;
		break;

	    case IA_Data:
		*((struct opGet *)msg)->opg_Storage = (ULONG) IM(o)->ImageData;
		break;

	    } /* switch */

	    /*
		Because we are a direct subclass of rootclass
		which has no settable/gettable attributes we
		we will NOT pass this method to our superclass!
	    */
	}
	break;

    case OM_SET:
	D( kprintf("ImageClass OM_SET\n") );

	if (o)
	{
	    struct TagItem *tstate = ((struct opSet *)msg)->ops_AttrList;
	    struct TagItem *tag;
	    ULONG tidata;

	    while ((tag = NextTagItem(&tstate)))
	    {
		tidata = tag->ti_Data;

		switch (tag->ti_Tag)
		{
		case IA_Left:
			IM(o)->LeftEdge = (WORD) tidata;
			retval = 1UL;
			break;

		case IA_Top:
			IM(o)->TopEdge = (WORD) tidata;
			retval = 1UL;
			break;

		case IA_Width:
			IM(o)->Width = (WORD) tidata;
			retval = 1UL;
			break;

		case IA_Height:
			IM(o)->Height = (WORD) tidata;
			retval = 1UL;
			break;

		case IA_FGPen:
			IM(o)->PlanePick = (WORD) tidata;
			retval = 1UL;
			break;

		case IA_BGPen:
			IM(o)->PlaneOnOff = (WORD) tidata;
			retval = 1UL;
			break;

		case IA_Data:
			IM(o)->ImageData = (UWORD *) tidata;
			retval = 1UL;
			break;
		} /* switch */
	    } /* while */

	    /*
		Because we are a direct subclass of rootclass
		which has no settable/gettable attributes we
		we will NOT pass this method to our superclass!
	    */
	}
	D( else { kprintf("Object pointer is NULL\n") } );
	break;

    case IM_ERASE:
	/*
	    Both erase methods are documented as handled the same
	    at this level, so we will fall thru...
	*/
    case IM_ERASEFRAME:
	D(kprintf("ImageClass IM_ERASE(FRAME)\n") );

	if (o)
	{
	    WORD left, top, width, height;

	    left   = IM(o)->LeftEdge + ((struct impErase *)msg)->imp_Offset.X;
	    top    = IM(o)->TopEdge + ((struct impErase *)msg)->imp_Offset.Y;
	    width  = IM(o)->Width - 1;
	    height = IM(o)->Height - 1;

	    EraseRect(((struct impErase *)msg)->imp_RPort,
		left, top,
		left + width, top + height
	    );
	} /* if */
	retval = 1UL;
	break;

    case IM_HITTEST:
	/*
	    Both hitmethods are documented as handled the same
	    at this level, so we will fall thru...
	*/
    case IM_HITFRAME:
	if(o)
	{
	    struct impHitTest *imp = (struct impHitTest *)msg;

	    /*
		Loosing my sanity, better check that I do not have
		have my X/Y mixed up here. :)
	    */
	    if( (imp->imp_Point.X >= IM(o)->LeftEdge && imp->imp_Point.X <= IM(o)->LeftEdge + IM(o)->Width) &&
		(imp->imp_Point.Y >= IM(o)->TopEdge  && imp->imp_Point.Y <= IM(o)->TopEdge + IM(o)->Height)
		)
	    {
		return(1UL);
	    } /* if */
	}
	break;

    /* case OM_DISPOSE */
    default:
	retval = DoSuperMethodA(cl, o, msg);
	break;

    } /* switch */

    return (retval);
} /* dispatch_imageclass */

/****************************************************************************/

/* Initialize our image class.
 */
struct IClass *InitImageClass(void)
{
    struct IClass *cl = NULL;

    /* This is the code to make the image class...
	*/
    if ((cl = MakeClass(IMAGECLASS, ROOTCLASS, NULL, sizeof(struct Image), 0)))
    {
	cl->cl_Dispatcher.h_Entry = (APTR)dispatch_imageclass;
	cl->cl_Dispatcher.h_SubEntry = NULL;

	AddClass (cl);
    }

    return (cl);
}

/* Remove the image class. Normally, you have no reason for this
 * for rom baseclass such as imageclass.
 */
BOOL FreeImageClass( struct IClass *cl)
{
    return ((BOOL)FreeClass(cl));
}

