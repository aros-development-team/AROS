#ifndef LIBRARIES_GADTOOLS_H
#define LIBRARIES_GADTOOLS_H

/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Definitions and structures for gadtools.library.
    Lang: english
*/

#ifndef EXEC_TYPES_H
#   include <exec/types.h>
#endif
#ifndef INTUITION_GADGETCLASS_H
#   include <intuition/gadgetclass.h>
#endif
#ifndef INTUITION_INTUITION_H
#   include <intuition/intuition.h>
#endif
#ifndef UTILITY_TAGITEM_H
#   include <utility/tagitem.h>
#endif


/* Name of the gadtools.library as constant. This is an AROS extension. */
#define GADTOOLSNAME "gadtools.library"

                               /* Gadgets */

/* Kinds for CreateGadgetA() */
#define GENERIC_KIND  0
/* normal button */
#define BUTTON_KIND   1
/* boolean gadget */
#define CHECKBOX_KIND 2
/* to enter numbers */
#define INTEGER_KIND  3
/* to list a bunch of entries */
#define LISTVIEW_KIND 4
/* mutually exclusive entry gadget */
#define MX_KIND       5
/* to show numbers */
#define NUMBER_KIND   6
/* like MX_KIND, but rendered differently */
#define CYCLE_KIND    7
/* to choose a color */
#define PALETTE_KIND  8
/* to select a value of a range of values */
#define SCROLLER_KIND 9
/* like SCROLLER_KIND, but with a fixed range */
#define SLIDER_KIND   11
/* to enter texts */
#define STRING_KIND   12
/* to show texts */
#define TEXT_KIND     13
#define NUM_KINDS     14

struct NewGadget
{
    WORD ng_LeftEdge;
    WORD ng_TopEdge;
    WORD ng_Width;
    WORD ng_Height;

    CONST_STRPTR      ng_GadgetText;
    struct TextAttr * ng_TextAttr;

    UWORD ng_GadgetID;
    ULONG ng_Flags;      /* see below */
    APTR  ng_VisualInfo;
    APTR  ng_UserData;
};

/* ng_Flags
   The PLACETEXT flags (specified in <intuition/gadgetclass.h>) specify where
   to put the label(s) of the gadget
*/
#define PLACETEXT_LEFT  (1L<<0)
#define PLACETEXT_RIGHT (1L<<1)
#define PLACETEXT_ABOVE (1L<<2)
#define PLACETEXT_BELOW (1L<<3)
#define PLACETEXT_IN    (1L<<4)
#define NG_HIGHLABEL    (1L<<5)

/* IDCMP-Flags necessary for certain gadgets */
#define ARROWIDCMP    (IDCMP_GADGETUP | \
                       IDCMP_GADGETDOWN | \
                       IDCMP_MOUSEBUTTONS | \
                       IDCMP_INTUITICKS)
#define BUTTONIDCMP   (IDCMP_GADGETUP)
#define CHECKBOXIDCMP (IDCMP_GADGETUP)
#define INTEGERIDCMP  (IDCMP_GADGETUP)
#define LISTVIEWIDCMP (ARROWIDCMP | \
                       IDCMP_GADGETUP | \
                       IDCMP_GADGETDOWN | \
                       IDCMP_MOUSEMOVE)
#define MXIDCMP       (IDCMP_GADGETDOWN)
#define NUMBERIDCMP   (0L)
#define CYCLEIDCMP    (IDCMP_GADGETUP)
#define PALETTEIDCMP  (IDCMP_GADGETUP)
#define SCROLLERIDCMP (IDCMP_GADGETUP | \
                       IDCMP_GADGETDOWN | \
                       IDCMP_MOUSEMOVE)
#define SLIDERIDCMP   (IDCMP_GADGETUP | \
                       IDCMP_GADGETDOWN | \
                       IDCMP_MOUSEMOVE)
