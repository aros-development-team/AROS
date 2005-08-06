/*
    Copyright © 1995-2003, The AROS Development Team. All rights reserved.
    Copyright © 2001-2003, The MorphOS Development Team. All Rights Reserved.
    $Id: imageclass.c 20651 2004-01-17 20:57:12Z chodorowski $
*/


#include <dos/dos.h>
#include <dos/dosextens.h>

#include <intuition/intuition.h>
#include <intuition/intuitionbase.h>
#include <intuition/classes.h>
#include <intuition/classusr.h>
#include <intuition/scrdecorclass.h>
#include <intuition/cghooks.h>
#include <intuition/icclass.h>
#include <intuition/extensions.h>

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

#include <proto/alib.h>

#include "intuition_intern.h"
#include "gadgets.h"

/**************************************************************************************************/

#ifdef __AROS__
#define USE_AROS_DEFSIZE 1
#else
#define USE_AROS_DEFSIZE 0
#endif

#define DEFSIZE_WIDTH  14
#define DEFSIZE_HEIGHT 14

#define HSPACING 3
#define VSPACING 3
/* Ralph Schmidt
 * heuristics for smaller arrows used in apps
 * like filer
 */
#define HSPACING_MIDDLE 2
#define VSPACING_MIDDLE 2
#define HSPACING_SMALL 1
#define VSPACING_SMALL 1

#define DRI(dri) ((struct DrawInfo *)(dri))

/**************************************************************************************************/

struct scrdecor_data
{
    struct IntDrawInfo  *dri;
    struct Screen   	*scr;
};

/**************************************************************************************************/

static void renderimageframe(struct RastPort *rp, ULONG which, ULONG state, UWORD *pens,
                             WORD left, WORD top, WORD width, WORD height,
                             struct IntuitionBase *IntuitionBase)
{
    WORD right = left + width - 1;
    WORD bottom = top + height - 1;
    BOOL leftedgegodown = FALSE;
    BOOL topedgegoright = FALSE;

    if (left == 0) leftedgegodown = TRUE;
    if (top == 0) topedgegoright = TRUE;

    SetAPen(rp, pens[((state == IDS_SELECTED) || (state == IDS_INACTIVESELECTED)) ? SHADOWPEN : SHINEPEN]);

    /* left edge */
    RectFill(rp, left,
             top,
             left,
             bottom - (leftedgegodown ? 0 : 1));

    /* top edge */
    RectFill(rp, left + 1,
             top,
             right - (topedgegoright ? 0 : 1),
             top);

    SetAPen(rp, pens[((state == IDS_SELECTED) || (state == IDS_INACTIVESELECTED)) ? SHINEPEN : SHADOWPEN]);

    /* right edge */
    RectFill(rp, right,
             top + (topedgegoright ? 1 : 0),
             right,
             bottom);

    /* bottom edge */
    RectFill(rp, left + (leftedgegodown ? 1 : 0),
             bottom,
             right - 1,
             bottom);
}

/**************************************************************************************************/

static UWORD getbgpen(ULONG state, UWORD *pens)
{
    UWORD bg;

    switch (state)
    {
	case IDS_NORMAL:
	case IDS_SELECTED:
            bg = pens[FILLPEN];
            break;

	default:
            bg = pens[BACKGROUNDPEN];
            break;
    }
    
    return bg;
}


/**************************************************************************************************/

#undef IntuitionBase
#define IntuitionBase   ((struct IntuitionBase *)(cl->cl_UserData))

/**************************************************************************************************/

static IPTR scrdecor_new(Class *cl, Object *obj, struct opSet *msg)
{
    struct scrdecor_data *data;
    
    obj = (Object *)DoSuperMethodA(cl, obj, (Msg)msg);
    if (obj)
    {
    	data = INST_DATA(cl, obj);
	
	data->dri = (struct IntDrawInfo *)GetTagData(SDA_DrawInfo, 0, msg->ops_AttrList);
	data->scr = (struct Screen *)GetTagData(SDA_Screen, 0, msg->ops_AttrList);
	
	if (!data->dri || !data->scr)
	{
    	    STACKULONG method = OM_DISPOSE;
	    
    	    CoerceMethodA(cl, obj, (Msg)&method);
	    
	    return 0;
	}	
	
    }
    
    return (IPTR)obj;
}

