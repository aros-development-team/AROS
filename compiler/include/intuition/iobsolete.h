#ifndef INTUITION_IOBSOLETE_H
#define INTUITION_IOBSOLETE_H

/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Structure of intuition.library
    Lang: english
*/

#ifndef INTUITION_INTUITION_H
#include <intuition/intuition.h>
#endif

#ifndef INTUI_V36_NAMES_ONLY


/* Gadget Type names: */

#define GTYPEMASK       GTYP_GTYPEMASK
#define CUSTOMGADGET    GTYP_CUSTOMGADGET
#define STRGADGET       GTYP_STRGADGET
#define PROPGADGET      GTYP_PROPGADGET
#define GADGET0002      GTYP_GADGET0002
#define BOOLGADGET      GTYP_BOOLGADGET
#define CLOSE           GTYP_CLOSE
#define SDOWNBACK       GTYP_SDOWNBACK
#define WDOWNBACK       GTYP_WDOWNBACK
#define SUPFRONT        GTYP_SUPFRONT
#define WUPFRONT        GTYP_WUPFRONT
#define SDRAGGING       GTYP_SDRAGGING
#define WDRAGGING       GTYP_WDRAGGING
#define SIZING          GTYP_SIZING
#define REQGADGET       GTYP_REQGADGET
#define GZZGADGET       GTYP_GZZGADGET
#define SCRGADGET       GTYP_SCRGADGET
#define SYSGADGET       GTYP_SYSGADGET
#define GADGETTYPE      GTYP_GADGETTYPE


/* Gadget Flags names: */

#define LABELIMAGE      GFLG_LABELIMAGE
#define LABELSTRING     GFLG_LABELSTRING
#define LABELITEXT      GFLG_LABELITEXT
#define LABELMASK       GFLG_LABELMASK
#define GADGDISABLED    GFLG_DISABLED
#define SELECTED        GFLG_SELECTED
#define GRELHEIGHT      GFLG_RELHEIGHT
#define GRELWIDTH       GFLG_RELWIDTH
#define GRELRIGHT       GFLG_RELRIGHT
#define GRELBOTTOM      GFLG_RELBOTTOM
#define GADGIMAGE       GFLG_GADGIMAGE
#define GADGHNONE       GFLG_GADGHNONE
#define GADGHIMAGE      GFLG_GADGHIMAGE
#define GADGHBOX        GFLG_GADGHBOX
#define GADGHCOMP       GFLG_GADGHCOMP
#define GADGHIGHBITS    GFLG_GADGHIGHBITS

/* Gadget Activation flag names: */

#define ACTIVEGADGET    GACT_ACTIVEGADGET
#define STRINGEXTEND    GACT_STRINGEXTEND
#define ALTKEYMAP       GACT_ALTKEYMAP
#define LONGINT         GACT_LONGINT
#define STRINGRIGHT     GACT_STRINGRIGHT
#define STRINGCENTER    GACT_STRINGCENTER
#define STRINGLEFT      GACT_STRINGLEFT
#define BOOLEXTEND      GACT_BOOLEXTEND
#define TOGGLESELECT    GACT_TOGGLESELECT
#define BORDERSNIFF     GACT_BORDERSNIFF
#define BOTTOMBORDER    GACT_BOTTOMBORDER
#define TOPBORDER       GACT_TOPBORDER
#define LEFTBORDER      GACT_LEFTBORDER
#define RIGHTBORDER     GACT_RIGHTBORDER
#define FOLLOWMOUSE     GACT_FOLLOWMOUSE
#define ENDGADGET       GACT_ENDGADGET
#define GADGIMMEDIATE   GACT_IMMEDIATE
#define RELVERIFY       GACT_RELVERIFY


/* Window Flags names: */

