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

#define BUTTON_CLASSNAME    "button.gadget"
#define BUTTON_VERSION      44

#define BUTTON_Dummy        (TAG_USER + 0x04000000)

#define BUTTON_Pushed           (BUTTON_Dummy + 0x0001) /* Pushed/selected state */
#define BUTTON_Child            (BUTTON_Dummy + 0x0002) /* Custom imagery object */
#define BUTTON_AutoButton       (BUTTON_Dummy + 0x0003) /* Glyph type (BAG_*) */
#define BUTTON_BevelStyle       (BUTTON_Dummy + 0x0004) /* Bevel style */
#define BUTTON_Justification    (BUTTON_Dummy + 0x0005) /* Text alignment */
#define BUTTON_SoftStyle        (BUTTON_Dummy + 0x0006) /* Text soft style */
#define BUTTON_TextPen          (BUTTON_Dummy + 0x0007) /* Text pen */
#define BUTTON_BackgroundPen    (BUTTON_Dummy + 0x0008) /* Background pen */
#define BUTTON_FillTextPen      (BUTTON_Dummy + 0x0009) /* Filled text pen */
#define BUTTON_FillPen          (BUTTON_Dummy + 0x000A) /* Fill pen */
#define BUTTON_Transparent      (BUTTON_Dummy + 0x000B) /* No background fill */
#define BUTTON_VarArgs          (BUTTON_Dummy + 0x000C) /* Printf-style format args */
#define BUTTON_Integer          (BUTTON_Dummy + 0x000D) /* Integer for format string */
#define BUTTON_SelChild         (BUTTON_Dummy + 0x000E) /* Image when selected */
#define BUTTON_RenderMode       (BUTTON_Dummy + 0x000F) /* Normal, toggle, or sticky */
#define BUTTON_DomainString     (BUTTON_Dummy + 0x0010) /* Domain string for sizing */

/* Glyph types for BUTTON_AutoButton */
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

#define BCJ_LEFT        0
#define BCJ_CENTER      1
#define BCJ_RIGHT       2

#ifndef ButtonObject
#define ButtonObject    NewObject(NULL, BUTTON_CLASSNAME
#endif
#ifndef ButtonEnd
#define ButtonEnd       TAG_END)
#endif
#define StartButton     ButtonObject

#endif /* GADGETS_BUTTON_H */
