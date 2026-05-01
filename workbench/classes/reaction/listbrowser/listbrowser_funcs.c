/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: Reaction listbrowser.gadget - Exported node/column management functions
*/

#include <string.h>
#include <stdlib.h>

#include <aros/libcall.h>

#include <exec/types.h>
#include <exec/memory.h>
#include <exec/lists.h>
#include <utility/tagitem.h>
#include <gadgets/listbrowser.h>

#include <proto/exec.h>
#include <proto/utility.h>
#include <proto/alib.h>

#include "listbrowser_intern.h"

/* Static helper for recursive list freeing (can't self-call AROS_LH functions) */
static void lb_free_list_internal(struct List *list)
{
    if (list)
    {
        struct Node *node, *next;
        for (node = list->lh_Head; (next = node->ln_Succ); node = next)
        {
            Remove(node);
            struct ListBrowserNode *lbn = (struct ListBrowserNode *)node;
            if (!IsListEmpty((struct List *)&lbn->lbn_Children))
            {
                lb_free_list_internal((struct List *)&lbn->lbn_Children);
            }
            {
                UWORD i;
                for (i = 0; i < lbn->lbn_Columns; i++)
                {
                    if (lbn->lbn_ColumnData[i].lbce_CopyText && lbn->lbn_ColumnData[i].lbce_Text)
                        FreeVec(lbn->lbn_ColumnData[i].lbce_Text);
                }
            }
            FreeVec(lbn);
        }
        NewList(list);
    }
}

/*****************************************************************************
 * AllocListBrowserNodeA - Allocate a new list browser node
 *****************************************************************************/
AROS_LH2(struct Node *, AllocListBrowserNodeA,
    AROS_LHA(UWORD,            columns, D0),
    AROS_LHA(struct TagItem *, tags,    A0),
    struct Library *, ListBrowserBase, 6, ListBrowser)
{
    AROS_LIBFUNC_INIT

    struct ListBrowserNode *lbn;
    struct TagItem *tag;

    lbn = AllocVec(sizeof(struct ListBrowserNode) +
        sizeof(struct LBColumnEntry) * (columns > 0 ? columns - 1 : 0),
        MEMF_CLEAR | MEMF_PUBLIC);
    if (!lbn) return NULL;

    lbn->lbn_Columns = columns;

    NewList((struct List *)&lbn->lbn_Children);

    /* Process tags */
    if (tags)
    {
        UWORD cur_column = 0;

        while ((tag = NextTagItem(&tags)))
        {
            switch (tag->ti_Tag)
            {
                case LBNA_Column:
                    cur_column = (UWORD)tag->ti_Data;
                    break;
                case LBNA_Flags:
                    lbn->lbn_Flags = tag->ti_Data;
                    break;
                case LBNA_UserData:
                    lbn->lbn_UserData = (APTR)tag->ti_Data;
                    break;
                case LBNA_Generation:
                    lbn->lbn_Generation = (UWORD)tag->ti_Data;
                    break;
                case LBNA_Selected:
                    lbn->lbn_Selected = (BOOL)tag->ti_Data;
                    break;
                case LBNA_Checked:
                    lbn->lbn_Checked = (BOOL)tag->ti_Data;
                    break;

                /* Column entry tags */
                case LBNCA_CopyText:
                    if (cur_column < columns)
                        lbn->lbn_ColumnData[cur_column].lbce_CopyText = (BOOL)tag->ti_Data;
                    break;
                case LBNCA_Text:
                    if (cur_column < columns)
                    {
                        STRPTR text = (STRPTR)tag->ti_Data;
                        if (lbn->lbn_ColumnData[cur_column].lbce_CopyText && text)
                        {
                            ULONG len = strlen(text) + 1;
                            lbn->lbn_ColumnData[cur_column].lbce_Text = AllocVec(len, MEMF_PUBLIC);
                            if (lbn->lbn_ColumnData[cur_column].lbce_Text)
                                CopyMem(text, lbn->lbn_ColumnData[cur_column].lbce_Text, len);
                        }
                        else
                        {
                            lbn->lbn_ColumnData[cur_column].lbce_Text = text;
                        }
                    }
                    break;
                case LBNCA_Integer:
                    if (cur_column < columns)
                        lbn->lbn_ColumnData[cur_column].lbce_Integer = (LONG)tag->ti_Data;
                    break;
                case LBNCA_Image:
                    if (cur_column < columns)
                        lbn->lbn_ColumnData[cur_column].lbce_Image = (Object *)tag->ti_Data;
                    break;
                case LBNCA_FGPen:
                    if (cur_column < columns)
                        lbn->lbn_ColumnData[cur_column].lbce_FGPen = (UBYTE)tag->ti_Data;
                    break;
                case LBNCA_BGPen:
                    if (cur_column < columns)
                        lbn->lbn_ColumnData[cur_column].lbce_BGPen = (UBYTE)tag->ti_Data;
                    break;
                case LBNCA_HorizJustify:
                    if (cur_column < columns)
                        lbn->lbn_ColumnData[cur_column].lbce_HorizJustify = (UBYTE)tag->ti_Data;
                    break;
                case LBNCA_Editable:
                    if (cur_column < columns)
                        lbn->lbn_ColumnData[cur_column].lbce_Editable = (UBYTE)tag->ti_Data;
                    break;
                case LBNCA_MaxChars:
                    if (cur_column < columns)
                        lbn->lbn_ColumnData[cur_column].lbce_MaxChars = (UWORD)tag->ti_Data;
                    break;
            }
        }
    }

