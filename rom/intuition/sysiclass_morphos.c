/*
    Copyright © 1995-2003, The AROS Development Team. All rights reserved.
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
#include <intuition/intuition.h>
#include <intuition/intuitionbase.h>
#include <intuition/screens.h>

#include <proto/graphics.h>
#include <proto/cybergraphics.h>
#include <graphics/rastport.h>
#include <cybergraphx/cybergraphics.h>

#include <proto/utility.h>
#include <utility/tagitem.h>

#include <proto/alib.h>

#include <aros/asmcall.h>
#include "intuition_intern.h"
#include "intuition_customize.h"
#include "intuition_extend.h"
#include "renderwindowframe.h"

#include "gadgets.h" /* Some handy rendering funtions */

#include "sysiclass.h"

extern ULONG HookEntry();

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


/**************************************************************************************************/

#ifndef SYSIDATA
struct SysIData
{
    ULONG                type;
    struct IntDrawInfo  *dri;
    struct Image        *frame;
    UWORD                flags;
    struct Window       *window;
};

#define SYSIFLG_GADTOOLS 1
#define SYSIFLG_NOBORDER 2
#define SYSIFLG_STDLOOK  16
#define SYSIFLG_WINDOWOK 32

void renderimageframe(struct RastPort *rp, ULONG which, ULONG state, UWORD *pens,
                             WORD left, WORD top, WORD width, WORD height,
                             struct IntuitionBase *IntuitionBase);
#endif

/**************************************************************************************************/
#if 0
static UWORD getbgpen(ULONG state, UWORD *pens);
#endif

#undef IntuitionBase
#define IntuitionBase   ((struct IntuitionBase *)(cl->cl_UserData))

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
    struct SysIData *data = INST_DATA(cl, obj);
    struct TagItem *taglist, *tag;
    struct TextFont *reffont = NULL;
    int size = SYSISIZE_MEDRES;
    int def_low_width = DEFSIZE_WIDTH, def_low_height = DEFSIZE_HEIGHT;
    int def_med_width = DEFSIZE_WIDTH, def_med_height = DEFSIZE_HEIGHT;
    int def_high_width = DEFSIZE_WIDTH, def_high_height = DEFSIZE_HEIGHT;
    //ULONG  type = 0;

    BOOL unsupported = FALSE;
    BOOL set_width = FALSE, set_height = FALSE;

    taglist = msg->ops_AttrList;
    while ((tag = NextTagItem((struct TagItem **)&taglist)))
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
            data->dri = (struct IntDrawInfo *)tag->ti_Data;
            reffont = data->dri->dri_Font;
            break;

        case SYSIA_Which:
            data->type = tag->ti_Data;

            D(bug("SYSIA_Which type: %d\n", data->type));

            switch (tag->ti_Data)
            {

#warning if IA_Width, IA_Height was not specified sysiclass should choose size depending on drawinfo (screen resolution)

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
            case MENUTOGGLEIMAGE:
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

        case IA_Window:
            if (tag->ti_Data)
            {
                data->window = (struct Window *)tag->ti_Data;
                data->flags |= SYSIFLG_WINDOWOK;
            }
            break;

        } /* switch(tag->ti_Tag) */

    } /* while ((tag = NextTagItem(&taglist))) */

    D(bug("dri: %p, unsupported: %d\n", data->dri, unsupported));

    if ((!data->dri) || (unsupported))
        return FALSE;

    switch(data->type)
    {
    case LEFTIMAGE:
    case RIGHTIMAGE:
        def_low_width = 16;
        def_med_width = 16;
        def_high_width = 23;
        def_low_height = 11;
        def_med_height = 10;
        def_high_height = 22;
        break;

    case UPIMAGE:
    case DOWNIMAGE:
        def_low_width = 13;
        def_med_width = 18;
        def_high_width = 23;
        def_low_height = 11;
        def_med_height = 11;
        def_high_height = 22;
        break;

    case DEPTHIMAGE:
    case ZOOMIMAGE:
        def_low_width = 18;
        def_med_width = 24;
        def_high_width = 24;
        break;

    case ICONIFYIMAGE:
    case LOCKIMAGE:
    case MUIIMAGE:
    case POPUPIMAGE:
    case SNAPSHOTIMAGE:
    case JUMPIMAGE:
        def_low_width = 18;
        def_med_width = 24;
        def_high_width = 24;
        break;

    case SDEPTHIMAGE:
        def_low_width = 17;
        def_med_width = 23;
        def_high_width = 23;
        break;

    case CLOSEIMAGE:
        def_low_width = 15;
        def_med_width = 20;
        def_high_width = 20;

        break;

    case SIZEIMAGE:
        def_low_width = 13;
        def_med_width = 18;
        def_high_width = 18;
        def_low_height = 11;
        def_med_height = 10;
        def_high_height = 10;
        break;

    case MENUCHECK:
        //def_low_width  = reffont->tf_XSize * 3 / 2;
        //jDc: what's wrong with 1:1 ratio?? ;)
        def_low_width = reffont->tf_YSize;
        def_low_height = reffont->tf_YSize;
        size = SYSISIZE_LOWRES;
        break;

    case AMIGAKEY:
#if MENUS_AMIGALOOK
        def_low_width  = reffont->tf_XSize * 2;
        def_low_height = reffont->tf_YSize;
#else
        def_low_width  = reffont->tf_XSize * 2;
        def_low_height = reffont->tf_YSize + 1;
#endif
        size = SYSISIZE_LOWRES;
        break;

    case MENUTOGGLEIMAGE:
    case SUBMENUIMAGE:
        def_low_width = reffont->tf_YSize;
        def_low_height = reffont->tf_YSize;
        size = SYSISIZE_LOWRES;
        break;

    case MXIMAGE:
        /*
         * We really need some aspect ratio here..this sucks
         */
        def_low_width  = reffont->tf_XSize * 3 - 1;
        def_low_height = reffont->tf_YSize + 1;
        size = SYSISIZE_LOWRES;
        break;

    case CHECKIMAGE:
        /*
         * We really need some aspect ratio here..this sucks
         */
        def_low_width  = 26;//reffont->tf_XSize * 2;
        def_low_height = reffont->tf_YSize + 3;
        size = SYSISIZE_LOWRES;
        break;

    } /* switch(data->type) */

    if (!set_width)
        IM(obj)->Width = size == SYSISIZE_LOWRES ? def_low_width :
                         (size == SYSISIZE_HIRES ? def_high_width : def_med_width);
    if (!set_height)
        IM(obj)->Height = size == SYSISIZE_LOWRES ? def_low_height :
                          (size == SYSISIZE_HIRES ? def_high_height : def_med_height);

    sysi_skinnew(cl,obj,set_width,set_height,IntuitionBase);

    return TRUE;
}

