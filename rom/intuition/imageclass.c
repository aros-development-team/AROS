/*
    (C) 1995-96 AROS - The Amiga Replacement OS
    $Id$

    Desc: Implementation of IMAGECLASS
    Lang: english
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

#ifdef _SASC
#include <proto/exec.h>
#include <proto/intuition.h>
#include <proto/graphics.h>
#include <proto/utility.h>
#elif __GNUC__
#include <clib/exec_protos.h>
#include <clib/intuition_protos.h>
#include <clib/graphics_protos.h>
#include <clib/utility_protos.h>
#endif

#ifdef _AROS
#include <aros/asmcall.h>
#include <clib/alib_protos.h>
#include "intuition_intern.h"
#endif

#if 0 /* This doesn't belong here, but don't loose it */
/* Image data */
#define ARROWDOWN_WIDTH    18
#define ARROWDOWN_HEIGHT   11

UWORD ArrowDown0Data[] =
{
    0x0000, 0x4000, 0x0000, 0x4000, 0x0000, 0x4000, 0x0C0C, 0x4000,
    0x0738, 0x4000, 0x03F0, 0x4000, 0x01E0, 0x4000, 0x00C0, 0x4000,
    0x0000, 0x4000, 0x0000, 0x4000, 0x7FFF, 0xC000,

    0xFFFF, 0x8000, 0x8000, 0x0000, 0x8000, 0x0000, 0x8000, 0x0000,
    0x8000, 0x0000, 0x8000, 0x0000, 0x8000, 0x0000, 0x8000, 0x0000,
    0x8000, 0x0000, 0x8000, 0x0000, 0x8000, 0x0000,
};

UWORD ArrowDown1Data[] =
{
    0xFFFF, 0x8000, 0x8000, 0x0000, 0x8000, 0x0000, 0x8C0C, 0x0000,
    0x8738, 0x0000, 0x83F0, 0x0000, 0x81E0, 0x0000, 0x80C0, 0x0000,
    0x8000, 0x0000, 0x8000, 0x0000, 0x8000, 0x0000,

    0x0000, 0x4000, 0x0000, 0x4000, 0x0000, 0x4000, 0x0000, 0x4000,
    0x0000, 0x4000, 0x0000, 0x4000, 0x0000, 0x4000, 0x0000, 0x4000,
    0x0000, 0x4000, 0x0000, 0x4000, 0x7FFF, 0xC000,
};

#define ARROWUP_WIDTH	 18
#define ARROWUP_HEIGHT	 11

UWORD ArrowUp0Data[] =
{
    0x0000, 0x4000, 0x0000, 0x4000, 0x0000, 0x4000, 0x00C0, 0x4000,
    0x01E0, 0x4000, 0x03F0, 0x4000, 0x0738, 0x4000, 0x0C0C, 0x4000,
    0x0000, 0x4000, 0x0000, 0x4000, 0x7FFF, 0xC000,

    0xFFFF, 0x8000, 0x8000, 0x0000, 0x8000, 0x0000, 0x8000, 0x0000,
    0x8000, 0x0000, 0x8000, 0x0000, 0x8000, 0x0000, 0x8000, 0x0000,
    0x8000, 0x0000, 0x8000, 0x0000, 0x8000, 0x0000,
};

UWORD ArrowUp1Data[] =
{
    0xFFFF, 0x8000, 0x8000, 0x0000, 0x8000, 0x0000, 0x80C0, 0x0000,
    0x81E0, 0x0000, 0x83F0, 0x0000, 0x8738, 0x0000, 0x8C0C, 0x0000,
    0x8000, 0x0000, 0x8000, 0x0000, 0x8000, 0x0000,

    0x0000, 0x4000, 0x0000, 0x4000, 0x0000, 0x4000, 0x0000, 0x4000,
    0x0000, 0x4000, 0x0000, 0x4000, 0x0000, 0x4000, 0x0000, 0x4000,
    0x0000, 0x4000, 0x0000, 0x4000, 0x7FFF, 0xC000,
};

#define ARROWLEFT_WIDTH    11
#define ARROWLEFT_HEIGHT   16

UWORD ArrowLeft0Data[] =
{
    0x0000, 0x0020, 0x0020, 0x0120, 0x0320, 0x0620, 0x0E20, 0x1C20,
    0x1C20, 0x0E20, 0x0620, 0x0320, 0x0120, 0x0020, 0x0020, 0xFFE0,

    0xFFE0, 0x8000, 0x8000, 0x8000, 0x8000, 0x8000, 0x8000, 0x8000,
    0x8000, 0x8000, 0x8000, 0x8000, 0x8000, 0x8000, 0x8000, 0x0000,
};