    return (struct Node *)lbn;

    AROS_LIBFUNC_EXIT
}

/*****************************************************************************
 * FreeListBrowserNode - Free a list browser node
 *****************************************************************************/
AROS_LH1(void, FreeListBrowserNode,
    AROS_LHA(struct Node *, node, A0),
    struct Library *, ListBrowserBase, 7, ListBrowser)
{
    AROS_LIBFUNC_INIT

    if (node)
    {
        struct ListBrowserNode *lbn = (struct ListBrowserNode *)node;

        /* Remove from list if still linked */
        if (node->ln_Pred && node->ln_Succ)
            Remove(node);

        /* Free copied text in column data */
        {
            UWORD i;
            for (i = 0; i < lbn->lbn_Columns; i++)
            {
                if (lbn->lbn_ColumnData[i].lbce_CopyText && lbn->lbn_ColumnData[i].lbce_Text)
                    FreeVec(lbn->lbn_ColumnData[i].lbce_Text);
            }
        }

        FreeVec(lbn);
    }

    AROS_LIBFUNC_EXIT
}

/*****************************************************************************
 * SetListBrowserNodeAttrsA - Set attributes on a list browser node
 *****************************************************************************/
AROS_LH2(void, SetListBrowserNodeAttrsA,
    AROS_LHA(struct Node *, node, A0),
    AROS_LHA(struct TagItem *, tags, A1),
    struct Library *, ListBrowserBase, 8, ListBrowser)
{
    AROS_LIBFUNC_INIT

    if (!node) return;

    struct ListBrowserNode *lbn = (struct ListBrowserNode *)node;
    struct TagItem *tag;
    UWORD cur_column = 0;