/**************************************************************************************************/

static IPTR scrdecor_get(Class *cl, Object *obj, struct opGet *msg)
{
    struct scrdecor_data *data = INST_DATA(cl, obj);

    switch(msg->opg_AttrID)
    {
    	case SDA_DrawInfo:
	    *msg->opg_Storage = (IPTR)data->dri;
	    break;
	    
    	case SDA_Screen:
	    *msg->opg_Storage = (IPTR)data->scr;
	    break;
	    
	case SDA_TrueColorOnly:
	    *msg->opg_Storage = FALSE;
	    break;
	    
	default:
	    return DoSuperMethodA(cl, obj, (Msg)msg);
    }
    
    return 1;    
}
    

/**************************************************************************************************/

IPTR scrdecor_getdefsize_sysimage(Class *cl, Object *obj, struct sdpGetDefSizeSysImage *msg)
{
    ULONG def_low_width = DEFSIZE_WIDTH, def_low_height = DEFSIZE_HEIGHT;
    ULONG def_med_width = DEFSIZE_WIDTH, def_med_height = DEFSIZE_HEIGHT;
    ULONG def_high_width = DEFSIZE_WIDTH, def_high_height = DEFSIZE_HEIGHT;
    
    switch(msg->sdp_Which)
    {
	case SDEPTHIMAGE:
	#if USE_AROS_DEFSIZE
    	    def_low_width = def_med_width = def_high_width = DEFSIZE_WIDTH;
	    def_low_height = def_med_height = def_high_height = DEFSIZE_HEIGHT;
	#else
            def_low_width = 17;
            def_med_width = 23;
            def_high_width = 23;
	#endif
            break;
	    
	default:
	    return FALSE;
    }
    
    switch(msg->sdp_SysiSize)
    {
    	case SYSISIZE_LOWRES:
	    *msg->sdp_Width = def_low_width;
	    *msg->sdp_Height = def_low_height;
	    break;

	case SYSISIZE_MEDRES:
	    *msg->sdp_Width = def_med_width;
	    *msg->sdp_Height = def_med_height;
	    break;
    	    
    	case SYSISIZE_HIRES:
    	default:
	    *msg->sdp_Width = def_high_width;
	    *msg->sdp_Height = def_high_height;
	    break;	    	
    }
    
    return TRUE;
}

/**************************************************************************************************/

IPTR scrdecor_draw_sysimage(Class *cl, Object *obj, struct sdpDrawSysImage *msg)
{
    struct scrdecor_data *data = INST_DATA(cl, obj);
    struct RastPort 	 *rp = msg->sdp_RPort;
    UWORD   	    	 *pens = DRI(data->dri)->dri_Pens;
    LONG    	    	  state = msg->sdp_State;
    LONG    	     	  left = msg->sdp_X;
    LONG    	     	  top = msg->sdp_Y;
    LONG   	          width = msg->sdp_Width;
    LONG   	    	  height = msg->sdp_Height;
    LONG    	    	  right = left + width - 1;
    LONG    	    	  bottom = top + height - 1;
    
    SetDrMd(rp, JAM1);
    
    switch(msg->sdp_Which)
    {
    	case SDEPTHIMAGE:
        {
            UWORD  bg;
            WORD   h_spacing;
            WORD   v_spacing;

            renderimageframe(rp, DEPTHIMAGE, state, pens,
                             left, top, width, height, IntuitionBase);
            left++;
            top++;
            right--;
            bottom--;
            width -= 2;
            height -= 2;

            h_spacing = width / 6;
            v_spacing = height / 6;

            bg = pens[BACKGROUNDPEN];
 
            /* Clear background into correct color */
            SetAPen(rp, bg);
            RectFill(rp, left, top, right, bottom);

            /* Draw a image of two partly overlapped tiny windows,
            */

            left += h_spacing;
            top  += v_spacing;

            width  -= h_spacing * 2;
            height -= v_spacing * 2;

            right  = left + width  - 1;
            bottom = top  + height - 1;

            /* Render top left window  */

            SetAPen(rp, pens[SHADOWPEN]);
            drawrect(rp, left, top, right - (width / 3 ), bottom - (height / 3), IntuitionBase);

            /* Render bottom right window  */
            SetAPen(rp, pens[SHADOWPEN]);
            drawrect(rp, left + (width / 3), top + (height / 3), right, bottom, IntuitionBase);

            /* Fill bottom right window (inside of the frame above) */
            SetAPen(rp, pens[SHINEPEN]);
            RectFill(rp, left + (width / 3) + 1, top + (height / 3) + 1,
	    	    	    right - 1, bottom - 1);


            if (state == IDS_SELECTED)
            {
                /* Re-Render top left window  */

                SetAPen(rp, pens[SHADOWPEN]);
                drawrect(rp, left, top, right - (width / 3 ), bottom - (height / 3), IntuitionBase);
            }
            break;
        }

	default:
	    return FALSE;
    }
    
    return TRUE;
}

