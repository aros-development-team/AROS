/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: Reaction listbrowser.gadget - BOOPSI class implementation
*/

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
            case LISTBROWSER_ReadOnly:
                data->lbd_ReadOnly = (BOOL)tag->ti_Data;
                break;
            case LISTBROWSER_ColumnTitles:
                data->lbd_ColumnTitles = (BOOL)tag->ti_Data;
                break;
            case LISTBROWSER_Striping:
                data->lbd_Striping = (UBYTE)tag->ti_Data;
                break;
            case LISTBROWSER_NumColumns:
                data->lbd_NumColumns = (UWORD)tag->ti_Data;
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
        }
    }
}

IPTR ListBrowser__OM_NEW(Class *cl, Object *obj, struct opSet *msg)
{
    obj = (Object *)DoSuperMethodA(cl, obj, (Msg)msg);
    if (obj)
    {
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
    return DoSuperMethodA(cl, obj, msg);
}

IPTR ListBrowser__OM_SET(Class *cl, Object *obj, struct opSet *msg)
{
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
        case LISTBROWSER_VisibleRows:
            *msg->opg_Storage = (IPTR)data->lbd_VisibleRows;
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
    }

    return DoSuperMethodA(cl, obj, (Msg)msg);
}

IPTR ListBrowser__GM_RENDER(Class *cl, Object *obj, struct gpRender *msg)
{
    struct ListBrowserData *data = INST_DATA_LB(cl, obj);
    struct RastPort *rp = msg->gpr_RPort;
    struct GadgetInfo *gi = msg->gpr_GInfo;
    WORD left, top, width, height;
    WORD row_y, row_h;
    LONG i;
    struct Node *node;
    UBYTE bgpen, fgpen;
    struct DrawInfo *dri = gi->gi_DrInfo;

    if (!data->lbd_DrawInfo && dri)
        data->lbd_DrawInfo = dri;

    left   = G(obj)->LeftEdge;
    top    = G(obj)->TopEdge;
    width  = G(obj)->Width;
    height = G(obj)->Height;

    /* Clear background */
    SetAPen(rp, dri ? dri->dri_Pens[BACKGROUNDPEN] : 0);
    RectFill(rp, left, top, left + width - 1, top + height - 1);

    /* Draw border unless borderless */
    if (!data->lbd_Borderless && dri)
    {
        SetAPen(rp, dri->dri_Pens[SHINEPEN]);
        Move(rp, left, top + height - 1);
        Draw(rp, left, top);
        Draw(rp, left + width - 1, top);

        SetAPen(rp, dri->dri_Pens[SHADOWPEN]);
        Draw(rp, left + width - 1, top + height - 1);
        Draw(rp, left, top + height - 1);
    }

    /* Compute row height from font */
    if (rp->Font)
        row_h = rp->Font->tf_YSize + 2;
    else
        row_h = 10;
    data->lbd_RowHeight = row_h;

    /* Title area */
    row_y = top + 2;
    if (data->lbd_ColumnTitles && data->lbd_ColumnInfo)
    {
        WORD col_x = left + 2;
        UWORD col;

        SetAPen(rp, dri ? dri->dri_Pens[FILLPEN] : 3);
        RectFill(rp, left + 1, row_y, left + width - 2, row_y + row_h - 1);

        SetAPen(rp, dri ? dri->dri_Pens[FILLTEXTPEN] : 1);
        for (col = 0; data->lbd_ColumnInfo[col].ci_Width != -1; col++)
        {
            WORD col_w;
            if (data->lbd_ColumnInfo[col].ci_Flags & CIF_WEIGHTED)
                col_w = (width - 4) * data->lbd_ColumnInfo[col].ci_Width / 100;
            else
                col_w = data->lbd_ColumnInfo[col].ci_Width;

            if (data->lbd_ColumnInfo[col].ci_Title)
            {
                Move(rp, col_x + 2, row_y + rp->Font->tf_Baseline);
                Text(rp, data->lbd_ColumnInfo[col].ci_Title,
                     strlen(data->lbd_ColumnInfo[col].ci_Title));
            }

            if (data->lbd_Separators && col > 0)
            {
                SetAPen(rp, dri ? dri->dri_Pens[SHADOWPEN] : 1);
                Move(rp, col_x - 1, row_y);
                Draw(rp, col_x - 1, row_y + row_h - 1);
                SetAPen(rp, dri ? dri->dri_Pens[FILLTEXTPEN] : 1);
            }

            col_x += col_w;
        }

        data->lbd_TitleHeight = row_h;
        row_y += row_h;
    }
    else
    {
        data->lbd_TitleHeight = 0;
    }

    /* Compute visible rows */
    data->lbd_VisibleRows = (top + height - 2 - row_y) / row_h;

    /* Draw rows */
    if (data->lbd_Labels)
    {
        /* Skip to Position */
        node = data->lbd_Labels->lh_Head;
        for (i = 0; i < data->lbd_Position && node->ln_Succ; i++)
            node = node->ln_Succ;

        for (i = 0; i < data->lbd_VisibleRows && node->ln_Succ; i++, node = node->ln_Succ)
        {
            struct ListBrowserNode *lbn = (struct ListBrowserNode *)node;
            BOOL selected = (data->lbd_Position + i == data->lbd_Selected) || lbn->lbn_Selected;

            if (lbn->lbn_Hidden) { i--; continue; }

            /* Row background */
            if (selected)
            {
                bgpen = dri ? dri->dri_Pens[FILLPEN] : 3;
                fgpen = dri ? dri->dri_Pens[FILLTEXTPEN] : 0;
            }
            else if (data->lbd_Striping == LBS_ROWS && (i & 1))
            {
                bgpen = dri ? dri->dri_Pens[SHINEPEN] : 2;
                fgpen = dri ? dri->dri_Pens[TEXTPEN] : 1;
            }
            else
            {
                bgpen = dri ? dri->dri_Pens[BACKGROUNDPEN] : 0;
                fgpen = dri ? dri->dri_Pens[TEXTPEN] : 1;
            }

            SetAPen(rp, bgpen);
            RectFill(rp, left + 1, row_y, left + width - 2, row_y + row_h - 1);

            /* Draw column entries */
            if (lbn->lbn_Columns && data->lbd_ColumnInfo)
            {
                WORD col_x = left + 2;
                UWORD col;
                WORD indent = lbn->lbn_Generation * 16;

                for (col = 0; col < lbn->lbn_Columns && data->lbd_ColumnInfo[col].ci_Width != -1; col++)
                {
                    WORD col_w;
                    if (data->lbd_ColumnInfo[col].ci_Flags & CIF_WEIGHTED)
                        col_w = (width - 4) * data->lbd_ColumnInfo[col].ci_Width / 100;
                    else
                        col_w = data->lbd_ColumnInfo[col].ci_Width;

                    /* Per-column pen overrides */
                    UBYTE cfgpen = (lbn->lbn_ColumnData[col].lbce_FGPen != 0) ?
                        lbn->lbn_ColumnData[col].lbce_FGPen : fgpen;

                    SetAPen(rp, cfgpen);

                    if (lbn->lbn_ColumnData[col].lbce_Text)
                    {
                        WORD tx = col_x + 2;
                        if (col == 0) tx += indent;
                        Move(rp, tx, row_y + rp->Font->tf_Baseline);
                        Text(rp, lbn->lbn_ColumnData[col].lbce_Text,
                             strlen(lbn->lbn_ColumnData[col].lbce_Text));
                    }
                    else if (lbn->lbn_ColumnData[col].lbce_Image)
                    {
                        DrawImageState(rp,
                            (struct Image *)lbn->lbn_ColumnData[col].lbce_Image,
                            col_x + 2, row_y, IDS_NORMAL, dri);
                    }

                    col_x += col_w;
                }
            }

            row_y += row_h;
        }
    }

    return (IPTR)TRUE;
}