    while ((tag = NextTagItem(&tags)))
    {
        switch (tag->ti_Tag)
        {
            case LBNA_Column:
                cur_column = (UWORD)tag->ti_Data;
                break;
            case LBNA_Flags:
                lbn->lbn_Flags = tag->ti_Data;
                break;
            case LBNA_UserData:
                lbn->lbn_UserData = (APTR)tag->ti_Data;
                break;
            case LBNA_Generation:
                lbn->lbn_Generation = (UWORD)tag->ti_Data;
                break;
            case LBNA_Selected:
                lbn->lbn_Selected = (BOOL)tag->ti_Data;
                break;
            case LBNA_Checked:
                lbn->lbn_Checked = (BOOL)tag->ti_Data;
                break;
            case LBNCA_Text:
                if (cur_column < lbn->lbn_Columns)
                {
                    if (lbn->lbn_ColumnData[cur_column].lbce_CopyText &&
                        lbn->lbn_ColumnData[cur_column].lbce_Text)
                        FreeVec(lbn->lbn_ColumnData[cur_column].lbce_Text);

                    STRPTR text = (STRPTR)tag->ti_Data;
                    if (lbn->lbn_ColumnData[cur_column].lbce_CopyText && text)
                    {
                        ULONG len = strlen(text) + 1;
                        lbn->lbn_ColumnData[cur_column].lbce_Text = AllocVec(len, MEMF_PUBLIC);
                        if (lbn->lbn_ColumnData[cur_column].lbce_Text)
                            CopyMem(text, lbn->lbn_ColumnData[cur_column].lbce_Text, len);
                    }
                    else
                    {
                        lbn->lbn_ColumnData[cur_column].lbce_Text = text;
                    }
                }
                break;
            case LBNCA_Integer:
                if (cur_column < lbn->lbn_Columns)
                    lbn->lbn_ColumnData[cur_column].lbce_Integer = (LONG)tag->ti_Data;
                break;
            case LBNCA_Image:
                if (cur_column < lbn->lbn_Columns)
                    lbn->lbn_ColumnData[cur_column].lbce_Image = (Object *)tag->ti_Data;
                break;
            case LBNCA_FGPen:
                if (cur_column < lbn->lbn_Columns)
                    lbn->lbn_ColumnData[cur_column].lbce_FGPen = (UBYTE)tag->ti_Data;
                break;
            case LBNCA_BGPen:
                if (cur_column < lbn->lbn_Columns)
                    lbn->lbn_ColumnData[cur_column].lbce_BGPen = (UBYTE)tag->ti_Data;
                break;
            case LBNCA_HorizJustify:
                if (cur_column < lbn->lbn_Columns)
                    lbn->lbn_ColumnData[cur_column].lbce_HorizJustify = (UBYTE)tag->ti_Data;
                break;
        }
    }

    AROS_LIBFUNC_EXIT
}

/*****************************************************************************
 * GetListBrowserNodeAttrsA - Get attributes from a list browser node
 *****************************************************************************/
AROS_LH2(void, GetListBrowserNodeAttrsA,
    AROS_LHA(struct Node *, node, A0),
    AROS_LHA(struct TagItem *, tags, A1),
    struct Library *, ListBrowserBase, 9, ListBrowser)
{
    AROS_LIBFUNC_INIT

    if (!node) return;

    struct ListBrowserNode *lbn = (struct ListBrowserNode *)node;
    struct TagItem *tag;
    UWORD cur_column = 0;

    while ((tag = NextTagItem(&tags)))
    {
        switch (tag->ti_Tag)
        {
            case LBNA_Column:
                cur_column = (UWORD)tag->ti_Data;
                break;
            case LBNA_Flags:
                *(ULONG *)tag->ti_Data = lbn->lbn_Flags;
                break;
            case LBNA_UserData:
                *(APTR *)tag->ti_Data = lbn->lbn_UserData;
                break;
            case LBNA_Generation:
                *(UWORD *)tag->ti_Data = lbn->lbn_Generation;
                break;
            case LBNA_Selected:
                *(BOOL *)tag->ti_Data = lbn->lbn_Selected;
                break;
            case LBNA_Checked:
                *(BOOL *)tag->ti_Data = lbn->lbn_Checked;
                break;
            case LBNCA_Text:
                if (cur_column < lbn->lbn_Columns)
                    *(STRPTR *)tag->ti_Data = lbn->lbn_ColumnData[cur_column].lbce_Text;
                break;
            case LBNCA_Integer:
                if (cur_column < lbn->lbn_Columns)
                    *(LONG *)tag->ti_Data = lbn->lbn_ColumnData[cur_column].lbce_Integer;
                break;
            case LBNCA_Image:
                if (cur_column < lbn->lbn_Columns)
                    *(Object **)tag->ti_Data = lbn->lbn_ColumnData[cur_column].lbce_Image;
                break;
        }
    }

    AROS_LIBFUNC_EXIT
}

/*****************************************************************************
 * ListBrowserSelectAll - Select all nodes in a list
 *****************************************************************************/
AROS_LH1(void, ListBrowserSelectAll,
    AROS_LHA(struct List *, list, A0),
    struct Library *, ListBrowserBase, 10, ListBrowser)
{
    AROS_LIBFUNC_INIT

    if (list)
    {
        struct Node *node;
        for (node = list->lh_Head; node->ln_Succ; node = node->ln_Succ)
        {
            ((struct ListBrowserNode *)node)->lbn_Selected = TRUE;
        }
    }

