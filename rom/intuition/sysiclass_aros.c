/*
    Copyright © 1995-2005, The AROS Development Team. All rights reserved.
    Copyright © 2001-2003, The MorphOS Development Team. All Rights Reserved.
    $Id$
*/

/**************************************************************************************************/

#include <exec/types.h>

#include <proto/intuition.h>
#include <proto/layers.h>
#include <intuition/classes.h>
#include <intuition/classusr.h>
#include <intuition/imageclass.h>
#include <intuition/windecorclass.h>
#include <intuition/scrdecorclass.h>
#include <intuition/intuition.h>
#include <intuition/intuitionbase.h>
#include <intuition/extensions.h>
#include <intuition/screens.h>

#include <proto/graphics.h>
#include <graphics/rastport.h>

#include <proto/utility.h>
#include <utility/tagitem.h>

#include <proto/alib.h>

#include <aros/asmcall.h>
#include "intuition_intern.h"
#ifdef __MORPHOS__
#include "intuition_extend.h"
#endif

#include "gadgets.h" /* Some handy rendering funtions */

#define INTDRI(dri) ((struct IntDrawInfo *)(dri))

#if 0
extern BYTE *ibPrefs;
extern BYTE *ibSnapshot;
extern BYTE *ibSnapshotSel;
extern BYTE *ibPopupSel;
extern BYTE *ibPopup;
extern BYTE *ibIconify;
extern BYTE *ibIconifySel;
extern BYTE *ibLock;
extern BYTE *ibLockSel;
extern VOID DrawIB(struct RastPort *rp,BYTE *ib,LONG cx,LONG cy,struct IntuitionBase *IntuitionBase);
extern void DrawJUMP(struct RastPort *rp,ULONG state,LONG cx,LONG cy,struct IntuitionBase *IntuitionBase);
#endif

#undef DEBUG
#define DEBUG 0
#include <aros/debug.h>

/**************************************************************************************************/

#define DEFSIZE_WIDTH  14
#define DEFSIZE_HEIGHT 14

/**************************************************************************************************/

/* Some handy transparent base class object casting defines.
 */
#define IM(o) ((struct Image *)o)

static UWORD getbgpen(ULONG state, UWORD *pens);
static UWORD getbgpen_gt(ULONG state, UWORD *pens);
static void renderimageframe(struct RastPort *rp, ULONG which, ULONG state, UWORD *pens,
                             WORD left, WORD top, WORD width, WORD height,
                             struct IntuitionBase *IntuitionBase);

#undef IntuitionBase
#define IntuitionBase   ((struct IntuitionBase *)(cl->cl_UserData))


/**************************************************************************************************/

#define SYSIFLG_GADTOOLS 1
#define SYSIFLG_NOBORDER 2

/**************************************************************************************************/

/* Some handy drawing functions */

void draw_thick_line(Class *cl, struct RastPort *rport,
                     LONG x1, LONG y1, LONG x2, LONG y2,
                     UWORD thickness)
{
    Move(rport, x1, y1);
    Draw(rport, x2, y2);
    /* Georg Steger */
    Move(rport, x1 + 1, y1);
    Draw(rport, x2 + 1, y2);
}

/**************************************************************************************************/

