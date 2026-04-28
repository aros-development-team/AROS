/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: Reaction - ClassAct/ReAction compatible gadgets/listbrowser.h
*/

#ifndef GADGETS_LISTBROWSER_H
#define GADGETS_LISTBROWSER_H

#ifndef EXEC_TYPES_H
#include <exec/types.h>
#endif
#ifndef EXEC_LISTS_H
#include <exec/lists.h>
#endif
#ifndef UTILITY_TAGITEM_H
#include <utility/tagitem.h>
#endif

/*
 * listbrowser.gadget - ClassAct/ReAction compatible multi-column list browser
 *
 * Superclass: gadgetclass
 * Include:    <gadgets/listbrowser.h>
 */

#define LISTBROWSER_CLASSNAME   "gadgets/listbrowser.gadget"
#define LISTBROWSER_VERSION     44

/* Tag base */
#define LISTBROWSER_Dummy       (TAG_USER + 0xC0000)

/* Attributes */
/* (ISG) Exec list of ListBrowserNode items */
#define LISTBROWSER_Labels          (LISTBROWSER_Dummy + 0x0001)
/* (ISG) Currently selected row (0-based) */
#define LISTBROWSER_Selected        (LISTBROWSER_Dummy + 0x0002)
/* (I..) ColumnInfo array */
#define LISTBROWSER_ColumnInfo      (LISTBROWSER_Dummy + 0x0003)
/* (ISG) Make selected node visible */
#define LISTBROWSER_MakeVisible     (LISTBROWSER_Dummy + 0x0004)
/* (I..) Multi-select mode */
#define LISTBROWSER_MultiSelect     (LISTBROWSER_Dummy + 0x0005)
/* (I..) Show column separators */
#define LISTBROWSER_Separators      (LISTBROWSER_Dummy + 0x0006)
/* (I..) Show column titles */
#define LISTBROWSER_ShowSelected    (LISTBROWSER_Dummy + 0x0007)
/* (I..) Visible rows count */
#define LISTBROWSER_VisibleRows     (LISTBROWSER_Dummy + 0x0008)
/* (I..) Total node count */
#define LISTBROWSER_TotalNodes      (LISTBROWSER_Dummy + 0x0009)
/* (I..) Horizontal scroller */
#define LISTBROWSER_HorizScroller   (LISTBROWSER_Dummy + 0x000A)
/* (I..) Vertical scroller */
#define LISTBROWSER_VertScroller    (LISTBROWSER_Dummy + 0x000B)
/* (I..) Read-only mode */
#define LISTBROWSER_ReadOnly        (LISTBROWSER_Dummy + 0x000C)
/* (.G.) Relative event info */
#define LISTBROWSER_RelEvent        (LISTBROWSER_Dummy + 0x000D)
/* (I..) Column title clicks */
#define LISTBROWSER_ColumnTitles    (LISTBROWSER_Dummy + 0x000E)
/* (I..) Striping mode */
#define LISTBROWSER_Striping        (LISTBROWSER_Dummy + 0x000F)
/* (I..) Number of columns */
#define LISTBROWSER_NumColumns      (LISTBROWSER_Dummy + 0x0010)
/* (I..) Hierarchical display */
#define LISTBROWSER_Hierarchical    (LISTBROWSER_Dummy + 0x0011)
/* (I..) Editable mode */
#define LISTBROWSER_Editable        (LISTBROWSER_Dummy + 0x0012)
/* (ISG) Position of list top */
#define LISTBROWSER_Position        (LISTBROWSER_Dummy + 0x0013)
/* (I..) Alternate row pen */
#define LISTBROWSER_AlternateRowPen (LISTBROWSER_Dummy + 0x0014)
/* (I..) Auto-fit columns */
#define LISTBROWSER_AutoFit         (LISTBROWSER_Dummy + 0x0015)
/* (I..) Border */
#define LISTBROWSER_Borderless      (LISTBROWSER_Dummy + 0x0016)
/* (.G.) Selected node pointer */
#define LISTBROWSER_SelectedNode    (LISTBROWSER_Dummy + 0x0017)

/* ListBrowserNode attributes (for AllocListBrowserNodeA) */
#define LBNA_Dummy              (TAG_USER + 0xC0100)
#define LBNA_Column             (LBNA_Dummy + 0x0001)
#define LBNA_Flags              (LBNA_Dummy + 0x0002)
#define LBNA_UserData           (LBNA_Dummy + 0x0003)
#define LBNA_Generation         (LBNA_Dummy + 0x0004)
#define LBNA_Selected           (LBNA_Dummy + 0x0005)
#define LBNA_Checked            (LBNA_Dummy + 0x0006)

/* Column entry types */
#define LBNCA_Dummy             (TAG_USER + 0xC0200)
#define LBNCA_CopyText          (LBNCA_Dummy + 0x0001)
#define LBNCA_Text              (LBNCA_Dummy + 0x0002)
#define LBNCA_Integer           (LBNCA_Dummy + 0x0003)
#define LBNCA_Image             (LBNCA_Dummy + 0x0004)
#define LBNCA_FGPen             (LBNCA_Dummy + 0x0005)
#define LBNCA_BGPen             (LBNCA_Dummy + 0x0006)
#define LBNCA_Justification     (LBNCA_Dummy + 0x0007)
#define LBNCA_Editable          (LBNCA_Dummy + 0x0008)
#define LBNCA_MaxChars          (LBNCA_Dummy + 0x0009)
#define LBNCA_HorizJustify      (LBNCA_Dummy + 0x000A)

/* Justification */
#define LCJ_LEFT        0
#define LCJ_CENTER      1
#define LCJ_RIGHT       2

/* Striping modes */
#define LBS_NONE        0
#define LBS_ROWS        1

/* ColumnInfo structure */
struct ColumnInfo
{
    WORD    ci_Width;       /* Column width (-1 = end marker) */
    STRPTR  ci_Title;       /* Column title (NULL for none) */
    ULONG   ci_Flags;       /* Column flags */
};

/* ColumnInfo flags */
#define CIF_WEIGHTED    (1 << 0)    /* Width is a weight */
#define CIF_FIXED       (1 << 1)    /* Fixed width */
#define CIF_DRAGGABLE   (1 << 2)    /* Column can be dragged */
#define CIF_SORTABLE    (1 << 3)    /* Column is sortable */

/* Object creation macros */
#define ListBrowserObject   NewObject(NULL, LISTBROWSER_CLASSNAME
#define ListBrowserEnd      TAG_END)

#endif /* GADGETS_LISTBROWSER_H */
