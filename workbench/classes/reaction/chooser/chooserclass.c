/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: Reaction chooser.gadget - BOOPSI class implementation
*/
#define DEBUG 0

#include <proto/exec.h>
#include <proto/intuition.h>
#include <proto/utility.h>
#include <proto/graphics.h>
#include <proto/alib.h>

#include <graphics/gfxmacros.h>
#include <exec/memory.h>
#include <intuition/intuition.h>
#include <intuition/classes.h>
#include <intuition/classusr.h>
#include <intuition/cghooks.h>
#include <intuition/gadgetclass.h>
#include <intuition/imageclass.h>
#include <gadgets/chooser.h>
#include <utility/tagitem.h>

#include <string.h>

#include "chooser_intern.h"
#include "../reaction_box.h"

#define ChooserBase ((struct Library *)(cl->cl_UserData))

/******************************************************************************/

static LONG chooser_count_labels(struct List *labels)
{
    LONG count = 0;

    if (labels)
    {
        struct MinNode *node;
        for (node = (struct MinNode *)labels->lh_Head;
             node->mln_Succ;
             node = node->mln_Succ)
        {
            count++;
        }
    }
    return count;
}

static struct ChooserNode *chooser_get_node(struct List *labels, LONG index)
{
    if (labels)
    {
        struct MinNode *node;
        LONG i = 0;
        for (node = (struct MinNode *)labels->lh_Head;
             node->mln_Succ;
             node = node->mln_Succ, i++)
        {
            if (i == index)
                return (struct ChooserNode *)node;
        }
    }
    return NULL;
}

/******************************************************************************/

static void chooser_free_own(struct ChooserData *data)
{
    struct Node *n;

    if (!data->cd_OwnLabels)
        return;
    while ((n = RemHead(data->cd_OwnLabels)))
        FreeVec(n);
    FreeVec(data->cd_OwnLabels);
    if (data->cd_Labels == data->cd_OwnLabels)
        data->cd_Labels = NULL;
    data->cd_OwnLabels = NULL;
}

/* CHOOSER_LabelArray: a NULL-terminated STRPTR array becomes the class's own list */
static void chooser_label_array(struct ChooserData *data, STRPTR *array)
{
    struct List *l;

    chooser_free_own(data);
    if (!array || !(l = AllocVec(sizeof(struct List), MEMF_ANY)))
        return;
    NewList(l);
    for (; *array; array++)
    {
        struct ChooserNode *cn = AllocVec(sizeof(struct ChooserNode), MEMF_CLEAR);
        if (!cn)
            break;
        cn->cn_Text = *array;
        AddTail(l, (struct Node *)cn);
    }
    data->cd_OwnLabels = l;
    data->cd_Labels = l;
    data->cd_NumLabels = chooser_count_labels(l);
}

static void chooser_set(Class *cl, Object *o, struct opSet *msg)
{
    struct ChooserData *data = INST_DATA(cl, o);
    struct TagItem *tags = msg->ops_AttrList;
    struct TagItem *tag;

    while ((tag = NextTagItem(&tags)))
    {
        switch (tag->ti_Tag)
        {
            case CHOOSER_LabelArray:
                chooser_label_array(data, (STRPTR *)tag->ti_Data);
                break;
            case CHOOSER_Labels:
                chooser_free_own(data);
                data->cd_Labels = (struct List *)tag->ti_Data;
                data->cd_NumLabels = chooser_count_labels(data->cd_Labels);
                break;
            case CHOOSER_Active:
                data->cd_Selected = (LONG)tag->ti_Data;
                break;
            case CHOOSER_MaxLabels:
                data->cd_MaxLabels = (LONG)tag->ti_Data;
                break;
            case CHOOSER_DropDown:
                data->cd_DropDown = (BOOL)tag->ti_Data;
                break;
            case CHOOSER_AutoFit:
                data->cd_AutoFit = (BOOL)tag->ti_Data;
                break;
            case CHOOSER_Title:
                data->cd_Title = (STRPTR)tag->ti_Data;
                break;
            case CHOOSER_PopUp:
                data->cd_PopUp = (BOOL)tag->ti_Data;
                break;
            case CHOOSER_Hidden:
                data->cd_Hidden = (BOOL)tag->ti_Data;
                break;
        }
    }
}

