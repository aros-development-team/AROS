#ifndef LOCALE_H
#define LOCALE_H
/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.
*/

#include <exec/types.h>

enum
{
    MSG_WINTITLE,
    MSG_EXPORTS,
    MSG_ADD,
    MSG_REMOVE,
    MSG_NAME,
    MSG_PATH,
    MSG_SECURITY,
    MSG_SEC_MOUNTS,
    MSG_SEC_MOUNTSFILES,
    MSG_SEC_NONE,
    MSG_READONLY,
    MSG_REMOVABLE,
    MSG_EMULATEEXALL,
    MSG_SNAPSHOT,
    MSG_LEFTOUT,
    MSG_ACCESS,
    MSG_ADDACCESS,
    MSG_REMOVEACCESS,
    MSG_USER,
    MSG_GROUP,
    MSG_NEWEXPORT,
    MSG_NONAME,
    MSG_PICKUSER,
    MSG_OK,
    MSG_NOACCOUNTS,
    MSG_NOENVOY,
    MSG_SECHELP,
    MSG_COUNT
};

CONST_STRPTR _(ULONG id);
#define __(id) ((IPTR)_(id))

VOID Locale_Initialize(VOID);
VOID Locale_Deinitialize(VOID);

#endif
