/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: AROS frameiclass implementation
    Lang: english

    Original version 10/24/96 by caldi@usa.nai.net
*/

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

#include <proto/exec.h>
#include <proto/intuition.h>
#include <proto/graphics.h>
#include <proto/utility.h>

#ifdef __AROS__
#include "intuition_intern.h"
#include "maybe_boopsi.h"
#include <aros/asmcall.h>
#include <proto/alib.h>
#include "gadgets.h"
#endif

/****************************************************************************/

/* Some handy transparent base class object casting defines.
 */
#define G(o)  ((struct Gadget *)o)
#define EG(o) ((struct ExtGadget *)o)
#define IM(o) ((struct Image *)o)

/****************************************************************************/

/* FrameIClass specific instance data.
 */
struct FrameIData
{
    /* render bevel only with no fill? */
    BOOL fid_EdgesOnly;

    /* inverted bevel pens? */
    BOOL fid_Recessed;

    /* frame style? */
    WORD fid_FrameType;
    
    WORD fid_HOffset;
    WORD fid_VOffset;
};

/****************************************************************************/

#undef IntuitionBase
#define IntuitionBase	((struct IntuitionBase *)(cl->cl_UserData))

/* This is utility function used by frameiclass to draw a simple
 * bevel.
 */
static void DrawFrame(
	Class * cl,
	struct RastPort *rport,
	UWORD shine, UWORD shadow,
	WORD left, WORD top, WORD width, WORD height,
	BOOL thicken)
{
    /*
	Here we attempt to render a bevel as quickly as possible using
	as few system calls as possible. Note the ORDER of the rendering
	and pen (or lack of) setting in important. This routine can be
	useful alone as a basis for GadTools DrawBevelBox() perhaps, but
	its real function is rendering the frameiclass components for the
	various supported styles.

	It should be futher noted, on the native Amiga chipset, rendering
	bevels via serious of RectFill()'s is potentially as much as two
	times faster. However, in the case of AROS the implementation
	of the graphics drivers would ofcourse be the determining factor.
	Just as on the native OS, 3rd party gfx boards would be a factor.

	Additionally, if the frame metrics are changed here for whatever
	reasons, you MUST also compensate the change in the frame class
	render method, and framebox specifically the offset values.
    */
    height -= 1;
    width  -= 1;

    /* Top/Left */
    SetABPenDrMd(rport, shine, 0, JAM1);
    Move(rport, left, top + height);
    Draw(rport, left, top);
    Draw(rport, left + width, top);

    /* Bottom/Right */
    SetAPen(rport, shadow);
    Draw(rport, left + width, top + height);
    Draw(rport, left + 1, top + height);

    if (FRAME_SIZE == 1)
    {
	if (thicken != FALSE)
	{
	    /* Thicken Right Side */
	    Move(rport, left + width - 1, top + height - 1);
	    Draw(rport, left + width - 1, top + 1);

	    /* Thicken Left Side */
	    SetAPen(rport, shine);
	    Move(rport, left + 1, top + height - 1);
	    Draw(rport, left + 1, top + 1);

	} /* if */
    }
    else if (FRAME_SIZE == 2)
    {
	if (thicken != FALSE)
	{
	    /* Thicken Right Side */
	    Move(rport, left + width - 1, top + 1);
	    Draw(rport, left + width - 1, top + height - 1);

	    /* Thicken Bottom Side */
	    Draw(rport, left + 2, top + height - 1);

	    /* Thicken Left Side */
	    SetAPen(rport, shine);

	    Move(rport, left + 1, top + height - 1);
	    Draw(rport, left + 1, top + 1);

	    /* Thicken Top Side */	
	    Draw(rport, left + width - 2, top + 1);
	} /* if */
    }

} /* DrawFrame */

/****************************************************************************/

