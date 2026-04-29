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

#define SLIDER_CLASSNAME        "slider.gadget"
#define SLIDER_VERSION          44

#define SLIDER_Dummy            (REACTION_Dummy + 0x0028000)

#define SLIDER_Min              (SLIDER_Dummy + 0x0001) /* Minimum value */
#define SLIDER_Max              (SLIDER_Dummy + 0x0002) /* Maximum value */
#define SLIDER_Level            (SLIDER_Dummy + 0x0003) /* Current level */
#define SLIDER_Orientation      (SLIDER_Dummy + 0x0004) /* Horiz or vert */
#define SLIDER_Ticks            (SLIDER_Dummy + 0x0005) /* Major tick count */
#define SLIDER_ShortTicks       (SLIDER_Dummy + 0x0006) /* Minor tick count */
#define SLIDER_Invert           (SLIDER_Dummy + 0x0007) /* Reverse direction */
#define SLIDER_LevelFormat      (SLIDER_Dummy + 0x0008) /* Printf format for level */
#define SLIDER_LevelPlace       (SLIDER_Dummy + 0x0009) /* Level text placement */
#define SLIDER_LevelMaxLen      (SLIDER_Dummy + 0x000A) /* Level string max chars */
#define SLIDER_KnobDelta        (SLIDER_Dummy + 0x000B) /* Knob movement step */

#define SLIDER_HORIZONTAL       0
#define SLIDER_VERTICAL         1

#define PLACETEXT_LEFT   (1 << 0)
#define PLACETEXT_RIGHT  (1 << 1)
#define PLACETEXT_ABOVE  (1 << 2)
#define PLACETEXT_BELOW  (1 << 3)
#define PLACETEXT_IN     (1 << 4)

#ifndef SliderObject
#define SliderObject    NewObject(NULL, SLIDER_CLASSNAME
#endif
#ifndef SliderEnd
#define SliderEnd       TAG_END)
#endif

#endif /* GADGETS_SLIDER_H */
