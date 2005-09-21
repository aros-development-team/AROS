/*
    Copyright © 1995-2005, The AROS Development Team. All rights reserved.
    Copyright © 2001-2003, The MorphOS Development Team. All Rights Reserved.
    $Id: imageclass.c 20651 2004-01-17 20:57:12Z chodorowski $
*/


#include <dos/dos.h>
#include <dos/dosextens.h>

#include <intuition/intuition.h>
#include <intuition/intuitionbase.h>
#include <intuition/classes.h>
#include <intuition/classusr.h>
#include <intuition/windecorclass.h>
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
    WORD right = left + width - 1;
    WORD bottom = top + height - 1;
    BOOL leftedgegodown = FALSE;
    BOOL topedgegoright = FALSE;

    switch(which)
    {
#if 0
	case CLOSEIMAGE:
            /* draw separator line at the right side */
            SetAPen(rp, pens[SHINEPEN]);
            RectFill(rp, right, top, right, bottom - 1);
            SetAPen(rp, pens[SHADOWPEN]);
            WritePixel(rp, right, bottom);

            right--;
            break;

	case ZOOMIMAGE:
	case DEPTHIMAGE:
	case SDEPTHIMAGE:
            /* draw separator line at the left side */
            SetAPen(rp, pens[SHINEPEN]);
            WritePixel(rp, left, top);
            SetAPen(rp, pens[SHADOWPEN]);
            RectFill(rp, left, top + 1, left, bottom);

            left++;
            break;
#endif

	case UPIMAGE:
	case DOWNIMAGE:
            leftedgegodown = TRUE;
            break;

	case LEFTIMAGE:
	case RIGHTIMAGE:
            topedgegoright = TRUE;
            break;
    }

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

