#ifndef PREFS_INPUT_H
#define PREFS_INPUT_H

/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Input prefs definitions
    Lang: english
*/

#ifndef EXEC_TYPES_H
#   include <exec/types.h>
#endif

#ifndef LIBRARIES_IFFPARSE_H
#   include <libraries/iffparse.h>
#endif

#ifndef DEVICES_TIMER_H
#   include <devices/timer.h>
#endif

#define ID_INPT MAKE_ID('I','N','P','T')

struct InputPrefs {
    char           ip_Keymap[16];
    UWORD          ip_PointerTicks;
    struct timeval ip_DoubleClick;
    struct timeval ip_KeyRptDelay;
    struct timeval ip_KeyRptSpeed;
    WORD           ip_MouseAccel;
};

#endif /* PREFS_INPUT_H */