#define STRINGIDCMP   (IDCMP_GADGETUP)
#define TEXTIDCMP     (0L)

#define MX_WIDTH  17
#define MX_HEIGHT 9
#define CHECKBOX_WIDTH  26
#define CHECKBOX_HEIGHT 11

/* Indicate that gadget is an gadtools-gadget (PRIVATE) */
#define GTYP_GADTOOLS 0x0100

                               /* Menus */

struct NewMenu
{
    UBYTE        nm_Type;          /* see below */
//    BYTE         nm_Pad;
    CONST_STRPTR nm_Label;         /* may be a STRPTR or NM_BARLABEL (see below) */
    CONST_STRPTR nm_CommKey;
    UWORD        nm_Flags;         /* see below */
    LONG         nm_MutualExclude;
    APTR         nm_UserData;
};

/* nm_Type */
#define NM_END    0x0000
#define NM_TITLE  0x0001
#define NM_ITEM   0x0002
#define NM_SUB    0x0003
#define NM_IGNORE 0x0040
#define IM_ITEM   0x0082
#define IM_SUB    0x0083

/* nm_Label */
#define NM_BARLABEL ((STRPTR)-1)

/* nm_Flags */
#define NM_MENUDISABLED  MENUENABLED
#define NM_ITEMDISABLED  ITEMENABLED
#define NM_COMMANDSTRING COMMSEQ

#define NM_FLAGMASK     (~(ITEMTEXT | HIGHFLAGS | COMMSEQ))
#define NM_FLAGMASK_V39 (~(ITEMTEXT | HIGHFLAGS))

/* Macros */
#define GTMENU_USERDATA(menu)     (*((APTR *)(((struct Menu     *)menu)+1)))
#define GTMENUITEM_USERDATA(item) (*((APTR *)(((struct MenuItem *)item)+1)))

#define GTMENU_TRIMMED 0x00000001
#define GTMENU_INVALID 0x00000002
#define GTMENU_NOMEM   0x00000003

                               /* Tags */

