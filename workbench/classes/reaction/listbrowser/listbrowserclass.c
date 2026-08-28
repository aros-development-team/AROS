/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: Reaction listbrowser.gadget - BOOPSI class implementation
*/
#define DEBUG 1

#include <string.h>
#include <stdlib.h>

#include <exec/types.h>
#include <exec/memory.h>
#include <intuition/intuition.h>
#include <intuition/gadgetclass.h>
#include <intuition/imageclass.h>
#include <intuition/classusr.h>
#include <intuition/cghooks.h>
#include <graphics/gfxmacros.h>
#include <gadgets/listbrowser.h>
#include <utility/tagitem.h>

#include <proto/exec.h>
#include <proto/intuition.h>
#include <proto/graphics.h>
#include <proto/utility.h>
#include <proto/alib.h>

#include "listbrowser_intern.h"

#define ListBrowserBase ((struct Library *)(cl->cl_UserData))

#define G(obj) ((struct Gadget *)(obj))
#define INST_DATA_LB(cl, obj) ((struct ListBrowserData *)INST_DATA(cl, obj))

/* Count nodes in a list */
static LONG CountNodes(struct List *list)
{
    LONG count = 0;
    struct Node *node;
    if (list)
    {
        for (node = list->lh_Head; node->ln_Succ; node = node->ln_Succ)
            count++;
    }
    return count;
}

/* Find a node by index */
static struct Node *FindNodeByIndex(struct List *list, LONG index)
{
    struct Node *node;
    LONG i = 0;
    if (!list) return NULL;
    for (node = list->lh_Head; node->ln_Succ; node = node->ln_Succ)
    {
        if (i == index) return node;
        i++;
    }
    return NULL;
}

/* Set attributes from tags */
static void lb_set_attrs(struct ListBrowserData *data, struct TagItem *tags)
{
    struct TagItem *tag;
    while ((tag = NextTagItem(&tags)))
    {
        switch (tag->ti_Tag)
        {
            case LISTBROWSER_Labels:
                /* ~0 is the ClassAct "detach list" sentinel, used while the
                 * application modifies the list */
                if (tag->ti_Data == (IPTR)~0)
                    data->lbd_Labels = NULL;
                else
                    data->lbd_Labels = (struct List *)tag->ti_Data;
                if (data->lbd_Labels)
                    data->lbd_TotalNodes = CountNodes(data->lbd_Labels);
                else
                    data->lbd_TotalNodes = 0;
                break;
            case LISTBROWSER_Selected:
                data->lbd_Selected = (LONG)tag->ti_Data;
                if (data->lbd_Labels)
                    data->lbd_SelectedNode = (struct ListBrowserNode *)
                        FindNodeByIndex(data->lbd_Labels, data->lbd_Selected);
                break;
            case LISTBROWSER_ColumnInfo:
                data->lbd_ColumnInfo = (struct ColumnInfo *)tag->ti_Data;
                break;
            case LISTBROWSER_MakeVisible:
                /* Scroll to make this row visible */
                {
                    LONG row = (LONG)tag->ti_Data;
                    if (row < data->lbd_Position)
                        data->lbd_Position = row;
                    else if (row >= data->lbd_Position + data->lbd_VisibleRows)
                        data->lbd_Position = row - data->lbd_VisibleRows + 1;
                }
                break;
            case LISTBROWSER_MultiSelect:
                data->lbd_MultiSelect = (BOOL)tag->ti_Data;
                break;
            case LISTBROWSER_Separators:
                data->lbd_Separators = (BOOL)tag->ti_Data;
                break;
            case LISTBROWSER_ShowSelected:
                data->lbd_ShowSelected = (BOOL)tag->ti_Data;
                break;
            case LISTBROWSER_ColumnTitles:
                data->lbd_ColumnTitles = (BOOL)tag->ti_Data;
                break;
            case LISTBROWSER_Hierarchical:
                data->lbd_Hierarchical = (BOOL)tag->ti_Data;
                break;
            case LISTBROWSER_Editable:
                data->lbd_Editable = (BOOL)tag->ti_Data;
                break;
            case LISTBROWSER_Position:
                data->lbd_Position = (LONG)tag->ti_Data;
                break;
            case LISTBROWSER_AutoFit:
                data->lbd_AutoFit = (BOOL)tag->ti_Data;
                break;
            case LISTBROWSER_Borderless:
                data->lbd_Borderless = (BOOL)tag->ti_Data;
                break;
            case LISTBROWSER_SortColumn:
                data->lbd_SortColumn = (LONG)tag->ti_Data;
                break;
            case LISTBROWSER_VerticalProp:
                data->lbd_VProp = (BOOL)tag->ti_Data;
                break;
            case LISTBROWSER_HorizontalProp:
                data->lbd_HProp = (BOOL)tag->ti_Data;
                break;
            case LISTBROWSER_TitleClickable:
                data->lbd_TitleClickable = (BOOL)tag->ti_Data;
                break;
            case LISTBROWSER_Striping:
                data->lbd_Striping = (LONG)tag->ti_Data;
                break;
        }
    }
}

