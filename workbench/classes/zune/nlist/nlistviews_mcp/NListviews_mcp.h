#ifndef MUI_NLISTVIEWS_MCP_H
#define MUI_NLISTVIEWS_MCP_H

/***************************************************************************

 NListviews.mcp - New Listview MUI Custom Class Preferences
 Registered MUI class, Serial Number: 1d51 (0x9d510001 to 0x9d51001F
                                            and 0x9d510101 to 0x9d51013F)

 Copyright (C) 1996-2001 by Gilles Masson
 Copyright (C) 2001-2014 NList Open Source Team

 This library is free software; you can redistribute it and/or
 modify it under the terms of the GNU Lesser General Public
 License as published by the Free Software Foundation; either
 version 2.1 of the License, or (at your option) any later version.

 This library is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 Lesser General Public License for more details.

 NList classes Support Site:  http://www.sf.net/projects/nlist-classes

 $Id$

***************************************************************************/

#ifndef LIBRARIES_MUI_H
#include <libraries/mui.h>
#endif

#ifndef MUI_NListview_MCC_H
#include <mui/NListview_mcc.h>
#endif

#include <devices/inputevent.h>

#if !defined(__AROS__) && defined(__PPC__)
  #if defined(__GNUC__)
    #pragma pack(2)
  #elif defined(__VBCC__)
    #pragma amiga-align
  #endif
#endif

#define MUIC_NListviews_mcp "NListviews.mcp"
#define NListviewsMcpObject MUI_NewObject(MUIC_NListviews_mcp

#define MUICFG_NList_Pen_Title        0x9d510001UL
#define MUICFG_NList_Pen_List         0x9d510002UL
#define MUICFG_NList_Pen_Select       0x9d510003UL
#define MUICFG_NList_Pen_Cursor       0x9d510004UL
#define MUICFG_NList_Pen_UnselCur     0x9d510005UL
#define MUICFG_NList_Pen_Inactive     0x9d510104UL

#define MUICFG_NList_BG_Title         0x9d510006UL
#define MUICFG_NList_BG_List          0x9d510007UL
#define MUICFG_NList_BG_Select        0x9d510008UL
#define MUICFG_NList_BG_Cursor        0x9d510009UL
#define MUICFG_NList_BG_UnselCur      0x9d51000aUL
#define MUICFG_NList_BG_Inactive      0x9d510105UL

#define MUICFG_NList_Font             0x9d51000bUL
#define MUICFG_NList_Font_Little      0x9d51000cUL
#define MUICFG_NList_Font_Fixed       0x9d51000dUL

#define MUICFG_NList_VertInc          0x9d51000eUL
#define MUICFG_NList_DragType         0x9d51000fUL
#define MUICFG_NList_MultiSelect      0x9d510010UL

#define MUICFG_NListview_VSB          0x9d510011UL
#define MUICFG_NListview_HSB          0x9d510012UL

#define MUICFG_NList_DragQualifier    0x9d510013UL /* OBSOLETE */
#define MUICFG_NList_Smooth           0x9d510014UL
#define MUICFG_NList_ForcePen         0x9d510015UL
#define MUICFG_NList_StackCheck       0x9d510016UL /* OBSOLETE */
#define MUICFG_NList_ColWidthDrag     0x9d510017UL
#define MUICFG_NList_PartialCol       0x9d510018UL
#define MUICFG_NList_List_Select      0x9d510019UL
#define MUICFG_NList_Menu             0x9d51001AUL
#define MUICFG_NList_PartialChar      0x9d51001BUL
#define MUICFG_NList_PointerColor     0x9d51001CUL /* OBSOLETE */
#define MUICFG_NList_SerMouseFix      0x9d51001DUL
#define MUICFG_NList_Keys             0x9d51001EUL
#define MUICFG_NList_DragLines        0x9d51001FUL
#define MUICFG_NList_VCenteredLines   0x9d510020UL
#define MUICFG_NList_SelectPointer    0x9d510106UL

#define MUICFG_NList_WheelStep        0x9d510101UL
#define MUICFG_NList_WheelFast        0x9d510102UL
#define MUICFG_NList_WheelMMB         0x9d510103UL

#define MUIV_NList_MultiSelect_MMB_On     0x0300
#define MUIV_NList_MultiSelect_MMB_Off    0x0100

#define MUIV_NList_ColWidthDrag_One       0
#define MUIV_NList_ColWidthDrag_All       1
#define MUIV_NList_ColWidthDrag_Visual    2