/* frame render method */
static ULONG draw_frameiclass(Class *cl, Object *o, struct impDraw *msg)
{
    struct FrameIData *fid = INST_DATA(cl, o);

    /*
	Default pen array, this should be globally accessable from
	all the boopsi objects, unless someone as a better idea...
    */
    UWORD default_pens[] =
    {
	1, /* detailPen      */
	0, /* blockPen	     */
	1, /* textPen	     */
	2, /* shinePen	     */
	1, /* shadowPen      */
	3, /* hiFillPen      */
	1, /* hifilltextPen  */
	0, /* backgroundPen  */
	2  /* hilighttextPen */
    };
    ULONG retval;

    /* we will check the rastport present */
    if(msg->imp_RPort != NULL)
    {
	UWORD *pens = default_pens;
	UWORD left, top;
	UWORD shine, shadow;
	BOOL  selected;

	/* set up our rendering pens */
	if (msg->imp_DrInfo)
	{
	    pens = msg->imp_DrInfo->dri_Pens;

	    /*
		Fall back to mono color bevels if depth is only 1 bitplane.
	    */
	    if (msg->imp_DrInfo->dri_Depth == 1)
	    {
		shine  = pens[SHADOWPEN];
	    }
	    else
	    {
		shine  = pens[SHINEPEN];
	    } /* if */
	}
	else
	{
	    shine  = pens[SHINEPEN];
	} /* if */

	shadow	= pens[SHADOWPEN];

	switch(msg->imp_State)
	{
	    case IDS_SELECTED:
	    case IDS_INACTIVESELECTED:
		selected = TRUE;
		break;

	    default:
		selected = FALSE;
		break;
	} /* switch */

	/*
	    I'm so clever :) We want to check if either of selected or
	    recessed is TRUE, and if so, swap the pens. However, if both
	    are true, they effectivly cancel each other out and we do
	    nothing. Rather than two compares against TRUE and a OR of the
	    results, pls the additional check to ignore the case where both
	    are TRUE, we will do an XOR of the bool's and check the result.
	    This should prove most efficient too.


	    Recess|select| XOR'd result
	    ------|------|-------
	      0   |  0	 |  0
	    ------|------|-------
	      0   |  1	 |  1
	    ------|------|-------
	      1   |  0	 |  1
	    ------|------|-------
	      1   |  1	 |  0
	    ------|------|-------
	*/

	if ( (fid->fid_Recessed ^ selected) != FALSE )
	{
		/* swap pens */
		shine  ^= shadow;
		shadow ^= shine;
		shine  ^= shadow;
	} /* if */

	left = IM(o)->LeftEdge + msg->imp_Offset.X;
	top  = IM(o)->TopEdge  + msg->imp_Offset.Y;

	switch(fid->fid_FrameType)
	{
	    case FRAME_DEFAULT:
		DrawFrame(
		    cl,
		    msg->imp_RPort,
		    shine, shadow,
		    left, top,
		    IM(o)->Width, IM(o)->Height,
		    FALSE
		);
		break;

	    case FRAME_BUTTON:
		DrawFrame(
		    cl,
		    msg->imp_RPort,
		    shine, shadow,
		    left, top,
		    IM(o)->Width, IM(o)->Height,
		    TRUE
		);
		break;

	    case FRAME_RIDGE:
		/* render outer pen-inverted thick bevel */
		DrawFrame(
		    cl,
		    msg->imp_RPort,
		    shine, shadow,
		    left, top,
		    IM(o)->Width, IM(o)->Height,
		    TRUE
		);

		/* render inner thick bevel */
		DrawFrame(
		    cl,
		    msg->imp_RPort,
		    shadow, shine,
		    left + fid->fid_HOffset / 2, top + fid->fid_VOffset / 2,
		    IM(o)->Width - fid->fid_HOffset, IM(o)->Height - fid->fid_VOffset,
		    TRUE
		);
		break;

	    case FRAME_ICONDROPBOX: {
		WORD hoffset = fid->fid_HOffset * 2 / 3;
		WORD voffset = fid->fid_VOffset * 2 / 3;

		/* render outer pen-inverted thick bevel */
		DrawFrame(
		    cl,
		    msg->imp_RPort,
		    shine, shadow,
		    left, top,
		    IM(o)->Width, IM(o)->Height,
		    TRUE
		);

		/* render inner thick bevel */
		DrawFrame(
		    cl,
		    msg->imp_RPort,
		    shadow, shine,
		    left + hoffset, top + voffset,
		    IM(o)->Width - hoffset * 2, IM(o)->Height - voffset * 2,
		    TRUE
		);
		break; }
		
	} /* switch */

	if(fid->fid_EdgesOnly == FALSE)
	{
	    if(selected)
	    {
		SetABPenDrMd(msg->imp_RPort, pens[FILLPEN], pens[BACKGROUNDPEN], JAM1);
	    }
	    else
	    {
		SetABPenDrMd(msg->imp_RPort, pens[BACKGROUNDPEN], pens[BACKGROUNDPEN], JAM1);
	    } /* if */

	    RectFill(msg->imp_RPort,
		left + fid->fid_HOffset,
		top  + fid->fid_VOffset,
		left + IM(o)->Width  - fid->fid_HOffset - 1,
		top  + IM(o)->Height - fid->fid_VOffset - 1);

	} /* if */

	switch(msg->imp_State)
	{
	    case IDS_DISABLED:
	    case IDS_INACTIVEDISABLED:
	    case IDS_SELECTEDDISABLED:
	        RenderDisabledPattern(msg->imp_RPort,
				      msg->imp_DrInfo,
				      left,
				      top,
				      left + IM(o)->Width - 1,
				      top + IM(o)->Height - 1,
				      IntuitionBase);
	        break;
	}

	retval = 1UL;
    }
    else
    {
	/* return failure */
	retval = 0UL;
    } /* if */

    return retval;
} /* draw_frameiclass */


