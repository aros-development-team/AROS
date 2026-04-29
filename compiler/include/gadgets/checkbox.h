/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: Reaction - ClassAct/ReAction compatible gadgets/checkbox.h
*/

#ifndef GADGETS_CHECKBOX_H
#define GADGETS_CHECKBOX_H

#ifndef REACTION_REACTION_H
#include <reaction/reaction.h>
#endif
#ifndef INTUITION_GADGETCLASS_H
#include <intuition/gadgetclass.h>
#endif

#define CHECKBOX_CLASSNAME  "checkbox.gadget"
#define CHECKBOX_VERSION    44

/*****************************************************************************/

/* Additional attributes defined by the checkbox.gadget class */

#define CHECKBOX_Dummy          (REACTION_Dummy + 0x11000)

#define CHECKBOX_TextPen        (CHECKBOX_Dummy+1)  /* (WORD) Text pen, ~0 for TEXTPEN */
#define CHECKBOX_FillTextPen    (CHECKBOX_Dummy+2)  /* (WORD) Fill text pen, ~0 for FILLTEXTPEN */
#define CHECKBOX_BackgroundPen  (CHECKBOX_Dummy+3)  /* (WORD) Background pen, ~0 for BACKGROUNDPEN */
#define CHECKBOX_BevelStyle     (CHECKBOX_Dummy+4)  /* (WORD) Outer bevel style (obsolete) */
#define CHECKBOX_TextPlace      (CHECKBOX_Dummy+5)  /* (LONG) Label placement (PLACETEXT_LEFT/RIGHT) */

#define CHECKBOX_Checked        GA_Selected         /* (BOOL) Checkmark state */

/*****************************************************************************/

#ifndef CheckBoxObject
#define CheckBoxObject  NewObject(NULL, CHECKBOX_CLASSNAME
#endif
#ifndef CheckBoxEnd
#define CheckBoxEnd     TAG_END)
#endif

#endif /* GADGETS_CHECKBOX_H */
