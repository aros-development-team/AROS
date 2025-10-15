#ifndef I8042_KBD_H
#define I8042_KBD_H

/*
    Copyright © 1995-2025, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Include for the kbd HIDD.
    Lang: English.
*/

#ifndef __OOP_NOMETHODBASES__
#define __OOP_NOMETHODBASES__
#endif

#include <exec/libraries.h>
#include <exec/semaphores.h>
#include <exec/interrupts.h>
#include <exec/io.h>
#include <hidd/input.h>
#include <hidd/keyboard.h>

#include <devices/timer.h>
#include <dos/bptr.h>
#include <oop/oop.h>

#include "i8042_intern.h"

/****************************************************************************************/

#define KBD_OUTCMD_SET_LEDS             0xED
#define KBD_OUTCMD_SET_RATE             0xF3
#define KBD_OUTCMD_ENABLE               0xF4
#define KBD_OUTCMD_DISABLE              0xF5
#define KBD_OUTCMD_RESET                0xFF

/****************************************************************************************/

struct kbd_data
{
    struct IORequest        *ioTimer;
    struct Task             *CtrlTask;

    InputIrqCallBack_t      kbd_callback;
    APTR                    callbackdata;
    APTR                    irq;

    ULONG                   LEDSigBit;

    ULONG                   kbd_keystate;
    ULONG                   kbd_ledstate;
    WORD                    prev_amigacode;
    UWORD                   prev_keycode;
};

/****************************************************************************************/

#endif /* I8042_KBD_H */
