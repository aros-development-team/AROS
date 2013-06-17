/*
    Copyright © 1995-2013, The AROS Development Team. All rights reserved.
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

/**************************************************************************************************/

#define SYSIFLG_GADTOOLS 1
#define SYSIFLG_NOBORDER 2

/**************************************************************************************************/

/* Some handy drawing functions */

void draw_thick_line(Class *cl, struct RastPort *rport,
                     LONG x1, LONG y1, LONG x2, LONG y2,
                     UWORD thickness)
{
    struct IntuitionBase *IntuitionBase = (struct IntuitionBase *)cl->cl_UserData;
    struct GfxBase *GfxBase = GetPrivIBase(IntuitionBase)->GfxBase;
    Move(rport, x1, y1);
    Draw(rport, x2, y2);
    /* Georg Steger */
    Move(rport, x1 + 1, y1);
    Draw(rport, x2 + 1, y2);
}

/**************************************************************************************************/

BOOL sysi_setnew(Class *cl, Object *obj, struct opSet *msg)
{
    struct IntuitionBase *IntuitionBase = (struct IntuitionBase *)cl->cl_UserData;
    struct Library      *UtilityBase = GetPrivIBase(IntuitionBase)->UtilityBase;
    struct SysIData 	*data = INST_DATA(cl, obj);
    struct TagItem  	*taglist, *tag;
    struct TextFont 	*reffont = NULL;
    int     	     	 size = SYSISIZE_MEDRES;
    BOOL    	     	 unsupported = FALSE;
    BOOL    	     	 set_width = FALSE, set_height = FALSE;

    taglist = msg->ops_AttrList;
    while ((tag = NextTagItem(&taglist)))
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
    	    	    // FIXME: if IA_Width, IA_Height was not specified sysiclass should choose size depending on drawinfo (screen resolution)"

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
		    case SUBMENUIMAGE:
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
        	if (!tag->ti_Data)
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

    D(bug("dri: %p, unsupported: %d version: %d\n", data->dri, unsupported, data->dri->dri_Version));

    if ((!data->dri) || (unsupported) || (data->dri->dri_Version != DRI_VERSION))
        return FALSE;

    {
	ULONG	width = DEFSIZE_WIDTH, height = DEFSIZE_HEIGHT;

    BOOL	 tc = (data->dri->dri_Flags & DRIF_DIRECTCOLOR);

	if (data->type == SDEPTHIMAGE)
	{
        struct sdpGetDefSizeSysImage  smsg;
	
		smsg.MethodID 	    	= SDM_GETDEFSIZE_SYSIMAGE;
        smsg.sdp_TrueColor      = tc;
		smsg.sdp_Which 	    	= data->type;
		smsg.sdp_SysiSize     	= size;
		smsg.sdp_ReferenceFont 	= reffont;
		smsg.sdp_Width 	    	= &width;
		smsg.sdp_Height	    	= &height;
		smsg.sdp_Flags	    	= 0;
		smsg.sdp_Dri            = data->dri;
		smsg.sdp_UserBuffer 	= 0;

        DoMethodA(((struct IntScreen *)(((struct IntDrawInfo *)data->dri)->dri_Screen))->ScrDecorObj, (Msg)&smsg);  

	}
	else if ((data->type == AMIGAKEY) || (data->type == MENUCHECK) || (data->type == SUBMENUIMAGE))
	{
        struct mdpGetDefSizeSysImage  mmsg;

		mmsg.MethodID 	    	= MDM_GETDEFSIZE_SYSIMAGE;
        mmsg.mdp_TrueColor      = tc;
		mmsg.mdp_Which 	    	= data->type;
		mmsg.mdp_SysiSize     	= size;
		mmsg.mdp_ReferenceFont 	= reffont;
		mmsg.mdp_Width 	    	= &width;
		mmsg.mdp_Height	    	= &height;
		mmsg.mdp_Flags	    	= 0;
		mmsg.mdp_Dri            = data->dri;
        DoMethodA(((struct IntScreen *)(((struct IntDrawInfo *)data->dri)->dri_Screen))->MenuDecorObj, (Msg)&mmsg); 

	}
	else
	{
        struct wdpGetDefSizeSysImage  wmsg;

		wmsg.MethodID 	    	= WDM_GETDEFSIZE_SYSIMAGE;
        wmsg.wdp_TrueColor      = tc;
		wmsg.wdp_Which 	    	= data->type;
		wmsg.wdp_SysiSize     	= size;
		wmsg.wdp_ReferenceFont 	= reffont;
		wmsg.wdp_Width 	    	= &width;
		wmsg.wdp_Height	    	= &height;
		wmsg.wdp_Flags	    	= 0;
		wmsg.wdp_Dri            = data->dri;
		wmsg.wdp_UserBuffer	= 0;

        DoMethodA(((struct IntScreen *)(((struct IntDrawInfo *)data->dri)->dri_Screen))->WinDecorObj, (Msg)&wmsg);  

	}

	
    	if (!set_width) IM(obj)->Width = width;
    	if (!set_height) IM(obj)->Height = height;	
    }

    return TRUE;
}

