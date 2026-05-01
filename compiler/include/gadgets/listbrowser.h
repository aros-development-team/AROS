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
#ifndef INTUITION_GADGETCLASS_H
#include <intuition/gadgetclass.h>
#endif

#define LISTBROWSER_CLASSNAME   "listbrowser.gadget"
#define LISTBROWSER_VERSION     44

/*****************************************************************************/

/* ListBrowser methods */

/* LBM_ADDNODE - Create and insert a node into the attached list (V41) */
#define LBM_ADDNODE         (0x580001L)

struct lbAddNode
{
    ULONG MethodID;
    struct GadgetInfo *lba_GInfo;
    struct Node *lba_Node;
    struct TagItem *lba_NodeAttrs;
};

/* LBM_REMNODE - Remove and free a node from the list (V41) */
#define LBM_REMNODE         (0x580002L)

struct lbRemNode
{
    ULONG MethodID;
    struct GadgetInfo *lbr_GInfo;
    struct Node *lbr_Node;
};

/* LBM_EDITNODE - Modify a node's attributes (V41) */
#define LBM_EDITNODE        (0x580003L)

struct lbEditNode
{
    ULONG MethodID;
    struct GadgetInfo *lbe_GInfo;
    struct Node *lbe_Node;
    struct TagItem *lbe_NodeAttrs;
};

/* LBM_SORT - Sort the list by column */
#define LBM_SORT            (0x580004L)

struct lbSort
{
    ULONG MethodID;
    struct GadgetInfo *lbs_GInfo;
    ULONG lbs_Column;
    ULONG lbs_Reverse;
    struct Hook *lbs_CompareHook;
};

/* LBM_SHOWCHILDREN - Show child nodes of a parent */
#define LBM_SHOWCHILDREN    (0x580005L)

struct lbShowChildren
{
    ULONG MethodID;
    struct GadgetInfo *lbsc_GInfo;
    struct Node *lbsc_Node;
    WORD lbsc_Depth;
};

/* LBM_HIDECHILDREN - Hide child nodes of a parent */
#define LBM_HIDECHILDREN    (0x580006L)

struct lbHideChildren
{
    ULONG MethodID;
    struct GadgetInfo *lbhc_GInfo;
    struct Node *lbhc_Node;
    WORD lbhc_Depth;
};

/*****************************************************************************/

/* ListBrowser Node attributes - LBNA_ and LBNCA_ share the same tag base */
#define LBNA_Dummy              (TAG_USER + 0x5003500)

#define LBNA_Selected           (LBNA_Dummy + 1)    /* Node selected state */
#define LBNA_Flags              (LBNA_Dummy + 2)    /* Node flags (LBFLG_*) */
#define LBNA_UserData           (LBNA_Dummy + 3)    /* User data pointer */
#define LBNA_Column             (LBNA_Dummy + 4)    /* Column for following attrs */
#define LBNCA_Text              (LBNA_Dummy + 5)    /* Column text string */
#define LBNCA_Integer           (LBNA_Dummy + 6)    /* Column integer pointer */
#define LBNCA_FGPen             (LBNA_Dummy + 7)    /* Column foreground pen */
#define LBNCA_BGPen             (LBNA_Dummy + 8)    /* Column background pen */
#define LBNCA_Image             (LBNA_Dummy + 9)    /* Column image */
#define LBNCA_SelImage          (LBNA_Dummy + 10)   /* Column selected image */
#define LBNCA_HorizJustify      (LBNA_Dummy + 11)   /* Column horizontal justify */
#define LBNCA_Justification     LBNCA_HorizJustify  /* Alias */
#define LBNA_Generation         (LBNA_Dummy + 12)   /* Tree depth level */
#define LBNCA_Editable          (LBNA_Dummy + 13)   /* Column is editable */
#define LBNCA_MaxChars          (LBNA_Dummy + 14)   /* Max chars for editable */
#define LBNCA_CopyText          (LBNA_Dummy + 15)   /* Copy text to internal buf */
#define LBNA_CheckBox           (LBNA_Dummy + 16)   /* Node has a checkbox */
#define LBNA_Checked            (LBNA_Dummy + 17)   /* Checkbox checked state */
#define LBNA_NodeSize           (LBNA_Dummy + 18)   /* Custom node size */
#define LBNCA_EditTags          (LBNA_Dummy + 19)   /* Tags for edit string gad */
#define LBNCA_RenderHook        (LBNA_Dummy + 20)   /* Custom render hook */
#define LBNCA_HookHeight        (LBNA_Dummy + 22)   /* Hook render height */
#define LBNA_MemPool            (LBNA_Dummy + 23)   /* Exec memory pool */
#define LBNA_NumColumns         (LBNA_Dummy + 24)   /* Number of columns (GET) */
#define LBNA_Priority           (LBNA_Dummy + 25)   /* Exec node priority */
#define LBNCA_CopyInteger       (LBNA_Dummy + 26)   /* Copy integer value */
#define LBNCA_WordWrap          (LBNA_Dummy + 27)   /* Word wrap text */
#define LBNCA_VertJustify       (LBNA_Dummy + 28)   /* Row vertical justify */

