/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: Reaction - ClassAct/ReAction compatible gadgets/slider.h
*/

#ifndef GADGETS_SLIDER_H
#define GADGETS_SLIDER_H

#ifndef EXEC_TYPES_H
#include <exec/types.h>
#endif
#ifndef REACTION_REACTION_H
#include <reaction/reaction.h>
#endif
#ifndef INTUITION_GADGETCLASS_H
#include <intuition/gadgetclass.h>
#endif

#define SLIDER_CLASSNAME        "slider.gadget"
#define SLIDER_VERSION          44

#define SLIDER_Dummy            (REACTION_Dummy + 0x0028000)

#define SLIDER_Min              (SLIDER_Dummy + 1)   /* Minimum value (WORD) */
#define SLIDER_Max              (SLIDER_Dummy + 2)   /* Maximum value (WORD) */
#define SLIDER_Level            (SLIDER_Dummy + 3)   /* Current level (WORD) */
#define SLIDER_Orientation      (SLIDER_Dummy + 4)   /* Horizontal or vertical (WORD) */
#define SLIDER_DispHook         (SLIDER_Dummy + 5)   /* Display hook (struct Hook *) */
#define SLIDER_Ticks            (SLIDER_Dummy + 6)   /* Number of tick marks (LONG) */
#define SLIDER_ShortTicks       (SLIDER_Dummy + 7)   /* Use short ticks (BOOL) */
#define SLIDER_TickSize         (SLIDER_Dummy + 8)   /* Tick mark size (WORD) */
#define SLIDER_KnobImage        (SLIDER_Dummy + 9)   /* Custom knob image (struct Image *) */
#define SLIDER_BodyFill         (SLIDER_Dummy + 10)  /* Body fill pen (WORD) */
#define SLIDER_BodyImage        (SLIDER_Dummy + 11)  /* Custom body image (struct Image *) */
#define SLIDER_Gradient         (SLIDER_Dummy + 12)  /* Gradient mode (BOOL) */
#define SLIDER_PenArray         (SLIDER_Dummy + 13)  /* Gradient pen array (UWORD *) */
#define SLIDER_Invert           (SLIDER_Dummy + 14)  /* Invert min/max positions (BOOL) */
#define SLIDER_KnobDelta        (SLIDER_Dummy + 15)  /* Knob movement delta (WORD) */

/* Orientation modes */
#define SORIENT_HORIZ           FREEHORIZ
#define SORIENT_VERT            FREEVERT

#define SLIDER_HORIZONTAL       SORIENT_HORIZ
#define SLIDER_VERTICAL         SORIENT_VERT

#ifndef SliderObject
#define SliderObject    NewObject(NULL, SLIDER_CLASSNAME
#endif
#ifndef SliderEnd
#define SliderEnd       TAG_END)
#endif

#endif /* GADGETS_SLIDER_H */