IPTR WinDecorClass__OM_NEW(Class *cl, Object *obj, struct opSet *msg)
{
    struct windecor_data *data;
    
    obj = (Object *)DoSuperMethodA(cl, obj, (Msg)msg);
    if (obj)
    {
    	data = INST_DATA(cl, obj);
	
	data->dri = (struct IntDrawInfo *)GetTagData(WDA_DrawInfo, 0, msg->ops_AttrList);
	data->scr = (struct Screen *)GetTagData(WDA_Screen, 0, msg->ops_AttrList);
	
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

IPTR WinDecorClass__OM_GET(Class *cl, Object *obj, struct opGet *msg)
{
    struct windecor_data *data = INST_DATA(cl, obj);

    switch(msg->opg_AttrID)
    {
    	case WDA_DrawInfo:
	    *msg->opg_Storage = (IPTR)data->dri;
	    break;
	    
    	case WDA_Screen:
	    *msg->opg_Storage = (IPTR)data->scr;
	    break;
	    
	case WDA_TrueColorOnly:
	    *msg->opg_Storage = FALSE;
	    break;
	    
	default:
	    return DoSuperMethodA(cl, obj, (Msg)msg);
    }
    
    return 1;    
}
    

/**************************************************************************************************/

IPTR WinDecorClass__WDM_GETDEFSIZE_SYSIMAGE(Class *cl, Object *obj, struct wdpGetDefSizeSysImage *msg)
{
    ULONG def_low_width = DEFSIZE_WIDTH, def_low_height = DEFSIZE_HEIGHT;
    ULONG def_med_width = DEFSIZE_WIDTH, def_med_height = DEFSIZE_HEIGHT;
    ULONG def_high_width = DEFSIZE_WIDTH, def_high_height = DEFSIZE_HEIGHT;
    
    #define REFHEIGHT (msg->wdp_ReferenceFont->tf_YSize)
    #define REFWIDTH  REFHEIGHT
    
    switch(msg->wdp_Which)
    {
    	case LEFTIMAGE:
	case RIGHTIMAGE:
	#if USE_AROS_DEFSIZE
    	    def_low_width = def_med_width = def_high_width = DEFSIZE_WIDTH;
	    def_low_height = def_med_height = def_high_height = DEFSIZE_HEIGHT;
	#else
            def_low_width = 16;
            def_med_width = 16;
            def_high_width = 23;
            def_low_height = 11;
            def_med_height = 10;
            def_high_height = 22;
	#endif
            break;
	   
	case UPIMAGE:
	case DOWNIMAGE:
	#if USE_AROS_DEFSIZE
    	    def_low_width = def_med_width = def_high_width = DEFSIZE_WIDTH;
	    def_low_height = def_med_height = def_high_height = DEFSIZE_HEIGHT;
	#else
            def_low_width = 13;
            def_med_width = 18;
            def_high_width = 23;
            def_low_height = 11;
            def_med_height = 11;
            def_high_height = 22;
	#endif
            break;

	case DEPTHIMAGE:
	case ZOOMIMAGE:
	case ICONIFYIMAGE:
	case LOCKIMAGE:
	case MUIIMAGE:
	case POPUPIMAGE:
	case SNAPSHOTIMAGE:
	case JUMPIMAGE:
	#if USE_AROS_DEFSIZE
    	    def_low_width = def_med_width = def_high_width = DEFSIZE_WIDTH;
	    def_low_height = def_med_height = def_high_height = DEFSIZE_HEIGHT;
	#else
            def_low_width = 18;
            def_med_width = 24;
            def_high_width = 24;
	#endif
    	    break;

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

	case CLOSEIMAGE:
	#if USE_AROS_DEFSIZE
    	    def_low_width = def_med_width = def_high_width = DEFSIZE_WIDTH;
	    def_low_height = def_med_height = def_high_height = DEFSIZE_HEIGHT;
	#else
            def_low_width = 15;
            def_med_width = 20;
            def_high_width = 20;
	#endif
            break;

	case SIZEIMAGE:
	#if USE_AROS_DEFSIZE
    	    def_low_width = def_med_width = def_high_width = DEFSIZE_WIDTH;
	    def_low_height = def_med_height = def_high_height = DEFSIZE_HEIGHT;
	#else
            def_low_width = 13;
            def_med_width = 18;
            def_high_width = 18;
            def_low_height = 11;
            def_med_height = 10;
            def_high_height = 10;
	#endif
            break;

	case MENUCHECK:
            def_low_width  =
	    def_med_width  =
	    def_high_width = REFWIDTH / 2 + 4; // reffont->tf_XSize * 3 / 2;
            def_low_height =
	    def_med_height =
	    def_high_height= REFHEIGHT;
            break;

	case MXIMAGE:
            def_low_width  =
	    def_med_width  =
	    def_high_width = (REFWIDTH + 1) * 2; // reffont->tf_XSize * 3 - 1;
            def_low_height = 
	    def_med_height =
	    def_high_height= REFHEIGHT + 1;
            break;

	case CHECKIMAGE:
            def_low_width  = (REFWIDTH + 3) * 2;//reffont->tf_XSize * 2;
            def_low_height = REFHEIGHT + 3;
            break;
	    
	default:
	    return FALSE;
    }
    
    switch(msg->wdp_SysiSize)
    {
    	case SYSISIZE_LOWRES:
	    *msg->wdp_Width = def_low_width;
	    *msg->wdp_Height = def_low_height;
	    break;

	case SYSISIZE_MEDRES:
	    *msg->wdp_Width = def_med_width;
	    *msg->wdp_Height = def_med_height;
	    break;
    	    
    	case SYSISIZE_HIRES:
    	default:
	    *msg->wdp_Width = def_high_width;
	    *msg->wdp_Height = def_high_height;
	    break;	    	
    }
    
    return TRUE;
}

/**************************************************************************************************/

IPTR WinDecorClass__WDM_DRAW_SYSIMAGE(Class *cl, Object *obj, struct wdpDrawSysImage *msg)
{
    struct windecor_data *data = INST_DATA(cl, obj);
    struct RastPort 	 *rp = msg->wdp_RPort;
    UWORD   	    	 *pens = DRI(data->dri)->dri_Pens;
    LONG    	    	  state = msg->wdp_State;
    LONG    	     	  left = msg->wdp_X;
    LONG    	     	  top = msg->wdp_Y;
    LONG   	          width = msg->wdp_Width;
    LONG   	    	  height = msg->wdp_Height;
    LONG    	    	  right = left + width - 1;
    LONG    	    	  bottom = top + height - 1;
    LONG    	    	  h_spacing, v_spacing;
    
    SetDrMd(rp, JAM1);
    
    switch(msg->wdp_Which)
    {
    	case CLOSEIMAGE:
	{
	    renderimageframe(rp, CLOSEIMAGE, state, pens, left, top, width, height, IntuitionBase);
	    left++;
	    top++;
	    width -= 2;
	    height -= 2;
	    
	    right = left + width - 1;
	    bottom = top + height - 1;
	    h_spacing = width * 4 / 10;
	    v_spacing = height * 3 / 10;
	    
	    SetAPen(rp, getbgpen(state, pens));
	    RectFill(rp, left, top, right, bottom);
	    
            left += h_spacing;
            right -= h_spacing;
            top += v_spacing;
            bottom -= v_spacing;

            SetAPen(rp, pens[SHADOWPEN]);
            RectFill(rp, left, top, right, bottom);

            left++;
            top++;
            right--;
            bottom--;

            SetAPen(rp, pens[(state == IDS_NORMAL) ? SHINEPEN : BACKGROUNDPEN]);
            RectFill(rp, left, top, right, bottom);

            break;	    
	}

    	case ZOOMIMAGE:
        {
            UWORD  bg;
            WORD   h_spacing;
            WORD   v_spacing;

            renderimageframe(rp, ZOOMIMAGE, state, pens,
                             left, top, width, height, IntuitionBase);
            left++;
            top++;
            width -= 2;
            height -= 2;
 
            right = left + width - 1;
            bottom = top + height - 1 ;
            h_spacing = width / 6;
            v_spacing = height / 6;

            bg = getbgpen(state, pens);

            /* Clear background into correct color */
            SetAPen(rp, bg);
            RectFill(rp, left, top, right, bottom);

            left += h_spacing;
            right -= h_spacing;
            top += v_spacing;
            bottom -= v_spacing;

            SetAPen(rp, pens[SHADOWPEN]);
            RectFill(rp, left, top, right, bottom);

            SetAPen(rp, pens[(state == IDS_SELECTED) ? SHINEPEN :
                                (state == IDS_NORMAL) ? FILLPEN : BACKGROUNDPEN]);
            RectFill(rp, left + 1, top + 1, right - 1, bottom - 1);

            right = left + (right - left + 1) / 2;
            bottom = top + (bottom - top + 1) / 2;

            if (right - left <  4) right = left + 4;

            SetAPen(rp, pens[SHADOWPEN]);
            RectFill(rp, left, top, right, bottom);

            left += 2;
            right -= 2;
            top += 1;
            bottom -= 1;

            SetAPen(rp, pens[(state == IDS_SELECTED) ? FILLPEN :
                                (state == IDS_NORMAL) ? SHINEPEN : BACKGROUNDPEN]);
            RectFill(rp,left, top, right, bottom);
            break;
        }

    	case DEPTHIMAGE:
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

            bg = getbgpen(state, pens);

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
            drawrect(rp
                     , left
                     , top
                     , right - (width / 3 )
                     , bottom - (height / 3)
                     , IntuitionBase);


            /* Fill top left window (inside of the frame above) */

            if ((state != IDS_INACTIVENORMAL))
            {
                SetAPen(rp, pens[BACKGROUNDPEN]);
                RectFill(rp, left + 1, top + 1,
		    	     right - (width / 3) - 1, bottom - (height / 3) - 1);

            }

            /* Render bottom right window  */
            SetAPen(rp, pens[SHADOWPEN]);
            drawrect(rp, left + (width / 3), top + (height / 3),
	    	    	 right, bottom, IntuitionBase);

            /* Fill bottom right window (inside of the frame above) */
            SetAPen(rp, pens[(state == IDS_INACTIVENORMAL) ? BACKGROUNDPEN : SHINEPEN]);
            RectFill(rp, left + (width / 3) + 1, top + (height / 3) + 1,
	    	    	 right - 1, bottom - 1);

            if (state == IDS_SELECTED)
            {
                /* Re-Render top left window  */

                SetAPen(rp, pens[SHADOWPEN]);
                drawrect(rp, left, top,
		    	     right - (width / 3 ), bottom - (height / 3), IntuitionBase);
            }
            break;
        }

    	case SIZEIMAGE:
        {
            UWORD  bg;
            WORD   h_spacing;
            WORD   v_spacing;
            WORD   x, y;

            renderimageframe(rp, SIZEIMAGE, state, pens,
                             left, top, width, height, IntuitionBase);
            left++;
            top++;
            right--;
            bottom--;
            width -= 2;
            height -= 2;

            h_spacing = width  / 5;
            v_spacing = height / 5;

            bg = getbgpen(state, pens);

            /* Clear background into correct color */
            SetAPen(rp, bg);
            RectFill(rp, left, top, right, bottom);

            /* A triangle image */

            left += h_spacing;
            top  += v_spacing;

            right  = left + width  - 1 - (h_spacing * 2);
            bottom = top  + height - 1 - (v_spacing * 2);

            width  = right  - left + 1;
            height = bottom - top  + 1;

            if (state != IDS_INACTIVENORMAL)
            {
                SetAPen(rp, pens[SHINEPEN]);
		
                for(y = top; y <= bottom; y++)
                {
                    x = left + (bottom - y) * width / height;
                    RectFill(rp, x, y, right, y);
                }
            }

            SetAPen(rp, pens[SHADOWPEN]);
            /* Draw triangle border */
            Move(rp, left, bottom);
            Draw(rp, right, top);
            Draw(rp, right, bottom);
            Draw(rp, left, bottom);

            break;
        }
	
    	case LEFTIMAGE:
        {
            UWORD hspacing,vspacing;
            WORD  cy, i;

            hspacing = HSPACING;
            vspacing = VSPACING;

            if (width <= 12)
            {
                hspacing = HSPACING_MIDDLE;
            }
	    
            if (width <= 10)
            {
                hspacing = HSPACING_SMALL;
            }

            if (height <= 12)
            {
                vspacing = VSPACING_MIDDLE;
            }
	    
            if (height <= 10)
            {
                vspacing = VSPACING_SMALL;
            }

            renderimageframe(rp, LEFTIMAGE, state, pens,
                             left, top, width, height, IntuitionBase);
            left++;
            top++;
            right--;
            bottom--;
            width -= 2;
            height -= 2;

            SetAPen(rp, getbgpen(state, pens));
            RectFill(rp, left, top, right, bottom);

            left += hspacing;
            top += vspacing;
            width -= hspacing * 2;
            height -= vspacing * 2;

            right = left + width - 1;
            bottom = top + height - 1;

            cy = (height + 1) / 2;

            SetAPen(rp, pens[SHADOWPEN]);

            for(i = 0; i < cy; i++)
            {
                RectFill(rp, left + (cy - i - 1) * width / cy,
                         top + i,
                         right - i * width / cy / 2,
                         top + i);
                RectFill(rp, left + (cy - i - 1) * width / cy,
                         bottom - i,
                         right - i * width / cy / 2,
                         bottom - i);
            }
            break;
        }

    	case UPIMAGE:
        {
            UWORD hspacing,vspacing;
            WORD  cx, i;

            hspacing = HSPACING;
            vspacing = VSPACING;

            if (width <= 12)
            {
                hspacing = HSPACING_MIDDLE;
            }
	    
            if (width <= 10)
            {
                hspacing = HSPACING_SMALL;
            }

            if (height <= 12)
            {
                vspacing = VSPACING_MIDDLE;
            }
	    
            if (height <= 10)
            {
                vspacing = VSPACING_SMALL;
            }

            renderimageframe(rp, UPIMAGE, state, pens,
                             left, top, width, height, IntuitionBase);
            left++;
            top++;
            right--;
            bottom--;
            width -= 2;
            height -= 2;

            SetAPen(rp, getbgpen(state, pens));
            RectFill(rp, left, top, right, bottom);

            left += hspacing;
            top += vspacing;
            width -= hspacing * 2;
            height -= vspacing * 2;

            right = left + width - 1;
            bottom = top + height - 1;

            cx = (width + 1) / 2;

            SetAPen(rp, pens[SHADOWPEN]);

            for(i = 0; i < cx; i++)
            {
                RectFill(rp, left + i,
                         top + (cx - i - 1) * height / cx,
                         left + i,
                         bottom - i * height / cx / 2);
                RectFill(rp, right - i,
                         top + (cx - i - 1) * height / cx,
                         right - i,
                         bottom - i * height / cx / 2);
            }

            break;
        }

    	case RIGHTIMAGE:
        {
            UWORD hspacing,vspacing;
            WORD  cy, i;

            hspacing = HSPACING;
            vspacing = VSPACING;

            if (width <= 12)
            {
                hspacing = HSPACING_MIDDLE;
            }
	    
            if (width <= 10)
            {
                hspacing = HSPACING_SMALL;
            }

            if (height <= 12)
            {
                vspacing = VSPACING_MIDDLE;
            }
	    
            if (height <= 10)
            {
                vspacing = VSPACING_SMALL;
            }

            renderimageframe(rp, RIGHTIMAGE, state, pens,
                             left, top, width, height, IntuitionBase);
            left++;
            top++;
            right--;
            bottom--;
            width -= 2;
            height -= 2;


            SetAPen(rp, getbgpen(state, pens));
            RectFill(rp, left, top, right, bottom);

            left += hspacing;
            top += vspacing;
            width -= hspacing * 2;
            height -= vspacing * 2;

            right = left + width - 1;
            bottom = top + height - 1;

            cy = (height + 1) / 2;

            SetAPen(rp, pens[SHADOWPEN]);

            for(i = 0; i < cy; i++)
            {
                RectFill(rp, left + i * width / cy / 2,
                         top + i,
                         right - (cy - i - 1) * width / cy,
                         top + i);
                RectFill(rp, left + i * width / cy / 2,
                         bottom - i,
                         right - (cy - i - 1) * width / cy,
                         bottom - i);
            }
            break;
        }

    	case DOWNIMAGE:
        {
            UWORD hspacing,vspacing;
            WORD  cx, i;
 
            hspacing = HSPACING;
            vspacing = VSPACING;

            if (width <= 12)
            {
                hspacing = HSPACING_MIDDLE;
            }
	    
            if (width <= 10)
            {
                hspacing = HSPACING_SMALL;
            }

            if (height <= 12)
            {
                vspacing = VSPACING_MIDDLE;
            }
	    
            if (height <= 10)
            {
                vspacing = VSPACING_SMALL;
            }

            renderimageframe(rp, DOWNIMAGE, state, pens,
                             left, top, width, height, IntuitionBase);
            left++;
            top++;
            right--;
            bottom--;
            width -= 2;
            height -= 2;

            SetAPen(rp, getbgpen(state, pens));
            RectFill(rp, left, top, right, bottom);

            left += hspacing;
            top += vspacing;
            width -= hspacing * 2;
            height -= vspacing * 2;

            right = left + width - 1;
            bottom = top + height - 1;

            cx = (width + 1) / 2;

            SetAPen(rp, pens[SHADOWPEN]);

            for(i = 0; i < cx; i++)
            {
                RectFill(rp, left + i,
                         top + i * height / cx / 2,
                         left + i,
                         bottom - (cx - i - 1) * height / cx);
                RectFill(rp, right - i,
                         top + i * height / cx / 2,
                         right -  i,
                         bottom - (cx - i - 1) * height / cx);

            }
            break;
        }

	
	default:
	    return FALSE;
    }
    
    return TRUE;
}

/**************************************************************************************************/

static void findtitlearea(struct Window *win, LONG *left, LONG *right)
{
    struct Gadget *g;

    *left = 0;
    *right = win->Width - 1;
    
    for (g = win->FirstGadget; g; g = g->NextGadget)
    {
        if (g->Activation & GACT_TOPBORDER && g != (struct Gadget *)IW(win)->sysgads[DRAGBAR])
        {
            if (!(g->Flags & GFLG_RELRIGHT))
            {
                if (g->LeftEdge + g->Width > *left)
                    *left = g->LeftEdge + g->Width;
            }
            else
            {
                if (g->LeftEdge + win->Width - 1 - 1 < *right)
                    *right = g->LeftEdge + win->Width - 1 - 1;
            }
        }
    }
    
}

/**************************************************************************************************/

IPTR WinDecorClass__WDM_DRAW_WINBORDER(Class *cl, Object *obj, struct wdpDrawWinBorder *msg)
{
    struct windecor_data *data = INST_DATA(cl, obj);
    struct RastPort 	 *rp = msg->wdp_RPort;
    struct Window   	 *window = msg->wdp_Window;
    UWORD   	    	 *pens = DRI(data->dri)->dri_Pens;
    LONG    	    	  left, right;
    
    SetDrMd(rp, JAM1);
    SetAPen(rp, pens[SHINEPEN]);
    
    if (window->BorderTop > 0)
    {
    	/* Outer shine edge on top side */
	
    	CheckRectFill(rp, 0, 0, window->Width - 1, 0, IntuitionBase);
    }

    if (!(msg->wdp_Flags & WDF_DWB_TOP_ONLY))
    {
	if (window->BorderLeft > 0)
	{
	    /* Outer shine edge on left side */

	    CheckRectFill(rp, 0, 0, 0, window->Height - 1, IntuitionBase);
	}

	if (window->BorderRight > 1)
	{
	    /* Inner shine edge on right side */

	    CheckRectFill(rp,
	    		  window->Width - window->BorderRight, window->BorderTop,
                	  window->Width - window->BorderRight, window->Height - window->BorderBottom,
                	  IntuitionBase);
	}

	if (window->BorderBottom > 1)
	{
	    /* Inner shine edge on bottom side */

	    CheckRectFill(rp,
                	  window->BorderLeft, window->Height - window->BorderBottom,
                	  window->Width - window->BorderRight, window->Height - window->BorderBottom,
                	  IntuitionBase);
	}
    }
    
    SetAPen(rp, pens[SHADOWPEN]);
    
    if (!(msg->wdp_Flags & WDF_DWB_TOP_ONLY))
    {
	if (window->BorderRight > 0)
	{
    	    /* Outer shadow edge on right side */

    	    CheckRectFill(rp, window->Width - 1, 1,
                	  window->Width - 1, window->Height - 1, IntuitionBase);
	}

	if (window->BorderBottom > 0)
	{
    	    /* Outer shadow edge on bottom side */

    	    CheckRectFill(rp, 1, window->Height - 1,
	    		  window->Width - 1, window->Height - 1, IntuitionBase);
	}

	if (window->BorderLeft > 1)
	{
    	    /* Inner shadow edge on left side */

    	    CheckRectFill(rp, window->BorderLeft - 1, window->BorderTop - 1,
                	  window->BorderLeft - 1, window->Height - window->BorderBottom,
                	  IntuitionBase);
	}
	
    }
    
    if (window->BorderTop > 1)
    {
    	/* Inner shadow edge on top side */
	
	CheckRectFill(rp, window->BorderLeft - 1, window->BorderTop - 1,
                      window->Width - window->BorderRight, window->BorderTop - 1,
                      IntuitionBase);
    }
    
    SetAPen(rp, pens[(window->Flags & WFLG_WINDOWACTIVE) ? FILLPEN : BACKGROUNDPEN]);

    if (window->BorderTop > 2)
    {
    	/* Fill on top side */
	
    	CheckRectFill(rp, 1, 1, window->Width - 2, window->BorderTop - 2, IntuitionBase);
    }

    if (!(msg->wdp_Flags & WDF_DWB_TOP_ONLY))
    {
	if (window->BorderLeft > 2)
	{
    	    /* Fill on left side */

    	    CheckRectFill(rp, 1, 1, window->BorderLeft - 2, window->Height - 2, IntuitionBase);

	}

	if (window->BorderRight > 2)
	{
    	    /* Fill on right side */

    	    CheckRectFill(rp, window->Width - window->BorderRight + 1, 1,
                	  window->Width - 2, window->Height - 2, IntuitionBase);
	}

	if (window->BorderBottom > 2)
	{
    	    /* Fill on bottom side */

	    CheckRectFill(rp, 1, window->Height - window->BorderBottom + 1,
                	  window->Width - 2, window->Height - 2, IntuitionBase);
	}
    }
    
    findtitlearea(window, &left, &right);
    
    if (left != 0)
    {
    	/* Left edge of title area */
	
        SetAPen(rp, pens[SHINEPEN]);
    	Move(rp, left, 1);
    	Draw(rp, left, window->BorderTop - 2);
    }
    
    if (right != window->Width - 1)
    {
    	/* Right edges of title area */
	
        SetAPen(rp, pens[SHADOWPEN]);
    	Move(rp, right, 1);
	Draw(rp, right, window->BorderTop - 2);
    }
    
    return TRUE;
}

/**************************************************************************************************/

IPTR WinDecorClass__WDM_DRAW_WINTITLE(Class *cl, Object *obj, struct wdpDrawWinTitle *msg)
{
    struct windecor_data *data = INST_DATA(cl, obj);
    struct RastPort 	 *rp = msg->wdp_RPort;
    struct Window   	 *window = msg->wdp_Window;
    UWORD   	    	 *pens = DRI(data->dri)->dri_Pens;
    LONG    	    	  right, left;

    findtitlearea(window, &left, &right);

    SetDrMd(rp, JAM1);
    SetAPen(rp, pens[(window->Flags & WFLG_WINDOWACTIVE) ? FILLPEN : BACKGROUNDPEN]);
    CheckRectFill(rp, left + 1, 1, right - 1, window->BorderTop - 2, IntuitionBase);
        
    if (right - left > 6)
    {
        ULONG   	    	textlen, titlelen, textpixellen;
        struct TextExtent 	te;

        SetFont(rp, DRI(data->dri)->dri_Font);

        titlelen = strlen(window->Title);
        textlen = TextFit(rp
                          , window->Title
                          , titlelen
                          , &te
                          , NULL
                          , 1
                          , right - left - 6
                          , window->BorderTop - 2);
    	if (textlen)
	{
	    textpixellen = te.te_Extent.MaxX - te.te_Extent.MinX + 1;
	    
	    switch(msg->wdp_TitleAlign)
	    {		    
		case WD_DWTA_CENTER:
		    if (textlen == titlelen)
		    {
		    	left = (left + right + 1 - textpixellen) / 2;
		    }
		    else
		    {
		    	left = left + 3;
		    }
		    break;
		    
		case WD_DWTA_RIGHT:
		    if (textlen == titlelen)
		    {
		    	left = right - textpixellen;
		    }
		    else
		    {
		    	left = left + 3;
		    }
		    break;

    	    	default:
	    	case WD_DWTA_LEFT:
		    left = left + 3;
		    break;

	    }
	    
            SetAPen(rp, pens[(window->Flags & WFLG_WINDOWACTIVE) ? FILLTEXTPEN : TEXTPEN]);

	    Move(rp, left, DRI(data->dri)->dri_Font->tf_Baseline + 2);
            Text(rp, window->Title, textlen);
	}
    }
    
    return TRUE;
}

/**************************************************************************************************/

IPTR WinDecorClass__WDM_LAYOUT_BORDERGADGETS(Class *cl, Object *obj, struct wdpLayoutBorderGadgets *msg)
{
    //struct windecor_data *data = INST_DATA(cl, obj);
    //struct Window   	 *window = msg->wdp_Window;
    struct Gadget   	 *gadget = msg->wdp_Gadgets;

    while(gadget)
    {
    	switch(gadget->GadgetType & GTYP_SYSTYPEMASK)
	{
	    case GTYP_CLOSE:
	    	gadget->LeftEdge = 0;
		gadget->Width = gadget->Height;
		gadget->Flags &= ~(GFLG_RELRIGHT | GFLG_RELWIDTH);
	    	break;
		
	    case GTYP_WDEPTH:
	    	gadget->LeftEdge = -gadget->Height + 1;
		gadget->Width = gadget->Height;
		gadget->Flags &= ~GFLG_RELWIDTH;
		gadget->Flags |= GFLG_RELRIGHT;
		break;
		
	    case GTYP_WZOOM:
	    	gadget->LeftEdge = -gadget->Height * 2 + 1;
		gadget->Width = gadget->Height;
		gadget->Flags &= ~GFLG_RELWIDTH;
		gadget->Flags |= GFLG_RELRIGHT;
		break;
		
	    case GTYP_WDRAGGING:
	    	gadget->LeftEdge = 0;
		gadget->Width = 0;
		gadget->Flags &= ~GFLG_RELRIGHT;
		gadget->Flags |= GFLG_RELWIDTH;
		break;
	}
	
	if (msg->wdp_Flags & WDF_LBG_MULTIPLE)
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

IPTR WinDecorClass__WDM_DRAW_BORDERPROPBACK(Class *cl, Object *obj, struct wdpDrawBorderPropBack *msg)
{
    struct windecor_data *data = INST_DATA(cl, obj);
    struct Window   	 *window = msg->wdp_Window;
    struct RastPort 	 *rp = msg->wdp_RPort;
    struct Gadget   	 *gadget = msg->wdp_Gadget;
    struct Rectangle	 *r = msg->wdp_RenderRect;
    struct PropInfo 	 *pi = ((struct PropInfo *)gadget->SpecialInfo);
    UWORD   	    	 *pens = DRI(data->dri)->dri_Pens;

    SetDrMd(rp, JAM2);
    
    if (pi->Flags & PROPNEWLOOK)
    {
        static UWORD pattern[] = {0x5555,0xAAAA};
	
    	SetAfPt(rp, pattern, 1);
	SetAPen(rp, pens[SHADOWPEN]);
	SetBPen(rp, pens[(window->Flags & WFLG_WINDOWACTIVE) ? FILLPEN : BACKGROUNDPEN]);	
	RectFill(rp, r->MinX, r->MinY, r->MaxX, r->MaxY);
    	SetAfPt(rp, NULL, 0);
    }
    else
    {
    	SetAPen(rp, pens[BACKGROUNDPEN]);
	RectFill(rp, r->MinX, r->MinY, r->MaxX, r->MaxY);
    }
    
    return TRUE;
}

/**************************************************************************************************/

IPTR WinDecorClass__WDM_DRAW_BORDERPROPKNOB(Class *cl, Object *obj, struct wdpDrawBorderPropKnob *msg)
{
    struct windecor_data *data = INST_DATA(cl, obj);
    struct Window   	 *window = msg->wdp_Window;
    struct RastPort 	 *rp = msg->wdp_RPort;
    struct Gadget   	 *gadget = msg->wdp_Gadget;
    struct Rectangle	 *r = msg->wdp_RenderRect;
    struct PropInfo 	 *pi = ((struct PropInfo *)gadget->SpecialInfo);
    UWORD   	    	 *pens = DRI(data->dri)->dri_Pens;

    SetDrMd(rp, JAM2);
    
    if (pi->Flags & PROPBORDERLESS)
    {
        SetAPen(rp, pens[SHINEPEN]);

        /* Top edge */
        RectFill(rp, r->MinX, r->MinY, r->MaxX - 1, r->MinY);

        /* Left edge */
        RectFill(rp, r->MinX, r->MinY + 1, r->MinX, r->MaxY - 1);

        SetAPen(rp, pens[SHADOWPEN]);

        /* Right edge */
        RectFill(rp, r->MaxX, r->MinY, r->MaxX, r->MaxY);

        /* Bottom edge */
        RectFill(rp, r->MinX, r->MaxY, r->MaxX - 1, r->MaxY);

    	r->MinX++;
	r->MinY++;
	r->MaxX--;
	r->MaxY--;

    } /* PROPBORDERLESS */
    else
    {
        SetAPen(rp, pens[SHADOWPEN]);

        if (pi->Flags & FREEHORIZ)
        {
            /* black line at the left and at the right */

            RectFill(rp, r->MinX, r->MinY, r->MinX, r->MaxY);
            RectFill(rp, r->MaxX, r->MinY, r->MaxX, r->MaxY);

    	    r->MinX++;
	    r->MaxX--;
	    
        }

        if (pi->Flags & FREEVERT)
        {
            /* black line at the top and at the bottom */

            RectFill(rp, r->MinX, r->MinY, r->MaxX, r->MinY);
            RectFill(rp, r->MinX, r->MaxY, r->MaxX, r->MaxY);

            r->MinY++,
	    r->MaxY--;
        }


    } /* not PROPBORDERLESS */


    SetAPen(rp, pens[(window->Flags & WFLG_WINDOWACTIVE) ? FILLPEN : BACKGROUNDPEN]);

    /* interior */
    RectFill(rp, r->MinX, r->MinY, r->MaxX, r->MaxY);

    return TRUE;
}


/**************************************************************************************************/