BOOL sysi_setnew(Class *cl, Object *obj, struct opSet *msg)
{
    struct SysIData 	*data = INST_DATA(cl, obj);
    struct TagItem  	*taglist, *tag;
    struct TextFont 	*reffont = NULL;
    int     	     	 size = SYSISIZE_MEDRES;
    BOOL    	     	 unsupported = FALSE;
    BOOL    	     	 set_width = FALSE, set_height = FALSE;

    taglist = msg->ops_AttrList;
    while ((tag = NextTagItem((const struct TagItem **)&taglist)))
    {
        switch(tag->ti_Tag)
        {
            case IA_Width:
        	set_width = TRUE;
        	break;

            case IA_Height:
        	set_height = TRUE;
        	break;

            case SYSIA_DrawInfo:
        	data->dri = (struct DrawInfo *)tag->ti_Data;
        	reffont = data->dri->dri_Font;
        	break;

            case SYSIA_Which:
        	data->type = tag->ti_Data;

        	D(bug("SYSIA_Which type: %d\n", data->type));

        	switch (tag->ti_Data)
        	{
    	    	    #warning "if IA_Width, IA_Height was not specified sysiclass should choose size depending on drawinfo (screen resolution)"

        	    case LEFTIMAGE:
        	    case UPIMAGE:
        	    case RIGHTIMAGE:
        	    case DOWNIMAGE:
        	    case CHECKIMAGE:
        	    case MXIMAGE:
        	    case DEPTHIMAGE:
        	    case SDEPTHIMAGE:
        	    case ZOOMIMAGE:
        	    case CLOSEIMAGE:
        	    case SIZEIMAGE:
        	    case MENUCHECK:
        	    case AMIGAKEY:
        	    case ICONIFYIMAGE:
        	    case LOCKIMAGE:
        	    case JUMPIMAGE:
        	    case MUIIMAGE:
        	    case POPUPIMAGE:
        	    case SNAPSHOTIMAGE:
                	break;

        	    default:
                	unsupported = TRUE;
                	break;
        	}
        	break;

            case SYSIA_ReferenceFont:
        	if (tag->ti_Data) reffont = (struct TextFont *)tag->ti_Data;
        	break;

            case SYSIA_Size:
        	size = tag->ti_Data;
        	break;

        	/* private tags */

            case SYSIA_WithBorder:
        	if (tag->ti_Data == FALSE)
        	{
                    data->flags |= SYSIFLG_NOBORDER;
        	}
        	break;

            case SYSIA_Style:
        	if (tag->ti_Data == SYSISTYLE_GADTOOLS)
        	{
                    data->flags |= SYSIFLG_GADTOOLS;
        	}
        	break;

        } /* switch(tag->ti_Tag) */

    } /* while ((tag = NextTagItem(&taglist))) */

    D(bug("dri: %p, unsupported: %d\n", data->dri, unsupported));

    if ((!data->dri) || (unsupported))
        return FALSE;

    {
    	struct wdpGetDefSizeSysImage  msg;
	ULONG       	    	      width = DEFSIZE_WIDTH, height = DEFSIZE_HEIGHT;
	
	msg.MethodID 	    	= (data->type == SDEPTHIMAGE) ? SDM_GETDEFSIZE_SYSIMAGE :
	    	    	    	    	    	    	    	WDM_GETDEFSIZE_SYSIMAGE;
	msg.wdp_Which 	    	= data->type;
	msg.wdp_SysiSize     	= size;
	msg.wdp_ReferenceFont 	= reffont;
	msg.wdp_Width 	    	= &width;
	msg.wdp_Height	    	= &height;
	msg.wdp_Flags	    	= 0;
	
	if (data->type == SDEPTHIMAGE)
	{
	    LOCKSHARED_SCRDECOR(data->dri);
	    DoMethodA(INTDRI(data->dri)->dri_ScrDecorObj, (Msg)&msg);	
	    UNLOCK_SCRDECOR(data->dri);
	}
	else
	{
	    LOCKSHARED_WINDECOR(data->dri);
	    DoMethodA(INTDRI(data->dri)->dri_WinDecorObj, (Msg)&msg);	
	    UNLOCK_WINDECOR(data->dri);
	}
	
    	if (!set_width) IM(obj)->Width = width;
    	if (!set_height) IM(obj)->Height = height;	
    }
    
    return TRUE;
}

/**************************************************************************************************/

Object *SysIClass__OM_NEW(Class *cl, Class *rootcl, struct opSet *msg)
{
    struct SysIData *data;
    Object  	    *obj;

    D(bug("sysi_new()\n"));
    obj = (Object *)DoSuperMethodA(cl, (Object *)rootcl, (Msg)msg);
    if (!obj)
        return NULL;

    D(bug("sysi_new,: obj=%p\n", obj));

    data = INST_DATA(cl, obj);
    data->type  = 0L;
    data->dri   = NULL;
    data->frame = NULL;
    data->flags = 0;
    
    if (!sysi_setnew(cl, obj, (struct opSet *)msg))
    {
        STACKULONG method = OM_DISPOSE;
        CoerceMethodA(cl, obj, (Msg)&method);
        return NULL;
    }

    D(bug("sysi_setnew called successfully\n"));

    switch (data->type)
    {
	case CHECKIMAGE:
        {
            struct TagItem tags[] =
            {
                {IA_FrameType, FRAME_BUTTON },
                {IA_EdgesOnly, FALSE    	},
                {TAG_MORE	    	    	}
            };

            tags[2].ti_Data = (IPTR)msg->ops_AttrList;

            data->frame = NewObjectA(NULL, FRAMEICLASS, tags);
            if (!data->frame)
            {
                STACKULONG method = OM_DISPOSE;
                CoerceMethodA(cl, obj, (Msg)&method);
                return NULL;
            }
            break;
        }

            /* Just to prevent it from reaching default: */
	case MXIMAGE:
	case LEFTIMAGE:
	case UPIMAGE:
	case RIGHTIMAGE:
	case DOWNIMAGE:

	case SDEPTHIMAGE:
	case DEPTHIMAGE:
	case ZOOMIMAGE:
	case CLOSEIMAGE:
	case SIZEIMAGE:

	case MENUCHECK:
	case AMIGAKEY:

	case ICONIFYIMAGE:
	case LOCKIMAGE:
	case MUIIMAGE:
	case POPUPIMAGE:
	case SNAPSHOTIMAGE:
	case JUMPIMAGE:
            break;

	default:
            {
        	STACKULONG method = OM_DISPOSE;
		
        	CoerceMethodA(cl, obj, (Msg)&method);
            }
            return NULL;
    }

    return obj;
}