IPTR ListBrowser__OM_NEW(Class *cl, Object *obj, struct opSet *msg)
{
    D(bug("[ListBrowser] OM_NEW: entry\n"));
    obj = (Object *)DoSuperMethodA(cl, obj, (Msg)msg);
    if (obj)
    {
        D(bug("[ListBrowser] OM_NEW: obj=%p\n", obj));
        struct ListBrowserData *data = INST_DATA_LB(cl, obj);
        memset(data, 0, sizeof(*data));

        data->lbd_Selected = -1;
        data->lbd_Separators = TRUE;
        data->lbd_ShowSelected = TRUE;

        lb_set_attrs(data, msg->ops_AttrList);
    }

    return (IPTR)obj;
}

IPTR ListBrowser__OM_DISPOSE(Class *cl, Object *obj, Msg msg)
{
    D(bug("[ListBrowser] OM_DISPOSE: entry\n"));
    return DoSuperMethodA(cl, obj, msg);
}

IPTR ListBrowser__OM_SET(Class *cl, Object *obj, struct opSet *msg)
{
    D(bug("[ListBrowser] OM_SET: entry\n"));
    struct ListBrowserData *data = INST_DATA_LB(cl, obj);
    IPTR retval;

    retval = DoSuperMethodA(cl, obj, (Msg)msg);
    lb_set_attrs(data, msg->ops_AttrList);

    /* Refresh if visible */
    if (msg->ops_GInfo)
    {
        struct RastPort *rp = ObtainGIRPort(msg->ops_GInfo);
        if (rp)
        {
            DoMethod(obj, GM_RENDER, (IPTR)msg->ops_GInfo, (IPTR)rp, GREDRAW_UPDATE);
            ReleaseGIRPort(rp);
        }
    }

    return retval;
}

IPTR ListBrowser__OM_GET(Class *cl, Object *obj, struct opGet *msg)
{
    struct ListBrowserData *data = INST_DATA_LB(cl, obj);

    switch (msg->opg_AttrID)
    {
        case LISTBROWSER_Labels:
            *msg->opg_Storage = (IPTR)data->lbd_Labels;
            return TRUE;
        case LISTBROWSER_Selected:
            *msg->opg_Storage = (IPTR)data->lbd_Selected;
            return TRUE;
        case LISTBROWSER_ColumnInfo:
            *msg->opg_Storage = (IPTR)data->lbd_ColumnInfo;
            return TRUE;
        case LISTBROWSER_TotalNodes:
            *msg->opg_Storage = (IPTR)data->lbd_TotalNodes;
            return TRUE;
        case LISTBROWSER_RelEvent:
            *msg->opg_Storage = (IPTR)data->lbd_RelEvent;
            return TRUE;
        case LISTBROWSER_Position:
            *msg->opg_Storage = (IPTR)data->lbd_Position;
            return TRUE;
        case LISTBROWSER_SelectedNode:
            *msg->opg_Storage = (IPTR)data->lbd_SelectedNode;
            return TRUE;
        case LISTBROWSER_SortColumn:
            *msg->opg_Storage = (IPTR)data->lbd_SortColumn;
            return TRUE;
        case LISTBROWSER_Striping:
            *msg->opg_Storage = (IPTR)data->lbd_Striping;
            return TRUE;
        case LISTBROWSER_RelColumn:
            *msg->opg_Storage = (IPTR)data->lbd_RelColumn;
            return TRUE;
    }

    return DoSuperMethodA(cl, obj, (Msg)msg);
}

