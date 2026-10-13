#ifndef SVCPREFS_LOCALE_H
#define SVCPREFS_LOCALE_H
/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: Envoy Services preferences - strings (built-in English, catalog
          "System/Prefs/Envoy/Services.catalog" used when present).
*/

#include <exec/types.h>

enum
{
    MSG_WINTITLE,
    MSG_DESCRIPTION,
    MSG_SERVICES,
    MSG_COL_NAME,
    MSG_COL_PATH,
    MSG_COL_ACTIVE,
    MSG_ADD,
    MSG_REMOVE,
    MSG_NAME,
    MSG_PATH,
    MSG_ACTIVE,
    MSG_AUTORUN,
    MSG_NEWSERVICE,
    MSG_YES,
    MSG_NO,
    MSG_ASLTITLE,
    MSG_COUNT
};

CONST_STRPTR _(ULONG id);
#define __(id) ((IPTR)_(id))

VOID Locale_Initialize(VOID);
VOID Locale_Deinitialize(VOID);

#endif