#define HASZOOM         WFLG_HASZOOM
#define ZOOMED          WFLG_ZOOMED
#define VISITOR         WFLG_VISITOR
#define NW_EXTENDED     WFLG_NW_EXTENDED
#define WINDOWTICKED    WFLG_WINDOWTICKED
#define WBENCHWINDOW    WFLG_WBENCHWINDOW
#define WINDOWREFRESH   WFLG_WINDOWREFRESH
#define NOCAREREFRESH   WFLG_NOCAREREFRESH
#define RMBTRAP         WFLG_RMBTRAP
#define MENUSTATE       WFLG_MENUSTATE
#define INREQUEST       WFLG_INREQUEST
#define WINDOWACTIVE    WFLG_WINDOWACTIVE
#define ACTIVATE        WFLG_ACTIVATE
#define BORDERLESS      WFLG_BORDERLESS
#define GIMMEZEROZERO   WFLG_GIMMEZEROZERO
#define REPORTMOUSE     WFLG_REPORTMOUSE
#define BACKDROP        WFLG_BACKDROP
#define OTHER_REFRESH   WFLG_OTHER_REFRESH
#define SUPER_BITMAP    WFLG_SUPER_BITMAP
#define SIMPLE_REFRESH  WFLG_SIMPLE_REFRESH
#define SMART_REFRESH   WFLG_SMART_REFRESH
#define REFRESHBITS     WFLG_REFRESHBITS
#define SIZEBBOTTOM     WFLG_SIZEBBOTTOM
#define SIZEBRIGHT      WFLG_SIZEBRIGHT
#define WINDOWCLOSE     WFLG_CLOSEGADGET
#define WINDOWDEPTH     WFLG_DEPTHGADGET
#define WINDOWDRAG      WFLG_DRAGBAR
#define WINDOWSIZING    WFLG_SIZEGADGET


/* IDCMP class names: */

#define LONELYMESSAGE   IDCMP_LONELYMESSAGE
#define CHANGEWINDOW    IDCMP_CHANGEWINDOW
#define MENUHELP        IDCMP_MENUHELP
#define IDCMPUPDATE     IDCMP_IDCMPUPDATE
#define INTUITICKS      IDCMP_INTUITICKS
#define VANILLAKEY      IDCMP_VANILLAKEY
#define DELTAMOVE       IDCMP_DELTAMOVE
#define INACTIVEWINDOW  IDCMP_INACTIVEWINDOW
#define ACTIVEWINDOW    IDCMP_ACTIVEWINDOW
#define WBENCHMESSAGE   IDCMP_WBENCHMESSAGE
#define DISKREMOVED     IDCMP_DISKREMOVED
#define DISKINSERTED    IDCMP_DISKINSERTED
#define NEWPREFS        IDCMP_NEWPREFS
#define MENUVERIFY      IDCMP_MENUVERIFY
#define REQCLEAR        IDCMP_REQCLEAR
#define REQVERIFY       IDCMP_REQVERIFY
#define RAWKEY          IDCMP_RAWKEY
#define CLOSEWINDOW     IDCMP_CLOSEWINDOW
#define MENUPICK        IDCMP_MENUPICK
#define REQSET          IDCMP_REQSET
#define GADGETUP        IDCMP_GADGETUP
#define GADGETDOWN      IDCMP_GADGETDOWN
#define MOUSEMOVE       IDCMP_MOUSEMOVE
#define MOUSEBUTTONS    IDCMP_MOUSEBUTTONS
#define REFRESHWINDOW   IDCMP_REFRESHWINDOW
#define NEWSIZE         IDCMP_NEWSIZE
#define SIZEVERIFY      IDCMP_SIZEVERIFY


/* Some obsolete tag names for image attributes.
 * Use the stuff from imageclass.h instead.
 */