/* Shared layout metrics for render and input hit-testing */
#define LB_VPROP_W  14
#define LB_HPROP_H  12
#define LB_CHECK_W  14

struct lb_metrics
{
    WORD left, top, width, height;   /* whole gadget */
    WORD c_left, c_top, c_width, c_height; /* content (rows) area */
    WORD row_h, title_h;
    WORD vp_x, vp_y, vp_h;           /* vertical prop trough */
    BOOL vprop, hprop;
};

static void lb_get_metrics(struct ListBrowserData *data, struct Gadget *g,
    struct RastPort *rp, struct lb_metrics *m)
{
    m->left = g->LeftEdge;
    m->top = g->TopEdge;
    m->width = g->Width;
    m->height = g->Height;

    if (rp && rp->Font)
        m->row_h = rp->Font->tf_YSize + 2;
    else if (data->lbd_RowHeight)
        m->row_h = data->lbd_RowHeight;
    else
        m->row_h = 10;

    m->title_h = (data->lbd_ColumnTitles && data->lbd_ColumnInfo) ? m->row_h + 2 : 0;

    m->vprop = data->lbd_VProp;
    m->hprop = data->lbd_HProp;

    m->c_left = m->left + 2;
    m->c_top = m->top + 2 + m->title_h;
    m->c_width = m->width - 4 - (m->vprop ? LB_VPROP_W : 0);
    m->c_height = m->height - 4 - m->title_h - (m->hprop ? LB_HPROP_H : 0);

    m->vp_x = m->left + m->width - 2 - LB_VPROP_W;
    m->vp_y = m->top + 2 + m->title_h;
    m->vp_h = m->height - 4 - m->title_h - (m->hprop ? LB_HPROP_H : 0);
}

static WORD lb_col_width(struct ListBrowserData *data, UWORD col, WORD avail)
{
    if (data->lbd_ColumnInfo[col].ci_Flags & CIF_WEIGHTED)
        return avail * data->lbd_ColumnInfo[col].ci_Width / 100;
    return data->lbd_ColumnInfo[col].ci_Width;
}

static void lb_ltoa(LONG v, char *buf, int bufsize)
{
    char tmp[16];
    int i = 0, j = 0;
    BOOL neg = v < 0;

    if (neg) v = -v;
    do { tmp[i++] = '0' + (v % 10); v /= 10; } while (v && i < 15);
    if (neg && j < bufsize - 1) buf[j++] = '-';
    while (i > 0 && j < bufsize - 1) buf[j++] = tmp[--i];
    buf[j] = 0;
}

/* Visible-row helpers: rows with lbn_Hidden set take no space, so both
 * rendering and hit-testing must address nodes by their visible index */
static struct ListBrowserNode *lb_visible_node(struct List *l, LONG n)
{
    struct Node *node;

    if (!l || n < 0) return NULL;
    for (node = l->lh_Head; node->ln_Succ; node = node->ln_Succ)
    {
        struct ListBrowserNode *lbn = (struct ListBrowserNode *)node;
        if (lbn->lbn_Hidden) continue;
        if (n-- == 0) return lbn;
    }
    return NULL;
}

static LONG lb_visible_count(struct List *l)
{
    struct Node *node;
    LONG n = 0;

    if (!l) return 0;
    for (node = l->lh_Head; node->ln_Succ; node = node->ln_Succ)
        if (!((struct ListBrowserNode *)node)->lbn_Hidden)
            n++;
    return n;
}

/* Compute how many rows the knob represents and draw the vertical prop */
static void lb_draw_vprop(struct ListBrowserData *data, struct RastPort *rp,
    struct DrawInfo *dri, struct lb_metrics *m)
{
    WORD x1 = m->vp_x, y1 = m->vp_y;
    WORD x2 = x1 + LB_VPROP_W - 1, y2 = y1 + m->vp_h - 1;
    LONG total = lb_visible_count(data->lbd_Labels);
    LONG vis = data->lbd_VisibleRows ? data->lbd_VisibleRows : 1;
    WORD kh, ky;

    if (total < 1) total = 1;

