/*
    Copyright © 2012, The AROS Development Team. All rights reserved.
    $Id$

    Desc:
    Lang: English
*/

#ifndef _PREFS_H_
#define _PREFS_H_

#include <dos/dos.h>

/*********************************************************************************************/

BOOL Prefs_HandleArgs(STRPTR from, BOOL use, BOOL save);
BOOL Prefs_ImportFH(BPTR fh);
BOOL Prefs_ExportFH(BPTR fh);
BOOL Prefs_Default(VOID);

/*********************************************************************************************/

#define BIB_MAX_PROGRAMS    20
#define BIB_MAX_DOCKS       10
#define BIB_MAX_PATH        300
#define BIB_MAX_NAME        50

struct Dock
{
    char name[BIB_MAX_NAME];
    char programs[BIB_MAX_PROGRAMS + 1][BIB_MAX_PATH];
};

struct BIBPrefs
{
    struct Dock docks[BIB_MAX_DOCKS];
};

extern struct BIBPrefs bibprefs;

/*********************************************************************************************/

#endif /* _PREFS_H_ */