UWORD ArrowLeft1Data[] =
{
    0xFFE0, 0x8000, 0x8000, 0x8100, 0x8300, 0x8600, 0x8E00, 0x9C00,
    0x9C00, 0x8E00, 0x8600, 0x8300, 0x8100, 0x8000, 0x8000, 0x0000,

    0x0000, 0x0020, 0x0020, 0x0020, 0x0020, 0x0020, 0x0020, 0x0020,
    0x0020, 0x0020, 0x0020, 0x0020, 0x0020, 0x0020, 0x0020, 0xFFE0,
};

#define ARROWRIGHT_WIDTH    11
#define ARROWRIGHT_HEIGHT   16

UWORD ArrowRight0Data[] =
{
    0x0000, 0x0020, 0x0020, 0x1020, 0x1820, 0x0C20, 0x0E20, 0x0720,
    0x0720, 0x0E20, 0x0C20, 0x1820, 0x1020, 0x0020, 0x0020, 0xFFE0,

    0xFFE0, 0x8000, 0x8000, 0x8000, 0x8000, 0x8000, 0x8000, 0x8000,
    0x8000, 0x8000, 0x8000, 0x8000, 0x8000, 0x8000, 0x8000, 0x0000,
};

UWORD ArrowRight1Data[] =
{
    0xFFE0, 0x8000, 0x8000, 0x9000, 0x9800, 0x8C00, 0x8E00, 0x8700,
    0x8700, 0x8E00, 0x8C00, 0x9800, 0x9000, 0x8000, 0x8000, 0x0000,

    0x0000, 0x0020, 0x0020, 0x0020, 0x0020, 0x0020, 0x0020, 0x0020,
    0x0020, 0x0020, 0x0020, 0x0020, 0x0020, 0x0020, 0x0020, 0xFFE0,
};

#endif


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

#undef IntuitionBase
#define IntuitionBase	((struct IntuitionBase *)(cl->cl_UserData))

/* Our imageclass dispatcher.
 */