/******************************************************************************/

IPTR Chooser__OM_NEW(Class *cl, Object *o, struct opSet *msg)
{
    IPTR retval;

    D(bug("[Chooser] OM_NEW: enter\n"));

    retval = DoSuperMethodA(cl, o, (Msg)msg);
    D(bug("[Chooser] OM_NEW: obj=%p\n", (void *)retval));
    if (retval)
    {
        struct ChooserData *data = INST_DATA(cl, (Object *)retval);

        memset(data, 0, sizeof(struct ChooserData));
        data->cd_Selected = 0;
        data->cd_Hot = -1;

        chooser_set(cl, (Object *)retval, msg);
    }

    return retval;
}

/******************************************************************************/

IPTR Chooser__OM_DISPOSE(Class *cl, Object *o, Msg msg)
{
    struct ChooserData *data = INST_DATA(cl, o);

    D(bug("[Chooser] OM_DISPOSE: obj=%p\n", (void *)o));

    chooser_popup_close(data);
    chooser_free_own(data);
    return DoSuperMethodA(cl, o, msg);
}

/******************************************************************************/

IPTR Chooser__OM_SET(Class *cl, Object *o, struct opSet *msg)
{
    D(bug("[Chooser] OM_SET: obj=%p\n", (void *)o));

    IPTR retval = DoSuperMethodA(cl, o, (Msg)msg);
    chooser_set(cl, o, msg);
    return retval;
}

/******************************************************************************/

IPTR Chooser__OM_GET(Class *cl, Object *o, struct opGet *msg)
{
    struct ChooserData *data = INST_DATA(cl, o);

    switch (msg->opg_AttrID)
    {
        case CHOOSER_Active:
            *msg->opg_Storage = data->cd_Selected;
            return TRUE;

        case CHOOSER_Labels:
            *msg->opg_Storage = (IPTR)data->cd_Labels;
            return TRUE;

        case CHOOSER_DropDown:
            *msg->opg_Storage = data->cd_DropDown;
            return TRUE;

        case CHOOSER_Title:
            *msg->opg_Storage = (IPTR)data->cd_Title;
            return TRUE;

        case CHOOSER_MaxLabels:
            *msg->opg_Storage = data->cd_MaxLabels;
            return TRUE;
    }

    return DoSuperMethodA(cl, o, (Msg)msg);
}

/******************************************************************************/

static void chooser_draw_arrow(struct RastPort *rp, WORD x, WORD y,
                               WORD w, WORD h, UWORD pen)
{
    WORD cx = x + w / 2;
    WORD ay = y + h / 3;
    WORD by = y + (h * 2) / 3;

    SetAPen(rp, pen);
    Move(rp, cx - 4, ay);
    Draw(rp, cx, by);
    Draw(rp, cx + 4, ay);
}

/* the pop-up glyph: two left-pointing chevrons, two pixels thick */
static void chooser_draw_chevrons(struct RastPort *rp, WORD x, WORD y, WORD w, WORD h, UWORD pen)
{
    WORD cy = y + h / 2, half = (h - 6) / 2, i, cx;

    if (half < 2) half = 2;
    if (half > 5) half = 5;
    SetAPen(rp, pen);
    for (i = 0; i < 2; i++)
    {
        cx = x + w / 2 - half + i * (half + 1) - 1;
        Move(rp, cx + half, cy - half); Draw(rp, cx, cy); Draw(rp, cx + half, cy + half);
        Move(rp, cx + half + 1, cy - half); Draw(rp, cx + 1, cy); Draw(rp, cx + half + 1, cy + half);
    }
}

