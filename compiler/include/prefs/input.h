#ifndef PREFS_INPUT_H
#define PREFS_INPUT_H

/*
    Copyright © 1995-2011, The AROS Development Team. All rights reserved.
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
    char             ip_Keymap[16];
    UWORD            ip_PointerTicks;
    struct timeval32 ip_DoubleClick;
    struct timeval32 ip_KeyRptDelay;
    struct timeval32 ip_KeyRptSpeed;
    WORD             ip_MouseAccel;

    /* The following fields are compatible with AmigaOS v4 */
    ULONG          ip_ClassicKeyboard;		/* Reserved		       */
    char           ip_KeymapName[64];		/* Longer version of ip_Keymap */
    ULONG          ip_SwitchMouseButtons;	/* Swap mouse buttons, boolean */
};

/* Experimental and AROS-specific, subject to change */

#define ID_KMSW MAKE_ID('K','M','S','W')

struct KMSPrefs
{
    UBYTE kms_Enabled;		/* Boolean - alternate keymap enabled */
    UBYTE kms_Reserved;
    UWORD kms_SwitchQual;	/* Switch key and qualifier	      */
    UWORD kms_SwitchCode;
    char  kms_AltKeymap[64];	/* Alternate keymap name	      */
};

#endif /* PREFS_INPUT_H */