IPTR ListBrowser__GM_GOACTIVE(Class *cl, Object *obj, struct gpInput *msg)
{
    struct ListBrowserData *data = INST_DATA_LB(cl, obj);

    if (data->lbd_ReadOnly)
        return GMR_NOREUSE;

    return GMR_MEACTIVE;
}

IPTR ListBrowser__GM_HANDLEINPUT(Class *cl, Object *obj, struct gpInput *msg)
{
    struct ListBrowserData *data = INST_DATA_LB(cl, obj);
    struct InputEvent *ie = msg->gpi_IEvent;
    IPTR retval = GMR_MEACTIVE;

    if (ie->ie_Class == IECLASS_RAWMOUSE)
    {
        if (ie->ie_Code == SELECTDOWN)
        {
            WORD my = msg->gpi_Mouse.Y;
            WORD content_top = data->lbd_TitleHeight + 2;
            LONG clicked_row;

            if (my < content_top)
            {
                /* Click on title row - column title click event */
                retval = GMR_NOREUSE;
            }
            else
            {
                clicked_row = data->lbd_Position + (my - content_top) / data->lbd_RowHeight;

                if (clicked_row >= 0 && clicked_row < data->lbd_TotalNodes)
                {
                    if (data->lbd_MultiSelect)
                    {
                        struct ListBrowserNode *lbn = (struct ListBrowserNode *)
                            FindNodeByIndex(data->lbd_Labels, clicked_row);
                        if (lbn) lbn->lbn_Selected = !lbn->lbn_Selected;
                    }

                    data->lbd_Selected = clicked_row;
                    data->lbd_SelectedNode = (struct ListBrowserNode *)
                        FindNodeByIndex(data->lbd_Labels, clicked_row);
                    data->lbd_RelEvent = 1;

                    /* Refresh */
                    struct RastPort *rp = ObtainGIRPort(msg->gpi_GInfo);
                    if (rp)
                    {
                        DoMethod(obj, GM_RENDER, (IPTR)msg->gpi_GInfo, (IPTR)rp, GREDRAW_UPDATE);
                        ReleaseGIRPort(rp);
                    }
                }
                retval = GMR_NOREUSE | GMR_VERIFY;
            }
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

    msg->gpd_Domain.Left   = 0;
    msg->gpd_Domain.Top    = 0;
    msg->gpd_Domain.Width  = 100;
    msg->gpd_Domain.Height = data->lbd_RowHeight * 4 + data->lbd_TitleHeight + 4;

    return (IPTR)TRUE;
}