    AROS_LIBFUNC_EXIT
}

/*****************************************************************************
 * ShowListBrowserNodeChildren - Show children of a hierarchical node
 *****************************************************************************/
AROS_LH2(void, ShowListBrowserNodeChildren,
    AROS_LHA(struct Node *, node, A0),
    AROS_LHA(WORD,          depth, D0),
    struct Library *, ListBrowserBase, 11, ListBrowser)
{
    AROS_LIBFUNC_INIT

    if (node)
    {
        struct ListBrowserNode *lbn = (struct ListBrowserNode *)node;
        struct Node *child;
        for (child = ((struct List *)&lbn->lbn_Children)->lh_Head; child->ln_Succ; child = child->ln_Succ)
        {
            ((struct ListBrowserNode *)child)->lbn_Hidden = !depth;
        }
    }

    AROS_LIBFUNC_EXIT
}

/*****************************************************************************
 * HideListBrowserNodeChildren - Hide children of a hierarchical node
 *****************************************************************************/
AROS_LH1(void, HideListBrowserNodeChildren,
    AROS_LHA(struct Node *, node, A0),
    struct Library *, ListBrowserBase, 12, ListBrowser)
{
    AROS_LIBFUNC_INIT

    if (node)
    {
        struct ListBrowserNode *lbn = (struct ListBrowserNode *)node;
        struct Node *child;
        for (child = ((struct List *)&lbn->lbn_Children)->lh_Head; child->ln_Succ; child = child->ln_Succ)
        {
            ((struct ListBrowserNode *)child)->lbn_Hidden = TRUE;
        }
    }

    AROS_LIBFUNC_EXIT
}

/*****************************************************************************
 * ShowAllListBrowserChildren - Show all children in entire list
 *****************************************************************************/
AROS_LH1(void, ShowAllListBrowserChildren,
    AROS_LHA(struct List *, list, A0),
    struct Library *, ListBrowserBase, 13, ListBrowser)
{
    AROS_LIBFUNC_INIT

    if (list)
    {
        struct Node *node;
        for (node = list->lh_Head; node->ln_Succ; node = node->ln_Succ)
        {
            ((struct ListBrowserNode *)node)->lbn_Hidden = FALSE;
        }
    }

    AROS_LIBFUNC_EXIT
}

/*****************************************************************************
 * HideAllListBrowserChildren - Hide all children in entire list
 *****************************************************************************/
AROS_LH1(void, HideAllListBrowserChildren,
    AROS_LHA(struct List *, list, A0),
    struct Library *, ListBrowserBase, 14, ListBrowser)
{
    AROS_LIBFUNC_INIT

    if (list)
    {
        struct Node *node;
        for (node = list->lh_Head; node->ln_Succ; node = node->ln_Succ)
        {
            struct ListBrowserNode *lbn = (struct ListBrowserNode *)node;
            if (lbn->lbn_Generation > 0)
                lbn->lbn_Hidden = TRUE;
        }
    }

    AROS_LIBFUNC_EXIT
}

/*****************************************************************************
 * FreeListBrowserList - Free all nodes in a list
 *****************************************************************************/
AROS_LH1(void, FreeListBrowserList,
    AROS_LHA(struct List *, list, A0),
    struct Library *, ListBrowserBase, 15, ListBrowser)
{
    AROS_LIBFUNC_INIT

    lb_free_list_internal(list);

    AROS_LIBFUNC_EXIT
}

/*****************************************************************************
 * AllocLBColumnInfoA - Allocate ColumnInfo array
 *****************************************************************************/
AROS_LH2(struct ColumnInfo *, AllocLBColumnInfoA,
    AROS_LHA(UWORD,            columns, D0),
    AROS_LHA(struct TagItem *, tags,    A0),
    struct Library *, ListBrowserBase, 16, ListBrowser)
{
    AROS_LIBFUNC_INIT

    struct ColumnInfo *ci;
    UWORD i;

    /* Allocate columns + 1 for end marker */
    ci = AllocVec(sizeof(struct ColumnInfo) * (columns + 1), MEMF_CLEAR | MEMF_PUBLIC);
    if (!ci) return NULL;

