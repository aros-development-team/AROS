/*
    Copyright © 2010, The AROS Development Team. All rights reserved.
    $Id$
*/

/*
    Copyright © 1995-2010, The AROS Development Team. All rights reserved.
    $Id$

    Desc:
    Lang: English
*/

#ifndef PREFS_H
#define PREFS_H

#include <prefs/serial.h>
#include <dos/dos.h>

/*********************************************************************************************/

BOOL Prefs_HandleArgs(STRPTR from, BOOL use, BOOL save);
BOOL Prefs_ImportFH(BPTR fh);
BOOL Prefs_ExportFH(BPTR fh);
BOOL Prefs_Default(VOID);

/*********************************************************************************************/

extern struct ScreenModePrefs screenmodeprefs;

/*********************************************************************************************/

#endif /* PREFS_H */
