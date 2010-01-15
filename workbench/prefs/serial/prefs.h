/*
    Copyright � 1995-2010, The AROS Development Team. All rights reserved.
    $Id$

    Desc:
    Lang: English
*/

#ifndef _PREFS_H_
#define _PREFS_H_

#include <prefs/serial.h>
#include <dos/dos.h>

/*********************************************************************************************/

BOOL Prefs_HandleArgs(STRPTR from, BOOL use, BOOL save);
BOOL Prefs_ImportFH(BPTR fh);
BOOL Prefs_ExportFH(BPTR fh);
BOOL Prefs_Default(VOID);

/*********************************************************************************************/

extern struct SerialPrefs serialprefs;

/*********************************************************************************************/

#endif /* _PREFS_H_ */