// default values for the above MUICFG values
#define DEFAULT_PEN_TITLE       "m5"
#define DEFAULT_PEN_LIST        "m5"
#define DEFAULT_PEN_SELECT      "m5"
#define DEFAULT_PEN_CURSOR      "m5"
#define DEFAULT_PEN_UNSELCUR    "m5"
#define DEFAULT_PEN_INACTIVE    "m5"
#define DEFAULT_BG_TITLE        "0:140"
#define DEFAULT_BG_LIST         "2:m2"
#define DEFAULT_BG_SELECT       "0:135"
#define DEFAULT_BG_CURSOR       "0:131"
#define DEFAULT_BG_UNSELCUR     "2:m3"
#define DEFAULT_BG_INACTIVE     "2:m3"
#define DEFAULT_VERT_INC        0
#define DEFAULT_HSB             MUIV_NListview_HSB_Auto
#define DEFAULT_VSB             MUIV_NListview_VSB_Auto
#define DEFAULT_CWD             MUIV_NList_ColWidthDrag_All
#define DEFAULT_CMENU           1
#define DEFAULT_MULTISELECT     0
#define DEFAULT_DRAGTYPE        0
#define DEFAULT_DRAGLINES       10
#define DEFAULT_WHEELSTEP       3
#define DEFAULT_WHEELFAST       5
#define DEFAULT_WHEELMMB        FALSE
#define DEFAULT_VCENTERED       FALSE
#define DEFAULT_SMOOTHSCROLL    TRUE
#define DEFAULT_SELECTPOINTER   TRUE
#define DEFAULT_PARTIALCHAR     FALSE
#define DEFAULT_PARTIALCOL      TRUE
#define DEFAULT_SERMOUSEFIX     FALSE
#define DEFAULT_LIST_SELECT     TRUE
#define DEFAULT_FORCEPEN        FALSE

#define KEYTAG_QUALIFIER_MULTISELECT       0x9d51C001UL
#define KEYTAG_QUALIFIER_DRAG              0x9d51C002UL
#define KEYTAG_QUALIFIER_BALANCE           0x9d51C003UL
#define KEYTAG_COPY_TO_CLIPBOARD           0x9d518001UL
#define KEYTAG_DEFAULT_WIDTH_COLUMN        0x9d518002UL
#define KEYTAG_DEFAULT_WIDTH_ALL_COLUMNS   0x9d518003UL
#define KEYTAG_DEFAULT_ORDER_COLUMN        0x9d518004UL
#define KEYTAG_DEFAULT_ORDER_ALL_COLUMNS   0x9d518005UL
#define KEYTAG_SELECT_TO_TOP               0x9d518006UL
#define KEYTAG_SELECT_TO_BOTTOM            0x9d518007UL
#define KEYTAG_SELECT_TO_PAGE_UP           0x9d518008UL
#define KEYTAG_SELECT_TO_PAGE_DOWN         0x9d518009UL
#define KEYTAG_SELECT_UP                   0x9d51800AUL
#define KEYTAG_SELECT_DOWN                 0x9d51800BUL
#define KEYTAG_TOGGLE_ACTIVE               0x9d51800CUL
#define KEYTAG_QUALIFIER_WHEEL_FAST        0x9d51800DUL
#define KEYTAG_QUALIFIER_WHEEL_HORIZ       0x9d51800EUL
#define KEYTAG_QUALIFIER_TITLECLICK2       0x9d51800FUL

struct KeyBinding {
  ULONG kb_KeyTag;
  UWORD kb_Code;
  UWORD kb_Qualifier;
};

#define KBQUAL_MASK   0x01FF    /* only qualifier keys bits are used in kb_Qualifier */
#define KBSYM_MASK    0x7000    /* upper kb_Qualifier bits are use for synonyms */

#define KBSYM_SHIFT   0x1000    /* left- and right- shift are equivalent     */
#define KBSYM_CAPS    0x2000    /* either shift or caps lock are equivalent  */
#define KBSYM_ALT     0x4000    /* left- and right- alt are equivalent       */

#define KBQUALIFIER_SHIFT  (KBSYM_SHIFT | IEQUALIFIER_LSHIFT | IEQUALIFIER_RSHIFT)  /* 'shift' */
#define KBQUALIFIER_CAPS   (KBSYM_CAPS | KBQUALIFIER_SHIFT | IEQUALIFIER_CAPSLOCK)  /* 'caps' */
#define KBQUALIFIER_ALT    (KBSYM_ALT | IEQUALIFIER_LALT | IEQUALIFIER_RALT)        /* 'alt' */

