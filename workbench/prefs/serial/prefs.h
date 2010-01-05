/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc:
    Lang: English
*/

#ifndef GLOBAL_H
#define GLOBAL_H

#include <prefs/serial.h>
#include <dos/dos.h>

/*********************************************************************************************/

BOOL Prefs_HandleArgs(STRPTR from, BOOL use, BOOL save);
BOOL Prefs_ImportFH(BPTR fh);
BOOL Prefs_ExportFH(BPTR fh);
BOOL Prefs_Default(VOID);

//ULONG InitPrefs   (STRPTR filename, BOOL use, BOOL save);
//VOID  CleanupPrefs(void);
//BOOL  LoadPrefs   (STRPTR filename);
//BOOL  LoadPrefsFH (BPTR fh);
//BOOL  SavePrefs   (STRPTR filename);
//BOOL  SavePrefsFH (BPTR fh);
//BOOL  SaveEnv     ();
//BOOL  DefaultPrefs(void);
//VOID  RestorePrefs(void);
//VOID  BackupPrefs (void);
//VOID  CopyPrefs   (struct SerialPrefs *s, struct SerialPrefs *d);

/*********************************************************************************************/

extern struct SerialPrefs serialprefs;

/*********************************************************************************************/

#endif /* GLOBAL_H */