#define GT_TagBase TAG_USER+0x00080000
#define GTCB_Checked         (GT_TagBase + 4)
#define GTLV_Top             (GT_TagBase + 5)
#define GTLV_Labels          (GT_TagBase + 6)
#define GTLV_ReadOnly        (GT_TagBase + 7)
#define GTLV_ScrollWidth     (GT_TagBase + 8)
#define GTMX_Labels          (GT_TagBase + 9)
#define GTMX_Active          (GT_TagBase + 10)
#define GTTX_Text            (GT_TagBase + 11)
#define GTTX_CopyText        (GT_TagBase + 12)
#define GTNM_Number          (GT_TagBase + 13)
#define GTCY_Labels          (GT_TagBase + 14)
#define GTCY_Active          (GT_TagBase + 15)
#define GTPA_Depth           (GT_TagBase + 16)
#define GTPA_Color           (GT_TagBase + 17)
#define GTPA_ColorOffset     (GT_TagBase + 18)
#define GTPA_IndicatorWidth  (GT_TagBase + 19)
#define GTPA_IndicatorHeight (GT_TagBase + 20)
#define GTSC_Top             (GT_TagBase + 21)
#define GTSC_Total           (GT_TagBase + 22)
#define GTSC_Visible         (GT_TagBase + 23)
#define GTSC_Overlap         (GT_TagBase + 24)
#define GTSL_Min             (GT_TagBase + 38)
#define GTSL_Max             (GT_TagBase + 39)
#define GTSL_Level           (GT_TagBase + 40)
#define GTSL_MaxLevelLen     (GT_TagBase + 41)
#define GTSL_LevelFormat     (GT_TagBase + 42)
#define GTSL_LevelPlace      (GT_TagBase + 43)
#define GTSL_DispFunc        (GT_TagBase + 44)
#define GTST_String          (GT_TagBase + 45)
#define GTST_MaxChars        (GT_TagBase + 46)
#define GTIN_Number          (GT_TagBase + 47)
#define GTIN_MaxChars        (GT_TagBase + 48)
#define GTMN_TextAttr        (GT_TagBase + 49)
#define GTMN_FrontPen        (GT_TagBase + 50)
#define GTBB_Recessed        (GT_TagBase + 51)
#define GT_VisualInfo        (GT_TagBase + 52)
#define GTLV_ShowSelected    (GT_TagBase + 53)
#define GTLV_Selected        (GT_TagBase + 54)
#define GTST_EditHook        (GT_TagBase + 55)
#define GTIN_EditHook        (GT_TagBase + 55)
#define GTTX_Border          (GT_TagBase + 57)
#define GTNM_Border          (GT_TagBase + 58)
#define GTSC_Arrows          (GT_TagBase + 59)
#define GTMN_Menu            (GT_TagBase + 60)
#define GTMX_Spacing         (GT_TagBase + 61)
#define GTMN_FullMenu        (GT_TagBase + 62)
#define GTMN_SecondaryError  (GT_TagBase + 63)
#define GT_Underscore        (GT_TagBase + 64)
#define GTMN_Checkmark       (GT_TagBase + 65)
#define GTMN_AmigaKey        (GT_TagBase + 66)
#define GTMN_NewLookMenus    (GT_TagBase + 67)
#define GTCB_Scaled          (GT_TagBase + 68)
#define GTMX_Scaled          (GT_TagBase + 69)
#define GTPA_NumColors       (GT_TagBase + 70)
#define GTMX_TitlePlace      (GT_TagBase + 71)
#define GTTX_FrontPen        (GT_TagBase + 72)
#define GTNM_FrontPen        (GT_TagBase + 72)
#define GTTX_BackPen         (GT_TagBase + 73)
#define GTNM_BackPen         (GT_TagBase + 73)
#define GTTX_Justification   (GT_TagBase + 74)
#define GTNM_Justification   (GT_TagBase + 74)
#define GTNM_Format          (GT_TagBase + 75)
#define GTNM_MaxNumberLen    (GT_TagBase + 76)
#define GTBB_FrameType       (GT_TagBase + 77)
#define GTLV_MakeVisible     (GT_TagBase + 78)
#define GTLV_ItemHeight      (GT_TagBase + 79)
#define GTSL_MaxPixelLen     (GT_TagBase + 80)
#define GTSL_Justification   (GT_TagBase + 81)
#define GTPA_ColorTable      (GT_TagBase + 82)
#define GTLV_CallBack        (GT_TagBase + 83)
#define GTLV_MaxPen          (GT_TagBase + 84)
#define GTTX_Clipped         (GT_TagBase + 85)
#define GTNM_Clipped         (GT_TagBase + 85)

/* GTTX_Justification and GTNM_Justification */
#define GTJ_LEFT   0
#define GTJ_RIGHT  1
#define GTJ_CENTER 2

/* GTBB_FrameType */
#define BBFT_BUTTON      1
#define BBFT_RIDGE       2
#define BBFT_ICONDROPBOX 3

/* GTLV_CallBack */
#define LV_DRAW 0x202
/* return values from these hooks */
#define LVCB_OK      0
#define LVCB_UNKNOWN 1

#define INTERWIDTH  8
#define INTERHEIGHT 4

struct LVDrawMsg
{
    ULONG              lvdm_MethodID; /* LV_DRAW */
    struct RastPort  * lvdm_RastPort;
    struct DrawInfo  * lvdm_DrawInfo;
    struct Rectangle   lvdm_Bounds;
    ULONG              lvdm_State;
};

/* lvdm_State */
#define LVR_NORMAL           0
#define LVR_SELECTED         1
#define LVR_NORMALDISABLED   2
#define LVR_SELECTEDDISABLED 8

#endif /* LIBRARIES_GADTOOLS_H */
