#ifndef SVCPREFS_PREFS_H
#define SVCPREFS_PREFS_H
/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: Envoy Services preferences - the data being edited: the service
          list of services.prefs and the AutoRun switch.
*/

#include <exec/types.h>
#include <exec/lists.h>
#include <dos/dos.h>

#include "../../services/tools/prefsfile.h"

#define PREFS_PATH          "Envoy/services.prefs"      /* relative to ENV: / ENVARC: */
#define PREFS_AUTORUN       "Envoy/AutoRun"
#define PREFS_PATH_ENV      "ENV:" PREFS_PATH
#define PREFS_PATH_ENVARC   "ENVARC:" PREFS_PATH

extern struct List SvcList;         /* struct PrefsEntry nodes */
extern BOOL SvcAutoRun;

BOOL Prefs_Initialize(VOID);
VOID Prefs_Deinitialize(VOID);
BOOL Prefs_Default(VOID);
BOOL Prefs_Load(CONST_STRPTR filename);
BOOL Prefs_Save(CONST_STRPTR filename);
BOOL Prefs_LoadAutoRun(CONST_STRPTR filename);
BOOL Prefs_SaveAutoRun(CONST_STRPTR filename);
BOOL Prefs_ImportFH(BPTR fh);
BOOL Prefs_ExportFH(BPTR fh);
BOOL Prefs_HandleArgs(STRPTR from, BOOL use, BOOL save);

#endif