IPTR Chooser__GM_RENDER(Class *cl, Object *o, struct gpRender *msg)
{
    D(bug("[Chooser] GM_RENDER: obj=%p redraw=%ld\n", (void *)o, msg->gpr_Redraw));

    struct ChooserData *data = INST_DATA(cl, o);
    struct RastPort *rp = msg->gpr_RPort;
    struct DrawInfo *dri = msg->gpr_GInfo ? msg->gpr_GInfo->gi_DrInfo : NULL;
    struct Gadget *gad = G(o);
    WORD x, y, w, h;
    UWORD shine, shadow, bg, text_pen;
    STRPTR label = NULL;

    if (!rp && msg->gpr_GInfo)
        rp = ObtainGIRPort(msg->gpr_GInfo);

    if (!rp)
        return FALSE;

    reaction_gadget_box(gad, msg->gpr_GInfo, &x, &y, &w, &h);

    shine   = dri ? dri->dri_Pens[SHINEPEN]       : 2;
    shadow  = dri ? dri->dri_Pens[SHADOWPEN]      : 1;
    bg      = dri ? dri->dri_Pens[BACKGROUNDPEN]  : 0;
    text_pen = dri ? dri->dri_Pens[TEXTPEN]       : 1;

    /* Raised frame, as a button */
    SetAPen(rp, shine);
    Move(rp, x, y + h - 1);
    Draw(rp, x, y);
    Draw(rp, x + w - 1, y);

    SetAPen(rp, shadow);
    Move(rp, x + 1, y + h - 1);
    Draw(rp, x + w - 1, y + h - 1);
    Draw(rp, x + w - 1, y + 1);

    /* Fill background */
    SetAPen(rp, bg);
    RectFill(rp, x + 1, y + 1, x + w - 2, y + h - 2);

    /* Get selected item text */
    if (data->cd_Labels && data->cd_Selected >= 0)
    {
        struct ChooserNode *cn = chooser_get_node(data->cd_Labels,
                                                  data->cd_Selected);
        if (cn)
            label = cn->cn_Text;
    }
    if (!label && data->cd_Title)
        label = data->cd_Title;

    /* Draw label text, centred in the area left of the glyph box */
    if (label)
    {
        WORD area = w - 18 - 2;
        WORD tw = TextLength(rp, label, strlen(label));
        WORD tx = x + 1 + (tw < area ? (area - tw) / 2 : 3);
        WORD ty = y + (h - rp->TxHeight) / 2 + rp->TxBaseline;

        SetAPen(rp, text_pen);
        SetDrMd(rp, JAM1);
        Move(rp, tx, ty);
        Text(rp, label, strlen(label));
    }

    /* Glyph box on the right: a down arrow for a drop-down menu, the
       double chevron of a pop-up list */
    {
        WORD aw = 16;

        /* Separator line */
        SetAPen(rp, shadow);
        Move(rp, x + w - aw - 1, y + 1);
        Draw(rp, x + w - aw - 1, y + h - 2);
        SetAPen(rp, shine);
        Move(rp, x + w - aw, y + 1);
        Draw(rp, x + w - aw, y + h - 2);

        if (data->cd_DropDown)
            chooser_draw_arrow(rp, x + w - aw, y, aw, h, text_pen);
        else
            chooser_draw_chevrons(rp, x + w - aw, y, aw, h, text_pen);
    }

    /* Disabled overlay */
    if (gad->Flags & GFLG_DISABLED)
    {
        ULONG pattern[] = { 0x88888888, 0x22222222 };
        SetAPen(rp, bg);
        SetAfPt(rp, (UWORD *)pattern, 1);
        RectFill(rp, x, y, x + w - 1, y + h - 1);
        SetAfPt(rp, NULL, 0);
    }

    if (!msg->gpr_RPort && msg->gpr_GInfo)
        ReleaseGIRPort(rp);

    return TRUE;
}

/******************************************************************************/

static BOOL chooser_selectable(struct ChooserData *data, LONG i)
{
    struct ChooserNode *cn = chooser_get_node(data->cd_Labels, i);
    return cn && !cn->cn_Disabled && !cn->cn_Separator && !cn->cn_ReadOnly;
}