/**************************************************************************************************/

/* Georg Steger
 */
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

IPTR SysIClass__IM_DRAW(Class *cl, Object *obj, struct impDraw *msg)
{
    struct SysIData 	    *data = INST_DATA(cl, obj);
    struct RastPort 	    *rport = msg->imp_RPort;
    WORD    	    	     left = IM(obj)->LeftEdge + msg->imp_Offset.X;
    WORD    	    	     top = IM(obj)->TopEdge + msg->imp_Offset.Y;
    UWORD   	    	     width = IM(obj)->Width;
    UWORD   	    	     height = IM(obj)->Height;
    WORD    	    	     right = left + width - 1;
    WORD    	    	     bottom = top + height - 1;
    struct wdpDrawSysImage   decormsg;

    decormsg.MethodID  = WDM_DRAW_SYSIMAGE;
    decormsg.wdp_RPort = rport;
    decormsg.wdp_X = left;
    decormsg.wdp_Y = top;
    decormsg.wdp_Width = width;
    decormsg.wdp_Height = height;
    decormsg.wdp_Which = data->type;
    decormsg.wdp_State = msg->imp_State;
    decormsg.wdp_Flags = 0;
    
    SetDrMd(rport, JAM1);

    switch(data->type)
    {
    	case CHECKIMAGE:
        {
            WORD h_spacing = width / 4;
            WORD v_spacing = height / 4;

            /* Draw frame */
            DrawImageState(rport, data->frame,
                           msg->imp_Offset.X, msg->imp_Offset.Y,
                           IDS_NORMAL, data->dri);

            /* Draw checkmark (only if image is in selected state) */
            if (msg->imp_State == IDS_SELECTED)
            {
                left += h_spacing;
                right -= h_spacing;
                width -= h_spacing * 2;
                top += v_spacing;
                bottom -= v_spacing;
                height -= v_spacing * 2;

                SetAPen(rport, data->dri->dri_Pens[SHADOWPEN]);
                draw_thick_line(cl, rport, left, top + height / 3 , left, bottom, 0);
                draw_thick_line(cl, rport, left + 1, bottom, right - 1, top, 0);
            }
            break;
        }
	
    	case MXIMAGE:
        {
            BOOL selected = FALSE;
            WORD col1 = SHINEPEN;
            WORD col2 = SHADOWPEN;

            if ((msg->imp_State == IDS_SELECTED) || (msg->imp_State == IDS_INACTIVESELECTED))
            {
                col1 = SHADOWPEN;
                col2 = SHINEPEN;
                selected = TRUE;
            }

            SetAPen(rport, data->dri->dri_Pens[BACKGROUNDPEN]);
            RectFill(rport, left, top, right, bottom);

    	#if 0
            /* THICK MX IMAGE */

            SetAPen(rport, data->dri->dri_Pens[col1]);
            RectFill(rport, left + 2, top, right - 3, top + 1);
            RectFill(rport, left + 1, top + 2, left + 2, top + 3);
            RectFill(rport, left, top + 4, left + 1, bottom - 4);
            RectFill(rport, left + 1, bottom - 3, left + 2, bottom - 2);
            RectFill(rport, left + 2, bottom - 1, left + 2, bottom);

            SetAPen(rport, data->dri->dri_Pens[col2]);
            RectFill(rport, right - 2, top, right - 2, top + 1);
            RectFill(rport, right - 2, top + 2, right - 1, top + 3);
            RectFill(rport, right - 1, top + 4, right, bottom - 4);
            RectFill(rport, right - 2, bottom - 3, right - 1, bottom - 2);
            RectFill(rport, left + 3, bottom - 1, right - 2, bottom);

            if (selected)
            {
                left += 4;
                right -= 4;
                width -= 8;
                top += 4;
                bottom -= 4;
                height -= 8;

                SetAPen(rport, data->dri->dri_Pens[FILLPEN]);
                if ((width >= 3) && (height >= 3))
                {
                    RectFill(rport, left + 1, top, right - 1, top);
                    RectFill(rport, left, top + 1, right, bottom - 1);
                    RectFill(rport, left + 1, bottom, right - 1, bottom);
                }
                else
                {
                    RectFill(rport, left, top, right, bottom);
                }
            }
    	#else
            /* THIN MX IMAGE */

            SetAPen(rport, data->dri->dri_Pens[col1]);
            RectFill(rport, left + 3, top, right - 3, top);
            WritePixel(rport, left + 2, top + 1);
            RectFill(rport, left + 1, top + 2, left + 1, top + 3);
            RectFill(rport, left, top + 4, left, bottom - 4);
            RectFill(rport, left + 1, bottom - 3, left + 1, bottom - 2);
            WritePixel(rport, left + 2, bottom - 1);

            SetAPen(rport, data->dri->dri_Pens[col2]);
            WritePixel(rport, right - 2, top + 1);
            RectFill(rport, right - 1, top + 2, right - 1, top + 3);
            RectFill(rport, right, top + 4, right, bottom - 4);
            RectFill(rport, right - 1, bottom - 3, right - 1, bottom - 2);
            WritePixel(rport, right - 2, bottom - 1);
            RectFill(rport, left + 3, bottom, right - 3, bottom);

            if (selected)
            {
                left += 3;
                right -= 3;
                width -= 6;
                top += 3;
                bottom -= 3;
                height -= 6;

                SetAPen(rport, data->dri->dri_Pens[FILLPEN]);
                if ((width >= 5) && (height >= 5))
                {
                    RectFill(rport, left, top + 2, left, bottom - 2);
                    RectFill(rport, left + 1, top + 1, left + 1, bottom - 1);
                    RectFill(rport, left + 2, top, right - 2, bottom);
                    RectFill(rport, right - 1, top + 1, right - 1, bottom - 1);
                    RectFill(rport, right, top + 2, right, bottom - 2);
                }
                else
                {
                    RectFill(rport, left, top, right, bottom);
                }
            }

    	#endif
            break;
        }

    	case LEFTIMAGE:
        {
            UWORD hspacing,vspacing;
            WORD  cy;

            if (!(data->flags & (SYSIFLG_NOBORDER | SYSIFLG_GADTOOLS)))
	    {
		LOCKSHARED_WINDECOR(data->dri)
    		DoMethodA(INTDRI(data->dri)->dri_WinDecorObj, (Msg)&decormsg);	
		UNLOCK_WINDECOR(data->dri)
	    	break;
	    }
	    
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

            if (!(data->flags & SYSIFLG_NOBORDER))
            {
                renderimageframe(rport, LEFTIMAGE, msg->imp_State, data->dri->dri_Pens,
                                 left, top, width, height, IntuitionBase);
                left++;
                top++;
                right--;
                bottom--;
                width -= 2;
                height -= 2;
		
		if (data->flags & SYSIFLG_GADTOOLS)
		{
                    SetAPen(rport, getbgpen_gt(msg->imp_State, data->dri->dri_Pens));
                    RectFill(rport, left, top, right, bottom);
		}
            }

            if (data->flags & SYSIFLG_GADTOOLS)
            {
                SetAPen(rport, data->dri->dri_Pens[SHADOWPEN]);

                cy = height / 2;

                Move(rport, left + width - 1 - hspacing, top + vspacing + 1);
                Draw(rport, left + hspacing, top + height - cy);
                Move(rport, left + width - 1 - hspacing, top + vspacing);
                Draw(rport, left + hspacing, top + height - cy - 1);

                Move(rport, left + width - 1 - hspacing, top + height - 1- vspacing - 1);
                Draw(rport, left + hspacing, top + cy - 1);
                Move(rport, left + width - 1 - hspacing, top + height - 1 - vspacing);
                Draw(rport, left + hspacing, top + cy);
            }
            else
            {
                WORD i;

                SetAPen(rport, getbgpen(msg->imp_State, data->dri->dri_Pens));

                RectFill(rport, left, top, right, bottom);

                left += hspacing;
                top += vspacing;
                width -= hspacing * 2;
                height -= vspacing * 2;

                right = left + width - 1;
                bottom = top + height - 1;

                cy = (height + 1) / 2;

                SetAPen(rport, data->dri->dri_Pens[SHADOWPEN]);

                for(i = 0; i < cy; i++)
                {
                    RectFill(rport, left + (cy - i - 1) * width / cy,
                             top + i,
                             right - i * width / cy / 2,
                             top + i);
                    RectFill(rport, left + (cy - i - 1) * width / cy,
                             bottom - i,
                             right - i * width / cy / 2,
                             bottom - i);
                }

            }
            break;
        }

    	case UPIMAGE:
        {
            UWORD hspacing,vspacing;
            WORD  cx;

            if (!(data->flags & (SYSIFLG_NOBORDER | SYSIFLG_GADTOOLS)))
	    {
		LOCKSHARED_WINDECOR(data->dri)
    		DoMethodA(INTDRI(data->dri)->dri_WinDecorObj, (Msg)&decormsg);	
		UNLOCK_WINDECOR(data->dri)
	    	break;
	    }

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

            if (!(data->flags & SYSIFLG_NOBORDER))
            {
                renderimageframe(rport, UPIMAGE, msg->imp_State, data->dri->dri_Pens,
                                 left, top, width, height, IntuitionBase);
                left++;
                top++;
                right--;
                bottom--;
                width -= 2;
                height -= 2;

		if (data->flags & SYSIFLG_GADTOOLS)
		{
                    SetAPen(rport, getbgpen_gt(msg->imp_State, data->dri->dri_Pens));
                    RectFill(rport, left, top, right, bottom);
		}

            }

            if (data->flags & SYSIFLG_GADTOOLS)
            {
                SetAPen(rport, data->dri->dri_Pens[SHADOWPEN]);

                cx = width / 2;

                Move(rport, left + hspacing + 1, top + height - 1 - vspacing);
                Draw(rport, left + width - cx, top + vspacing);
                Move(rport, left + hspacing, top + height - 1 - vspacing);
                Draw(rport, left + width - cx - 1, top + vspacing);

                Move(rport, left + width - 1 - hspacing - 1, top + height - 1 - vspacing);
                Draw(rport, left + cx - 1, top + vspacing);
                Move(rport, left + width - 1 - hspacing, top + height - 1 - vspacing);
                Draw(rport, left + cx, top + vspacing);
            }
            else
            {
                WORD i;

                SetAPen(rport, getbgpen(msg->imp_State, data->dri->dri_Pens));

                RectFill(rport, left, top, right, bottom);

                left += hspacing;
                top += vspacing;
                width -= hspacing * 2;
                height -= vspacing * 2;

                right = left + width - 1;
                bottom = top + height - 1;

                cx = (width + 1) / 2;

                SetAPen(rport, data->dri->dri_Pens[SHADOWPEN]);

                for(i = 0; i < cx; i++)
                {
                    RectFill(rport, left + i,
                             top + (cx - i - 1) * height / cx,
                             left + i,
                             bottom - i * height / cx / 2);
                    RectFill(rport, right - i,
                             top + (cx - i - 1) * height / cx,
                             right - i,
                             bottom - i * height / cx / 2);
                }

            }
            break;
        }

    	case RIGHTIMAGE:
        {
            UWORD hspacing,vspacing;
            WORD  cy;

            if (!(data->flags & (SYSIFLG_NOBORDER | SYSIFLG_GADTOOLS)))
	    {
		LOCKSHARED_WINDECOR(data->dri)
    		DoMethodA(INTDRI(data->dri)->dri_WinDecorObj, (Msg)&decormsg);	
		UNLOCK_WINDECOR(data->dri)
	    	break;
	    }

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

            if (!(data->flags & SYSIFLG_NOBORDER))
            {
                renderimageframe(rport, RIGHTIMAGE, msg->imp_State, data->dri->dri_Pens,
                                 left, top, width, height, IntuitionBase);
                left++;
                top++;
                right--;
                bottom--;
                width -= 2;
                height -= 2;

		if (data->flags & SYSIFLG_GADTOOLS)
		{
                    SetAPen(rport, getbgpen_gt(msg->imp_State, data->dri->dri_Pens));
                    RectFill(rport, left, top, right, bottom);
		}

            }

            if (data->flags & SYSIFLG_GADTOOLS)
            {
                SetAPen(rport, data->dri->dri_Pens[SHADOWPEN]);

                cy = height / 2;

                Move(rport, left + hspacing, top + vspacing + 1);
                Draw(rport, left + width - 1 - hspacing, top + height - cy);
                Move(rport, left + hspacing, top + vspacing);
                Draw(rport, left + width - 1 - hspacing, top + height - cy - 1);

                Move(rport, left + hspacing, top + height - 1- vspacing - 1);
                Draw(rport, left + width - 1 - hspacing, top + cy - 1);
                Move(rport, left + hspacing, top + height - 1 - vspacing);
                Draw(rport, left + width - 1 - hspacing, top + cy);

            }
            else
            {
                WORD i;

                SetAPen(rport, getbgpen(msg->imp_State, data->dri->dri_Pens));

                RectFill(rport, left, top, right, bottom);

                left += hspacing;
                top += vspacing;
                width -= hspacing * 2;
                height -= vspacing * 2;

                right = left + width - 1;
                bottom = top + height - 1;

                cy = (height + 1) / 2;

                SetAPen(rport, data->dri->dri_Pens[SHADOWPEN]);

                for(i = 0; i < cy; i++)
                {
                    RectFill(rport, left + i * width / cy / 2,
                             top + i,
                             right - (cy - i - 1) * width / cy,
                             top + i);
                    RectFill(rport, left + i * width / cy / 2,
                             bottom - i,
                             right - (cy - i - 1) * width / cy,
                             bottom - i);
                }

            }
            break;
        }

    	case DOWNIMAGE:
        {
            UWORD hspacing,vspacing;
            WORD  cx;
 
            if (!(data->flags & (SYSIFLG_NOBORDER | SYSIFLG_GADTOOLS)))
	    {
		LOCKSHARED_WINDECOR(data->dri)
    		DoMethodA(INTDRI(data->dri)->dri_WinDecorObj, (Msg)&decormsg);	
		UNLOCK_WINDECOR(data->dri)
	    	break;
	    }

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

            if (!(data->flags & SYSIFLG_NOBORDER))
            {
                renderimageframe(rport, DOWNIMAGE, msg->imp_State, data->dri->dri_Pens,
                                 left, top, width, height, IntuitionBase);
                left++;
                top++;
                right--;
                bottom--;
                width -= 2;
                height -= 2;

		if (data->flags & SYSIFLG_GADTOOLS)
		{
                    SetAPen(rport, getbgpen_gt(msg->imp_State, data->dri->dri_Pens));
                    RectFill(rport, left, top, right, bottom);
		}
            }

            if (data->flags & SYSIFLG_GADTOOLS)
            {
                SetAPen(rport, data->dri->dri_Pens[SHADOWPEN]);

                cx = width / 2;

                Move(rport, left + hspacing + 1, top + vspacing);
                Draw(rport, left + width - cx, top + height - 1 - vspacing);
                Move(rport, left + hspacing, top + vspacing);
                Draw(rport, left + width - cx - 1, top + height - 1 - vspacing);

                Move(rport, left + width - 1 - hspacing - 1, top + vspacing);
                Draw(rport, left + cx - 1, top + height - 1 - vspacing);
                Move(rport, left + width - 1 - hspacing, top + vspacing);
                Draw(rport, left + cx, top + height - 1 - vspacing);
            }
            else
            {
                WORD i;

                SetAPen(rport, getbgpen(msg->imp_State, data->dri->dri_Pens));

                RectFill(rport, left, top, right, bottom);

                left += hspacing;
                top += vspacing;
                width -= hspacing * 2;
                height -= vspacing * 2;

                right = left + width - 1;
                bottom = top + height - 1;

                cx = (width + 1) / 2;

                SetAPen(rport, data->dri->dri_Pens[SHADOWPEN]);

                for(i = 0; i < cx; i++)
                {
                    RectFill(rport, left + i,
                             top + i * height / cx / 2,
                             left + i,
                             bottom - (cx - i - 1) * height / cx);
                    RectFill(rport, right - i,
                             top + i * height / cx / 2,
                             right -  i,
                             bottom - (cx - i - 1) * height / cx);

                }

            }
            break;
        }

    	case CLOSEIMAGE:
	case ZOOMIMAGE:
	case DEPTHIMAGE:
	case SIZEIMAGE:
        {
	    LOCKSHARED_WINDECOR(data->dri)
    	    DoMethodA(INTDRI(data->dri)->dri_WinDecorObj, (Msg)&decormsg);	
	    UNLOCK_WINDECOR(data->dri)
            break;
        }

    	case SDEPTHIMAGE:
	{
	    decormsg.MethodID  = SDM_DRAW_SYSIMAGE;

	    LOCKSHARED_SCRDECOR(data->dri)
    	    DoMethodA(INTDRI(data->dri)->dri_ScrDecorObj, (Msg)&decormsg);	
	    UNLOCK_SCRDECOR(data->dri)
	    break;
	}
	
    	case MENUCHECK:
        {
            UWORD *pens = data->dri->dri_Pens;

            if (MENUS_AMIGALOOK)
            {
                SetAPen(rport, pens[BARBLOCKPEN]);
            }
            else
            {
                SetAPen(rport, pens[(msg->imp_State == IDS_SELECTED) ? FILLPEN : BACKGROUNDPEN]);
            }

            RectFill(rport, left, top, right, bottom);

            SetAPen(rport, pens[BARDETAILPEN]);
            draw_thick_line(cl, rport, left + 1, top + height / 3 , left + 1, bottom, 0);
            draw_thick_line(cl, rport, left + 2, bottom, right - 2, top, 0);

            break;
        }

    	case AMIGAKEY:
        {
            UWORD   	    *pens = data->dri->dri_Pens;
            struct TextFont *oldfont;
            UBYTE   	     oldstyle;
            
            if (MENUS_AMIGALOOK)
            {
                SetAPen(rport, pens[BARDETAILPEN]);
            }
            else
            {
                SetAPen(rport, pens[SHINEPEN]);
            }

            RectFill(rport, left, top, right, bottom);

            if (MENUS_AMIGALOOK)
            {
                SetAPen(rport, pens[BARBLOCKPEN]);
    
                oldfont = rport->Font;
                oldstyle = rport->AlgoStyle;
    
                SetFont(rport, GfxBase->DefaultFont);
                SetSoftStyle(rport, FSF_ITALIC, AskSoftStyle(rport));
    
                Move(rport, left + (width - rport->TxWidth) / 2,
                     top  + (height - rport->TxHeight) / 2 + rport->TxBaseline);
                Text(rport, "A", 1);
    
                SetSoftStyle(rport, oldstyle, AskSoftStyle(rport));
                SetFont(rport, oldfont);
    
                SetAPen(rport, pens[BARBLOCKPEN]);
            }
            else
            {
                SetAPen(rport, pens[SHADOWPEN]);
    
                RectFill(rport, left + 1, top, right - 1, top);
                RectFill(rport, right, top + 1, right, bottom - 1);
                RectFill(rport, left + 1, bottom, right - 1, bottom);
                RectFill(rport, left, top + 1, left, bottom - 1);
    
                SetAPen(rport, pens[BACKGROUNDPEN]);
                RectFill(rport, left + 1, bottom - 1, right - 1, bottom - 1);
                RectFill(rport, right - 1, top + 1, right - 1, bottom - 2);
    
                RectFill(rport, left + 2, top + 2, left + 4, top + 2);
                RectFill(rport, left + 2, top + 3, left + 2, top + 4);
    
                SetAPen(rport, pens[SHADOWPEN]);
                RectFill(rport, left + 2, bottom - 2, right - 2, bottom - 2);
                RectFill(rport, right - 2, top + 2, right - 2, bottom - 4);
    
                {
                    WORD a_size   = height - 7;
                    WORD a_left   = left + 5;
                    WORD a_top    = top + 2;
                    WORD a_right  = a_left + a_size;
                    WORD a_bottom = a_top + a_size;
    
                    Move(rport, a_left, a_bottom);
                    Draw(rport, a_right, a_top);
                    Draw(rport, a_right, a_bottom);
                    Move(rport, a_right - 1, a_top + 1);
                    Draw(rport, a_right - 1, a_bottom);
                }
    
                SetAPen(rport, pens[(msg->imp_State == IDS_SELECTED) ? FILLPEN : BACKGROUNDPEN]);
            }
            
            WritePixel(rport, left, top);
            WritePixel(rport, right, top);
            WritePixel(rport, right, bottom);
            WritePixel(rport, left, bottom);

            break;
        }

        /* MUI and other non-std images */
    #if 0
    	case MUIIMAGE:
        {
            UWORD *pens = data->dri->dri_Pens;
            UWORD bg;

            if (!(data->flags & SYSIFLG_NOBORDER))
            {
                renderimageframe(rport, SIZEIMAGE, msg->imp_State, pens,
                                 left, top, width, height, IntuitionBase);
                left++;
                top++;
                right--;
                bottom--;
                width -= 2;
                height -= 2;
            }

            bg = getbgpen(msg->imp_State, pens);

            /* Clear background into correct color */
            SetAPen(rport, bg);
            RectFill(rport, left, top, right, bottom);

            /* DRAW IMAGE :) */

            DrawIB(rport,(BYTE *)ibPrefs,left+(width/2),top+(height/2),IntuitionBase);

            break;
        }

    	case SNAPSHOTIMAGE:
    	case POPUPIMAGE:
    	case ICONIFYIMAGE:
    	case LOCKIMAGE:
        {
            UWORD *pens = data->dri->dri_Pens;
            UWORD  bg;

            if (!(data->flags & SYSIFLG_NOBORDER))
            {
                renderimageframe(rport, SIZEIMAGE, msg->imp_State, pens,
                                 left, top, width, height, IntuitionBase);
                left++;
                top++;
                right--;
                bottom--;
                width -= 2;
                height -= 2;
            }

            bg = getbgpen(msg->imp_State, pens);

            /* Clear background into correct color */
            SetAPen(rport, bg);
            RectFill(rport, left, top, right, bottom);

            /* DRAW IMAGE :) */

            if (data->type == SNAPSHOTIMAGE)
            {
                if (msg->imp_State == IDS_SELECTED)
                {
                    DrawIB(rport,(BYTE *)ibSnapshotSel,left+(width/2),top+(height/2),IntuitionBase);
                }
                else
                {
                    DrawIB(rport,(BYTE *)ibSnapshot,left+(width/2),top+(height/2),IntuitionBase);
                }
            }

            if (data->type == POPUPIMAGE)
            {
                if (msg->imp_State == IDS_SELECTED)
                {
                    DrawIB(rport,(BYTE *)ibPopupSel,left+(width/2),top+(height/2),IntuitionBase);
                }
                else
                {
                    DrawIB(rport,(BYTE *)ibPopup,left+(width/2),top+(height/2),IntuitionBase);
                }
            }

            if (data->type == ICONIFYIMAGE)
            {
                if (msg->imp_State == IDS_SELECTED)
                {
                    DrawIB(rport,(BYTE *)ibIconifySel,left+(width/2),top+(height/2),IntuitionBase);
                }
                else
                {
                    DrawIB(rport,(BYTE *)ibIconify,left+(width/2),top+(height/2),IntuitionBase);
                }
            }

            if (data->type == LOCKIMAGE)
            {
                if ((msg->imp_State == IDS_SELECTED) || (msg->imp_State == IDS_INACTIVESELECTED))
                {
                    DrawIB(rport,(BYTE *)ibLockSel,left+(width/2),top+(height/2),IntuitionBase);
                }
                else
                {
                    DrawIB(rport,(BYTE *)ibLock,left+(width/2),top+(height/2),IntuitionBase);
                }
            }

            break;
        }

    	case JUMPIMAGE:
        {
            UWORD *pens = data->dri->dri_Pens;
            UWORD  bg;

            if (!(data->flags & SYSIFLG_NOBORDER))
            {
                renderimageframe(rport, SIZEIMAGE, msg->imp_State, pens,
                                 left, top, width, height, IntuitionBase);
                left++;
                top++;
                right--;
                bottom--;
                width -= 2;
                height -= 2;
            }

            bg = getbgpen(msg->imp_State, pens);

            /* Clear background into correct color */
            SetAPen(rport, bg);
            RectFill(rport, left, top, right, bottom);

            /* DRAW IMAGE :) */

            DrawJUMP(rport,msg->imp_State,left+(width/2),top+(height/2),IntuitionBase);

            break;
        }
    #endif
    } /* switch (image type) */

    return (IPTR)0;
}

IPTR SysIClass__OM_DISPOSE(Class *cl, Object *obj, Msg msg)
{
    struct SysIData *data = INST_DATA(cl, obj);
    
    DisposeObject(data->frame);
    return DoSuperMethodA(cl, obj, msg);
}

IPTR SysIClass__OM_SET(Class *cl, Object *obj, Msg msg)
{
    struct SysIData *data = INST_DATA(cl, obj);
    
    if (data->frame)
	DoMethodA((Object *)data->frame, msg);
    return DoSuperMethodA(cl, obj, msg);
}

/**************************************************************************************************/

#undef IntuitionBase

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

static UWORD getbgpen_gt(ULONG state, UWORD *pens)
{
    UWORD bg;

    switch (state)
    {
	case IDS_SELECTED:
	case IDS_INACTIVESELECTED:
            bg = pens[FILLPEN];
            break;

	default:
            bg = pens[BACKGROUNDPEN];
            break;
    }
    
    return bg;
}

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
