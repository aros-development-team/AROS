/*
    Copyright (C) 1995-2026, The AROS Development Team. All rights reserved.
    Copyright (C) 2001-2003, The MorphOS Development Team. All Rights Reserved.
*/

/**************************************************************************************************/

#include <exec/types.h>
#include <stddef.h>

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
#include "decorsupport.h"

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

static UWORD getbgpen_gt(ULONG state, UWORD *pens);

/**************************************************************************************************/

#define SYSIFLG_GADTOOLS 1
#define SYSIFLG_NOBORDER 2

/**************************************************************************************************/

BOOL sysi_setnew(Class *cl, Object *obj, struct opSet *msg)
{
    struct IntuitionBase *IntuitionBase = (struct IntuitionBase *)cl->cl_UserData;
    struct Library      *UtilityBase = GetPrivIBase(IntuitionBase)->UtilityBase;
    struct SysIData     *data = INST_DATA(cl, obj);
    struct TagItem      *taglist, *tag;
    struct TextFont     *reffont = NULL;
    int                  size = SYSISIZE_MEDRES;
    BOOL                 unsupported = FALSE;
    BOOL                 set_width = FALSE, set_height = FALSE;

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
        ULONG   width = DEFSIZE_WIDTH, height = DEFSIZE_HEIGHT;

    BOOL         tc = (data->dri->dri_Flags & DRIF_DIRECTCOLOR) ? TRUE : FALSE;

        /* The same message serves all three decorator classes, see SYSI_SAMEFIELD */
        struct wdpGetDefSizeSysImage  wmsg;
        struct IntScreen             *scr = (struct IntScreen *)(((struct IntDrawInfo *)data->dri)->dri_Screen);
        Object                       *decorobj;

        wmsg.wdp_TrueColor      = tc;
        wmsg.wdp_Which          = data->type;
        wmsg.wdp_SysiSize       = size;
        wmsg.wdp_ReferenceFont  = reffont;
        wmsg.wdp_Width          = &width;
        wmsg.wdp_Height         = &height;
        wmsg.wdp_Flags          = 0;
        wmsg.wdp_Dri            = data->dri;
        wmsg.wdp_UserBuffer     = 0;

        if (data->type == SDEPTHIMAGE)
        {
            wmsg.MethodID = SDM_GETDEFSIZE_SYSIMAGE;
            decorobj = scr->ScrDecorObj;
        }
        else if ((data->type == AMIGAKEY) || (data->type == MENUCHECK) || (data->type == SUBMENUIMAGE))
        {
            wmsg.MethodID = MDM_GETDEFSIZE_SYSIMAGE;
            decorobj = scr->MenuDecorObj;
        }
        else
        {
            wmsg.MethodID = WDM_GETDEFSIZE_SYSIMAGE;
            decorobj = scr->WinDecorObj;
        }

        DoMethodA(decorobj, (Msg)&wmsg);

        
        if (!set_width) IM(obj)->Width = width;
        if (!set_height) IM(obj)->Height = height;

#ifdef WIN_GADGETS_KEEP_ASPECT
        /*
         * AROS: This little tweak replaces CreateStdSysImage() function
         * and keeps aspect rate of the image if only height is supplied.
         * Currently only OpenScreen() relies on this.
         */
        if (set_height && (!set_width))
            IM(obj)->Width = IM(obj)->Height * width / height;
#endif
    }

    return TRUE;
}

/**************************************************************************************************/

Object *SysIClass__OM_NEW(Class *cl, Class *rootcl, struct opSet *msg)
{
    struct IntuitionBase *IntuitionBase = (struct IntuitionBase *)cl->cl_UserData;
    struct SysIData *data;
    Object          *obj;

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

    /* sysi_setnew() has already rejected every unsupported image type */
    if (data->type == CHECKIMAGE)
    {
        struct TagItem tags[] =
        {
            {IA_FrameType, FRAME_BUTTON },
            {IA_EdgesOnly, FALSE            },
            {TAG_MORE                       }
        };

        tags[2].ti_Data = (IPTR)msg->ops_AttrList;

        data->frame = NewObjectA(NULL, FRAMEICLASS, tags);
        if (!data->frame)
        {
            STACKULONG method = OM_DISPOSE;
            CoerceMethodA(cl, obj, (Msg)&method);
            return NULL;
        }
    }

    return obj;
}

/**************************************************************************************************/

/*
 * The window, screen and menu decorator classes take the same message for
 * drawing a system image and (except for the user buffer, which the menu
 * decorator does not get) for asking its default size, so one structure is
 * filled for whichever of them is responsible for the image type.
 */
