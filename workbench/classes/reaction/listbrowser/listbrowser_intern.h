#ifndef LISTBROWSER_INTERN_H
#define LISTBROWSER_INTERN_H

/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.
    
    Desc: Reaction listbrowser.gadget internal definitions
*/

#include <exec/types.h>
#include <exec/lists.h>
#include <exec/nodes.h>
#include <intuition/classes.h>
#include <intuition/gadgetclass.h>
#include <gadgets/listbrowser.h>

#include LC_LIBDEFS_FILE

#include <exec/libraries.h>

/* Module library base with stored class pointer */
struct ListBrowserBase_intern
{
    struct Library lib;
    Class *rc_Class;
};

/* Per-column data in a ListBrowserNode */
struct LBColumnEntry
{
    STRPTR  lbce_Text;
    LONG    lbce_Integer;
    Object *lbce_Image;
    UBYTE   lbce_FGPen;
    UBYTE   lbce_BGPen;
    UBYTE   lbce_HorizJustify;
    UBYTE   lbce_Editable;
    UWORD   lbce_MaxChars;
    BOOL    lbce_CopyText;      /* TRUE if text was copied (needs freeing) */
};

/* Extended list browser node */
struct ListBrowserNode
{
    struct Node         lbn_Node;
    UWORD               lbn_Columns;        /* Number of columns */
    ULONG               lbn_Flags;
    APTR                lbn_UserData;
    UWORD               lbn_Generation;     /* Hierarchy depth */
    BOOL                lbn_Selected;
    BOOL                lbn_Checked;
    BOOL                lbn_Hidden;         /* Hidden by hierarchy */
    struct MinList      lbn_Children;       /* Child nodes for hierarchical */
    struct LBColumnEntry lbn_ColumnData[1]; /* Variable length column data */
};

/* Gadget instance data */
struct ListBrowserData
{
    struct List        *lbd_Labels;
    struct ColumnInfo  *lbd_ColumnInfo;
    LONG                lbd_Selected;
    LONG                lbd_Position;       /* Top visible row */
    LONG                lbd_TotalNodes;
    LONG                lbd_VisibleRows;
    BOOL                lbd_MultiSelect;
    BOOL                lbd_Separators;
    BOOL                lbd_ShowSelected;
    BOOL                lbd_ColumnTitles;
    BOOL                lbd_Hierarchical;
    BOOL                lbd_Editable;
    BOOL                lbd_AutoFit;
    BOOL                lbd_Borderless;
    UWORD               lbd_RowHeight;
    UWORD               lbd_TitleHeight;
    struct TextFont    *lbd_Font;
    struct DrawInfo    *lbd_DrawInfo;
    LONG                lbd_RelEvent;
    struct ListBrowserNode *lbd_SelectedNode;
};

#endif /* LISTBROWSER_INTERN_H */
