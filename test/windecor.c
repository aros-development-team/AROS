#include <dos/dos.h>
#include <intuition/classusr.h>
#include <intuition/windecorclass.h>
#include <intuition/scrdecorclass.h>
#include <graphics/rpattr.h>
#include <graphics/gfxmacros.h>
#include <utility/tagitem.h>

#include <proto/exec.h>
#include <proto/graphics.h>
#include <proto/layers.h>
#include <proto/intuition.h>
#include <proto/utility.h>
#include <proto/cybergraphics.h>
#include <clib/alib_protos.h>

#include <stdio.h>

#include <aros/debug.h>

/**************************************************************************************************/

struct IClass 	*cl, *scrcl;
Object	      	*thisdecorobj, *olddecorobj;
Object	      	*thisscrdecorobj, *oldscrdecorobj;
struct Screen 	*scr;
struct DrawInfo *dri;

#define ACTIVE_1    0x6c7be9
#define ACTIVE_2    0x00006e
#define INACTIVE_1  0xeeeeee
#define INACTIVE_2  0x888888

/**************************************************************************************************/

struct windecor_data
{
    struct DrawInfo *dri;
    struct Screen *scr;
};

/**************************************************************************************************/

static void CheckRectFill(struct RastPort *rp, LONG x1, LONG y1, LONG x2, LONG y2)
{
    ULONG fgcolor;
    ULONG bgcolor;
    LONG y;
    
    if ((x2 < x1) || (y2 < y1)) return;
    
    GetRPAttrs(rp, RPTAG_FgColor, &fgcolor, RPTAG_BgColor, &bgcolor, TAG_DONE);
    
    if ((fgcolor == bgcolor) || (y1 == y2))
    {
    	RectFill(rp, x1, y1, x2, y2);
    }
    else
    {
	LONG r1 = (fgcolor >> 16) & 0xFF;
	LONG g1 = (fgcolor >> 8) & 0xFF;
    	LONG b1 = fgcolor & 0xFF;
	LONG r2 = (bgcolor >> 16) & 0xFF;
	LONG g2 = (bgcolor >> 8) & 0xFF;
    	LONG b2 = bgcolor & 0xFF;
	
	for(y = y1; y <= y2; y++)
	{
	    LONG r, g, b, rgb;
	    LONG mul = (y - y1);
	    LONG div = (y2 - y1);
	    
	    r = r1 + (r2 - r1) * mul / div;
	    g = g1 + (g2 - g1) * mul / div;
	    b = b1 + (b2 - b1) * mul / div;
	    
	    rgb = (r << 16) + (g << 8) + b;
	    
    	    SetRPAttrs(rp, RPTAG_FgColor, rgb, TAG_DONE);
	    RectFill(rp, x1, y, x2, y);

	}
	
	SetRPAttrs(rp, RPTAG_FgColor, fgcolor, TAG_DONE);
		
    }
}

/**************************************************************************************************/

