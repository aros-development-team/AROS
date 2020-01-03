/*
    Copyright © 2010-2020, The AROS Development Team. All rights reserved.
    $Id$

    Desc:
    Lang: English
*/

#ifndef _PREFS_H_
#define _PREFS_H_

#include <intuition/screens.h>
#include <prefs/palette.h>
#include <dos/dos.h>

/*********************************************************************************************/

#define MAXPENS       NUMDRIPENS

/*********************************************************************************************/

BOOL Prefs_HandleArgs(STRPTR from, BOOL use, BOOL save);
BOOL Prefs_ImportFH(BPTR fh);
BOOL Prefs_ExportFH(BPTR fh);
BOOL Prefs_Default(VOID);

/*********************************************************************************************/

extern struct PalettePrefs paletteprefs;

/*********************************************************************************************/

#endif /* _PREFS_H_ */
