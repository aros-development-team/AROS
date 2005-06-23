#ifndef PREFS_SCREENMODE_H
#define PREFS_SCREENMODE_H

/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Screenmode prefs definitions
    Lang: english
*/

#ifndef EXEC_TYPES_H
#   include <exec/types.h>
#endif

#ifndef LIBRARIES_IFFPARSE_H
    #include <libraries/iffparse.h>
#endif


#define ID_SCRM MAKE_ID('S','C','R','M')


struct ScreenModePrefs
{
    ULONG smp_Reserved[4];
    ULONG smp_DisplayID;
    UWORD smp_Width;
    UWORD smp_Height;
    UWORD smp_Depth;
    UWORD smp_Control;
};

#define SMB_AUTOSCROLL 1

#define SMF_AUTOSCROLL (1<<0)

#endif /* PREFS_SCREENMODE_H */
