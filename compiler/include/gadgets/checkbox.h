/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: Reaction - ClassAct/ReAction compatible gadgets/checkbox.h
*/

#ifndef GADGETS_CHECKBOX_H
#define GADGETS_CHECKBOX_H

#ifndef EXEC_TYPES_H
#include <exec/types.h>
#endif
#ifndef REACTION_REACTION_H
#include <reaction/reaction.h>
#endif

#define CHECKBOX_CLASSNAME  "checkbox.gadget"
#define CHECKBOX_VERSION    44

#define CHECKBOX_Dummy      (REACTION_Dummy + 0x11000)

#define CHECKBOX_Checked        (CHECKBOX_Dummy + 0x0001) /* Current checked state */
#define CHECKBOX_TextPen        (CHECKBOX_Dummy + 0x0002) /* Text pen */
#define CHECKBOX_BackgroundPen  (CHECKBOX_Dummy + 0x0003) /* Background pen */
#define CHECKBOX_FillPen        (CHECKBOX_Dummy + 0x0004) /* Fill pen */
#define CHECKBOX_TextPlace      (CHECKBOX_Dummy + 0x0005) /* Label placement */

#ifndef CheckBoxObject
#define CheckBoxObject  NewObject(NULL, CHECKBOX_CLASSNAME
#endif
#ifndef CheckBoxEnd
#define CheckBoxEnd     TAG_END)
#endif

#endif /* GADGETS_CHECKBOX_H */