/****************************************************************************/

/* frame attribute setting method */
static ULONG set_frameiclass(Class *cl, Object *o, struct opSet *msg)
{
    struct FrameIData 	*fid = INST_DATA(cl, o);

    struct TagItem 	*tstate = msg->ops_AttrList;
    struct TagItem 	*tag;
    ULONG 		retval = 0UL;

    while ((tag = NextTagItem(&tstate)))
    {
	switch(tag->ti_Tag)
	{
	case IA_Recessed:
	    fid->fid_Recessed	= (BOOL)( ( (BOOL)tag->ti_Data != FALSE ) ? TRUE : FALSE );
	    break;

	case IA_EdgesOnly:
	    fid->fid_EdgesOnly	= (BOOL)( ( (BOOL)tag->ti_Data != FALSE ) ? TRUE : FALSE );
	    break;

	case IA_FrameType:
	    /*
		Data values for IA_FrameType (recognized by FrameIClass)

		FRAME_DEFAULT:	The standard V37-type frame, which has
			thin edges.
		FRAME_BUTTON:  Standard button gadget frames, having thicker
			sides and edged corners.
		FRAME_RIDGE:  A ridge such as used by standard string gadgets.
			You can recess the ridge to get a groove image.
		FRAME_ICONDROPBOX: A broad ridge which is the standard imagery
			for areas in AppWindows where icons may be dropped.
	    */
	    fid->fid_FrameType = (WORD)tag->ti_Data;

	    switch(fid->fid_FrameType)
	    {
	        case FRAME_DEFAULT:
		    fid->fid_HOffset = fid->fid_VOffset = 1;
		    break;

		case FRAME_BUTTON:
		    fid->fid_HOffset = 1;
		    fid->fid_VOffset = 1;
		    break;

		case FRAME_RIDGE:
		    fid->fid_HOffset = 2;
		    fid->fid_VOffset = 2;
		    break;

		case FRAME_ICONDROPBOX:
		    fid->fid_HOffset = 3;
		    fid->fid_VOffset = 3;
		    break;

	    } /* switch(fid->fid_FrameType) */

	    if (FRAME_SIZE > 0)
	    {
		fid->fid_HOffset *= 2;
	    }
	    if (FRAME_SIZE == 2)
	    {
		fid->fid_VOffset *= 2;
	    }
	    break;

	} /* switch */
    } /* while */

    return(retval);
} /* set_frameiclass */

