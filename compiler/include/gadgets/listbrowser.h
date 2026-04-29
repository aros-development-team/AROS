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
#ifndef REACTION_REACTION_H
#include <reaction/reaction.h>
#endif

#define LISTBROWSER_CLASSNAME   "listbrowser.gadget"
#define LISTBROWSER_VERSION     44

#define LISTBROWSER_Dummy       (REACTION_Dummy + 0x0003000)

#define LISTBROWSER_Labels          (LISTBROWSER_Dummy + 0x0001) /* List of ListBrowserNodes */
#define LISTBROWSER_Selected        (LISTBROWSER_Dummy + 0x0002) /* Active row index */
#define LISTBROWSER_ColumnInfo      (LISTBROWSER_Dummy + 0x0003) /* Column definitions array */
#define LISTBROWSER_MakeVisible     (LISTBROWSER_Dummy + 0x0004) /* Scroll to show selection */
#define LISTBROWSER_MultiSelect     (LISTBROWSER_Dummy + 0x0005) /* Allow multiple selection */
#define LISTBROWSER_Separators      (LISTBROWSER_Dummy + 0x0006) /* Column separator lines */
#define LISTBROWSER_ShowSelected    (LISTBROWSER_Dummy + 0x0007) /* Highlight selected row */
#define LISTBROWSER_VisibleRows     (LISTBROWSER_Dummy + 0x0008) /* Number of visible rows */
#define LISTBROWSER_TotalNodes      (LISTBROWSER_Dummy + 0x0009) /* Total entry count */
#define LISTBROWSER_HorizScroller   (LISTBROWSER_Dummy + 0x000A) /* Horizontal scrollbar */
#define LISTBROWSER_VertScroller    (LISTBROWSER_Dummy + 0x000B) /* Vertical scrollbar */
#define LISTBROWSER_ReadOnly        (LISTBROWSER_Dummy + 0x000C) /* Non-editable list */
#define LISTBROWSER_RelEvent        (LISTBROWSER_Dummy + 0x000D) /* Relative event data */
#define LISTBROWSER_ColumnTitles    (LISTBROWSER_Dummy + 0x000E) /* Clickable column headers */
#define LISTBROWSER_Striping        (LISTBROWSER_Dummy + 0x000F) /* Alternating row colors */
#define LISTBROWSER_NumColumns      (LISTBROWSER_Dummy + 0x0010) /* Column count */
#define LISTBROWSER_Hierarchical    (LISTBROWSER_Dummy + 0x0011) /* Tree display mode */
#define LISTBROWSER_Editable        (LISTBROWSER_Dummy + 0x0012) /* In-place editing */
#define LISTBROWSER_Position        (LISTBROWSER_Dummy + 0x0013) /* Top visible position */
#define LISTBROWSER_AlternateRowPen (LISTBROWSER_Dummy + 0x0014) /* Pen for alternate rows */
#define LISTBROWSER_AutoFit         (LISTBROWSER_Dummy + 0x0015) /* Auto-size columns */
#define LISTBROWSER_Borderless      (LISTBROWSER_Dummy + 0x0016) /* No border */
#define LISTBROWSER_SelectedNode    (LISTBROWSER_Dummy + 0x0017) /* Selected node pointer */

/* ListBrowserNode tags */
#define LBNA_Dummy              (TAG_USER + 0x5003500)
#define LBNA_Column             (LBNA_Dummy + 0x0001) /* Column sub-tags follow */
#define LBNA_Flags              (LBNA_Dummy + 0x0002) /* Node flags */
#define LBNA_UserData           (LBNA_Dummy + 0x0003) /* User data pointer */
#define LBNA_Generation         (LBNA_Dummy + 0x0004) /* Tree depth level */
#define LBNA_Selected           (LBNA_Dummy + 0x0005) /* Selected state */
#define LBNA_Checked            (LBNA_Dummy + 0x0006) /* Checkbox state */

/* Column entry tags */
#define LBNCA_Dummy             (TAG_USER + 0xC0200)
#define LBNCA_CopyText          (LBNCA_Dummy + 0x0001) /* Copy text to buffer */
#define LBNCA_Text              (LBNCA_Dummy + 0x0002) /* Column text */
#define LBNCA_Integer           (LBNCA_Dummy + 0x0003) /* Column integer value */
#define LBNCA_Image             (LBNCA_Dummy + 0x0004) /* Column image */
#define LBNCA_FGPen             (LBNCA_Dummy + 0x0005) /* Foreground pen */
#define LBNCA_BGPen             (LBNCA_Dummy + 0x0006) /* Background pen */
#define LBNCA_Justification     (LBNCA_Dummy + 0x0007) /* Text alignment */
#define LBNCA_Editable          (LBNCA_Dummy + 0x0008) /* In-place editing */
#define LBNCA_MaxChars          (LBNCA_Dummy + 0x0009) /* Max edit chars */
#define LBNCA_HorizJustify      (LBNCA_Dummy + 0x000A) /* Horizontal alignment */

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
    WORD    ci_Width;       /* Width (-1 = end marker) */
    STRPTR  ci_Title;       /* Title or NULL */
    ULONG   ci_Flags;       /* CIF_* flags */
};

#define CIF_WEIGHTED    (1 << 0)
#define CIF_FIXED       (1 << 1)
#define CIF_DRAGGABLE   (1 << 2)
#define CIF_SORTABLE    (1 << 3)

#ifndef ListBrowserObject
#define ListBrowserObject   NewObject(NULL, LISTBROWSER_CLASSNAME
#endif
#ifndef ListBrowserEnd
#define ListBrowserEnd      TAG_END)
#endif

#endif /* GADGETS_LISTBROWSER_H */