    if (m->vp_h < 8) return;

    /* recessed trough */
    SetAPen(rp, dri->dri_Pens[BACKGROUNDPEN]);
    RectFill(rp, x1, y1, x2, y2);
    SetAPen(rp, dri->dri_Pens[SHADOWPEN]);
    Move(rp, x1, y2); Draw(rp, x1, y1); Draw(rp, x2, y1);
    SetAPen(rp, dri->dri_Pens[SHINEPEN]);
    Draw(rp, x2, y2); Draw(rp, x1 + 1, y2);

    /* knob */
    if (vis >= total) { kh = m->vp_h - 4; ky = y1 + 2; }
    else
    {
        kh = (LONG)(m->vp_h - 4) * vis / total;
        if (kh < 8) kh = 8;
        ky = y1 + 2 + (LONG)(m->vp_h - 4 - kh) * data->lbd_Position /
             ((total - vis) > 0 ? (total - vis) : 1);
    }
    SetAPen(rp, dri->dri_Pens[SHINEPEN]);
    RectFill(rp, x1 + 2, ky, x2 - 2, ky + kh - 1);
    SetAPen(rp, dri->dri_Pens[SHADOWPEN]);
    Move(rp, x1 + 2, ky + kh - 1); Draw(rp, x2 - 2, ky + kh - 1);
    Draw(rp, x2 - 2, ky);
}

static void lb_draw_hprop(struct ListBrowserData *data, struct RastPort *rp,
    struct DrawInfo *dri, struct lb_metrics *m)
{
    WORD x1 = m->c_left, x2 = m->c_left + m->c_width - 1;
    WORD y2 = m->top + m->height - 3;
    WORD y1 = y2 - LB_HPROP_H + 1;

    if (x2 - x1 < 8) return;

    SetAPen(rp, dri->dri_Pens[BACKGROUNDPEN]);
    RectFill(rp, x1, y1, x2, y2);
    SetAPen(rp, dri->dri_Pens[SHADOWPEN]);
    Move(rp, x1, y2); Draw(rp, x1, y1); Draw(rp, x2, y1);
    SetAPen(rp, dri->dri_Pens[SHINEPEN]);
    Draw(rp, x2, y2); Draw(rp, x1 + 1, y2);

    /* full width knob - no horizontal virtual scrolling yet */
    SetAPen(rp, dri->dri_Pens[SHINEPEN]);
    RectFill(rp, x1 + 2, y1 + 2, x2 - 2, y2 - 2);
    SetAPen(rp, dri->dri_Pens[SHADOWPEN]);
    Move(rp, x1 + 2, y2 - 2); Draw(rp, x2 - 2, y2 - 2); Draw(rp, x2 - 2, y1 + 2);
}