static void chooser_popup_draw(Class *cl, struct ChooserData *data, struct DrawInfo *dri)
{
    struct Window *pw = data->cd_PopWin;
    struct RastPort *rp = pw->RPort;
    UWORD shine = dri ? dri->dri_Pens[SHINEPEN] : 2, shadow = dri ? dri->dri_Pens[SHADOWPEN] : 1;
    UWORD bg = dri ? dri->dri_Pens[BACKGROUNDPEN] : 0, text = dri ? dri->dri_Pens[TEXTPEN] : 1;
    UWORD fill = dri ? dri->dri_Pens[FILLPEN] : 3, filltext = dri ? dri->dri_Pens[FILLTEXTPEN] : 2;
    WORD w = pw->Width, h = pw->Height, i = 0;
    struct MinNode *node;

    SetAPen(rp, bg);
    RectFill(rp, 0, 0, w - 1, h - 1);
    SetAPen(rp, shine);
    Move(rp, 0, h - 1); Draw(rp, 0, 0); Draw(rp, w - 1, 0);
    SetAPen(rp, shadow);
    Move(rp, 1, h - 1); Draw(rp, w - 1, h - 1); Draw(rp, w - 1, 1);

    for (node = (struct MinNode *)data->cd_Labels->lh_Head; node->mln_Succ; node = node->mln_Succ, i++)
    {
        struct ChooserNode *cn = (struct ChooserNode *)node;
        WORD y = 2 + i * data->cd_ItemH;

        if (cn->cn_Separator)
        {
            SetAPen(rp, shadow);
            Move(rp, 4, y + data->cd_ItemH / 2 - 1); Draw(rp, w - 5, y + data->cd_ItemH / 2 - 1);
            SetAPen(rp, shine);
            Move(rp, 4, y + data->cd_ItemH / 2); Draw(rp, w - 5, y + data->cd_ItemH / 2);
            continue;
        }
        if (i == data->cd_Hot)
        {
            SetAPen(rp, fill);
            RectFill(rp, 2, y, w - 3, y + data->cd_ItemH - 1);
        }
        if (cn->cn_Text)
        {
            SetAPen(rp, i == data->cd_Hot ? filltext : text);
            SetDrMd(rp, JAM1);
            Move(rp, 6, y + 1 + rp->TxBaseline);
            Text(rp, cn->cn_Text, strlen(cn->cn_Text));
        }
        if (cn->cn_Disabled)
        {
            UWORD ghost[] = { 0x8888, 0x2222 };
            SetAPen(rp, bg);
            SetAfPt(rp, ghost, 1);
            RectFill(rp, 2, y, w - 3, y + data->cd_ItemH - 1);
            SetAfPt(rp, NULL, 0);
        }
    }
}

/* open the list at screen position (sx, sy) */
static BOOL chooser_popup_open(Class *cl, Object *o, struct ChooserData *data,
                               struct GadgetInfo *gi, WORD sx, WORD sy)
{
    struct Window *win = gi ? gi->gi_Window : NULL;
    struct Screen *scr = gi ? gi->gi_Screen : NULL;
    struct RastPort *wrp = win ? win->RPort : NULL;
    struct MinNode *node;
    WORD w = G(o)->Width, h;

    if (!win || !scr || !data->cd_Labels || data->cd_NumLabels <= 0)
        return FALSE;
    data->cd_ItemH = wrp->TxHeight + 2;
    for (node = (struct MinNode *)data->cd_Labels->lh_Head; node->mln_Succ; node = node->mln_Succ)
    {
        struct ChooserNode *cn = (struct ChooserNode *)node;
        if (cn->cn_Text)
        {
            WORD tw = TextLength(wrp, cn->cn_Text, strlen(cn->cn_Text)) + 12;
            if (tw > w) w = tw;
        }
    }
    h = data->cd_NumLabels * data->cd_ItemH + 4;
    if (h > scr->Height) h = scr->Height;
    if (sx + w > scr->Width) sx = scr->Width - w;
    if (sy + h > scr->Height) sy = scr->Height - h;
    if (sx < 0) sx = 0;
    if (sy < 0) sy = 0;

    data->cd_PopWin = OpenWindowTags(NULL,
        WA_CustomScreen, (IPTR)scr,
        WA_Left, sx, WA_Top, sy, WA_Width, w, WA_Height, h,
        WA_Borderless, TRUE, WA_SmartRefresh, TRUE, WA_RMBTrap, TRUE,
        WA_Activate, FALSE, WA_AutoAdjust, TRUE,
        TAG_DONE);
    if (!data->cd_PopWin)
        return FALSE;
    SetFont(data->cd_PopWin->RPort, wrp->Font);
    data->cd_Hot = data->cd_Selected;
    data->cd_Sticky = FALSE;
    chooser_popup_draw(cl, data, gi->gi_DrInfo);
    return TRUE;
}

void chooser_popup_close(struct ChooserData *data)
{
    if (data->cd_PopWin)
    {
        CloseWindow(data->cd_PopWin);
        data->cd_PopWin = NULL;
    }
    data->cd_Hot = -1;
}