/**************************************************************************************************/

static void findtitlearea(struct Screen *scr, LONG *left, LONG *right)
{
    struct Gadget *g;

    *left = 0;
    *right = scr->Width - 1;
    
    for (g = scr->FirstGadget; g; g = g->NextGadget)
    {
        if (!(g->Flags & GFLG_RELRIGHT))
        {
            if (g->LeftEdge + g->Width > *left)
                *left = g->LeftEdge + g->Width;
        }
        else
        {
            if (g->LeftEdge + scr->Width - 1 - 1 < *right)
                *right = g->LeftEdge + scr->Width - 1 - 1;
        }
    }
    
}

/**************************************************************************************************/

IPTR scrdecor_draw_screenbar(Class *cl, Object *obj, struct sdpDrawScreenBar *msg)
{
    struct scrdecor_data *data = INST_DATA(cl, obj);
    struct RastPort 	 *rp = msg->sdp_RPort;
    UWORD   	    	 *pens = DRI(data->dri)->dri_Pens;
    LONG    	    	  left, right;
    BOOL    	    	  beeping = FALSE;
    
#if 0
#if USE_NEWDISPLAYBEEP
        beeping = (scr->Flags & BEEPING) && GetBitMapAttr(rp->BitMap, BMA_DEPTH) > 8;
#endif
#endif

    SetDrMd(rp, JAM1);
    
    SetAPen(rp, pens[beeping ? BARDETAILPEN : BARBLOCKPEN]);
    RectFill(rp, 0, 0, data->scr->Width - 1, data->scr->BarHeight - 1);

    SetAPen(rp, pens[beeping ? BARDETAILPEN : BARTRIMPEN]);
    RectFill(rp, 0, data->scr->BarHeight, data->scr->Width - 1, data->scr->BarHeight);

    findtitlearea(data->scr, &left, &right);

    SetAPen(rp, pens[SHADOWPEN]);
    RectFill(rp, right, 1, right, data->scr->BarHeight - 1);
        
    return TRUE;
}

/**************************************************************************************************/

IPTR scrdecor_draw_screentitle(Class *cl, Object *obj, struct sdpDrawScreenTitle *msg)
{
    struct scrdecor_data *data = INST_DATA(cl, obj);
    struct RastPort 	 *rp = msg->sdp_RPort;
    UWORD   	    	 *pens = DRI(data->dri)->dri_Pens;
    LONG    	    	  right, left;
    BOOL    	    	  beeping = FALSE;
    
#if 0
#if USE_NEWDISPLAYBEEP
        beeping = (scr->Flags & BEEPING) && GetBitMapAttr(rp->BitMap, BMA_DEPTH) > 8;
#endif
#endif

    findtitlearea(data->scr, &left, &right);

    SetDrMd(rp, JAM1);

    SetAPen(rp, pens[beeping ? BARDETAILPEN : BARBLOCKPEN]);
    RectFill(rp, left + 1, 0, right - 1, data->scr->BarHeight - 1);

    SetAPen(rp, pens[beeping ? BARBLOCKPEN: BARDETAILPEN]);
    SetBPen(rp, pens[beeping ? BARDETAILPEN : BARBLOCKPEN]);

    Move(rp, data->scr->BarHBorder, data->scr->BarVBorder + rp->TxBaseline);

    Text(rp, data->scr->Title, strlen(data->scr->Title));
    
    return TRUE;
}

