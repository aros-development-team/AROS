/*
    Copyright  1995-2011, The AROS Development Team. All rights reserved.
    Copyright  2001-2003, The MorphOS Development Team. All Rights Reserved.
    $Id$
*/

#define DEBUG 0
#include <aros/debug.h>

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

static void renderimageframe(struct RastPort *rp, ULONG which, ULONG state, UWORD *pens,
                             WORD left, WORD top, WORD width, WORD height,
                             struct IntuitionBase *IntuitionBase)
{
    struct GfxBase *GfxBase = GetPrivIBase(IntuitionBase)->GfxBase;
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

#if 0
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
#endif

/**************************************************************************************************/

IPTR ScrDecorClass__OM_NEW(Class *cl, Object *obj, struct opSet *msg)
{
    struct IntuitionBase *IntuitionBase = (struct IntuitionBase *)cl->cl_UserData;
    struct Library *UtilityBase = GetPrivIBase(IntuitionBase)->UtilityBase;
    struct scrdecor_data *data;

    D(bug("[SCRDECOR] ScrDecorClass__OM_NEW()\n"));

    obj = (Object *)DoSuperMethodA(cl, obj, (Msg)msg);
    if (obj)
    {
    	data = INST_DATA(cl, obj);

	data->userbuffersize = (ULONG) GetTagData(SDA_UserBuffer, 0, msg->ops_AttrList);
	
    }
    
    return (IPTR)obj;
}

/**************************************************************************************************/

IPTR ScrDecorClass__OM_GET(Class *cl, Object *obj, struct opGet *msg)
{
    struct scrdecor_data *data = INST_DATA(cl, obj);

    D(bug("[SCRDECOR] ScrDecorClass__OM_GET()\n"));

    switch(msg->opg_AttrID)
    {
    	case SDA_UserBuffer:
	    *msg->opg_Storage = (IPTR) data->userbuffersize;
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

IPTR ScrDecorClass__SDM_GETDEFSIZE_SYSIMAGE(Class *cl, Object *obj, struct sdpGetDefSizeSysImage *msg)
{
    ULONG def_low_width = DEFSIZE_WIDTH, def_low_height = DEFSIZE_HEIGHT;
    ULONG def_med_width = DEFSIZE_WIDTH, def_med_height = DEFSIZE_HEIGHT;
    ULONG def_high_width = DEFSIZE_WIDTH, def_high_height = DEFSIZE_HEIGHT;

    D(bug("[SCRDECOR] ScrDecorClass__SDM_GETDEFSIZE_SYSIMAGE()\n"));

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

IPTR ScrDecorClass__SDM_DRAW_SYSIMAGE(Class *cl, Object *obj, struct sdpDrawSysImage *msg)
{
    struct IntuitionBase *IntuitionBase = (struct IntuitionBase *)cl->cl_UserData;
    struct GfxBase       *GfxBase = GetPrivIBase(IntuitionBase)->GfxBase;
    struct RastPort 	 *rp = msg->sdp_RPort;
    UWORD   	    	 *pens = DRI(msg->sdp_Dri)->dri_Pens;
    LONG    	    	  state = msg->sdp_State;
    LONG    	     	  left = msg->sdp_X;
    LONG    	     	  top = msg->sdp_Y;
    LONG   	          width = msg->sdp_Width;
    LONG   	    	  height = msg->sdp_Height;
    LONG    	    	  right = left + width - 1;
    LONG    	    	  bottom = top + height - 1;

    D(bug("[SCRDECOR] ScrDecorClass__SDM_DRAW_SYSIMAGE()\n"));

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
/*
IPTR ScrDecorClass__SDM_DRAW_SCREENBAR(Class *cl, Object *obj, struct sdpDrawScreenBar *msg)
{
    struct scrdecor_data *data = INST_DATA(cl, obj);
    struct RastPort 	 *rp = msg->sdp_RPort;
    UWORD   	    	 *pens = DRI(msg->sdp_Dri)->dri_Pens;
    LONG    	    	  left, right;
    BOOL    	    	  beeping = FALSE;
    
#if 0
#if USE_NEWDISPLAYBEEP
        beeping = (msg->sdp_Screen->Flags & BEEPING) && GetBitMapAttr(rp->BitMap, BMA_DEPTH) > 8;
#endif
#endif

    SetDrMd(rp, JAM1);
    
    SetAPen(rp, pens[beeping ? BARDETAILPEN : BARBLOCKPEN]);
    RectFill(rp, 0, 0, msg->sdp_Screen->Width - 1, msg->sdp_Screen->BarHeight - 1);

    SetAPen(rp, pens[beeping ? BARDETAILPEN : BARTRIMPEN]);
    RectFill(rp, 0, msg->sdp_Screen->BarHeight, msg->sdp_Screen->Width - 1, msg->sdp_Screen->BarHeight);

    findtitlearea(msg->sdp_Screen, &left, &right);

    SetAPen(rp, pens[SHADOWPEN]);
    RectFill(rp, right, 1, right, msg->sdp_Screen->BarHeight - 1);
        
    return TRUE;
}
*/

/**************************************************************************************************/

IPTR ScrDecorClass__SDM_DRAW_SCREENBAR(Class *cl, Object *obj, struct sdpDrawScreenBar *msg)
{
    struct IntuitionBase *IntuitionBase = (struct IntuitionBase *)cl->cl_UserData;
    struct GfxBase       *GfxBase = GetPrivIBase(IntuitionBase)->GfxBase;
    struct RastPort 	 *rp = msg->sdp_RPort;
    UWORD   	    	 *pens = DRI(msg->sdp_Dri)->dri_Pens;
    LONG    	    	  right, left;
    BOOL    	    	  beeping = FALSE;

    D(bug("[SCRDECOR] ScrDecorClass__SDM_DRAW_SCREENBAR()\n"));

#if USE_NEWDISPLAYBEEP
    beeping = msg->sdp_Screen->Flags & BEEPING;
#endif

    findtitlearea(msg->sdp_Screen, &left, &right);

    D(bug("[SCRDECOR] ScrDecorClass__SDM_DRAW_SCREENBAR: Title_Left = %d, Title_Right = %d\n", left, right));

    D(bug("[SCRDECOR] ScrDecorClass__SDM_DRAW_SCREENBAR: RastPort @  %p, Screen @ %p\n", rp, msg->sdp_Screen));
    D(bug("[SCRDECOR] ScrDecorClass__SDM_DRAW_SCREENBAR: Screen Dimensions  %dx%d\n", msg->sdp_Screen->Width, msg->sdp_Screen->Height));
    D(bug("[SCRDECOR] ScrDecorClass__SDM_DRAW_SCREENBAR: Bar Height  %d\n", msg->sdp_Screen->BarHeight));

    SetDrMd(rp, JAM1);

    SetAPen(rp, pens[beeping ? BARDETAILPEN : BARBLOCKPEN]);
    RectFill(rp, left + 1, 0, right - 1, msg->sdp_Screen->BarHeight - 1);

    D(bug("[SCRDECOR] ScrDecorClass__SDM_DRAW_SCREENBAR: Filled Bar Area\n"));

    SetAPen(rp, pens[beeping ? BARDETAILPEN : BARTRIMPEN]);
    RectFill(rp, 0, msg->sdp_Screen->BarHeight, msg->sdp_Screen->Width - 1, msg->sdp_Screen->BarHeight);

    D(bug("[SCRDECOR] ScrDecorClass__SDM_DRAW_SCREENBAR: Filled Bar Area\n"));

    if (msg->sdp_Screen->Title)
    {
        SetAPen(rp, pens[beeping ? BARBLOCKPEN: BARDETAILPEN]);
        SetBPen(rp, pens[beeping ? BARDETAILPEN : BARBLOCKPEN]);

        Move(rp, msg->sdp_Screen->BarHBorder, msg->sdp_Screen->BarVBorder + rp->TxBaseline);

        D(bug("[SCRDECOR] ScrDecorClass__SDM_DRAW_SCREENBAR: Title Text @ %p\n", msg->sdp_Screen->Title));
        D(bug("[SCRDECOR] ScrDecorClass__SDM_DRAW_SCREENBAR: Title '%s'\n", msg->sdp_Screen->Title));
        Text(rp, msg->sdp_Screen->Title, strlen(msg->sdp_Screen->Title));

        D(bug("[SCRDECOR] ScrDecorClass__SDM_DRAW_SCREENBAR: Text Rendered\n"));
    }
    return TRUE;
}

/**************************************************************************************************/

IPTR ScrDecorClass__SDM_LAYOUT_SCREENGADGETS(Class *cl, Object *obj, struct sdpLayoutScreenGadgets *msg)
{
    struct Gadget *gadget = msg->sdp_Gadgets;

    D(bug("[SCRDECOR] ScrDecorClass__SDM_LAYOUT_SCREENGADGETS()\n"));

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

IPTR ScrDecorClass__SDM_INITSCREEN(Class *cl, Object *obj, struct sdpInitScreen *msg)
{
    D(bug("[SCRDECOR] ScrDecorClass__SDM_INITSCREEN()\n"));

    return TRUE;
}

IPTR ScrDecorClass__SDM_EXITSCREEN(Class *cl, Object *obj, struct sdpExitScreen *msg)
{
    D(bug("[SCRDECOR] ScrDecorClass__SDM_EXITSCREEN()\n"));

    return TRUE;
}

/**************************************************************************************************/