/* entry under the pointer, -1 outside the list; gpi_Mouse is relative
   to the gadget, which stays the active one while the list is open */
static LONG chooser_popup_hit(struct ChooserData *data, Object *o, struct gpInput *msg)
{
    struct Window *pw = data->cd_PopWin, *win = msg->gpi_GInfo->gi_Window;
    WORD mx = win->LeftEdge + G(o)->LeftEdge + msg->gpi_Mouse.X - pw->LeftEdge;
    WORD my = win->TopEdge + G(o)->TopEdge + msg->gpi_Mouse.Y - pw->TopEdge;
    LONG i;

    if (mx < 0 || mx >= pw->Width || my < 2 || my >= pw->Height - 2)
        return -1;
    i = (my - 2) / data->cd_ItemH;
    return i < data->cd_NumLabels ? i : -1;
}

static void chooser_redraw(Object *o, struct GadgetInfo *gi)
{
    struct RastPort *rp = ObtainGIRPort(gi);
    if (rp)
    {
        struct gpRender render;
        render.MethodID   = GM_RENDER;
        render.gpr_GInfo  = gi;
        render.gpr_RPort  = rp;
        render.gpr_Redraw = GREDRAW_REDRAW;
        DoMethodA(o, (Msg)&render);
        ReleaseGIRPort(rp);
    }
}

/******************************************************************************/

IPTR Chooser__GM_GOACTIVE(Class *cl, Object *o, struct gpInput *msg)
{
    D(bug("[Chooser] GM_GOACTIVE: obj=%p\n", (void *)o));

    struct ChooserData *data = INST_DATA(cl, o);
    struct Gadget *gad = G(o);

    if (gad->Flags & GFLG_DISABLED)
        return GMR_NOREUSE;

    data->cd_Active = TRUE;
    data->cd_HideReq = FALSE;

    if (msg->gpi_GInfo)
    {
        struct Window *win = msg->gpi_GInfo->gi_Window;
        WORD sx = win->LeftEdge + gad->LeftEdge, sy = win->TopEdge + gad->TopEdge + gad->Height;

        if (data->cd_ShowAt)
        {
            sx = win->LeftEdge + data->cd_ShowX;
            sy = win->TopEdge + data->cd_ShowY;
            data->cd_ShowAt = FALSE;
        }
        chooser_popup_open(cl, o, data, msg->gpi_GInfo, sx, sy);
    }

    return GMR_MEACTIVE;
}

/******************************************************************************/

IPTR Chooser__GM_HANDLEINPUT(Class *cl, Object *o, struct gpInput *msg)
{
    D(bug("[Chooser] GM_HANDLEINPUT: obj=%p\n", (void *)o));

    struct ChooserData *data = INST_DATA(cl, o);
    struct Gadget *gad = G(o);
    struct InputEvent *ie = msg->gpi_IEvent;

    if (!ie)
        return GMR_MEACTIVE;

    if (data->cd_HideReq)
    {
        data->cd_HideReq = FALSE;
        chooser_popup_close(data);
        return GMR_NOREUSE;
    }

    if (data->cd_PopWin && ie->ie_Class == IECLASS_RAWMOUSE)
    {
        LONG hit = chooser_popup_hit(data, o, msg);

        if (hit != data->cd_Hot && (hit < 0 || chooser_selectable(data, hit)))
        {
            data->cd_Hot = hit;
            chooser_popup_draw(cl, data, msg->gpi_GInfo->gi_DrInfo);
        }
        switch (ie->ie_Code)
        {
        case SELECTUP:
            if (hit >= 0 && chooser_selectable(data, hit))
            {
                data->cd_Selected = hit;
                chooser_popup_close(data);
                chooser_redraw(o, msg->gpi_GInfo);
                *msg->gpi_Termination = data->cd_Selected;
                return GMR_NOREUSE | GMR_VERIFY;
            }
            /* a click on the gadget itself leaves the list open for the next click */
            if (!data->cd_Sticky && msg->gpi_Mouse.X >= 0 && msg->gpi_Mouse.X < gad->Width &&
                msg->gpi_Mouse.Y >= 0 && msg->gpi_Mouse.Y < gad->Height)
            {
                data->cd_Sticky = TRUE;
                return GMR_MEACTIVE;
            }
            chooser_popup_close(data);
            return GMR_NOREUSE;
        case MENUDOWN:
            chooser_popup_close(data);
            return GMR_REUSE;
        }
        return GMR_MEACTIVE;
    }

    if (ie->ie_Class == IECLASS_RAWMOUSE)
    {
        if (ie->ie_Code == SELECTUP)
        {
            WORD mx = msg->gpi_Mouse.X;
            WORD my = msg->gpi_Mouse.Y;

            /* Check if mouse is within gadget bounds */
            if (mx >= 0 && mx < gad->Width &&
                my >= 0 && my < gad->Height)
            {
                /* Cycle to next item */
                if (data->cd_NumLabels > 0)
                {
                    data->cd_Selected++;
                    if (data->cd_Selected >= data->cd_NumLabels)
                        data->cd_Selected = 0;
                }

                /* Redraw */
                struct RastPort *rp = ObtainGIRPort(msg->gpi_GInfo);
                if (rp)
                {
                    struct gpRender render;
                    render.MethodID   = GM_RENDER;
                    render.gpr_GInfo  = msg->gpi_GInfo;
                    render.gpr_RPort  = rp;
                    render.gpr_Redraw = GREDRAW_REDRAW;
                    DoMethodA(o, (Msg)&render);
                    ReleaseGIRPort(rp);
                }

                *msg->gpi_Termination = data->cd_Selected;
                return GMR_NOREUSE | GMR_VERIFY;
            }
            else
            {
                return GMR_NOREUSE;
            }
        }
        else if (ie->ie_Code == MENUDOWN)
        {
            return GMR_REUSE;
        }
    }

    return GMR_MEACTIVE;
}