    /* Set end marker */
    ci[columns].ci_Width = -1;

    /* Default all columns to equal weighted */
    for (i = 0; i < columns; i++)
    {
        ci[i].ci_Width = 100 / columns;
        ci[i].ci_Flags = CIF_WEIGHTED;
    }

    /* Apply tags */
    if (tags)
    {
        struct TagItem *tag;
        UWORD cur_col = 0;
        while ((tag = NextTagItem(&tags)))
        {
            switch (tag->ti_Tag)
            {
                case LBNA_Column:
                    cur_col = (UWORD)tag->ti_Data;
                    break;
                case LBNCA_CopyText:
                    if (cur_col < columns)
                        ci[cur_col].ci_Flags = (ULONG)tag->ti_Data;
                    break;
                case LBNCA_Text:
                    if (cur_col < columns)
                        ci[cur_col].ci_Title = (STRPTR)tag->ti_Data;
                    break;
            }
        }
    }

    return ci;

    AROS_LIBFUNC_EXIT
}

/*****************************************************************************
 * SetLBColumnInfoAttrsA - Set attributes on ColumnInfo
 *****************************************************************************/
AROS_LH2(LONG, SetLBColumnInfoAttrsA,
    AROS_LHA(struct ColumnInfo *, colinfo, A1),
    AROS_LHA(struct TagItem *,    tags,    A0),
    struct Library *, ListBrowserBase, 17, ListBrowser)
{
    AROS_LIBFUNC_INIT

    if (!colinfo) return 0;

    struct TagItem *tag;
    UWORD cur_col = 0;
    LONG count = 0;

    while ((tag = NextTagItem(&tags)))
    {
        switch (tag->ti_Tag)
        {
            case LBNA_Column:
                cur_col = (UWORD)tag->ti_Data;
                break;
            case LBNCA_Text:
                colinfo[cur_col].ci_Title = (STRPTR)tag->ti_Data;
                count++;
                break;
            case LBNCA_CopyText:
                colinfo[cur_col].ci_Flags = (ULONG)tag->ti_Data;
                count++;
                break;
        }
    }

    return count;

    AROS_LIBFUNC_EXIT
}

/*****************************************************************************
 * GetLBColumnInfoAttrsA - Get attributes from ColumnInfo
 *****************************************************************************/
AROS_LH2(LONG, GetLBColumnInfoAttrsA,
    AROS_LHA(struct ColumnInfo *, colinfo, A1),
    AROS_LHA(struct TagItem *,    tags,    A0),
    struct Library *, ListBrowserBase, 18, ListBrowser)
{
    AROS_LIBFUNC_INIT

    if (!colinfo) return 0;

    struct TagItem *tag;
    UWORD cur_col = 0;
    LONG count = 0;

    while ((tag = NextTagItem(&tags)))
    {
        switch (tag->ti_Tag)
        {
            case LBNA_Column:
                cur_col = (UWORD)tag->ti_Data;
                break;
            case LBNCA_Text:
                *(STRPTR *)tag->ti_Data = colinfo[cur_col].ci_Title;
                count++;
                break;
            case LBNCA_CopyText:
                *(ULONG *)tag->ti_Data = colinfo[cur_col].ci_Flags;
                count++;
                break;
        }
    }

    return count;

    AROS_LIBFUNC_EXIT
}

/*****************************************************************************
 * FreeLBColumnInfo - Free ColumnInfo array
 *****************************************************************************/
AROS_LH1(void, FreeLBColumnInfo,
    AROS_LHA(struct ColumnInfo *, colinfo, A0),
    struct Library *, ListBrowserBase, 19, ListBrowser)
{
    AROS_LIBFUNC_INIT

    if (colinfo)
        FreeVec(colinfo);

    AROS_LIBFUNC_EXIT
}

/*****************************************************************************
 * ListBrowserClearAll - Clear all nodes from list (free + reinit)
 *****************************************************************************/
AROS_LH1(void, ListBrowserClearAll,
    AROS_LHA(struct List *, list, A0),
    struct Library *, ListBrowserBase, 20, ListBrowser)
{
    AROS_LIBFUNC_INIT

    lb_free_list_internal(list);

    AROS_LIBFUNC_EXIT
}