/**************************************************************************************************/

Object *SysIClass__OM_NEW(Class *cl, Class *rootcl, struct opSet *msg)
{
    struct IntuitionBase *IntuitionBase = (struct IntuitionBase *)cl->cl_UserData;
    struct SysIData *data;
    Object  	    *obj;

    D(bug("sysi_new()\n"));
    obj = (Object *)DoSuperMethodA(cl, (Object *)rootcl, (Msg)msg);
    if (!obj)
        return NULL;

    D(bug("sysi_new,: obj=%p\n", obj));

    data = INST_DATA(cl, obj);
    data->type  = 0;
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
	case SUBMENUIMAGE:
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
    struct IntuitionBase    *IntuitionBase = (struct IntuitionBase *)cl->cl_UserData;
    struct GfxBase          *GfxBase = GetPrivIBase(IntuitionBase)->GfxBase;
    struct SysIData 	    *data = INST_DATA(cl, obj);
    struct RastPort 	    *rport = msg->imp_RPort;
    struct Window           *win = NULL;
    WORD    	    	     left = IM(obj)->LeftEdge + msg->imp_Offset.X;
    WORD    	    	     top = IM(obj)->TopEdge + msg->imp_Offset.Y;
    UWORD   	    	     width = IM(obj)->Width;
    UWORD   	    	     height = IM(obj)->Height;
    WORD    	    	     right = left + width - 1;
    WORD    	    	     bottom = top + height - 1;
    struct wdpDrawSysImage   wdecormsg;
    struct sdpDrawSysImage   sdecormsg;
    struct mdpDrawSysImage   mdecormsg;

    BOOL	 tc = (data->dri->dri_Flags & DRIF_DIRECTCOLOR);

    if (rport) if (rport->Layer) win = (struct Window *) rport->Layer->Window;

    wdecormsg.MethodID  = WDM_DRAW_SYSIMAGE;
    wdecormsg.wdp_TrueColor      = tc;
    wdecormsg.wdp_RPort = rport;
    wdecormsg.wdp_X = left;
    wdecormsg.wdp_Y = top;
    wdecormsg.wdp_Width = width;
    wdecormsg.wdp_Height = height;
    wdecormsg.wdp_Which = data->type;
    wdecormsg.wdp_State = msg->imp_State;
    wdecormsg.wdp_Flags = 0;
    wdecormsg.wdp_Dri = data->dri;
    wdecormsg.wdp_UserBuffer = (win == NULL) ? 0 : ((struct IntWindow *)win)->DecorUserBuffer;

    sdecormsg.MethodID  = SDM_DRAW_SYSIMAGE;
    sdecormsg.sdp_TrueColor      = tc;
    sdecormsg.sdp_RPort = rport;
    sdecormsg.sdp_X = left;
    sdecormsg.sdp_Y = top;
    sdecormsg.sdp_Width = width;
    sdecormsg.sdp_Height = height;
    sdecormsg.sdp_Which = data->type;
    sdecormsg.sdp_State = msg->imp_State;
    sdecormsg.sdp_Flags = 0;
    sdecormsg.sdp_Dri = data->dri;
    sdecormsg.sdp_UserBuffer = GetPrivScreen(data->dri->dri_Screen)->DecorUserBuffer;

    mdecormsg.MethodID  = MDM_DRAW_SYSIMAGE;
    mdecormsg.mdp_TrueColor      = tc;
    mdecormsg.mdp_RPort = rport;
    mdecormsg.mdp_X = left;
    mdecormsg.mdp_Y = top;
    mdecormsg.mdp_Width = width;
    mdecormsg.mdp_Height = height;
    mdecormsg.mdp_Which = data->type;
    mdecormsg.mdp_State = msg->imp_State;
    mdecormsg.mdp_Flags = 0;
    mdecormsg.mdp_Dri = data->dri;
    mdecormsg.mdp_UserBuffer = GetPrivScreen(data->dri->dri_Screen)->DecorUserBuffer;

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
    		DoMethodA(((struct IntScreen *)(((struct IntDrawInfo *)data->dri)->dri_Screen))->WinDecorObj, (Msg)&wdecormsg);	
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
    		DoMethodA(((struct IntScreen *)(((struct IntDrawInfo *)data->dri)->dri_Screen))->WinDecorObj, (Msg)&wdecormsg);	
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
    		DoMethodA(((struct IntScreen *)(((struct IntDrawInfo *)data->dri)->dri_Screen))->WinDecorObj, (Msg)&wdecormsg);	
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
    		DoMethodA(((struct IntScreen *)(((struct IntDrawInfo *)data->dri)->dri_Screen))->WinDecorObj, (Msg)&wdecormsg);	
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
    	    DoMethodA(((struct IntScreen *)(((struct IntDrawInfo *)data->dri)->dri_Screen))->WinDecorObj, (Msg)&wdecormsg);	
            break;
        }

    	case SDEPTHIMAGE:
	{
    	    DoMethodA(((struct IntScreen *)(((struct IntDrawInfo *)data->dri)->dri_Screen))->ScrDecorObj, (Msg)&sdecormsg);	
	    break;
	}
	
    	case MENUCHECK:
        {
    	    DoMethodA(((struct IntScreen *)(((struct IntDrawInfo *)data->dri)->dri_Screen))->MenuDecorObj, (Msg)&mdecormsg);	
            break;
        }

    	case AMIGAKEY:
        {
    	    DoMethodA(((struct IntScreen *)(((struct IntDrawInfo *)data->dri)->dri_Screen))->MenuDecorObj, (Msg)&mdecormsg);	
            break;
        }

    	case SUBMENUIMAGE:
        {
    	    DoMethodA(((struct IntScreen *)(((struct IntDrawInfo *)data->dri)->dri_Screen))->MenuDecorObj, (Msg)&mdecormsg);	
            break;
        }

        /* MUI and other non-std images */
    	case MUIIMAGE:
        case SNAPSHOTIMAGE:
        case POPUPIMAGE:
        case ICONIFYIMAGE:
        case LOCKIMAGE:
        {
            DoMethodA(((struct IntScreen *)(((struct IntDrawInfo *)data->dri)->dri_Screen))->WinDecorObj, (Msg)&wdecormsg); 
            break;
        }



    } /* switch (image type) */

    return (IPTR)0;
}

IPTR SysIClass__OM_DISPOSE(Class *cl, Object *obj, Msg msg)
{
    struct IntuitionBase *IntuitionBase = (struct IntuitionBase *)cl->cl_UserData;
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
    struct GfxBase *GfxBase = GetPrivIBase(IntuitionBase)->GfxBase;
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
