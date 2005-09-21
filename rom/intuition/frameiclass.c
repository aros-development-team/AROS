/*
    Copyright © 1995-2005, The AROS Development Team. All rights reserved.
    Copyright © 2001-2003, The MorphOS Development Team. All Rights Reserved.
    $Id$
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

#ifndef __MORPHOS__
#include "intuition_intern.h"
#include <aros/asmcall.h>
#include <proto/alib.h>
#include "gadgets.h"
#endif /* !__MORPHOS__ */

#define DEBUG_IFRAME(x) ;

/****************************************************************************/

/* Some handy transparent base class object casting defines.
 */
#define IM(o) ((struct Image *)o)

/****************************************************************************/

#undef IntuitionBase
#define IntuitionBase   ((struct IntuitionBase *)(cl->cl_UserData))

/****************************************************************************/

/* This is utility function used by frameiclass to draw a simple
 * bevel.
 */
static void DrawFrame(Class * cl, struct RastPort *rport, UWORD shine, UWORD shadow,
    	    	      WORD left, WORD top, WORD width, WORD height, BOOL thicken)
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

    DEBUG_IFRAME(dprintf("drawframe: width %ld height %ld\n",width,height));

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

    if (thicken != FALSE)
    {
        if (FRAME_SIZE == 1)
        {
            /* Thicken Right Side */
            Move(rport, left + width - 1, top + height - 1);
            Draw(rport, left + width - 1, top + 1);
    
            /* Thicken Left Side */
            SetAPen(rport, shine);
            Move(rport, left + 1, top + height - 1);
            Draw(rport, left + 1, top + 1);
        }
        else if (FRAME_SIZE == 2)
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
        }
	
    } /* if */
    
} /* DrawFrame */

/****************************************************************************/