/**************************************************************************************************/

IPTR scrdecor_layout_screengadgets(Class *cl, Object *obj, struct sdpLayoutScreenGadgets *msg)
{
    struct Gadget *gadget = msg->sdp_Gadgets;

    while(gadget)
    {
    	switch(gadget->GadgetType & GTYP_SYSTYPEMASK)
	{
	    case GTYP_SDEPTH:
	    	gadget->LeftEdge = -gadget->Height + 1;
		gadget->Width = gadget->Height;
		gadget->Flags &= ~GFLG_RELWIDTH;
		gadget->Flags |= GFLG_RELRIGHT;
		break;
		
	}
	
	if (msg->sdp_Flags & SDF_LSG_MULTIPLE)
	{
	    gadget = gadget->NextGadget;
	}
	else
	{
	    gadget = NULL;
	}
    }
    
    return TRUE;
}

/**************************************************************************************************/

AROS_UFH3S(IPTR, dispatch_scrdecorclass,
           AROS_UFHA(Class *,  cl,  A0),
           AROS_UFHA(Object *, o,   A2),
           AROS_UFHA(Msg,      msg, A1)
          )
{
    AROS_USERFUNC_INIT

    IPTR retval;

    switch (msg->MethodID)
    {
	case OM_NEW:
    	    retval = scrdecor_new(cl, o, (struct opSet *)msg);
	    break;

    	case OM_GET:
	    retval = scrdecor_get(cl, o, (struct opGet *)msg);
	    break;
	    
    	case SDM_GETDEFSIZE_SYSIMAGE:
	    retval = scrdecor_getdefsize_sysimage(cl, o, (struct sdpGetDefSizeSysImage *)msg);
	    break;
	    
	case SDM_DRAW_SYSIMAGE:
	    retval = scrdecor_draw_sysimage(cl, o, (struct sdpDrawSysImage *)msg);
	    break;
	    
	case SDM_DRAW_SCREENBAR:
	    retval = scrdecor_draw_screenbar(cl, o, (struct sdpDrawScreenBar *)msg);
	    break;
	    
	case SDM_DRAW_SCREENTITLE:
	    retval = scrdecor_draw_screentitle(cl, o, (struct sdpDrawScreenTitle *)msg);
	    break;
	
	case SDM_LAYOUT_SCREENGADGETS:
	    retval = scrdecor_layout_screengadgets(cl, o, (struct sdpLayoutScreenGadgets *)msg);
	    break;
	    
	default:
            retval = DoSuperMethodA(cl, o, msg);
            break;

    } /* switch */

    return (retval);

    AROS_USERFUNC_EXIT
}

/**************************************************************************************************/

#undef IntuitionBase

/**************************************************************************************************/

struct IClass *InitScrDecorClass (struct IntuitionBase * IntuitionBase)
{
    struct IClass *cl = NULL;

    /* This is the code to make the scrdecor class...
    */
    if ((cl = MakeClass(SCRDECORCLASS, ROOTCLASS, NULL, sizeof(struct scrdecor_data), 0)))
    {
        cl->cl_Dispatcher.h_Entry    = (APTR)AROS_ASMSYMNAME(dispatch_scrdecorclass);
        cl->cl_Dispatcher.h_SubEntry = NULL;
        cl->cl_UserData              = (IPTR)IntuitionBase;

        AddClass (cl);
    }

    return (cl);
}

/***********************************************************************************/