static IPTR windecor_new(Class *cl, Object *obj, struct opSet *msg)
{
    struct windecor_data *data;

    obj = (Object *)DoSuperMethodA(cl, obj, (Msg)msg);
    if (obj)
    {
    	data = INST_DATA(cl, obj);
	
	data->dri = (struct DrawInfo *)GetTagData(WDA_DrawInfo, 0, msg->ops_AttrList);
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

static IPTR windecor_get(Class *cl, Object *obj, struct opGet *msg)
{
    //struct windecor_data *data = INST_DATA(cl, obj);

    switch(msg->opg_AttrID)
    {
	case WDA_TrueColorOnly:
	    *msg->opg_Storage = TRUE;
	    break;
	    
	default:
	    return DoSuperMethodA(cl, obj, (Msg)msg);
    }
    
    return 1;    
}
    

/**************************************************************************************************/

#define HSPACING 3
#define VSPACING 3
#define HSPACING_MIDDLE 2
#define VSPACING_MIDDLE 2
#define HSPACING_SMALL 1
#define VSPACING_SMALL 1

static void renderimageframe(struct RastPort *rp, ULONG which, ULONG state, UWORD *pens,
                             WORD left, WORD top, WORD width, WORD height)
{
    WORD right = left + width - 1;
    WORD bottom = top + height - 1;
 
    SetAPen(rp, pens[SHADOWPEN]);

    left--;top--;
    right++;bottom++;
    
    /* left edge */
    RectFill(rp, left,
             top,
             left,
             bottom);

    /* top edge */
    RectFill(rp, left + 1,
             top,
             right,
             top);

    SetAPen(rp, pens[SHINEPEN]);

    /* right edge */
    RectFill(rp, right,
             top + 1,
             right,
             bottom);

    /* bottom edge */
    RectFill(rp, left + 1,
             bottom,
             right - 1,
             bottom);

    left++;top++;
    right--;bottom--;
    
    SetAPen(rp, pens[((state == IDS_SELECTED) || (state == IDS_INACTIVESELECTED)) ? SHADOWPEN : SHINEPEN]);

    /* left edge */
    RectFill(rp, left,
             top,
             left,
             bottom);

    /* top edge */
    RectFill(rp, left + 1,
             top,
             right,
             top);

    SetAPen(rp, pens[((state == IDS_SELECTED) || (state == IDS_INACTIVESELECTED)) ? SHINEPEN : SHADOWPEN]);

    /* right edge */
    RectFill(rp, right,
             top + 1,
             right,
             bottom);

    /* bottom edge */
    RectFill(rp, left + 1,
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

static void drawrect(struct RastPort *rp, WORD x1, WORD y1, WORD x2, WORD y2)
{
    Move(rp, x1, y1);

    /* We RectFill() because it is generally faster than Draw()
       (Draw() uses Set/GetPixel() while RectFill() can do higherlevel
       clipping. Also it is MUCH faster in the x11gfx hidd.
    */

    RectFill(rp, x1, y1, x2 - 1, y1);   /* Top      */
    RectFill(rp, x2, y1, x2, y2 - 1);   /* Right    */
    RectFill(rp, x1 + 1, y2, x2, y2);   /* Bottom   */
    RectFill(rp, x1, y1 + 1, x1, y2);   /* Left     */

    return;
}

/**************************************************************************************************/

IPTR windecor_draw_sysimage(Class *cl, Object *obj, struct wdpDrawSysImage *msg)
{
    struct windecor_data *data = INST_DATA(cl, obj);
    struct RastPort 	 *rp = msg->wdp_RPort;
    UWORD   	    	 *pens = data->dri->dri_Pens;
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
	    left += 3; top += 3;
	    right -= 3; bottom -= 3;
	    width -= 6; height -= 6;
	    
	    renderimageframe(rp, CLOSEIMAGE, state, pens, left, top, width, height);
	    left++;
	    top++;
	    width -= 2;
	    height -= 2;
	    
	    right = left + width - 1;
	    bottom = top + height - 1;
	    h_spacing = width * 4 / 10;
	    v_spacing = height * 3 / 10;
	    
	#if 0
	    SetAPen(rp, getbgpen(state, pens));
	    RectFill(rp, left, top, right, bottom);
	#endif
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

	    left += 3; top += 3;
	    right -= 3; bottom -= 3;
	    width -= 6; height -= 6;

            renderimageframe(rp, ZOOMIMAGE, state, pens,
                             left, top, width, height);
            left++;
            top++;
            width -= 2;
            height -= 2;
 
            right = left + width - 1;
            bottom = top + height - 1 ;
            h_spacing = width / 6;
            v_spacing = height / 6;

            bg = getbgpen(state, pens);

    	#if 0
            /* Clear background into correct color */
            SetAPen(rp, bg);
            RectFill(rp, left, top, right, bottom);
    	#endif
	
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

	    left += 3; top += 3;
	    right -= 3; bottom -= 3;
	    width -= 6; height -= 6;

            renderimageframe(rp, DEPTHIMAGE, state, pens,
                             left, top, width, height);
            left++;
            top++;
            right--;
            bottom--;
            width -= 2;
            height -= 2;
 
            h_spacing = width / 6;
            v_spacing = height / 6;

            bg = getbgpen(state, pens);

    	#if 0
            /* Clear background into correct color */
            SetAPen(rp, bg);
            RectFill(rp, left, top, right, bottom);
    	#endif
	
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
                     , bottom - (height / 3));


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
	    	    	 right, bottom);

            /* Fill bottom right window (inside of the frame above) */
            SetAPen(rp, pens[(state == IDS_INACTIVENORMAL) ? BACKGROUNDPEN : SHINEPEN]);
            RectFill(rp, left + (width / 3) + 1, top + (height / 3) + 1,
	    	    	 right - 1, bottom - 1);

            if (state == IDS_SELECTED)
            {
                /* Re-Render top left window  */

                SetAPen(rp, pens[SHADOWPEN]);
                drawrect(rp, left, top,
		    	     right - (width / 3 ), bottom - (height / 3));
            }
            break;
        }

    	case SIZEIMAGE:
        {
            UWORD  bg;
            WORD   h_spacing;
            WORD   v_spacing;
            WORD   x, y;

    	#if 0
            renderimageframe(rp, SIZEIMAGE, state, pens,
                             left, top, width, height);
	#endif
            left++;
            top++;
            right--;
            bottom--;
            width -= 2;
            height -= 2;

            h_spacing = width  / 5;
            v_spacing = height / 5;

            bg = getbgpen(state, pens);

    	#if 0
            /* Clear background into correct color */
            SetAPen(rp, bg);
            RectFill(rp, left, top, right, bottom);
    	#endif
	
            /* A triangle image */

            left += h_spacing;
            top  += v_spacing;

            right  = left + width  - 1 - (h_spacing * 2);
            bottom = top  + height - 1 - (v_spacing * 2);

            width  = right  - left + 1;
            height = bottom - top  + 1;

            if (state != IDS_INACTIVENORMAL)
            {
                SetAPen(rp, pens[state == IDS_SELECTED ? BACKGROUNDPEN : SHINEPEN]);
		
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
            WORD  cy, i, j;

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

    	#if 0
            renderimageframe(rp, LEFTIMAGE, state, pens,
                             left, top, width, height);
    	#endif
	
            left++;
            top++;
            right--;
            bottom--;
            width -= 2;
            height -= 2;

    	#if 0
            SetAPen(rp, getbgpen(state, pens));
            RectFill(rp, left, top, right, bottom);
    	#endif
	
            left += hspacing;
            top += vspacing;
            width -= hspacing * 2;
            height -= vspacing * 2;

            right = left + width - 1;
            bottom = top + height - 1;

            cy = (height + 1) / 2;

            SetAPen(rp, pens[SHADOWPEN]);

    	    for(j = 0; j < 2; j++)
	    {
        	for(i = 0; i < cy; i++)
        	{
                    RectFill(rp, 1 - j + left + (cy - i - 1) * width / cy,
                             1 - j + top + i,
                             1 - j + right - i * width / cy / 2,
                             1 - j + top + i);
                    RectFill(rp, 1 - j + left + (cy - i - 1) * width / cy,
                             1 - j + bottom - i,
                             1 - j + right - i * width / cy / 2,
                             1 - j + bottom - i);

        	}
                SetAPen(rp, pens[state == IDS_SELECTED ? BACKGROUNDPEN : SHINEPEN]);
	    }
            break;
        }

    	case UPIMAGE:
        {
            UWORD hspacing,vspacing;
            WORD  cx, i, j;

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

    	#if 0
            renderimageframe(rp, UPIMAGE, state, pens,
                             left, top, width, height);
    	#endif
	
            left++;
            top++;
            right--;
            bottom--;
            width -= 2;
            height -= 2;

    	#if 0
            SetAPen(rp, getbgpen(state, pens));
            RectFill(rp, left, top, right, bottom);
	#endif

            left += hspacing;
            top += vspacing;
            width -= hspacing * 2;
            height -= vspacing * 2;

            right = left + width - 1;
            bottom = top + height - 1;

            cx = (width + 1) / 2;

            SetAPen(rp, pens[SHADOWPEN]);

    	    for(j = 0; j < 2; j++)
	    {
        	for(i = 0; i < cx; i++)
        	{
                    RectFill(rp, 1 - j + left + i,
                             1 - j + top + (cx - i - 1) * height / cx,
                             1 - j + left + i,
                             1 - j + bottom - i * height / cx / 2);
                    RectFill(rp, 1 - j + right - i,
                             1 - j + top + (cx - i - 1) * height / cx,
                             1 - j + right - i,
                             1 - j + bottom - i * height / cx / 2);
        	}
		SetAPen(rp, pens[state == IDS_SELECTED ? BACKGROUNDPEN : SHINEPEN]);
    	    }
            break;
        }

    	case RIGHTIMAGE:
        {
            UWORD hspacing,vspacing;
            WORD  cy, i, j;

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

    	#if 0
            renderimageframe(rp, RIGHTIMAGE, state, pens,
                             left, top, width, height);
	#endif
	
            left++;
            top++;
            right--;
            bottom--;
            width -= 2;
            height -= 2;


    	#if 0
            SetAPen(rp, getbgpen(state, pens));
            RectFill(rp, left, top, right, bottom);
	#endif

            left += hspacing;
            top += vspacing;
            width -= hspacing * 2;
            height -= vspacing * 2;

            right = left + width - 1;
            bottom = top + height - 1;

            cy = (height + 1) / 2;

            SetAPen(rp, pens[SHADOWPEN]);

    	    for(j = 0; j < 2; j++)
	    {
        	for(i = 0; i < cy; i++)
        	{
                    RectFill(rp, 1 - j + left + i * width / cy / 2,
                             1 - j + top + i,
                             1 - j + right - (cy - i - 1) * width / cy,
                             1 - j + top + i);
                    RectFill(rp, 1 - j + left + i * width / cy / 2,
                             1 - j + bottom - i,
                             1 - j + right - (cy - i - 1) * width / cy,
                             1 - j + bottom - i);
        	}
	    	SetAPen(rp, pens[state == IDS_SELECTED ? BACKGROUNDPEN : SHINEPEN]);
	    }
            break;
        }

    	case DOWNIMAGE:
        {
            UWORD hspacing,vspacing;
            WORD  cx, i, j;
 
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

    	#if 0
            renderimageframe(rp, DOWNIMAGE, state, pens,
                             left, top, width, height);
	#endif
	
            left++;
            top++;
            right--;
            bottom--;
            width -= 2;
            height -= 2;

    	#if 0
            SetAPen(rp, getbgpen(state, pens));
            RectFill(rp, left, top, right, bottom);
    	#endif

            left += hspacing;
            top += vspacing;
            width -= hspacing * 2;
            height -= vspacing * 2;

            right = left + width - 1;
            bottom = top + height - 1;

            cx = (width + 1) / 2;

            SetAPen(rp, pens[SHADOWPEN]);

    	    for(j = 0; j < 2; j++)
	    {
        	for(i = 0; i < cx; i++)
        	{
                    RectFill(rp, 1 - j + left + i,
                             1 - j + top + i * height / cx / 2,
                             1 - j + left + i,
                             1 - j + bottom - (cx - i - 1) * height / cx);
                    RectFill(rp, 1 - j + right - i,
                             1 - j + top + i * height / cx / 2,
                             1 - j + right -  i,
                             1 - j + bottom - (cx - i - 1) * height / cx);

        	}
	    	SetAPen(rp, pens[state == IDS_SELECTED ? BACKGROUNDPEN : SHINEPEN]);
	    }
            break;
        }

	
	default:
	    return DoSuperMethodA(cl, obj, (Msg)msg);
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
        if (g->Activation & GACT_TOPBORDER &&
	    (g->GadgetType & GTYP_SYSTYPEMASK) != GTYP_WDRAGGING)
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

IPTR windecor_draw_winborder(Class *cl, Object *obj, struct wdpDrawWinBorder *msg)
{
    //struct windecor_data *data = INST_DATA(cl, obj);
    struct RastPort 	 *rp = msg->wdp_RPort;
    struct Window   	 *window = msg->wdp_Window;
    //LONG    	    	  left, right;
    
    SetDrMd(rp, JAM1);
    SetRPAttrs(rp, RPTAG_FgColor, 0xFFFFFF, RPTAG_BgColor, 0xFFFFFF, TAG_DONE);
    
    if (window->BorderTop > 0)
    {
    	/* Outer shine edge on top side */
	
    	CheckRectFill(rp, 0, 0, window->Width - 1, 0);
    }

    if (!(msg->wdp_Flags & WDF_DWB_TOP_ONLY))
    {
	if (window->BorderLeft > 0)
	{
	    /* Outer shine edge on left side */

	    CheckRectFill(rp, 0, 0, 0, window->Height - 1);
	}

	if (window->BorderRight > 1)
	{
	    /* Inner shine edge on right side */

	    CheckRectFill(rp,
	    		  window->Width - window->BorderRight, window->BorderTop,
                	  window->Width - window->BorderRight, window->Height - window->BorderBottom);
	}

	if (window->BorderBottom > 1)
	{
	    /* Inner shine edge on bottom side */

	    CheckRectFill(rp,
                	  window->BorderLeft, window->Height - window->BorderBottom,
                	  window->Width - window->BorderRight, window->Height - window->BorderBottom);
	}
    }
    
    SetRPAttrs(rp, RPTAG_FgColor, 0x0, RPTAG_BgColor, 0x0, TAG_DONE);
    
    if (!(msg->wdp_Flags & WDF_DWB_TOP_ONLY))
    {
	if (window->BorderRight > 0)
	{
    	    /* Outer shadow edge on right side */

    	    CheckRectFill(rp, window->Width - 1, 1,
                	  window->Width - 1, window->Height - 1);
	}

	if (window->BorderBottom > 0)
	{
    	    /* Outer shadow edge on bottom side */

    	    CheckRectFill(rp, 1, window->Height - 1,
	    		  window->Width - 1, window->Height - 1);
	}

	if (window->BorderLeft > 1)
	{
    	    /* Inner shadow edge on left side */

    	    CheckRectFill(rp, window->BorderLeft - 1, window->BorderTop - 1,
                	  window->BorderLeft - 1, window->Height - window->BorderBottom);
	}
	
    }
    
    if (window->BorderTop > 1)
    {
    	/* Inner shadow edge on top side */
	
	CheckRectFill(rp, window->BorderLeft - 1, window->BorderTop - 1,
                      window->Width - window->BorderRight, window->BorderTop - 1);
    }
    
    if ((window->Flags & WFLG_WINDOWACTIVE))
    {
    	SetRPAttrs(rp, RPTAG_FgColor, ACTIVE_1, RPTAG_BgColor, ACTIVE_2, TAG_DONE);
    }
    else
    {
    	SetRPAttrs(rp, RPTAG_FgColor, INACTIVE_1, RPTAG_BgColor, INACTIVE_2, TAG_DONE);
    }
    
    if (window->BorderTop > 2)
    {
    	/* Fill on top side */
	
    	CheckRectFill(rp, 1, 1, window->Width - 2, window->BorderTop - 2);
    }

    if (!(msg->wdp_Flags & WDF_DWB_TOP_ONLY))
    {
	if ((window->Flags & WFLG_WINDOWACTIVE))
	{
    	    SetRPAttrs(rp, RPTAG_FgColor, ACTIVE_2, RPTAG_BgColor, ACTIVE_1, TAG_DONE);
	}
	else
	{
    	    SetRPAttrs(rp, RPTAG_FgColor, INACTIVE_2, RPTAG_BgColor, INACTIVE_1, TAG_DONE);
	}

	if (window->BorderLeft > 2)
	{
    	    /* Fill on left side */

    	    CheckRectFill(rp, 1, window->BorderTop, window->BorderLeft - 2, window->Height - 2);

	}

	if (window->BorderRight > 2)
	{
    	    /* Fill on right side */

    	    CheckRectFill(rp, window->Width - window->BorderRight + 1, window->BorderTop,
                	  window->Width - 2, window->Height - 2);
	}

	if ((window->Flags & WFLG_WINDOWACTIVE))
	{
    	    SetRPAttrs(rp, RPTAG_FgColor, ACTIVE_1, RPTAG_BgColor, ACTIVE_2, TAG_DONE);
	}
	else
	{
    	    SetRPAttrs(rp, RPTAG_FgColor, INACTIVE_1, RPTAG_BgColor, INACTIVE_2, TAG_DONE);
	}

	if (window->BorderBottom > 2)
	{
    	    /* Fill on bottom side */

	    CheckRectFill(rp, 1, window->Height - window->BorderBottom + 1,
                	  window->Width - 2, window->Height - 2);
	}
    }
    
#if 0    
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
#endif

    return TRUE;
}

/**************************************************************************************************/

IPTR windecor_draw_wintitle(Class *cl, Object *obj, struct wdpDrawWinTitle *msg)
{
    struct windecor_data *data = INST_DATA(cl, obj);
    struct RastPort 	 *rp = msg->wdp_RPort;
    struct Window   	 *window = msg->wdp_Window;
    UWORD   	    	 *pens = data->dri->dri_Pens;
    LONG    	    	  right, left;

    findtitlearea(window, &left, &right);

    SetDrMd(rp, JAM1);
    if (window->Flags & WFLG_WINDOWACTIVE)
    {
    	SetRPAttrs(rp, RPTAG_FgColor, ACTIVE_1, RPTAG_BgColor, ACTIVE_2, TAG_DONE);
    }
    else
    {
    	SetRPAttrs(rp, RPTAG_FgColor, INACTIVE_1, RPTAG_BgColor, INACTIVE_2, TAG_DONE);
    }
    
    SetAPen(rp, pens[(window->Flags & WFLG_WINDOWACTIVE) ? FILLPEN : BACKGROUNDPEN]);
    CheckRectFill(rp, left + 1, 1, right - 1, window->BorderTop - 2);
        
    if (right - left > 6)
    {
        ULONG   	    	textlen, titlelen, textpixellen;
        struct TextExtent 	te;

        SetFont(rp, data->dri->dri_Font);

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

    	    SetAPen(rp, pens[SHADOWPEN]);
	    Move(rp, left + 1, data->dri->dri_Font->tf_Baseline + 2 + 1);
            Text(rp, window->Title, textlen);

    	    SetAPen(rp, pens[SHINEPEN]);
	    Move(rp, left, data->dri->dri_Font->tf_Baseline + 2);
            Text(rp, window->Title, textlen);

	}
    }
    
    return TRUE;
}

/**************************************************************************************************/

IPTR windecor_layout_bordergadgets(Class *cl, Object *obj, struct wdpLayoutBorderGadgets *msg)
{
    //struct windecor_data *data = INST_DATA(cl, obj);
    struct Window   	 *window = msg->wdp_Window;
    struct Gadget   	 *gadget = msg->wdp_Gadgets;
    BOOL    	    	  hasdepth;
    BOOL    	    	  haszoom;
    BOOL    	    	  hasclose;
    
    DoSuperMethodA(cl, obj, (Msg)msg);
    
    hasclose = (window->Flags & WFLG_CLOSEGADGET) ? TRUE : FALSE;
    hasdepth = (window->Flags & WFLG_DEPTHGADGET) ? TRUE : FALSE;
    haszoom = ((window->Flags & WFLG_HASZOOM) ||
    	       ((window->Flags & WFLG_SIZEGADGET) && hasdepth)) ? TRUE : FALSE;
	       
    while(gadget)
    {
    	switch(gadget->GadgetType & GTYP_SYSTYPEMASK)
	{
	    case GTYP_CLOSE:
	    	gadget->LeftEdge = -gadget->Height + 1;
		gadget->Width = gadget->Height;
		gadget->Flags &= ~GFLG_RELWIDTH;
		gadget->Flags |= GFLG_RELRIGHT;
	    	break;
		
	    case GTYP_WDEPTH:
	    	gadget->LeftEdge = -gadget->Height * (hasclose ? 2 : 1) - hasclose * 3 + 1;
		gadget->Width = gadget->Height;
		gadget->Flags &= ~GFLG_RELWIDTH;
		gadget->Flags |= GFLG_RELRIGHT;
		break;
		
	    case GTYP_WZOOM:
	    	gadget->LeftEdge = -gadget->Height * (hasclose ? 3 : 1) - hasclose * 3 + 1;
		gadget->Width = gadget->Height;
		gadget->Flags &= ~GFLG_RELWIDTH;
		gadget->Flags |= GFLG_RELRIGHT;
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

IPTR windecor_draw_borderpropback(Class *cl, Object *obj, struct wdpDrawBorderPropBack *msg)
{
    struct windecor_data *data = INST_DATA(cl, obj);
    struct Window   	 *window = msg->wdp_Window;
    struct RastPort 	 *rp, *winrp = msg->wdp_RPort;
    struct Gadget   	 *gadget = msg->wdp_Gadget;
    struct Rectangle	 *r = msg->wdp_RenderRect;
    struct PropInfo 	 *pi = ((struct PropInfo *)gadget->SpecialInfo);
    UWORD   	    	 *pens = data->dri->dri_Pens;

    if (!(pi->Flags & PROPNEWLOOK))
    {
    	return DoSuperMethodA(cl, obj, (Msg)msg);
    }

    if ((rp = CloneRastPort(winrp)))
    {
    	struct TagItem rptags[] =
	{
	    {RPTAG_ClipRectangle    	, (IPTR)msg->wdp_RenderRect },
	    {RPTAG_ClipRectangleFlags	, 0    	    	    	    },
	    {RPTAG_DrMd     	    	, JAM2	    	    	    },
	    {TAG_DONE	    	    	    	    	    	    }
	};
	
	SetRPAttrsA(rp, rptags);
	
	r = msg->wdp_PropRect;
	
	SetAPen(rp, pens[SHADOWPEN]);
	RectFill(rp, r->MinX, r->MinY, r->MaxX, r->MinY);
	RectFill(rp, r->MinX, r->MinY + 1, r->MinX, r->MaxY);
	SetAPen(rp, pens[SHINEPEN]);
	RectFill(rp, r->MaxX, r->MinY + 1, r->MaxX, r->MaxY);
	RectFill(rp, r->MinX + 1, r->MaxY, r->MaxX - 1, r->MaxY);

	SetAPen(rp, pens[(window->Flags & WFLG_WINDOWACTIVE) ? SHADOWPEN : BACKGROUNDPEN]);
	RectFill(rp, r->MinX + 1, r->MinY + 1, r->MaxX - 1, r->MaxY - 1);
    
    	FreeRastPort(rp);
		
    }
    
    return TRUE;
}

/**************************************************************************************************/

IPTR windecor_draw_borderpropknob(Class *cl, Object *obj, struct wdpDrawBorderPropKnob *msg)
{
    struct windecor_data *data = INST_DATA(cl, obj);
    struct Window   	 *window = msg->wdp_Window;
    struct RastPort 	 *rp = msg->wdp_RPort;
    struct Gadget   	 *gadget = msg->wdp_Gadget;
    struct Rectangle	 *r = msg->wdp_RenderRect;
    struct PropInfo 	 *pi = ((struct PropInfo *)gadget->SpecialInfo);
    UWORD   	    	 *pens = data->dri->dri_Pens;
    BOOL    	    	  hit = (msg->wdp_Flags & WDF_DBPK_HIT) ? TRUE : FALSE;
    
    if (!(pi->Flags & PROPBORDERLESS))
    {
    	return DoSuperMethodA(cl, obj, (Msg)msg);
    }
    
    SetDrMd(rp, JAM2);

    SetAPen(rp, pens[hit ? SHADOWPEN : SHINEPEN]);

    /* Top edge */
    RectFill(rp, r->MinX, r->MinY, r->MaxX - 1, r->MinY);

    /* Left edge */
    RectFill(rp, r->MinX, r->MinY + 1, r->MinX, r->MaxY - 1);

    SetAPen(rp, pens[hit ? SHINEPEN : SHADOWPEN]);

    /* Right edge */
    RectFill(rp, r->MaxX, r->MinY, r->MaxX, r->MaxY);

    /* Bottom edge */
    RectFill(rp, r->MinX, r->MaxY, r->MaxX - 1, r->MaxY);

    r->MinX++;
    r->MinY++;
    r->MaxX--;
    r->MaxY--;

    if ((window->Flags & WFLG_WINDOWACTIVE))
    {
    	ULONG col = hit ? 0xb9b6ff : 0xadaaee;

    	SetRPAttrs(rp, RPTAG_FgColor, col, TAG_DONE);	
    }
    else
    {
    	SetAPen(rp, pens[BACKGROUNDPEN]);
    }

    /* interior */
    RectFill(rp, r->MinX, r->MinY, r->MaxX, r->MaxY);

    return TRUE;
}

/**************************************************************************************************/

IPTR windecor_dispatcher(struct IClass *cl, Object *obj, Msg msg)
{
    IPTR retval;
    
    switch(msg->MethodID)
    {
    	case OM_NEW:
	    retval = windecor_new(cl, obj, (struct opSet *)msg);
	    break;

    	case OM_GET:
	    retval = windecor_get(cl, obj, (struct opGet *)msg);
	    break;
	    
	case WDM_DRAW_SYSIMAGE:
	    retval = windecor_draw_sysimage(cl, obj, (struct wdpDrawSysImage *)msg);
	    break;
	    
	case WDM_DRAW_WINBORDER:
	    retval = windecor_draw_winborder(cl, obj, (struct wdpDrawWinBorder *)msg);
	    break;

	case WDM_DRAW_WINTITLE:
	    retval = windecor_draw_wintitle(cl, obj, (struct wdpDrawWinTitle *)msg);
	    break;
	    
	case WDM_LAYOUT_BORDERGADGETS:
	    retval = windecor_layout_bordergadgets(cl, obj, (struct wdpLayoutBorderGadgets *)msg);
	    break;
	    
	case WDM_DRAW_BORDERPROPBACK:
	    retval = windecor_draw_borderpropback(cl, obj, (struct wdpDrawBorderPropBack *)msg);
	    break;
	    
	case WDM_DRAW_BORDERPROPKNOB:
	    retval = windecor_draw_borderpropknob(cl, obj, (struct wdpDrawBorderPropKnob *)msg);
	    break;
	    
	default:
	    retval = DoSuperMethodA(cl, obj, msg);
	    break;
	    
    }
    
    return retval;
}

/**************************************************************************************************/

struct scrdecor_data
{
    struct DrawInfo *dri;
    struct Screen *scr;
};

/**************************************************************************************************/

static IPTR scrdecor_new(Class *cl, Object *obj, struct opSet *msg)
{
    struct scrdecor_data *data;

    obj = (Object *)DoSuperMethodA(cl, obj, (Msg)msg);
    if (obj)
    {
    	data = INST_DATA(cl, obj);
	
	data->dri = (struct DrawInfo *)GetTagData(SDA_DrawInfo, 0, msg->ops_AttrList);
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
    //struct scrdecor_data *data = INST_DATA(cl, obj);

    switch(msg->opg_AttrID)
    {
	case SDA_TrueColorOnly:
	    *msg->opg_Storage = TRUE;
	    break;
	    
	default:
	    return DoSuperMethodA(cl, obj, (Msg)msg);
    }
    
    return 1;    
}

/**************************************************************************************************/

static void findscrtitlearea(struct Screen *scr, LONG *left, LONG *right)
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

ULONG coltab[] =
{
    0xEEEEEE,
    0xFFFFFF,
    0xFFFFFF,
    0xFFFFFF,
    0xEEEEEE,
    0xFFFFFF,
    0xFFFFFF,
    0xFFFFFF,
    0xEEEEEE,
    0xFFFFFF,
    0xFFFFFF,
    0xFFFFFF,
    0xDDDDDD,
    0x777777
};

/**************************************************************************************************/

IPTR scrdecor_draw_screenbar(Class *cl, Object *obj, struct sdpDrawScreenBar *msg)
{
    struct scrdecor_data *data = INST_DATA(cl, obj);
    struct RastPort 	 *rp = msg->sdp_RPort;
    UWORD   	    	 *pens = data->dri->dri_Pens;
    LONG    	    	  left, right, y;
   
    SetDrMd(rp, JAM1);
    for(y = 0; y <= data->scr->BarHeight; y++)
    {
    	ULONG col;
	
	col = coltab[y * (sizeof(coltab) / sizeof(coltab[0])) / data->scr->BarHeight];
	
    	FillPixelArray(rp, 0, y, data->scr->Width, 1, col);
    }
    
    findscrtitlearea(data->scr, &left, &right);

    SetAPen(rp, pens[SHADOWPEN]);
    RectFill(rp, right, 1, right, data->scr->BarHeight - 1);
        
    FillPixelArray(rp, data->scr->BarHBorder + 4, 4,
    	    	       data->scr->BarHeight - 8, data->scr->BarHeight - 8,
		       0);
    FillPixelArray(rp, data->scr->BarHBorder + 4, 4,
    	    	       data->scr->BarHeight - 8, 1,
		       0x777777);
    FillPixelArray(rp, data->scr->BarHBorder + 4, 4,
    	    	       1, data->scr->BarHeight - 8,
		       0x777777);
    FillPixelArray(rp, data->scr->BarHBorder + 5, 5,
    	    	       data->scr->BarHeight - 9, data->scr->BarHeight - 9,
		       0x0b750e);
    
    return TRUE;
}

/**************************************************************************************************/

IPTR scrdecor_draw_screentitle(Class *cl, Object *obj, struct sdpDrawScreenTitle *msg)
{
    struct scrdecor_data *data = INST_DATA(cl, obj);
    struct RastPort 	 *rp = msg->sdp_RPort;
    LONG    	    	  right, left, y;
    WORD    	    	  oldspacing;
    
    findscrtitlearea(data->scr, &left, &right);

    SetDrMd(rp, JAM1);
    for(y = 0; y <= data->scr->BarHeight; y++)
    {
    	ULONG col;
	
	col = coltab[y * (sizeof(coltab) / sizeof(coltab[0])) / data->scr->BarHeight];
	
    	FillPixelArray(rp, data->scr->BarHBorder + data->scr->BarHeight, y,
	    	    	   (right - 1) - (data->scr->BarHBorder + data->scr->BarHeight) + 1, 1, col);
    }

    oldspacing = rp->TxSpacing;
    rp->TxSpacing = 1;

    Move(rp, data->scr->BarHBorder + data->scr->BarHeight + data->scr->BarHBorder, data->scr->BarVBorder + rp->TxBaseline);
    Text(rp, data->scr->Title, strlen(data->scr->Title));
    Move(rp, data->scr->BarHBorder + data->scr->BarHeight + data->scr->BarHBorder + 1, data->scr->BarVBorder + rp->TxBaseline);
    Text(rp, data->scr->Title, strlen(data->scr->Title));
    
    rp->TxSpacing = oldspacing;
        
    return TRUE;
}


/**************************************************************************************************/

IPTR scrdecor_dispatcher(struct IClass *cl, Object *obj, Msg msg)
{
    IPTR retval;
    
    switch(msg->MethodID)
    {
    	case OM_NEW:
	    retval = scrdecor_new(cl, obj, (struct opSet *)msg);
	    break;

    	case OM_GET:
	    retval = scrdecor_get(cl, obj, (struct opGet *)msg);
	    break;
	    
	case SDM_DRAW_SCREENBAR:
	    retval = scrdecor_draw_screenbar(cl, obj, (struct sdpDrawScreenBar *)msg);
	    break;
	    
	case SDM_DRAW_SCREENTITLE:
	    retval = scrdecor_draw_screentitle(cl, obj, (struct sdpDrawScreenTitle *)msg);
	    break;
	    
	default:
	    retval = DoSuperMethodA(cl, obj, msg);
	    break;
	    
    }
    
    return retval;
}

/**************************************************************************************************/

int main(void)
{
    scr = LockPubScreen(NULL);
    if (scr)
    {
    	dri = GetScreenDrawInfo(scr);
	if (dri)
	{
	    cl = MakeClass("testwindecor", WINDECORCLASS, NULL, sizeof(struct windecor_data), 0);
	    if (cl)
	    {
            	cl->cl_Dispatcher.h_Entry    = HookEntry;
            	cl->cl_Dispatcher.h_SubEntry = (HOOKFUNC)windecor_dispatcher;
		
		AddClass(cl);
	    	
		scrcl = MakeClass("testscrdecor", SCRDECORCLASS, NULL, sizeof(struct scrdecor_data), 0);
		if (scrcl)
		{
            	    scrcl->cl_Dispatcher.h_Entry    = HookEntry;
            	    scrcl->cl_Dispatcher.h_SubEntry = (HOOKFUNC)scrdecor_dispatcher;
		
		    AddClass(scrcl);
		
   	    	    olddecorobj = ChangeDecorationA(DECORATION_WINDOW, "testwindecor", dri, NULL);
		    if (!olddecorobj) puts("Coult not change window decoration!\n");

   	    	    oldscrdecorobj = ChangeDecorationA(DECORATION_SCREEN, "testscrdecor", dri, NULL);
		    if (!oldscrdecorobj) puts("Coult not change screen decoration!\n");
		    
		    if (olddecorobj || oldscrdecorobj)
		    {
		    	puts("Press CTRL-C to quit\n");
		    	Wait(SIGBREAKF_CTRL_C);
		    }
			
		    if (olddecorobj)
		    {
			thisdecorobj = ChangeDecorationA(DECORATION_WINDOW, OCLASS(olddecorobj)->cl_ID, dri, NULL);

			DisposeObject(olddecorobj);
			DisposeObject(thisdecorobj);
		    }

		    if (oldscrdecorobj)
		    {
			thisscrdecorobj = ChangeDecorationA(DECORATION_SCREEN, OCLASS(oldscrdecorobj)->cl_ID, dri, NULL);

			DisposeObject(oldscrdecorobj);
			DisposeObject(thisscrdecorobj);
		    }		    
		    
		    FreeClass(scrcl);
		}
		else puts("Could not create testscrdecor class!\n");
		
    		FreeClass(cl);
	    }
	    else puts("Coult not create testwindecor class!\n");
	    
	    FreeScreenDrawInfo(scr, dri);
	}
	else puts("Could not get DrawInfo!\n");
	
	UnlockPubScreen(NULL, scr);
    }
    else puts("Could not lock pub screen!");
    
    return 0;
}
