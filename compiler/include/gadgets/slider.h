/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: Reaction - ClassAct/ReAction compatible gadgets/slider.h
*/

#ifndef GADGETS_SLIDER_H
#define GADGETS_SLIDER_H

#ifndef EXEC_TYPES_H
#include <exec/types.h>
#endif
#ifndef UTILITY_TAGITEM_H
#include <utility/tagitem.h>
#endif

#define SLIDER_CLASSNAME        "gadgets/slider.gadget"
#define SLIDER_VERSION          44

#define SLIDER_Dummy            (TAG_USER + 0x100000)

#define SLIDER_Min              (SLIDER_Dummy + 0x0001)
#define SLIDER_Max              (SLIDER_Dummy + 0x0002)
#define SLIDER_Level            (SLIDER_Dummy + 0x0003)
#define SLIDER_Orientation      (SLIDER_Dummy + 0x0004)
#define SLIDER_Ticks            (SLIDER_Dummy + 0x0005)
#define SLIDER_ShortTicks       (SLIDER_Dummy + 0x0006)
#define SLIDER_Invert           (SLIDER_Dummy + 0x0007)
#define SLIDER_LevelFormat      (SLIDER_Dummy + 0x0008)
#define SLIDER_LevelPlace       (SLIDER_Dummy + 0x0009)
#define SLIDER_LevelMaxLen      (SLIDER_Dummy + 0x000A)
#define SLIDER_KnobDelta        (SLIDER_Dummy + 0x000B)

#define SLIDER_HORIZONTAL       0
#define SLIDER_VERTICAL         1

#define PLACETEXT_LEFT   (1 << 0)
#define PLACETEXT_RIGHT  (1 << 1)
#define PLACETEXT_ABOVE  (1 << 2)
#define PLACETEXT_BELOW  (1 << 3)
#define PLACETEXT_IN     (1 << 4)

#define SliderObject    NewObject(NULL, SLIDER_CLASSNAME
#define SliderEnd       TAG_END)

#endif /* GADGETS_SLIDER_H */