#define IA_HIGHLIGHTPEN         IA_HighlightPen
#define IA_SHADOWPEN            IA_ShadowPen
#define IA_EDGESONLY            IA_EdgesOnly
#define IA_DOUBLEEMBOSS         IA_DoubleEmboss
#define IA_RECESSED             IA_Recessed
#define IA_OUTLINE              IA_Outline
#define IA_FONT                 IA_Font
#define IA_MODE                 IA_Mode
#define IA_APATSIZE             IA_APatSize
#define IA_APATTERN             IA_APattern
#define IA_RESOLUTION           IA_Resolution
#define IA_PENS                 IA_Pens
#define IA_LINEWIDTH            IA_LineWidth
#define IA_DATA                 IA_Data
#define IA_BGPEN                IA_BGPen
#define IA_FGPEN                IA_FGPen
#define IA_HEIGHT               IA_Height
#define IA_WIDTH                IA_Width
#define IA_TOP                  IA_Top
#define IA_LEFT                 IA_Left
#define IMAGE_ATTRIBUTES        (IA_Dummy)


/* Some obsolete identifiers for the various DrawInfo pens.
 * Use the stuff in screens.h instead.
 */

#define detailPen       DETAILPEN
#define blockPen        BLOCKPEN
#define textPen         TEXTPEN
#define shinePen        SHINEPEN
#define shadowPen       SHADOWPEN
#define hifillPen       FILLPEN
#define hifilltextPen   FILLTEXTPEN
#define backgroundPen   BACKGROUNDPEN
#define hilighttextPen  HIGHLIGHTTEXTPEN
#define numDrIPens      NUMDRIPENS


/* Some obsolete tag names for general, proportional and string gadgets.
 * Use the stuff in gadgetclass.h instead.
 */

#define LAYOUTA_SPACING         LAYOUTA_Spacing
#define LAYOUTA_ORIENTATION     LAYOUTA_Orientation
#define LAYOUTA_LAYOUTOBJ       LAYOUTA_LayoutObj


#define PGA_FREEDOM             PGA_Freedom
#define PGA_BORDERLESS          PGA_Borderless
#define PGA_HORIZPOT            PGA_HorizPot
#define PGA_HORIZBODY           PGA_HorizBody
#define PGA_VERTPOT             PGA_VertPot
#define PGA_VERTBODY            PGA_VertBody
#define PGA_TOTAL               PGA_Total
#define PGA_VISIBLE             PGA_Visible
#define PGA_TOP                 PGA_Top


#define GA_LABELIMAGE           GA_LabelImage
#define GA_INTUITEXT            GA_IntuiText
#define GA_DRAWINFO             GA_DrawInfo
#define GA_NEXT                 GA_Next
#define GA_PREVIOUS             GA_Previous
#define GA_SYSGTYPE             GA_SysGType
#define GA_SYSGADGET            GA_SysGadget
#define GA_TOGGLESELECT         GA_ToggleSelect
#define GA_BOTTOMBORDER         GA_BottomBorder
#define GA_TOPBORDER            GA_TopBorder
#define GA_LEFTBORDER           GA_LeftBorder
#define GA_RIGHTBORDER          GA_RightBorder
#define GA_FOLLOWMOUSE          GA_FollowMouse
#define GA_RELVERIFY            GA_RelVerify
#define GA_IMMEDIATE            GA_Immediate
#define GA_ENDGADGET            GA_EndGadget
#define GA_SELECTED             GA_Selected
#define GA_SPECIALINFO          GA_SpecialInfo
#define GA_USERDATA             GA_UserData
#define GA_GZZGADGET            GA_GZZGadget
#define GA_DISABLED             GA_Disabled
#define GA_HIGHLIGHT            GA_Highlight
#define GA_SELECTRENDER         GA_SelectRender
#define GA_BORDER               GA_Border
#define GA_IMAGE                GA_Image
#define GA_TEXT                 GA_Text
#define GA_RELHEIGHT            GA_RelHeight
#define GA_HEIGHT               GA_Height
#define GA_RELWIDTH             GA_RelWidth
#define GA_WIDTH                GA_Width
#define GA_RELBOTTOM            GA_RelBottom
#define GA_TOP                  GA_Top
#define GA_RELRIGHT             GA_RelRight
#define GA_LEFT                 GA_Left


#endif /* !INTUI_V36_NAMES_ONLY */

#endif /* INTUITION_IOBSOLETE_H */
