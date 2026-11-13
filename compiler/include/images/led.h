/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: Reaction - ClassAct/ReAction compatible images/led.h
*/

#ifndef IMAGES_LED_H
#define IMAGES_LED_H

#ifndef EXEC_TYPES_H
#include <exec/types.h>
#endif
#ifndef REACTION_REACTION_H
#include <reaction/reaction.h>
#endif

#define LED_CLASSNAME       "led.image"
#define LED_VERSION         44

#define LED_Dummy           (REACTION_Dummy + 0x1A000)

#define LED_Pairs           (LED_Dummy + 0x0001) /* Digit pair count */
#define LED_Time            (LED_Dummy + 0x0002) /* Time display mode */
#define LED_Values          (LED_Dummy + 0x0003) /* Value array */
#define LED_EditMode        (LED_Dummy + 0x0004) /* Editable digits */
#define LED_Pen             (LED_Dummy + 0x0005) /* LED color pen */

#ifndef LEDObject
#define LEDObject       NewObject(NULL, LED_CLASSNAME
#endif
#ifndef LEDEnd
#define LEDEnd          TAG_END)
#endif

#endif /* IMAGES_LED_H */