/* frame attribute setting method */
static ULONG set_frameiclass(Class *cl, Object *o, struct opSet *msg)
{
    struct FrameIData   *fid = INST_DATA(cl, o);

    struct TagItem  	*tstate = msg->ops_AttrList;
    struct TagItem  	*tag;
    ULONG            	 retval = 0UL;

    while ((tag = NextTagItem((const struct TagItem **)&tstate)))
    {
        switch(tag->ti_Tag)
        {
            case IA_Recessed:
        	fid->fid_Recessed   = (BOOL)( ( tag->ti_Data != FALSE ) ? TRUE : FALSE );
        	break;

            case IA_EdgesOnly:
        	fid->fid_EdgesOnly  = (BOOL)( ( tag->ti_Data != FALSE ) ? TRUE : FALSE );
        	break;

            case IA_FrameType:
        	/*
        	Data values for IA_FrameType (recognized by FrameIClass)

        	FRAME_DEFAULT:  The standard V37-type frame, which has
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
                	DEBUG_IFRAME(dprintf("draw_frameiclass: FRAME_BUTTON\n"));
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

/* frame render method */
static IPTR draw_frameiclass(Class *cl, struct Image *im, struct impDraw *msg, WORD width, WORD height)
{
    struct FrameIData *fid = INST_DATA(cl, (Object *)im);

    /*
    Default pen array, this should be globally accessable from
    all the boopsi objects, unless someone as a better idea...
    */
    UWORD default_pens[] =
    {
        1, /* detailPen      */
        0, /* blockPen       */
        1, /* textPen        */
        2, /* shinePen       */
        1, /* shadowPen      */
        3, /* hiFillPen      */
        1, /* hifilltextPen  */
        0, /* backgroundPen  */
        2  /* hilighttextPen */
    };
    ULONG retval;

    DEBUG_IFRAME(dprintf("draw_frameiclass: width %ld height %ld\n",width,height));

    /* we will check the rastport present */    
    if(msg->imp_RPort)
    {
        UWORD 	    	*pens = default_pens;
        UWORD 	    	 left, top;
        UWORD 	    	 shine, shadow;
        BOOL 	    	 selected;
        struct RastPort *rp = msg->imp_RPort;
        BYTE	    	 oldapen = rp->FgPen;
        BYTE	    	 oldbpen = rp->BgPen;
        BYTE	    	 olddrmd = rp->DrawMode;

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

        shadow = pens[SHADOWPEN];

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
              0   |  0   |  0
            ------|------|-------
              0   |  1   |  1
            ------|------|-------
              1   |  0   |  1
            ------|------|-------
              1   |  1   |  0
            ------|------|-------
        */

        if ( (fid->fid_Recessed ^ selected) != FALSE )
        {
            /* swap pens */
            UWORD tmp;
	    
            tmp = shine;
            shine = shadow;
            shadow = tmp;
        } /* if */

        left = im->LeftEdge + msg->imp_Offset.X;
        top  = im->TopEdge  + msg->imp_Offset.Y;

        DEBUG_IFRAME(dprintf("draw_frameiclass: type %ld height %ld\n",fid->fid_FrameType));

        switch(fid->fid_FrameType)
        {
            case FRAME_DEFAULT:
        	DrawFrame(cl, rp, shine, shadow, left, top, width, height, FALSE);
        	break;

            case FRAME_BUTTON:
        	DrawFrame(cl, rp, shine, shadow, left, top, width, height, TRUE);
        	break;

            case FRAME_RIDGE:
        	/* render outer pen-inverted thick bevel */
        	DrawFrame(cl, rp, shine, shadow, left, top, width, height, TRUE);

        	/* render inner thick bevel */
        	DrawFrame(cl, rp, shadow, shine,
                    	  left + fid->fid_HOffset / 2, top + fid->fid_VOffset / 2,
                    	  width - fid->fid_HOffset, height - fid->fid_VOffset,
                    	  TRUE);
        	break;

            case FRAME_ICONDROPBOX:
            {
                WORD hoffset = fid->fid_HOffset * 2 / 3;
                WORD voffset = fid->fid_VOffset * 2 / 3;

                /* render outer pen-inverted thick bevel */
                DrawFrame(cl, rp, shine, shadow, left, top, width, height, TRUE);

                /* render inner thick bevel */
                DrawFrame(cl, rp, shadow, shine, left + hoffset, top + voffset,
                    	 width - hoffset * 2, height - voffset * 2, TRUE);
                break;
            }

        } /* switch */

        if(fid->fid_EdgesOnly == FALSE)
        {
            if(selected)
            {
                SetABPenDrMd(rp, pens[FILLPEN], pens[BACKGROUNDPEN], JAM1);
            }
            else
            {
                SetABPenDrMd(rp, pens[BACKGROUNDPEN], pens[BACKGROUNDPEN], JAM1);
            } /* if */

            RectFill(rp,
                     left + fid->fid_HOffset,
                     top  + fid->fid_VOffset,
                     left + width  - fid->fid_HOffset - 1,
                     top  + height - fid->fid_VOffset - 1);

        } /* if */

        switch(msg->imp_State)
        {
            case IDS_DISABLED:
            case IDS_INACTIVEDISABLED:
            case IDS_SELECTEDDISABLED:
        	RenderDisabledPattern(rp,
                                      msg->imp_DrInfo,
                                      left,
                                      top,
                                      left + width - 1,
                                      top + height - 1,
                                      IntuitionBase);
        	break;
        }

        SetABPenDrMd(rp, oldapen, oldbpen, olddrmd);

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

/* frameiclass framebox method */
IPTR FrameIClass__IM_FRAMEBOX(Class *cl, Object *o, struct impFrameBox *msg)
{
    struct FrameIData *fid = INST_DATA(cl, o);

    /* stegerg: the RKRM docs seem to be wrong. source box (around which
       the frame goes) is imp_ContentsBox and dest box filled in by
       this method is imp_FrameBox. */

    DEBUG_IFRAME(dprintf("framebox_frameiclass: Contents Left %ld Top %ld Width %ld Height %ld\n",
                         msg->imp_ContentsBox->Left,
                         msg->imp_ContentsBox->Top,
                         msg->imp_ContentsBox->Width,
                         msg->imp_ContentsBox->Height));

    DEBUG_IFRAME(dprintf("framebox_frameiclass: Flags 0x%lx\n",
                         msg->imp_FrameFlags));

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

    DEBUG_IFRAME(dprintf("framebox_frameiclass: Left %ld Top %ld Width %ld Height %ld\n",
                         msg->imp_FrameBox->Left,
                         msg->imp_FrameBox->Top,
                         msg->imp_FrameBox->Width,
                         msg->imp_FrameBox->Height));

    return (IPTR)1;
} /* framebox_frameiclass */

IPTR FrameIClass__OM_SET(Class *cl, Object *o, struct opSet *msg)
{
    DEBUG_IFRAME(dprintf("dispatch_frameiclass: set\n"));
    return DoSuperMethodA(cl, o, (Msg)msg) + (IPTR)set_frameiclass(cl, o, msg);
}

IPTR FrameIClass__OM_NEW(Class *cl, Object *o, struct opSet *msg)
{
    DEBUG_IFRAME(dprintf("dispatch_frameiclass: new\n"));

    o = (Object *)DoSuperMethodA(cl, o, (Msg)msg);
    if (o)
    {
	struct FrameIData *fid = INST_DATA(cl, o);

	/* set some defaults */
	fid->fid_EdgesOnly = FALSE;
	fid->fid_Recessed  = FALSE;
	fid->fid_FrameType = FRAME_DEFAULT;
	fid->fid_HOffset   = 1;
	fid->fid_VOffset   = 1;
 
	/* Handle our special tags - overrides defaults */
	set_frameiclass(cl, o, msg);
    }
    
    return (IPTR)o;
}

IPTR FrameIClass__IM_DRAWFRAME(Class *cl, struct Image *im, struct impDraw *msg)
{
    DEBUG_IFRAME(dprintf("dispatch_frameiclass: drawframe Width %ld Height %ld\n", msg->imp_Dimensions.Width, msg->imp_Dimensions.Height));
    return draw_frameiclass(cl, im, msg, msg->imp_Dimensions.Width, msg->imp_Dimensions.Height);
}

IPTR FrameIClass__IM_DRAW(Class *cl, struct Image *im, struct impDraw *msg)
{
    DEBUG_IFRAME(dprintf("dispatch_frameiclass: draw Width %ld Height %ld\n",im->Width, im->Height));
    return draw_frameiclass(cl, im, msg, im->Width, im->Height);
}
