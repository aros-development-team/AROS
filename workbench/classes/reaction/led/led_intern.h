/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: Reaction led.image - Internal definitions
*/

#ifndef LED_INTERN_H
#define LED_INTERN_H

#include <exec/types.h>
#include <intuition/intuition.h>
#include <intuition/classes.h>
#include <intuition/imageclass.h>
#include <images/led.h>

#ifdef __AROS__
#include <aros/debug.h>
#endif

#include LC_LIBDEFS_FILE

#include <exec/libraries.h>

/* Module library base with stored class pointer */
struct LEDBase_intern
{
    struct Library lib;
    Class *rc_Class;
};

struct LEDData
{
    UWORD           ld_Pairs;           /* Number of digit pairs */
    BOOL            ld_Time;            /* Time display mode (colons) */
    UBYTE          *ld_Values;          /* Digit values array */
    BOOL            ld_EditMode;        /* Edit mode flag */
    UWORD           ld_Pen;             /* LED color pen */
};

#endif /* LED_INTERN_H */