/******************************************************************************/

IPTR Chooser__GM_GOINACTIVE(Class *cl, Object *o, struct gpGoInactive *msg)
{
    struct ChooserData *data = INST_DATA(cl, o);

    data->cd_Active = FALSE;
    chooser_popup_close(data);

    /* Redraw in inactive state */
    if (msg->gpgi_GInfo)
    {
        struct RastPort *rp = ObtainGIRPort(msg->gpgi_GInfo);
        if (rp)
        {
            struct gpRender render;
            render.MethodID   = GM_RENDER;
            render.gpr_GInfo  = msg->gpgi_GInfo;
            render.gpr_RPort  = rp;
            render.gpr_Redraw = GREDRAW_REDRAW;
            DoMethodA(o, (Msg)&render);
            ReleaseGIRPort(rp);
        }
    }

    return DoSuperMethodA(cl, o, (Msg)msg);
}

/******************************************************************************/

IPTR Chooser__GM_DOMAIN(Class *cl, Object *o, struct gpDomain *msg)
{
    struct ChooserData *data = INST_DATA(cl, o);
    struct TextFont *font = NULL;
    struct RastPort rp;
    UWORD minW = 40, minH = 14, textW = 0;
    struct MinNode *node;

    if (msg->gpd_GInfo && msg->gpd_GInfo->gi_DrInfo)
        font = msg->gpd_GInfo->gi_DrInfo->dri_Font;
    if (!font && msg->gpd_RPort)
        font = msg->gpd_RPort->Font;

    if (font)
    {
        InitRastPort(&rp);
        SetFont(&rp, font);
        if (data->cd_Labels)
            for (node = (struct MinNode *)data->cd_Labels->lh_Head; node->mln_Succ; node = node->mln_Succ)
            {
                struct ChooserNode *cn = (struct ChooserNode *)node;
                if (cn->cn_Text)
                {
                    UWORD w = TextLength(&rp, cn->cn_Text, strlen(cn->cn_Text));
                    if (w > textW) textW = w;
                }
            }
        if (data->cd_Title)
        {
            UWORD w = TextLength(&rp, data->cd_Title, strlen(data->cd_Title));
            if (w > textW) textW = w;
        }
        /* text, its margins and the arrow box */
        minW = textW + 8 + 18;
        minH = font->tf_YSize + 6;
    }

    msg->gpd_Domain.Left   = 0;
    msg->gpd_Domain.Top    = 0;
    msg->gpd_Domain.Width  = minW;
    msg->gpd_Domain.Height = minH;
    return TRUE;
}
