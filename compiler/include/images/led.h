/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: Reaction - ClassAct/ReAction compatible images/led.h
*/

#ifndef IMAGES_LED_H
#define IMAGES_LED_H

#ifndef EXEC_TYPES_H
#include <exec/types.h>
#endif
#ifndef UTILITY_TAGITEM_H
#include <utility/tagitem.h>
#endif

#define LED_CLASSNAME       "images/led.image"
#define LED_VERSION         44

#define LED_Dummy           (TAG_USER + 0x1B0000)

#define LED_Pairs           (LED_Dummy + 0x0001)
#define LED_Time            (LED_Dummy + 0x0002)
#define LED_Values          (LED_Dummy + 0x0003)
#define LED_EditMode        (LED_Dummy + 0x0004)
#define LED_Pen             (LED_Dummy + 0x0005)

#define LEDObject       NewObject(NULL, LED_CLASSNAME
#define LEDEnd          TAG_END)

#endif /* IMAGES_LED_H */