/* Node flags for LBNA_Flags */
#define LBFLG_READONLY      1
#define LBFLG_CUSTOMPENS    2
#define LBFLG_HASCHILDREN   4
#define LBFLG_SHOWCHILDREN  8
#define LBFLG_HIDDEN        16

/* Horizontal justification values for LBNCA_HorizJustify */
#define LCJ_LEFT            0
#define LCJ_CENTER          1
#define LCJ_RIGHT           2
#define LCJ_CENTRE          LCJ_CENTER

/* Vertical justification values for LBNCA_VertJustify */
#define LRJ_BOTTOM          0
#define LRJ_CENTER          1
#define LRJ_TOP             2
#define LRJ_CENTRE          LRJ_CENTER

/*****************************************************************************/

/* Render hook definitions for LBNCA_RenderHook */

/* Hook message types */
#define LB_DRAW             0x202L  /* Draw with state */

/* Hook return values */
#define LBCB_OK             0       /* Hook understood the message */
#define LBCB_UNKNOWN        1       /* Hook did not understand */

/* States for LBDrawMsg.lbdm_State */
#define LBR_NORMAL          0       /* Normal rendering */
#define LBR_SELECTED        1       /* Selected rendering */

/* LB_DRAW message structure */
struct LBDrawMsg
{
    ULONG lbdm_MethodID;
    struct RastPort *lbdm_RastPort;
    struct DrawInfo *lbdm_DrawInfo;
    struct Rectangle lbdm_Bounds;
    ULONG lbdm_State;
};

/* Sort hook data structure */
struct LBSortMsg
{
    ULONG lbsm_TypeA;
    union
    {
        LONG Integer;
        STRPTR Text;
    } lbsm_DataA;
    APTR lbsm_UserDataA;

    ULONG lbsm_TypeB;
    union
    {
        LONG Integer;
        STRPTR Text;
    } lbsm_DataB;
    APTR lbsm_UserDataB;
};

/*****************************************************************************/

/* ColumnInfo structure */
struct ColumnInfo
{
    WORD ci_Width;
    STRPTR ci_Title;
    ULONG ci_Flags;
};

/* ColumnInfo flags */
#define CIF_WEIGHTED        0
#define CIF_FIXED           1
#define CIF_DRAGGABLE       2
#define CIF_NOSEPARATORS    4
#define CIF_SORTABLE        8

/* ColumnInfo attributes (V45) - using LBNA_Dummy base */
#define LBCIA_MemPool       (LBNA_Dummy + 50)   /* MemPool for ColumnInfo */
#define LBCIA_Column        (LBNA_Dummy + 51)   /* Column number */
#define LBCIA_Title         (LBNA_Dummy + 52)   /* Column title */
#define LBCIA_Weight        (LBNA_Dummy + 53)   /* Column weight */
#define LBCIA_Width         (LBNA_Dummy + 54)   /* Column pixel width */
#define LBCIA_Flags         (LBNA_Dummy + 55)   /* Column flags (CIF_*) */

