#ifndef _PREFS_H_
#define _PREFS_H_

/*
    Copyright © 2010, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <prefs/pointer.h>
#include <dos/dos.h>

/*********************************************************************************************/

#define MAXPOINTER (2)

/*********************************************************************************************/

BOOL Prefs_HandleArgs(STRPTR from, BOOL use, BOOL save);
BOOL Prefs_ImportFH(BPTR fh);
BOOL Prefs_ExportFH(BPTR fh);
BOOL Prefs_Default(VOID);

/*********************************************************************************************/

extern struct NewPointerPrefs pointerprefs[MAXPOINTER];

/*********************************************************************************************/

#endif /* _PREFS_H_ */
