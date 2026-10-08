/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: Envoy Services preferences - loading and saving. services.prefs
          is read and written by the code ServicesConfig and the Services
          Manager use (services/tools/prefsfile.c), which works on file
          names; the PrefsEditor class's backup copy goes through file
          handles, so those are bridged with a temporary file.
*/

#include <aros/debug.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <exec/memory.h>
#include <string.h>

#include "prefs.h"

struct List SvcList;
BOOL SvcAutoRun = TRUE;

#define DEFAULT_NAME    "Filesystem"
#define DEFAULT_PATH    "SYS:System/Network/Envoy/Services/filesystem.service"

BOOL Prefs_Initialize(VOID)
{
    NEWLIST(&SvcList);
    return TRUE;
}

VOID Prefs_Deinitialize(VOID)
{
    FreePrefsEntries(&SvcList);
}

BOOL Prefs_Default(VOID)
{
    FreePrefsEntries(&SvcList);
    SvcAutoRun = TRUE;
    return AddPrefsEntry(&SvcList, DEFAULT_NAME, DEFAULT_PATH, TRUE) != NULL;
}

BOOL Prefs_Load(CONST_STRPTR filename)
{
    struct List tmp;
    struct Node *n;

    LONG r;

    NEWLIST(&tmp);
    r = ReadServicesPrefs(filename, &tmp);
    D(bug("[Services] Prefs_Load '%s' -> %ld\n", filename, (long)r));
    if (r < 0)
    {
        FreePrefsEntries(&tmp);
        return FALSE;
    }
    FreePrefsEntries(&SvcList);
    while ((n = RemHead(&tmp)))
        AddTail(&SvcList, n);
    return TRUE;
}

BOOL Prefs_Save(CONST_STRPTR filename)
{
    return WriteServicesPrefs(filename, &SvcList);
}

/* AutoRun: a variable file holding "True" or "False"; missing means True */
BOOL Prefs_LoadAutoRun(CONST_STRPTR filename)
{
    char buf[16];
    BPTR fh;
    LONG n;

    SvcAutoRun = TRUE;
    if (!(fh = Open(filename, MODE_OLDFILE)))
        return FALSE;
    n = Read(fh, buf, sizeof(buf) - 1);
    Close(fh);
    if (n < 0)
        return FALSE;
    buf[n] = '\0';
    SvcAutoRun = !(buf[0] == 'F' || buf[0] == 'f' || buf[0] == '0' || buf[0] == 'N' || buf[0] == 'n');
    return TRUE;
}

BOOL Prefs_SaveAutoRun(CONST_STRPTR filename)
{
    CONST_STRPTR text = SvcAutoRun ? "True" : "False";
    BPTR fh;
    BOOL ok;

    if (!(fh = Open(filename, MODE_NEWFILE)))
        return FALSE;
    ok = Write(fh, (APTR)text, strlen(text)) == (LONG)strlen(text);
    Close(fh);
    return ok;
}

/* ---- file handles through a temporary file -------------------------------- */

static void TempName(char *buf, ULONG size)
{
    static ULONG count;
    char num[24];
    ULONG v = (ULONG)(IPTR)FindTask(NULL) ^ (++count << 20), i = sizeof(num) - 1;

    num[i] = '\0';
    do { num[--i] = "0123456789abcdef"[v & 15]; v >>= 4; } while (v && i);
    strncpy(buf, "T:envoy-svcprefs-", size - 1);
    buf[size - 1] = '\0';
    strncat(buf, num + i, size - strlen(buf) - 1);
}

static BOOL CopyFH(BPTR from, BPTR to)
{
    UBYTE buf[512];
    LONG n;

    while ((n = Read(from, buf, sizeof(buf))) > 0)
    {
        if (Write(to, buf, n) != n)
            return FALSE;
    }
    return n == 0;
}

BOOL Prefs_ImportFH(BPTR fh)
{
    char name[64];
    BPTR tmp;
    BOOL ok = FALSE;

    TempName(name, sizeof(name));
    if ((tmp = Open(name, MODE_NEWFILE)))
    {
        ok = CopyFH(fh, tmp);
        Close(tmp);
        if (ok)
            ok = Prefs_Load(name);
        DeleteFile(name);
    }
    return ok;
}

BOOL Prefs_ExportFH(BPTR fh)
{
    char name[64];
    BPTR tmp;
    BOOL ok = FALSE;

    TempName(name, sizeof(name));
    if (Prefs_Save(name) && (tmp = Open(name, MODE_OLDFILE)))
    {
        ok = CopyFH(tmp, fh);
        Close(tmp);
    }
    DeleteFile(name);
    return ok;
}

/* ---- USE / SAVE from the shell, without the window ------------------------- */

BOOL Prefs_HandleArgs(STRPTR from, BOOL use, BOOL save)
{
    BOOL ok;

    if (from)
    {
        if (!Prefs_Load(from))
        {
            Printf("Services: cannot read %s\n", from);
            return FALSE;
        }
    }
    else if (!Prefs_Load(PREFS_PATH_ENV) && !Prefs_Load(PREFS_PATH_ENVARC))
        Prefs_Default();

    ok = TRUE;
    if (use || save)
        ok = Prefs_Save(PREFS_PATH_ENV);
    if (save)
        ok = Prefs_Save(PREFS_PATH_ENVARC) && ok;
    if (!ok)
        PutStr("Services: cannot write the preferences\n");
    return ok;
}