/**************************************************************************************************/

Object *sysi_new(Class *cl, Class *rootcl, struct opSet *msg)
{
    struct SysIData *data;
    Object *obj;

    D(bug("sysi_new()\n"));
    obj = (Object *)DoSuperMethodA(cl, (Object *)rootcl, (Msg)msg);
    if (!obj)
        return NULL;

    D(bug("sysi_new,: obj=%p\n", obj));

    data = INST_DATA(cl, obj);
    data->type = 0L;
    data->dri = NULL;
    data->frame = NULL;
    data->flags = 0;
    if (!sysi_setnew(cl, obj, (struct opSet *)msg))
    {
        ULONG method = OM_DISPOSE;
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
                {IA_FrameType, FRAME_BUTTON},
                {IA_EdgesOnly, FALSE},
                {TAG_MORE, 0L}
            };

            tags[2].ti_Data = (IPTR)msg->ops_AttrList;
            data->frame = NewObjectA(NULL, FRAMEICLASS, tags);
            if (!data->frame)
            {
                ULONG method = OM_DISPOSE;
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
    case MENUTOGGLEIMAGE:
    case SUBMENUIMAGE:
        break;

    default:
        {
            ULONG method = OM_DISPOSE;
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

void sysi_draw(Class *cl, Object *obj, struct impDraw *msg)
{
    struct SysIData *data = INST_DATA(cl, obj);
    struct RastPort *rport = msg->imp_RPort;
    WORD left = IM(obj)->LeftEdge + msg->imp_Offset.X;
    WORD top = IM(obj)->TopEdge + msg->imp_Offset.Y;
    UWORD width = IM(obj)->Width;
    UWORD height = IM(obj)->Height;
    WORD right = left + width - 1;
    WORD bottom = top + height - 1;
    struct sysiclassprefs *sysiclassprefs;

    SANITY_CHECK(rport)

    SetDrMd(rport, JAM1);

    if (sysi_drawskin(cl,obj,msg,IntuitionBase)) return;

    sysiclassprefs = (struct sysiclassprefs *)int_GetCustomPrefs(TYPE_SYSICLASS,data->dri,IntuitionBase);

    switch(data->type)
    {
    case CHECKIMAGE:
        {
            WORD h_spacing = width / 4;
            WORD v_spacing = height / 4;

            /* Draw frame */
            DrawImageState(rport, data->frame,
                           msg->imp_Offset.X, msg->imp_Offset.Y,
                           IDS_NORMAL, (struct DrawInfo *)data->dri);

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

#if 0
                draw_thick_line(cl, rport, left, top + height/2, left + width/3, bottom, 0);
                draw_thick_line(cl, rport, left + width/3, bottom, right - 2, top, 0);
                Move(rport, right -1 , top);
                Draw(rport, right, top);
#else
draw_thick_line(cl, rport, left, top + height / 3 , left, bottom, 0);
                draw_thick_line(cl, rport, left + 1, bottom, right - 1, top, 0);
#endif

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

    case MENUCHECK:
    case SUBMENUIMAGE:
    case MENUTOGGLEIMAGE:
        {
            UWORD *pens = data->dri->dri_Pens;

//#if MENUS_AMIGALOOK
//            SetAPen(rport, pens[BARBLOCKPEN]);
//#else
//            SetAPen(rport, pens[(msg->imp_State == IDS_SELECTED) ? FILLPEN : BACKGROUNDPEN]);
//#endif

//            if (hd.image)
//            {
//                /*EraseRect(rport,left,top,right,bottom);*/
//                /*menutask will handle this!*/
//            }
//            else
//            {
//                RectFill(rport,left, top, right, bottom);
//            }

            SetAPen(rport, pens[BARDETAILPEN]);
            
            switch (data->type)
            {
                case MENUCHECK:
                    draw_thick_line(cl, rport, left + 1, top + height / 3 , left + 1, bottom, 0);
                    draw_thick_line(cl, rport, left + 2, bottom, right - 2, top, 0);
                    break;

                case MENUTOGGLEIMAGE:
                    RectFill(rport,left,top,left + width - 1,top);
                    RectFill(rport,left,top + height - 1,left + width - 1,top + height - 1);
                    RectFill(rport,left,top,left,top + height - 1);
                    RectFill(rport,left + width - 1, top, left + width - 1, top + height -1);
                    left++;top++;width -= 2; height -=2;
                    if (msg->imp_State == IDS_SELECTED)
                    {
                        draw_thick_line(cl, rport, left + 1, top + height / 3 , left + 1, bottom, 0);
                        draw_thick_line(cl, rport, left + 2, bottom, right - 2, top, 0);
                    }
                    break;

                case SUBMENUIMAGE:
                    Move(rport,left + width - 2, top + height/2);
                    Draw(rport,left + width - 6, top + height/2 - 2);
                    Move(rport,left + width - 2, top + height/2);
                    Draw(rport,left + width - 6, top + height/2 + 2);
                    break;
            }

            break;
        }

    case MUIIMAGE:
        {
            //UWORD *pens = data->dri->dri_Pens;

            int_RenderWindowFrame(data->window,left,top,width,height,data->dri,IntuitionBase);

            if (!(data->flags & SYSIFLG_NOBORDER))
            {
                if (data->window)
                {
                    int_RenderWindowFrameBorder(data->window,left,top,width,height,data->dri,IntuitionBase);
                }
                else
                {
                    renderimageframe(rport, data->type, msg->imp_State, data->dri->dri_Pens,
                                     left, top, width, height, IntuitionBase);
                }
                left++;
                top++;
                right--;
                bottom--;
                width -= 2;
                height -= 2;
            }

            /* DRAW IMAGE :) */

            DrawIB(rport,(BYTE*)muiimage[SIM_PREFS],left+(width/2),top+(height/2),IntuitionBase);

            break;
        }

    case SNAPSHOTIMAGE:
    case POPUPIMAGE:
    case ICONIFYIMAGE:
    case LOCKIMAGE:
        {
            //UWORD *pens = data->dri->dri_Pens;

            int_RenderWindowFrame(data->window,left,top,width,height,data->dri,IntuitionBase);

            if (!(data->flags & SYSIFLG_NOBORDER))
            {
                if (data->window)
                {
                    int_RenderWindowFrameBorder(data->window,left,top,width,height,data->dri,IntuitionBase);
                }
                else
                {
                    renderimageframe(rport, data->type, msg->imp_State, data->dri->dri_Pens,
                                     left, top, width, height, IntuitionBase);
                }
                left++;
                top++;
                right--;
                bottom--;
                width -= 2;
                height -= 2;
            }

            /* DRAW IMAGE :) */

            if (data->type == SNAPSHOTIMAGE)
            {
                if (msg->imp_State == IDS_SELECTED)
                {
                    DrawIB(rport,(BYTE*)muiimage[SIM_SNAPSHOTSEL],left+(width/2),top+(height/2),IntuitionBase);
                }
                else
                {
                    DrawIB(rport,(BYTE*)muiimage[SIM_SNAPSHOT],left+(width/2),top+(height/2),IntuitionBase);
                }
            }

            if (data->type == POPUPIMAGE)
            {
                if (msg->imp_State == IDS_SELECTED)
                {
                    DrawIB(rport,(BYTE*)muiimage[SIM_POPUPSEL],left+(width/2),top+(height/2),IntuitionBase);
                }
                else
                {
                    DrawIB(rport,(BYTE*)muiimage[SIM_POPUP],left+(width/2),top+(height/2),IntuitionBase);
                }
            }

            if (data->type == ICONIFYIMAGE)
            {
                if (msg->imp_State == IDS_SELECTED)
                {
                    DrawIB(rport,(BYTE*)muiimage[SIM_ICONIFYSEL],left+(width/2),top+(height/2),IntuitionBase);
                }
                else
                {
                    DrawIB(rport,(BYTE*)muiimage[SIM_ICONIFY],left+(width/2),top+(height/2),IntuitionBase);
                }
            }

            if (data->type == LOCKIMAGE)
            {
                if ((msg->imp_State == IDS_SELECTED) || (msg->imp_State == IDS_INACTIVESELECTED))
                {
                    DrawIB(rport,(BYTE*)muiimage[SIM_LOCKSEL],left+(width/2),top+(height/2),IntuitionBase);
                }
                else
                {
                    DrawIB(rport,(BYTE*)muiimage[SIM_LOCK],left+(width/2),top+(height/2),IntuitionBase);
                }
            }

            break;
        }

    case JUMPIMAGE:
        {
            //UWORD *pens = data->dri->dri_Pens;

            int_RenderWindowFrame(data->window,left,top,width,height,data->dri,IntuitionBase);

            if (!(data->flags & SYSIFLG_NOBORDER))
            {
                if (data->window)
                {
                    int_RenderWindowFrameBorder(data->window,left,top,width,height,data->dri,IntuitionBase);
                }
                else
                {
                    renderimageframe(rport, data->type, msg->imp_State, data->dri->dri_Pens,
                                     left, top, width, height, IntuitionBase);
                }
                left++;
                top++;
                right--;
                bottom--;
                width -= 2;
                height -= 2;
            }

            /* DRAW IMAGE :) */

            DrawJUMP(rport,msg->imp_State,left+(width/2),top+(height/2),IntuitionBase);

            break;
        }

    } /* switch (image type) */
    int_FreeCustomPrefs(TYPE_SYSICLASS,data->dri,IntuitionBase);
    return;
}

/**************************************************************************************************/

AROS_UFH3S(IPTR, dispatch_sysiclass,
           AROS_UFHA(Class *,  cl,  A0),
           AROS_UFHA(Object *, obj, A2),
           AROS_UFHA(Msg,      msg, A1)
          )
{
    AROS_USERFUNC_INIT

    IPTR retval = 0UL;
    struct SysIData *data;

    D(bug(dprintf("dispatch_sysiclass: class 0x%lx object 0x%lx\n",cl,obj);))

    switch (msg->MethodID)
    {
    case OM_NEW:

        D(bug(dprintf("dispatch_sysiclass: OM_NEW\n");))

        retval = (IPTR)sysi_new(cl, (Class *)obj, (struct opSet *)msg);
        break;

    case OM_DISPOSE:
        D(bug(dprintf("dispatch_sysiclass: OM_DISPOSE\n");))

        data = INST_DATA(cl, obj);
        DisposeObject(data->frame);
        retval = DoSuperMethodA(cl, obj, msg);
        break;

    case OM_SET:

        D(bug(dprintf("dispatch_sysiclass: OM_SET\n");))

        data = INST_DATA(cl, obj);
        if (data->frame)
            DoMethodA((Object *)data->frame, msg);
        retval = DoSuperMethodA(cl, obj, msg);
        break;

    case IM_DRAW:

        D(bug(dprintf("dispatch_sysiclass: IM_DRAW\n");))

        sysi_draw(cl, obj, (struct impDraw *)msg);
        break;

    default:
        retval = DoSuperMethodA(cl, obj, msg);
        break;
    }

    D(bug(dprintf("dispatch_sysiclass: done\n");))

    return retval;

    AROS_USERFUNC_EXIT
}

/**************************************************************************************************/

#undef IntuitionBase

/**************************************************************************************************/

/* Initialize our class. */
struct IClass *InitSysIClass (struct IntuitionBase * IntuitionBase)
{
    struct IClass *cl = NULL;

    cl = MakeClass(SYSICLASS, IMAGECLASS, NULL, sizeof(struct SysIData), 0);
    if (cl == NULL)
        return NULL;

    cl->cl_Dispatcher.h_Entry    = (APTR)AROS_ASMSYMNAME(dispatch_sysiclass);
    cl->cl_Dispatcher.h_SubEntry = NULL;
    cl->cl_UserData              = (IPTR)IntuitionBase;

    AddClass (cl);

    return (cl);
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

    case IDS_INACTIVENORMAL:
        bg = pens[BACKGROUNDPEN];
        break;
    default:
        bg = pens[BACKGROUNDPEN];
        break;
    }
    return bg;
}
#endif

/**************************************************************************************************/

void renderimageframe(struct RastPort *rp, ULONG which, ULONG state, UWORD *pens,
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