IPTR ListBrowser__GM_RENDER(Class *cl, Object *obj, struct gpRender *msg)
{
    D(bug("[ListBrowser] GM_RENDER: redraw=%d\n", msg->gpr_Redraw));
    struct ListBrowserData *data = INST_DATA_LB(cl, obj);
    struct RastPort *rp = msg->gpr_RPort;
    struct GadgetInfo *gi = msg->gpr_GInfo;
    struct lb_metrics m;
    WORD row_y;
    LONG i;
    struct Node *node;
    UBYTE bgpen, fgpen;
    struct DrawInfo *dri = gi->gi_DrInfo;

    if (!data->lbd_DrawInfo && dri)
        data->lbd_DrawInfo = dri;

    if (!dri || !rp)
        return FALSE;

    lb_get_metrics(data, G(obj), rp, &m);

    if (m.width <= 4 || m.height <= 4)
        return FALSE;

    data->lbd_RowHeight = m.row_h;
    data->lbd_TitleHeight = m.title_h;

    /* Clear background */
    SetDrMd(rp, JAM1);
    SetAPen(rp, dri->dri_Pens[BACKGROUNDPEN]);
    RectFill(rp, m.left, m.top, m.left + m.width - 1, m.top + m.height - 1);

    /* Recessed border unless borderless */
    if (!data->lbd_Borderless)
    {
        SetAPen(rp, dri->dri_Pens[SHADOWPEN]);
        Move(rp, m.left, m.top + m.height - 1);
        Draw(rp, m.left, m.top);
        Draw(rp, m.left + m.width - 1, m.top);
        SetAPen(rp, dri->dri_Pens[SHINEPEN]);
        Draw(rp, m.left + m.width - 1, m.top + m.height - 1);
        Draw(rp, m.left, m.top + m.height - 1);
    }

    /* Column title cells: raised bevels with black text and a sort arrow */
    row_y = m.top + 2;
    if (m.title_h)
    {
        WORD col_x = m.c_left;
        WORD ty = row_y + (m.title_h - 2 - (rp->Font ? rp->Font->tf_YSize : 8)) / 2
                  + (rp->Font ? rp->Font->tf_Baseline : 6) + 1;
        UWORD col;

        for (col = 0; data->lbd_ColumnInfo[col].ci_Width != -1; col++)
        {
            WORD col_w = lb_col_width(data, col, m.c_width);
            WORD cx2 = col_x + col_w - 1;

            if (col_x > m.c_left + m.c_width) break;
            if (cx2 > m.c_left + m.c_width - 1) cx2 = m.c_left + m.c_width - 1;

            /* raised cell */
            SetAPen(rp, dri->dri_Pens[BACKGROUNDPEN]);
            RectFill(rp, col_x, row_y, cx2, row_y + m.title_h - 1);
            SetAPen(rp, dri->dri_Pens[SHINEPEN]);
            Move(rp, col_x, row_y + m.title_h - 1); Draw(rp, col_x, row_y);
            Draw(rp, cx2, row_y);
            SetAPen(rp, dri->dri_Pens[SHADOWPEN]);
            Draw(rp, cx2, row_y + m.title_h - 1);
            Draw(rp, col_x, row_y + m.title_h - 1);

            if (data->lbd_ColumnInfo[col].ci_Title)
            {
                ULONG tlen = strlen(data->lbd_ColumnInfo[col].ci_Title);
                WORD tw;

                /* clip title text into the cell */
                while (tlen &&
                    (WORD)TextLength(rp, data->lbd_ColumnInfo[col].ci_Title, tlen)
                        > col_w - 12)
                    tlen--;

                SetAPen(rp, dri->dri_Pens[TEXTPEN]);
                Move(rp, col_x + 3, ty);
                Text(rp, data->lbd_ColumnInfo[col].ci_Title, tlen);
                tw = TextLength(rp, data->lbd_ColumnInfo[col].ci_Title, tlen);

                /* sort direction arrow on the sorted column */
                if (data->lbd_SortColumn == (LONG)col &&
                    (data->lbd_ColumnInfo[col].ci_Flags & CIF_SORTABLE) &&
                    col_w - tw > 14)
                {
                    WORD ax = col_x + 5 + tw, i2;
                    WORD ay = row_y + m.title_h / 2 - 2;

                    SetAPen(rp, dri->dri_Pens[SHADOWPEN]);
                    for (i2 = 0; i2 < 4; i2++)
                    {
                        Move(rp, ax + i2, ay + i2);
                        Draw(rp, ax + 7 - i2, ay + i2);
                    }
                }
            }

            col_x += col_w;
        }

        row_y += m.title_h;
    }

    /* Compute visible rows */
    data->lbd_VisibleRows = m.c_height / m.row_h;

    /* Draw rows */
    if (data->lbd_Labels)
    {
        for (i = 0; i < data->lbd_VisibleRows; i++)
        {
            struct ListBrowserNode *lbn =
                lb_visible_node(data->lbd_Labels, data->lbd_Position + i);
            BOOL selected;
            WORD row_x2 = m.c_left + m.c_width - 1;

            if (!lbn)
                break;
            node = (struct Node *)lbn;
            selected = (data->lbd_Position + i == data->lbd_Selected) || lbn->lbn_Selected;

            if (selected)
            {
                bgpen = dri->dri_Pens[FILLPEN];
                fgpen = dri->dri_Pens[FILLTEXTPEN];
            }
            else
            {
                bgpen = dri->dri_Pens[BACKGROUNDPEN];
                fgpen = dri->dri_Pens[TEXTPEN];
            }

            SetAPen(rp, bgpen);
            RectFill(rp, m.c_left, row_y, row_x2, row_y + m.row_h - 1);

            /* Draw column entries */
            if (lbn->lbn_Columns && data->lbd_ColumnInfo)
            {
                WORD col_x = m.c_left;
                UWORD col;
                WORD indent = lbn->lbn_Generation * 16;

                for (col = 0; col < lbn->lbn_Columns && data->lbd_ColumnInfo[col].ci_Width != -1; col++)
                {
                    WORD col_w = lb_col_width(data, col, m.c_width);
                    WORD inset = 2;

                    if (col_x > row_x2) break;

                    if (col == 0)
                    {
                        inset += indent;

                        /* Row checkbox: recessed box, check mark when set */
                        if (lbn->lbn_HasCheckBox)
                        {
                            WORD bx = col_x + inset;
                            WORD bs = m.row_h - 2;
                            WORD by = row_y + 1;

                            if (bs > 11) bs = 11;

                            SetAPen(rp, dri->dri_Pens[SHINEPEN]);
                            RectFill(rp, bx, by, bx + bs, by + bs);
                            SetAPen(rp, dri->dri_Pens[SHADOWPEN]);
                            Move(rp, bx, by + bs); Draw(rp, bx, by); Draw(rp, bx + bs, by);
                            if (lbn->lbn_Checked)
                            {
                                Move(rp, bx + 2, by + bs / 2);
                                Draw(rp, bx + bs / 2, by + bs - 2);
                                Draw(rp, bx + bs - 2, by + 2);
                                Move(rp, bx + 3, by + bs / 2);
                                Draw(rp, bx + bs / 2 + 1, by + bs - 2);
                            }
                            inset += bs + 4;
                        }
                    }

                    /* Per-column pen overrides */
                    UBYTE cfgpen = (lbn->lbn_ColumnData[col].lbce_FGPen != 0) ?
                        lbn->lbn_ColumnData[col].lbce_FGPen : fgpen;

                    SetAPen(rp, cfgpen);

                    if (lbn->lbn_ColumnData[col].lbce_Text)
                    {
                        STRPTR ct = lbn->lbn_ColumnData[col].lbce_Text;
                        ULONG tlen = strlen(ct);

                        while (tlen && (WORD)TextLength(rp, ct, tlen) > col_w - inset - 2)
                            tlen--;
                        Move(rp, col_x + inset, row_y + rp->Font->tf_Baseline + 1);
                        Text(rp, ct, tlen);
                    }
                    else if (lbn->lbn_ColumnData[col].lbce_Image)
                    {
                        struct Image *nim =
                            (struct Image *)lbn->lbn_ColumnData[col].lbce_Image;

                        /* Clamp the image into the cell; BOOPSI images
                         * (labels and the like) default to 80x40 and would
                         * otherwise spill over neighbouring rows */
                        if (nim->Width > col_w - inset || nim->Height > m.row_h)
                        {
                            SetAttrs((Object *)nim,
                                IA_Width,  (nim->Width  > col_w - inset) ? col_w - inset : nim->Width,
                                IA_Height, (nim->Height > m.row_h) ? m.row_h : nim->Height,
                                TAG_DONE);
                        }

                        DrawImageState(rp, nim, col_x + inset, row_y,
                            selected ? IDS_SELECTED : IDS_NORMAL, dri);
                    }
                    else if (lbn->lbn_ColumnData[col].lbce_HasInteger)
                    {
                        char nbuf[16];
                        ULONG tlen;

                        lb_ltoa(lbn->lbn_ColumnData[col].lbce_Integer, nbuf, sizeof(nbuf));
                        tlen = strlen(nbuf);
                        while (tlen && (WORD)TextLength(rp, nbuf, tlen) > col_w - inset - 2)
                            tlen--;
                        Move(rp, col_x + inset, row_y + rp->Font->tf_Baseline + 1);
                        Text(rp, nbuf, tlen);
                    }

                    col_x += col_w;
                }
            }

            row_y += m.row_h;
        }
    }

    /* Scrollers */
    if (m.vprop)
        lb_draw_vprop(data, rp, dri, &m);
    if (m.hprop)
        lb_draw_hprop(data, rp, dri, &m);

    return (IPTR)TRUE;
}