/****************************************************************************/

/* frameiclass framebox method */
static ULONG framebox_frameiclass(Class *cl, Object *o, struct impFrameBox *msg)
{
    struct FrameIData *fid = INST_DATA(cl, o);

    /* stegerg: the RKRM docs seem to be wrong. source box (around which
       the frame goes) is imp_ContentsBox and dest box filled in by
       this method is imp_FrameBox. */ 
       
    if (msg->imp_FrameFlags & FRAMEF_SPECIFY)
    {
	msg->imp_FrameBox->Left   = msg->imp_ContentsBox->Left +
				    (msg->imp_ContentsBox->Width - msg->imp_FrameBox->Width) / 2;
	msg->imp_FrameBox->Top    = msg->imp_ContentsBox->Top +
				    (msg->imp_ContentsBox->Height - msg->imp_FrameBox->Height) / 2;
    }
    else
    {
	msg->imp_FrameBox->Left   = msg->imp_ContentsBox->Left   - fid->fid_HOffset;
	msg->imp_FrameBox->Top    = msg->imp_ContentsBox->Top    - fid->fid_VOffset;
	msg->imp_FrameBox->Width  = msg->imp_ContentsBox->Width  + fid->fid_HOffset * 2;
	msg->imp_FrameBox->Height = msg->imp_ContentsBox->Height + fid->fid_VOffset * 2;
    } /* if */

    return 1UL;
} /* framebox_frameiclass */

/****************************************************************************/

/* frameiclass boopsi dispatcher
 */
AROS_UFH3S(IPTR, dispatch_frameiclass,
    AROS_UFHA(Class *,  cl,  A0),
    AROS_UFHA(Object *, o,   A2),
    AROS_UFHA(Msg,      msg, A1)
)
{
    AROS_USERFUNC_INIT

    IPTR retval = 0UL;

    switch(msg->MethodID)
    {
    case IM_FRAMEBOX:
	retval = framebox_frameiclass(cl, o, (struct impFrameBox *)msg);
	break;

    case IM_DRAWFRAME:
	/* For this release, drawframe (wrongly) handled the same as... */

    case IM_DRAW:
	retval = draw_frameiclass(cl, o, (struct impDraw *)msg);
	break;

    case OM_SET:
	retval = DoSuperMethodA(cl, o, msg);
	retval += (IPTR)set_frameiclass(cl, o, (struct opSet *)msg);
	break;

    case OM_NEW:
	retval = DoSuperMethodA(cl, o, msg);
	if (retval)
	{
	    struct FrameIData *fid = INST_DATA(cl, retval);

	    /* set some defaults */
	    fid->fid_EdgesOnly = FALSE;
	    fid->fid_Recessed  = FALSE;
	    fid->fid_FrameType = FRAME_DEFAULT;
	    fid->fid_HOffset = 1;
	    fid->fid_VOffset = 1;
	    
	    /* Handle our special tags - overrides defaults */
	    set_frameiclass(cl, (Object*)retval, (struct opSet *)msg);
	}
	break;

    default:
	retval = DoSuperMethodA(cl, o, msg);
	break;
    } /* switch */

    return retval;

    AROS_USERFUNC_EXIT
}  /* dispatch_frameiclass */

#undef IntuitionBase

/****************************************************************************/

/* Initialize our image class. */
struct IClass *InitFrameIClass (struct IntuitionBase * IntuitionBase)
{
    struct IClass *cl = NULL;

    /* This is the code to make the frameiclass...
	*/
    if ((cl = MakeClass(FRAMEICLASS, IMAGECLASS, NULL, sizeof(struct FrameIData), 0)))
    {
	cl->cl_Dispatcher.h_Entry    = (APTR)AROS_ASMSYMNAME(dispatch_frameiclass);
	cl->cl_Dispatcher.h_SubEntry = NULL;
	cl->cl_UserData 	     = (IPTR)IntuitionBase;

	AddClass (cl);
    }

    return (cl);
}

