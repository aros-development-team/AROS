/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: Reaction - ClassAct/ReAction compatible gadgets/button.h
*/

#ifndef GADGETS_BUTTON_H
#define GADGETS_BUTTON_H

#ifndef REACTION_REACTION_H
#include <reaction/reaction.h>
#endif
#ifndef INTUITION_GADGETCLASS_H
#include <intuition/gadgetclass.h>
#endif
#ifndef IMAGES_BEVEL_H
#include <images/bevel.h>
#endif

#define BUTTON_CLASSNAME    "button.gadget"
#define BUTTON_VERSION      44

/*****************************************************************************/

/* Additional attributes defined by the button.gadget class */

#define BUTTON_Dummy            (TAG_USER + 0x04000000)

#define BUTTON_PushButton       (BUTTON_Dummy+1)    /* (BOOL) Toggle/push mode */
#define BUTTON_Glyph            (BUTTON_Dummy+2)    /* (struct Image *) Single-plane glyph */
#define BUTTON_TextPen          (BUTTON_Dummy+5)    /* (LONG) Text pen, -1 for TEXTPEN */
#define BUTTON_FillPen          (BUTTON_Dummy+6)    /* (LONG) Fill pen, -1 for FILLPEN */
#define BUTTON_FillTextPen      (BUTTON_Dummy+7)    /* (LONG) Fill text pen, -1 for FILLTEXTPEN */
#define BUTTON_BackgroundPen    (BUTTON_Dummy+8)    /* (LONG) Background pen, -1 for BACKGROUNDPEN */

#define BUTTON_RenderImage      GA_Image            /* Normal image */
#define BUTTON_SelectImage      GA_SelectRender     /* Selected state image */

#define BUTTON_BevelStyle       (BUTTON_Dummy+13)   /* Bevel box style */
#define BUTTON_Transparent      (BUTTON_Dummy+15)   /* (BOOL) Transparent background */
#define BUTTON_Justification    (BUTTON_Dummy+16)   /* Text justification (BCJ_*) */
#define BUTTON_SoftStyle        (BUTTON_Dummy+17)   /* Font soft style flags */
#define BUTTON_AutoButton       (BUTTON_Dummy+18)   /* Built-in glyph type (BAG_*) */
#define BUTTON_VarArgs          (BUTTON_Dummy+19)   /* Argument array for GA_Text varargs */
#define BUTTON_DomainString     (BUTTON_Dummy+20)   /* (STRPTR) String for domain calculation */
#define BUTTON_Integer          (BUTTON_Dummy+21)   /* (int) Numeric value to display */
#define BUTTON_BitMap           (BUTTON_Dummy+22)   /* (struct BitMap *) BitMap to render */

#define BUTTON_AnimButton       (BUTTON_Dummy+50)   /* (BOOL) Enable animation mode */
#define BUTTON_AnimImages       (BUTTON_Dummy+51)   /* (struct Image *) Animation image array */
#define BUTTON_SelAnimImages    (BUTTON_Dummy+52)   /* (struct Image *) Selected animation array */
#define BUTTON_MaxAnimImages    (BUTTON_Dummy+53)   /* (LONG) Number of animation frames */
#define BUTTON_AnimImageNumber  (BUTTON_Dummy+54)   /* (LONG) Current frame index */
#define BUTTON_AddAnimImageNumber (BUTTON_Dummy+55) /* (ULONG) Increment frame counter */
#define BUTTON_SubAnimImageNumber (BUTTON_Dummy+56) /* (ULONG) Decrement frame counter */

/*****************************************************************************/

/* Justification modes for BUTTON_Justification */
#define BCJ_LEFT        0
#define BCJ_CENTER      1
#define BCJ_RIGHT       2
#define BCJ_CENTRE      BCJ_CENTER

/* Built-in glyph types for BUTTON_AutoButton */
#define BAG_POPFILE     1   /* Popup file requester */
#define BAG_POPDRAWER   2   /* Popup drawer requester */
#define BAG_POPFONT     3   /* Popup font requester */
#define BAG_CHECKBOX    4   /* Check glyph */
#define BAG_CANCELBOX   5   /* Cancel glyph */
#define BAG_UPARROW     6   /* Up arrow */
#define BAG_DNARROW     7   /* Down arrow */
#define BAG_RTARROW     8   /* Right arrow */
#define BAG_LFARROW     9   /* Left arrow */
#define BAG_POPTIME     10  /* Popup time glyph */
#define BAG_POPSCREEN   11  /* Popup screen mode glyph */
#define BAG_POPUP       12  /* Generic popup glyph */

/*****************************************************************************/

#ifndef ButtonObject
#define ButtonObject    NewObject(NULL, BUTTON_CLASSNAME
#endif
#ifndef ButtonEnd
#define ButtonEnd       TAG_END)
#endif
#define StartButton     ButtonObject

#endif /* GADGETS_BUTTON_H */