/*****************************************************************************/

/* ListBrowser gadget attributes */
#define LISTBROWSER_Dummy           (REACTION_Dummy + 0x0003000)

#define LISTBROWSER_Top             (LISTBROWSER_Dummy + 1)  /* Top visible node */
#define LISTBROWSER_Reserved1       (LISTBROWSER_Dummy + 2)  /* Reserved */
#define LISTBROWSER_Labels          (LISTBROWSER_Dummy + 3)  /* Node list */
#define LISTBROWSER_Selected        (LISTBROWSER_Dummy + 4)  /* Selected node index */
#define LISTBROWSER_SelectedNode    (LISTBROWSER_Dummy + 5)  /* Selected node pointer */
#define LISTBROWSER_MultiSelect     (LISTBROWSER_Dummy + 6)  /* Allow multi-select */
#define LISTBROWSER_VertSeparators  (LISTBROWSER_Dummy + 7)  /* Column separators */
#define LISTBROWSER_Separators      LISTBROWSER_VertSeparators /* Alias */
#define LISTBROWSER_ColumnInfo      (LISTBROWSER_Dummy + 8)  /* Column info array */
#define LISTBROWSER_MakeVisible     (LISTBROWSER_Dummy + 9)  /* Scroll node visible */
#define LISTBROWSER_VirtualWidth    (LISTBROWSER_Dummy + 10) /* Virtual pixel width */
#define LISTBROWSER_Borderless      (LISTBROWSER_Dummy + 11) /* No border */
#define LISTBROWSER_VerticalProp    (LISTBROWSER_Dummy + 12) /* Vertical scrollbar */
#define LISTBROWSER_HorizontalProp  (LISTBROWSER_Dummy + 13) /* Horizontal scrollbar */
#define LISTBROWSER_Left            (LISTBROWSER_Dummy + 14) /* Left scroll position */
#define LISTBROWSER_Reserved2       (LISTBROWSER_Dummy + 15) /* Reserved */
#define LISTBROWSER_AutoFit         (LISTBROWSER_Dummy + 16) /* Auto-fit columns */
#define LISTBROWSER_ColumnTitles    (LISTBROWSER_Dummy + 17) /* Show column titles */
#define LISTBROWSER_ShowSelected    (LISTBROWSER_Dummy + 18) /* Highlight selection */
#define LISTBROWSER_VPropTotal      (LISTBROWSER_Dummy + 19) /* Vert prop total */
#define LISTBROWSER_VPropTop        (LISTBROWSER_Dummy + 20) /* Vert prop top */
#define LISTBROWSER_VPropVisible    (LISTBROWSER_Dummy + 21) /* Vert prop visible */
#define LISTBROWSER_HPropTotal      (LISTBROWSER_Dummy + 22) /* Horiz prop total */
#define LISTBROWSER_HPropTop        (LISTBROWSER_Dummy + 23) /* Horiz prop top */
#define LISTBROWSER_HPropVisible    (LISTBROWSER_Dummy + 24) /* Horiz prop visible */
#define LISTBROWSER_MouseX          (LISTBROWSER_Dummy + 25) /* Mouse release X */
#define LISTBROWSER_MouseY          (LISTBROWSER_Dummy + 26) /* Mouse release Y */
#define LISTBROWSER_Hierarchical    (LISTBROWSER_Dummy + 27) /* Tree mode */
#define LISTBROWSER_ShowImage       (LISTBROWSER_Dummy + 28) /* Expanded branch image */
#define LISTBROWSER_HideImage       (LISTBROWSER_Dummy + 29) /* Collapsed branch image */
#define LISTBROWSER_LeafImage       (LISTBROWSER_Dummy + 30) /* Leaf item image */
#define LISTBROWSER_ScrollRaster    (LISTBROWSER_Dummy + 31) /* Use ScrollRaster */
#define LISTBROWSER_Spacing         (LISTBROWSER_Dummy + 32) /* Row spacing */
#define LISTBROWSER_Editable        (LISTBROWSER_Dummy + 33) /* Enable editing */
#define LISTBROWSER_Position        (LISTBROWSER_Dummy + 34) /* Scroll position cmd */
#define LISTBROWSER_EditNode        (LISTBROWSER_Dummy + 35) /* Node to edit */
#define LISTBROWSER_EditColumn      (LISTBROWSER_Dummy + 36) /* Column to edit */
#define LISTBROWSER_RelEvent        (LISTBROWSER_Dummy + 37) /* Release event type */
#define LISTBROWSER_NumSelected     (LISTBROWSER_Dummy + 38) /* Selected count */
#define LISTBROWSER_EditTags        (LISTBROWSER_Dummy + 39) /* Edit string tags */
#define LISTBROWSER_RelColumn       (LISTBROWSER_Dummy + 40) /* Clicked column */
#define LISTBROWSER_HorizSeparators (LISTBROWSER_Dummy + 41) /* Row separators */
#define LISTBROWSER_CheckImage      (LISTBROWSER_Dummy + 42) /* Custom check image */
#define LISTBROWSER_UncheckedImage  (LISTBROWSER_Dummy + 43) /* Custom uncheck image */
#define LISTBROWSER_TotalNodes      (LISTBROWSER_Dummy + 44) /* Total node count */
#define LISTBROWSER_MinNodeSize     (LISTBROWSER_Dummy + 45) /* Min node pool size */
#define LISTBROWSER_TitleClickable  (LISTBROWSER_Dummy + 46) /* Clickable titles */
#define LISTBROWSER_MinVisible      (LISTBROWSER_Dummy + 47) /* Min visible nodes */
#define LISTBROWSER_Reserved6       (LISTBROWSER_Dummy + 48) /* Reserved */
#define LISTBROWSER_Reserved7       (LISTBROWSER_Dummy + 49) /* Reserved */
#define LISTBROWSER_PersistSelect   (LISTBROWSER_Dummy + 50) /* No shift for multi */
#define LISTBROWSER_CursorSelect    (LISTBROWSER_Dummy + 51) /* Cursor selected idx */
#define LISTBROWSER_CursorNode      (LISTBROWSER_Dummy + 52) /* Cursor selected node */
#define LISTBROWSER_FastRender      (LISTBROWSER_Dummy + 53) /* Use mask planes */
#define LISTBROWSER_TotalVisibleNodes (LISTBROWSER_Dummy + 54) /* Visible node count */
#define LISTBROWSER_WrapText        (LISTBROWSER_Dummy + 55) /* Enable word wrap */

/*****************************************************************************/

/* Values for LISTBROWSER_Position */
#define LBP_LINEUP          1
#define LBP_LINEDOWN        2
#define LBP_PAGEUP          3
#define LBP_PAGEDOWN        4
#define LBP_TOP             5
#define LBP_BOTTOM          6
#define LBP_SHIFTLEFT       10
#define LBP_SHIFTRIGHT      11
#define LBP_LEFTEDGE        12
#define LBP_RIGHTEDGE       13

/* Values for LISTBROWSER_RelEvent */
#define LBRE_NORMAL         1
#define LBRE_HIDECHILDREN   2
#define LBRE_SHOWCHILDREN   4
#define LBRE_EDIT           8
#define LBRE_DOUBLECLICK    16
#define LBRE_CHECKED        32
#define LBRE_UNCHECKED      64
#define LBRE_TITLECLICK     128
#define LBRE_COLUMNADJUST   256

/*****************************************************************************/

/* Convenience macros */
#ifndef ListBrowserObject
#define ListBrowserObject   NewObject(NULL, LISTBROWSER_CLASSNAME
#endif
#ifndef ListBrowserEnd
#define ListBrowserEnd      TAG_END)
#endif

#endif /* GADGETS_LISTBROWSER_H */
