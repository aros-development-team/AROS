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

#define CONFIGNAME_ENV	    	"ENV:Sys/serial.prefs"
#define CONFIGNAME_ENVARC   	"ENVARC:Sys/serial.prefs"

/*********************************************************************************************/

/* main.c */

VOID ShowMsg      (char *msg);

/* locale.c */

VOID InitLocale   (VOID);
VOID CleanupLocale(VOID);
CONST_STRPTR MSG  (ULONG id);

/* prefs.c */

ULONG InitPrefs   (STRPTR filename, BOOL use, BOOL save);
VOID  CleanupPrefs(void);
BOOL  LoadPrefs   (STRPTR filename);
BOOL  LoadPrefsFH (BPTR fh);
BOOL  SavePrefs   (STRPTR filename);
BOOL  SavePrefsFH (BPTR fh);
BOOL  SaveEnv     ();
BOOL  DefaultPrefs(void);
VOID  RestorePrefs(void);
VOID  BackupPrefs (void);
VOID  CopyPrefs   (struct SerialPrefs *s, struct SerialPrefs *d);

/*********************************************************************************************/

#endif /* GLOBAL_H */