#define SYSI_SAMEFIELD(msgtype, field)                                              \
    _Static_assert(offsetof(struct sdp##msgtype, sdp_##field) ==                    \
                   offsetof(struct wdp##msgtype, wdp_##field),                      \
                   "sdp" #msgtype " does not match wdp" #msgtype);                  \
    _Static_assert(offsetof(struct mdp##msgtype, mdp_##field) ==                    \
                   offsetof(struct wdp##msgtype, wdp_##field),                      \
                   "mdp" #msgtype " does not match wdp" #msgtype)

SYSI_SAMEFIELD(DrawSysImage, TrueColor);
SYSI_SAMEFIELD(DrawSysImage, Dri);
SYSI_SAMEFIELD(DrawSysImage, RPort);
SYSI_SAMEFIELD(DrawSysImage, X);
SYSI_SAMEFIELD(DrawSysImage, Y);
SYSI_SAMEFIELD(DrawSysImage, Width);
SYSI_SAMEFIELD(DrawSysImage, Height);
SYSI_SAMEFIELD(DrawSysImage, Which);
SYSI_SAMEFIELD(DrawSysImage, State);
SYSI_SAMEFIELD(DrawSysImage, Flags);
SYSI_SAMEFIELD(DrawSysImage, UserBuffer);
_Static_assert(sizeof(struct sdpDrawSysImage) == sizeof(struct wdpDrawSysImage),
               "sdpDrawSysImage does not match wdpDrawSysImage");
_Static_assert(sizeof(struct mdpDrawSysImage) == sizeof(struct wdpDrawSysImage),
               "mdpDrawSysImage does not match wdpDrawSysImage");

SYSI_SAMEFIELD(GetDefSizeSysImage, TrueColor);
SYSI_SAMEFIELD(GetDefSizeSysImage, Dri);
SYSI_SAMEFIELD(GetDefSizeSysImage, ReferenceFont);
SYSI_SAMEFIELD(GetDefSizeSysImage, Which);
SYSI_SAMEFIELD(GetDefSizeSysImage, SysiSize);
SYSI_SAMEFIELD(GetDefSizeSysImage, Width);
SYSI_SAMEFIELD(GetDefSizeSysImage, Height);
SYSI_SAMEFIELD(GetDefSizeSysImage, Flags);
_Static_assert(offsetof(struct sdpGetDefSizeSysImage, sdp_UserBuffer) ==
               offsetof(struct wdpGetDefSizeSysImage, wdp_UserBuffer),
               "sdpGetDefSizeSysImage does not match wdpGetDefSizeSysImage");
_Static_assert(sizeof(struct mdpGetDefSizeSysImage) <= sizeof(struct wdpGetDefSizeSysImage),
               "mdpGetDefSizeSysImage does not match wdpGetDefSizeSysImage");

/* Let a decorator object of the image's screen draw the image */
static void sysi_decordraw(Class *cl, Object *obj, struct impDraw *msg, ULONG methodid,
                           Object *decorobj, IPTR userbuffer)
{
    struct SysIData         *data = INST_DATA(cl, obj);
    struct wdpDrawSysImage   decormsg;

    decormsg.MethodID       = methodid;
    decormsg.wdp_TrueColor  = (data->dri->dri_Flags & DRIF_DIRECTCOLOR) ? TRUE : FALSE;
    decormsg.wdp_RPort      = msg->imp_RPort;
    decormsg.wdp_X          = (WORD)(IM(obj)->LeftEdge + msg->imp_Offset.X);
    decormsg.wdp_Y          = (WORD)(IM(obj)->TopEdge + msg->imp_Offset.Y);
    decormsg.wdp_Width      = (UWORD)IM(obj)->Width;
    decormsg.wdp_Height     = (UWORD)IM(obj)->Height;
    decormsg.wdp_Which      = data->type;
    decormsg.wdp_State      = msg->imp_State;
    decormsg.wdp_Flags      = 0;
    decormsg.wdp_Dri        = data->dri;
    decormsg.wdp_UserBuffer = userbuffer;

    DoMethodA(decorobj, (Msg)&decormsg);
}

IPTR SysIClass__IM_DRAW(Class *cl, Object *obj, struct impDraw *msg)
{
    struct IntuitionBase    *IntuitionBase = (struct IntuitionBase *)cl->cl_UserData;
    struct GfxBase          *GfxBase = GetPrivIBase(IntuitionBase)->GfxBase;
    struct SysIData         *data = INST_DATA(cl, obj);
    struct RastPort         *rport = msg->imp_RPort;
    struct Window           *win = NULL;
    WORD                     left = IM(obj)->LeftEdge + msg->imp_Offset.X;
    WORD                     top = IM(obj)->TopEdge + msg->imp_Offset.Y;
    UWORD                    width = IM(obj)->Width;
    UWORD                    height = IM(obj)->Height;
    WORD                     right = left + width - 1;
    WORD                     bottom = top + height - 1;
    struct IntScreen        *scr = GetPrivScreen(data->dri->dri_Screen);

    if (rport) if (rport->Layer) win = (struct Window *) rport->Layer->Window;

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
        case UPIMAGE:
        case RIGHTIMAGE:
        case DOWNIMAGE:
        {
            UWORD hspacing,vspacing;
            BOOL  horiz = (data->type == LEFTIMAGE) || (data->type == RIGHTIMAGE);
            BOOL  flip = (data->type == RIGHTIMAGE) || (data->type == DOWNIMAGE);

            if (!(data->flags & (SYSIFLG_NOBORDER | SYSIFLG_GADTOOLS)))
            {
                sysi_decordraw(cl, obj, msg, WDM_DRAW_SYSIMAGE, scr->WinDecorObj,
                               (win == NULL) ? 0 : ((struct IntWindow *)win)->DecorUserBuffer);
                break;
            }

            getarrowspacing(width, height, &hspacing, &vspacing);

            if (!(data->flags & SYSIFLG_NOBORDER))
            {
                drawimageframe(rport, data->type, msg->imp_State, data->dri->dri_Pens,
                                 left, top, width, height, 1, 1, IntuitionBase);
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
                /* Two double-width strokes meeting at the arrow tip. LEFT/RIGHT
                 * and UP/DOWN are mirror images: only the two ends swap. */
                WORD i;

                SetAPen(rport, data->dri->dri_Pens[SHADOWPEN]);

                if (horiz)
                {
                    WORD cy = height / 2;
                    WORD xa = left + width - 1 - hspacing;  /* base of the arrow */
                    WORD xb = left + hspacing;              /* tip of the arrow  */
                    WORD ya[4], yb[4];

                    ya[0] = top + vspacing + 1;             yb[0] = top + height - cy;
                    ya[1] = top + vspacing;                 yb[1] = top + height - cy - 1;
                    ya[2] = top + height - 1- vspacing - 1; yb[2] = top + cy - 1;
                    ya[3] = top + height - 1 - vspacing;    yb[3] = top + cy;

                    if (flip)
                    {
                        WORD t = xa; xa = xb; xb = t;
                    }

                    for (i = 0; i < 4; i++)
                    {
                        Move(rport, xa, ya[i]);
                        Draw(rport, xb, yb[i]);
                    }
                }
                else
                {
                    WORD cx = width / 2;
                    WORD ya = top + height - 1 - vspacing;  /* base of the arrow */
                    WORD yb = top + vspacing;               /* tip of the arrow  */
                    WORD xa[4], xb[4];

                    xa[0] = left + hspacing + 1;             xb[0] = left + width - cx;
                    xa[1] = left + hspacing;                 xb[1] = left + width - cx - 1;
                    xa[2] = left + width - 1 - hspacing - 1; xb[2] = left + cx - 1;
                    xa[3] = left + width - 1 - hspacing;     xb[3] = left + cx;

                    if (flip)
                    {
                        WORD t = ya; ya = yb; yb = t;
                    }

                    for (i = 0; i < 4; i++)
                    {
                        Move(rport, xa[i], ya);
                        Draw(rport, xb[i], yb);
                    }
                }
            }
            else
            {
                SetAPen(rport, getbgpen(msg->imp_State, data->dri->dri_Pens));

                RectFill(rport, left, top, right, bottom);

                left += hspacing;
                top += vspacing;
                width -= hspacing * 2;
                height -= vspacing * 2;

                drawarrowimage(rport, data->type, data->dri->dri_Pens[SHADOWPEN],
                               left, top, width, height, IntuitionBase);
            }
            break;
        }

        case CLOSEIMAGE:
        case ZOOMIMAGE:
        case DEPTHIMAGE:
        case SIZEIMAGE:
        /* MUI and other non-std images */
        case MUIIMAGE:
        case SNAPSHOTIMAGE:
        case POPUPIMAGE:
        case ICONIFYIMAGE:
        case LOCKIMAGE:
            sysi_decordraw(cl, obj, msg, WDM_DRAW_SYSIMAGE, scr->WinDecorObj,
                           (win == NULL) ? 0 : ((struct IntWindow *)win)->DecorUserBuffer);
            break;

        case SDEPTHIMAGE:
            sysi_decordraw(cl, obj, msg, SDM_DRAW_SYSIMAGE, scr->ScrDecorObj, scr->DecorUserBuffer);
            break;

        case MENUCHECK:
        case AMIGAKEY:
        case SUBMENUIMAGE:
            sysi_decordraw(cl, obj, msg, MDM_DRAW_SYSIMAGE, scr->MenuDecorObj, scr->DecorUserBuffer);
            break;

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