#define DEFAULT_KEYS_ARRAY                                            \
  struct KeyBinding default_keys[] =                                  \
  {                                                                   \
    { KEYTAG_QUALIFIER_MULTISELECT    , (UWORD)~0  , KBQUALIFIER_SHIFT    }, \
    { KEYTAG_QUALIFIER_DRAG           , (UWORD)~0  , IEQUALIFIER_CONTROL  }, \
    { KEYTAG_QUALIFIER_DRAG           , (UWORD)~0  , IEQUALIFIER_CONTROL|KBQUALIFIER_SHIFT  }, \
    { KEYTAG_QUALIFIER_BALANCE        , (UWORD)~0  , KBQUALIFIER_SHIFT    }, \
    { KEYTAG_COPY_TO_CLIPBOARD        , 0x33, IEQUALIFIER_RCOMMAND }, \
    { KEYTAG_COPY_TO_CLIPBOARD        , 0x33, IEQUALIFIER_LCOMMAND }, \
    { KEYTAG_COPY_TO_CLIPBOARD        , 0x32, IEQUALIFIER_RCOMMAND }, \
    { KEYTAG_COPY_TO_CLIPBOARD        , 0x32, IEQUALIFIER_LCOMMAND }, \
    { KEYTAG_DEFAULT_WIDTH_COLUMN     , 0x1D, IEQUALIFIER_RCOMMAND|IEQUALIFIER_NUMERICPAD }, \
    { KEYTAG_DEFAULT_WIDTH_ALL_COLUMNS, 0x1E, IEQUALIFIER_RCOMMAND|IEQUALIFIER_NUMERICPAD }, \
    { KEYTAG_DEFAULT_ORDER_COLUMN     , 0x1F, IEQUALIFIER_RCOMMAND|IEQUALIFIER_NUMERICPAD }, \
    { KEYTAG_DEFAULT_ORDER_ALL_COLUMNS, 0x2D, IEQUALIFIER_RCOMMAND|IEQUALIFIER_NUMERICPAD }, \
    { KEYTAG_SELECT_TO_TOP            , 0x4C, KBQUALIFIER_ALT|IEQUALIFIER_CONTROL },   \
    { KEYTAG_SELECT_TO_BOTTOM         , 0x4D, KBQUALIFIER_ALT|IEQUALIFIER_CONTROL },   \
    { KEYTAG_SELECT_TO_PAGE_UP        , 0x4C, KBQUALIFIER_ALT|KBQUALIFIER_SHIFT }, \
    { KEYTAG_SELECT_TO_PAGE_DOWN      , 0x4D, KBQUALIFIER_ALT|KBQUALIFIER_SHIFT }, \
    { KEYTAG_SELECT_UP                , 0x4C, KBQUALIFIER_ALT }, \
    { KEYTAG_SELECT_DOWN              , 0x4D, KBQUALIFIER_ALT }, \
    { KEYTAG_TOGGLE_ACTIVE            , 0x40, KBQUALIFIER_ALT }, \
    { KEYTAG_QUALIFIER_WHEEL_FAST     , (UWORD)~0  , KBQUALIFIER_SHIFT }, \
    { KEYTAG_QUALIFIER_WHEEL_HORIZ    , (UWORD)~0  , KBQUALIFIER_ALT }, \
    { KEYTAG_QUALIFIER_TITLECLICK2    , (UWORD)~0  , KBQUALIFIER_SHIFT }, \
    { 0L, (UWORD)~0, 0 } \
  };

/*
 *  #define IEQUALIFIER_LSHIFT        0x0001   'lshift'
 *  #define IEQUALIFIER_RSHIFT        0x0002   'rshift'
 *  #define IEQUALIFIER_CAPSLOCK      0x0004   'capslock'
 *  #define IEQUALIFIER_CONTROL       0x0008   'control'
 *  #define IEQUALIFIER_LALT          0x0010   'lalt'
 *  #define IEQUALIFIER_RALT          0x0020   'ralt'
 *  #define IEQUALIFIER_LCOMMAND      0x0040   'lcommand'
 *  #define IEQUALIFIER_RCOMMAND      0x0080   'rcommand'
 *  #define IEQUALIFIER_NUMERICPAD    0x0100   'numpad'
 */

#if !defined(__AROS__) && defined(__PPC__)
  #if defined(__GNUC__)
    #pragma pack()
  #elif defined(__VBCC__)
    #pragma default-align
  #endif
#endif

#endif /* MUI_NLISTVIEWS_MCP_H */
