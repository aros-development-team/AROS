/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: Reaction - ClassAct/ReAction compatible gadgets/checkbox.h
*/

#ifndef GADGETS_CHECKBOX_H
#define GADGETS_CHECKBOX_H

#ifndef EXEC_TYPES_H
#include <exec/types.h>
#endif
#ifndef UTILITY_TAGITEM_H
#include <utility/tagitem.h>
#endif

/*
 * checkbox.gadget - ClassAct/ReAction compatible checkbox gadget
 *
 * Superclass: gadgetclass
 * Include:    <gadgets/checkbox.h>
 */

#define CHECKBOX_CLASSNAME  "gadgets/checkbox.gadget"
#define CHECKBOX_VERSION    44

/* Tag base */
#define CHECKBOX_Dummy      (TAG_USER + 0x60000)

/* Attributes */
/* (ISG) Checked state */
#define CHECKBOX_Checked        (CHECKBOX_Dummy + 0x0001)
/* (I..) Text pen */
#define CHECKBOX_TextPen        (CHECKBOX_Dummy + 0x0002)
/* (I..) Background pen */
#define CHECKBOX_BackgroundPen  (CHECKBOX_Dummy + 0x0003)
/* (I..) Fill pen */
#define CHECKBOX_FillPen        (CHECKBOX_Dummy + 0x0004)
/* (I..) Text placement */
#define CHECKBOX_TextPlace      (CHECKBOX_Dummy + 0x0005)

/* Object creation macros */
#define CheckBoxObject  NewObject(NULL, CHECKBOX_CLASSNAME
#define CheckBoxEnd     TAG_END)

#endif /* GADGETS_CHECKBOX_H */