AROS_UFH3(static IPTR, dispatch_imageclass,
    AROS_UFHA(Class *,  cl,  A0),
    AROS_UFHA(Object *, o,   A2),
    AROS_UFHA(Msg,      msg, A1)
)
{
    IPTR retval = 0UL;

    switch (msg->MethodID)
    {
    case OM_NEW:
    {
	D(kprintf("ImageClass OM_NEW\n"));

	/*
	    We are paranoid, and will check validity of objects.
	    I believe the AmigaOS, at least at some level, allows NULL.
	*/

	if (o)
	{
	    if (!cl)
		cl = OCLASS(o);

	    if (cl)
	    {
		D( kprintf("ImageClass Instance Size; %ld bytes\n", SIZEOF_INSTANCE(cl)) );

		retval = (IPTR)DoSuperMethodA(cl, o, msg);

		if(retval)
		{
		    /*
			This is how Intuition knows an image is a boopsi
			object!
		    */
		    memset ((void *)retval, 0, (cl->cl_InstOffset + cl->cl_InstSize));
		    IM(retval)->Depth = CUSTOMIMAGEDEPTH;
		}
	    }
	    D( else { kprintf("Class pointer is NULL\n") } );
	}
	D( else { kprintf("Carrier Object pointer is NULL\n") } );

	o = (Object *)retval;
	/*
	    Fall through -> allow the class the set all the initial
	    attributes
	*/
    }

    case OM_SET:
	D( kprintf("ImageClass OM_SET\n") );

	if (o)
	{
	    struct TagItem *tstate = ((struct opSet *)msg)->ops_AttrList;
	    struct TagItem *tag;
	    IPTR tidata;
	    BOOL unsupported;

	    unsupported = FALSE;

	    while ((tag = NextTagItem(&tstate)))
	    {
		tidata = tag->ti_Data;

		switch (tag->ti_Tag)
		{
		case IA_Left:
		    IM(o)->LeftEdge = (WORD) tidata;
		    break;

		case IA_Top:
		    IM(o)->TopEdge = (WORD) tidata;
		    break;

		case IA_Width:
		    IM(o)->Width = (WORD) tidata;
		    break;

		case IA_Height:
		    IM(o)->Height = (WORD) tidata;
		    break;

		case IA_FGPen:
		    IM(o)->PlanePick = (WORD) tidata;
		    break;

		case IA_BGPen:
		    IM(o)->PlaneOnOff = (WORD) tidata;
		    break;

		case IA_Data:
		    IM(o)->ImageData = (UWORD *) tidata;
		    break;

#if 0 /* This doesn't belong here, but don't loose it */
		case SYSIA_Which:
		    switch (tidata)
		    {
		    case DEPTHIMAGE:
		    case ZOOMIMAGE:
		    case SIZEIMAGE:
		    case CLOSEIMAGE:
		    case SDEPTHIMAGE:
			IM(o)->ImageData = NULL;
			unsupported = TRUE;
			break;

		    case LEFTIMAGE:
			IM(o)->ImageData = ArrowLeft0Data;
			IM(o)->Width     = ARROWLEFT_WIDTH;
			IM(o)->Height    = ARROWLEFT_HEIGHT;
			break;

		    case UPIMAGE:
			IM(o)->ImageData = ArrowUp0Data;
			IM(o)->Width     = ARROWUP_WIDTH;
			IM(o)->Height    = ARROWUP_HEIGHT;
			break;

		    case RIGHTIMAGE:
			IM(o)->ImageData = ArrowRight0Data;
			IM(o)->Width     = ARROWRIGHT_WIDTH;
			IM(o)->Height    = ARROWRIGHT_HEIGHT;
			break;

		    case DOWNIMAGE:
			IM(o)->ImageData = ArrowDown0Data;
			IM(o)->Width     = ARROWDOWN_WIDTH;
			IM(o)->Height    = ARROWDOWN_HEIGHT;
			break;

		    case CHECKIMAGE:
		    case MXIMAGE:
		    case MENUCHECK:
		    case AMIGAKEY:
			IM(o)->ImageData = NULL;
			unsupported = TRUE;
			break;

		    } /* Which image ? */

		    break;
#endif

		default:
		    unsupported = TRUE;
		    break;

		} /* switch (Tag) */
	    } /* while (Tag) */

	    /*
		If all attributes were supported and there is no retval yet,
		set retval to 1.
	    */
	    if (!unsupported && !retval)
		retval = 1UL;
	    /*
		Because we are a direct subclass of rootclass
		which has no settable/gettable attributes we
		we will NOT pass this method to our superclass!
	    */
	}
	D( else { kprintf("Object pointer is NULL\n") } );
	break;

    case OM_GET:
	D( kprintf("ImageClass OM_GET\n") );

	if (o)
	{
	    retval = 1UL;

	    switch (((struct opGet *)msg)->opg_AttrID)
	    {
	    case IA_Left:
		*((struct opGet *)msg)->opg_Storage = (IPTR) IM(o)->LeftEdge;
		break;

	    case IA_Top:
		*((struct opGet *)msg)->opg_Storage = (IPTR) IM(o)->TopEdge;
		break;

	    case IA_Width:
		*((struct opGet *)msg)->opg_Storage = (IPTR) IM(o)->Width;
		break;

	    case IA_Height:
		*((struct opGet *)msg)->opg_Storage = (IPTR) IM(o)->Height;
		break;

	    case IA_FGPen:
		*((struct opGet *)msg)->opg_Storage = (IPTR) IM(o)->PlanePick;
		break;

	    case IA_BGPen:
		*((struct opGet *)msg)->opg_Storage = (IPTR) IM(o)->PlaneOnOff;
		break;

	    case IA_Data:
		*((struct opGet *)msg)->opg_Storage = (IPTR) IM(o)->ImageData;
		break;

	    default:
		retval = 0UL;
		break;

	    } /* switch */

	    /*
		Because we are a direct subclass of rootclass
		which has no settable/gettable attributes we
		we will NOT pass this method to our superclass!
	    */
	}
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

	    /* Leave retval=0: No further rendering necessary */
	} /* if */
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
		retval = 1UL;
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

#undef IntuitionBase

/****************************************************************************/

/* Initialize our image class. */
struct IClass *InitImageClass (struct IntuitionBase * IntuitionBase)
{
    struct IClass *cl = NULL;

    /* This is the code to make the image class...
	*/
    if ((cl = MakeClass(IMAGECLASS, ROOTCLASS, NULL, sizeof(struct Image), 0)))
    {
	cl->cl_Dispatcher.h_Entry    = (APTR)AROS_ASMSYMNAME(dispatch_imageclass);
	cl->cl_Dispatcher.h_SubEntry = NULL;
	cl->cl_UserData 	     = (IPTR)IntuitionBase;

	AddClass (cl);
    }

    return (cl);
}

