/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: Reaction - ClassAct/ReAction compatible gadgets/button.h
*/

#ifndef GADGETS_BUTTON_H
#define GADGETS_BUTTON_H

#ifndef EXEC_TYPES_H
#include <exec/types.h>
#endif
#ifndef UTILITY_TAGITEM_H
#include <utility/tagitem.h>
#endif

/*
 * button.gadget - ClassAct/ReAction compatible button gadget
 *
 * Superclass: gadgetclass (FRBUTTONCLASS)
 * Include:    <gadgets/button.h>
 */

#define BUTTON_CLASSNAME    "gadgets/button.gadget"
#define BUTTON_VERSION      44

/* Tag base */
#define BUTTON_Dummy        (TAG_USER + 0x30000)

/* Attributes */
/* (ISG) Render as pushed/selected */
#define BUTTON_Pushed           (BUTTON_Dummy + 0x0001)
/* (I..) Child object for custom imagery */
#define BUTTON_Child            (BUTTON_Dummy + 0x0002)
/* (I..) Auto-button type (see BGLYPH_*) */
#define BUTTON_AutoButton       (BUTTON_Dummy + 0x0003)
/* (I..) Bevel style */
#define BUTTON_BevelStyle       (BUTTON_Dummy + 0x0004)
/* (I..) Text justification */
#define BUTTON_Justification    (BUTTON_Dummy + 0x0005)
/* (I..) Soft style for text */
#define BUTTON_SoftStyle        (BUTTON_Dummy + 0x0006)
/* (I..) Text pen */
#define BUTTON_TextPen          (BUTTON_Dummy + 0x0007)
/* (I..) Background pen */
#define BUTTON_BackgroundPen    (BUTTON_Dummy + 0x0008)
/* (I..) Fill text pen */
#define BUTTON_FillTextPen      (BUTTON_Dummy + 0x0009)
/* (I..) Fill pen */
#define BUTTON_FillPen          (BUTTON_Dummy + 0x000A)
/* (I..) Transparent background */
#define BUTTON_Transparent      (BUTTON_Dummy + 0x000B)
/* (I..) Varargs format support */
#define BUTTON_VarArgs          (BUTTON_Dummy + 0x000C)
/* (I..) Integer arg for VarArgs */
#define BUTTON_Integer          (BUTTON_Dummy + 0x000D)
/* (I..) Selected child image */
#define BUTTON_SelChild         (BUTTON_Dummy + 0x000E)
/* (I..) Render mode (normal, toggle, sticky) */
#define BUTTON_RenderMode       (BUTTON_Dummy + 0x000F)
/* (I..) Domino mode */
#define BUTTON_DomainString     (BUTTON_Dummy + 0x0010)

/* Auto-button glyph types for BUTTON_AutoButton */
#define BAG_POPFILE     0
#define BAG_POPFONT     1
#define BAG_POPUP       2
#define BAG_POPDRAWER   3
#define BAG_POPSCREEN   4
#define BAG_POPTIME     5
#define BAG_ARROWLEFT   6
#define BAG_ARROWRIGHT  7
#define BAG_ARROWUP     8
#define BAG_ARROWDOWN   9

/* Text justification */
#define BCJ_LEFT        0
#define BCJ_CENTER      1
#define BCJ_RIGHT       2

/* Object creation macros */
#define ButtonObject    NewObject(NULL, BUTTON_CLASSNAME
#define ButtonEnd       TAG_END)
#define StartButton     ButtonObject

#endif /* GADGETS_BUTTON_H */