static IPTR lb_handle_input(Class *cl, Object *obj, struct gpInput *msg);

IPTR ListBrowser__GM_GOACTIVE(Class *cl, Object *obj, struct gpInput *msg)
{
    D(bug("[ListBrowser] GM_GOACTIVE: entry\n"));
    /* The activating click arrives here, not in GM_HANDLEINPUT */
    return lb_handle_input(cl, obj, msg);
}

IPTR ListBrowser__GM_HANDLEINPUT(Class *cl, Object *obj, struct gpInput *msg)
{
    D(bug("[ListBrowser] GM_HANDLEINPUT: entry\n"));
    return lb_handle_input(cl, obj, msg);
}

static IPTR lb_handle_input(Class *cl, Object *obj, struct gpInput *msg)
{
    struct ListBrowserData *data = INST_DATA_LB(cl, obj);
    struct InputEvent *ie = msg->gpi_IEvent;
    IPTR retval = GMR_MEACTIVE;

    if (!ie)
        return GMR_MEACTIVE;

    if (ie->ie_Class == IECLASS_RAWMOUSE)
    {
        if (ie->ie_Code == SELECTDOWN)
        {
            struct lb_metrics m;
            WORD mx = msg->gpi_Mouse.X;
            WORD my = msg->gpi_Mouse.Y;
            struct RastPort *girp = NULL;

            lb_get_metrics(data, G(obj), NULL, &m);
            if (data->lbd_RowHeight)
                m.row_h = data->lbd_RowHeight;

            /* gpi_Mouse is gadget relative */
            mx += m.left;
            my += m.top;

            D(bug("[ListBrowser] HI: mouse=(%d,%d) abs=(%d,%d) c_top=%d row_h=%d vis=%ld pos=%ld total=%ld\n",
                (int)msg->gpi_Mouse.X, (int)msg->gpi_Mouse.Y, (int)mx, (int)my,
                (int)m.c_top, (int)m.row_h, (LONG)data->lbd_VisibleRows,
                (LONG)data->lbd_Position, lb_visible_count(data->lbd_Labels)));

            /* Vertical scroller: page up/down */
            if (m.vprop && mx >= m.vp_x && my >= m.vp_y && my < m.vp_y + m.vp_h)
            {
                LONG total = lb_visible_count(data->lbd_Labels);
                LONG vis = data->lbd_VisibleRows ? data->lbd_VisibleRows : 1;
                LONG maxpos = (total > vis) ? total - vis : 0;
                WORD kh, ky;

                if (vis >= total) { kh = m.vp_h - 4; ky = m.vp_y + 2; }
                else
                {
                    kh = (LONG)(m.vp_h - 4) * vis / (total ? total : 1);
                    if (kh < 8) kh = 8;
                    ky = m.vp_y + 2 + (LONG)(m.vp_h - 4 - kh) * data->lbd_Position /
                         (maxpos ? maxpos : 1);
                }

                if (my < ky)
                    data->lbd_Position -= vis;
                else if (my >= ky + kh)
                    data->lbd_Position += vis;

                if (data->lbd_Position > maxpos) data->lbd_Position = maxpos;
                if (data->lbd_Position < 0) data->lbd_Position = 0;

                girp = ObtainGIRPort(msg->gpi_GInfo);
                if (girp)
                {
                    DoMethod(obj, GM_RENDER, (IPTR)msg->gpi_GInfo, (IPTR)girp, GREDRAW_REDRAW);
                    ReleaseGIRPort(girp);
                }
                retval = GMR_NOREUSE;
            }
            else if (m.title_h && my < m.c_top && my >= m.top + 2)
            {
                /* Title row click */
                if (data->lbd_TitleClickable)
                {
                    WORD col_x = m.c_left;
                    UWORD col;

                    for (col = 0; data->lbd_ColumnInfo &&
                         data->lbd_ColumnInfo[col].ci_Width != -1; col++)
                    {
                        WORD col_w = lb_col_width(data, col, m.c_width);
                        if (mx >= col_x && mx < col_x + col_w)
                        {
                            data->lbd_RelColumn = col;
                            break;
                        }
                        col_x += col_w;
                    }
                    data->lbd_RelEvent = LBRE_TITLECLICK;
                    *msg->gpi_Termination = data->lbd_Selected;
                    return GMR_NOREUSE | GMR_VERIFY;
                }
                retval = GMR_NOREUSE;
            }
            else if (my >= m.c_top)
            {
                LONG clicked_row = data->lbd_Position + (my - m.c_top) / m.row_h;

                if (clicked_row >= 0 &&
                    clicked_row < lb_visible_count(data->lbd_Labels))
                {
                    struct ListBrowserNode *lbn =
                        lb_visible_node(data->lbd_Labels, clicked_row);
                    BOOL dbl = FALSE;

                    /* Checkbox hit? */
                    if (lbn && lbn->lbn_HasCheckBox)
                    {
                        WORD bs = m.row_h - 2;
                        WORD bx = m.c_left + 2 + lbn->lbn_Generation * 16;

                        if (bs > 11) bs = 11;
                        if (mx >= bx && mx <= bx + bs + 2)
                        {
                            lbn->lbn_Checked = !lbn->lbn_Checked;
                            data->lbd_SelectedNode = lbn;
                            data->lbd_RelEvent = lbn->lbn_Checked ? LBRE_CHECKED
                                                                  : LBRE_UNCHECKED;

                            girp = ObtainGIRPort(msg->gpi_GInfo);
                            if (girp)
                            {
                                DoMethod(obj, GM_RENDER, (IPTR)msg->gpi_GInfo,
                                    (IPTR)girp, GREDRAW_UPDATE);
                                ReleaseGIRPort(girp);
                            }
                            *msg->gpi_Termination = clicked_row;
                            return GMR_NOREUSE | GMR_VERIFY;
                        }
                    }

                    /* Double click on the same row? */
                    if (clicked_row == data->lbd_LastRow &&
                        DoubleClick(data->lbd_LastSecs, data->lbd_LastMicros,
                            ie->ie_TimeStamp.tv_secs, ie->ie_TimeStamp.tv_micro))
                    {
                        dbl = TRUE;
                    }
                    data->lbd_LastSecs = ie->ie_TimeStamp.tv_secs;
                    data->lbd_LastMicros = ie->ie_TimeStamp.tv_micro;
                    data->lbd_LastRow = clicked_row;

                    if (data->lbd_MultiSelect && lbn && !dbl)
                        lbn->lbn_Selected = !lbn->lbn_Selected;

                    data->lbd_Selected = clicked_row;
                    data->lbd_SelectedNode = (struct ListBrowserNode *)lbn;
                    data->lbd_RelEvent = dbl ? LBRE_DOUBLECLICK : LBRE_NORMAL;

                    girp = ObtainGIRPort(msg->gpi_GInfo);
                    if (girp)
                    {
                        DoMethod(obj, GM_RENDER, (IPTR)msg->gpi_GInfo, (IPTR)girp, GREDRAW_UPDATE);
                        ReleaseGIRPort(girp);
                    }
                }
                retval = GMR_NOREUSE | GMR_VERIFY;
            }
            else
                retval = GMR_NOREUSE;
        }
        else if (ie->ie_Code == SELECTUP)
        {
            retval = GMR_NOREUSE | GMR_VERIFY;
        }
    }

    *msg->gpi_Termination = data->lbd_Selected;
    return retval;
}

IPTR ListBrowser__GM_GOINACTIVE(Class *cl, Object *obj, struct gpGoInactive *msg)
{
    return 0;
}

IPTR ListBrowser__GM_DOMAIN(Class *cl, Object *obj, struct gpDomain *msg)
{
    struct ListBrowserData *data = INST_DATA_LB(cl, obj);
    WORD row_h = data->lbd_RowHeight;

    /* Before the first render no row height has been computed yet, so
     * derive one from the measuring rastport's font */
    if (row_h <= 0)
    {
        if (msg->gpd_RPort && msg->gpd_RPort->Font)
            row_h = msg->gpd_RPort->Font->tf_YSize + 2;
        else
            row_h = 10;
    }

    msg->gpd_Domain.Left   = 0;
    msg->gpd_Domain.Top    = 0;
    msg->gpd_Domain.Width  = 100;
    msg->gpd_Domain.Height = row_h * 4 +
        (data->lbd_ColumnTitles ? row_h : data->lbd_TitleHeight) + 4;

    return (IPTR)TRUE;
}
