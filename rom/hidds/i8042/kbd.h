#ifndef HIDD_KBD_H
#define HIDD_KBD_H

/*
    Copyright © 1995-2013, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Include for the kbd HIDD.
    Lang: English.
*/

#define __OOP_NOMETHODBASES__

#include <exec/libraries.h>
#include <oop/oop.h>
#include <exec/semaphores.h>
#include <exec/interrupts.h>
#include <dos/bptr.h>

#include "libbase.h"

/****************************************************************************************/

#define KBD_OUTCMD_SET_LEDS             0xED
#define KBD_OUTCMD_SET_RATE             0xF3
#define KBD_OUTCMD_ENABLE               0xF4
#define KBD_OUTCMD_DISABLE              0xF5
#define KBD_OUTCMD_RESET                0xFF

/****************************************************************************************/

struct kbd_data
{
    VOID    (*kbd_callback)(APTR, UWORD);
    APTR    callbackdata;

    ULONG   kbd_keystate;
    WORD    prev_amigacode;
    UWORD   prev_keycode;
};

/****************************************************************************************/

#endif /* HIDD_KBD_H */
