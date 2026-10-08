/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: Filesystem Imports - mounts a volume exported by another Envoy
          host. Picks the host, lists its volumes for a user and writes the
          same text mount file as the original program (to T:, DEVS:DOSDrivers
          or SYS:Storage/DOSDrivers) before running C:Mount on it.

    FilesystemImports HOST/K,USER/K,PASSWORD/K,LIST/S,EXPORT/K,LOCATION/K,MOUNT/S,PUBSCREEN/K

    LIST prints the volumes HOST offers USER, one per line (return code 5
    when there are none, 10 on failure). EXPORT=name MOUNT mounts one without
    a window; LOCATION is TEMPORARY (default), PERMANENT or STORAGE. Without
    LIST or MOUNT the window opens, preset from the arguments.
*/

#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/utility.h>
#include <exec/memory.h>
#include <string.h>

#include "imports.h"
#include "locale.h"

#define VERSION "$VER: FilesystemImports 50.0 (8.10.2026) AROS Dev Team"
const char version[] = VERSION;

struct Library *NIPCBase, *ServicesBase, *AccountsBase, *EnvoyBase, *IconBase;

int Gui_Run(CONST_STRPTR host, CONST_STRPTR user, CONST_STRPTR pubscreen);

enum { ARG_HOST, ARG_USER, ARG_PASSWORD, ARG_LIST, ARG_EXPORT, ARG_LOCATION, ARG_MOUNT, ARG_PUBSCREEN, ARG_COUNT };

static LONG ParseLocation(CONST_STRPTR s)
{
    if (!s || !Stricmp(s, "TEMPORARY"))
        return LOC_TEMPORARY;
    if (!Stricmp(s, "PERMANENT"))
        return LOC_PERMANENT;
    if (!Stricmp(s, "STORAGE"))
        return LOC_STORAGE;
    return -1;
}

static int CliList(CONST_STRPTR host, CONST_STRPTR user, CONST_STRPTR password)
{
    struct List list;
    struct Node *n;
    char buf[64];
    ULONG err;
    int count = 0;

    NEWLIST(&list);
    if ((err = Imp_ListExports(host, user ? user : (CONST_STRPTR)"", password ? password : (CONST_STRPTR)"", &list)))
    {
        Printf("FilesystemImports: %s: %s\n", host, Imp_ErrorText(err, buf, sizeof(buf)));
        return RETURN_ERROR;
    }
    for (n = list.lh_Head; n->ln_Succ; n = n->ln_Succ, count++)
        Printf("%s\n", n->ln_Name);
    Imp_FreeList(&list);
    return count ? RETURN_OK : RETURN_WARN;
}

static int CliMount(CONST_STRPTR host, CONST_STRPTR export, CONST_STRPTR user, CONST_STRPTR password, LONG location)
{
    struct ImpLogin login;
    char path[300], vol[108], dev[200];
    LONG err;

    if (!Imp_MakeLogin(&login, user ? user : (CONST_STRPTR)"", password ? password : (CONST_STRPTR)""))
    {
        Printf(_(MSG_ERR_NOLIB), "accounts.library");
        PutStr("\n");
        return RETURN_FAIL;
    }
    Imp_DeviceName(dev, sizeof(dev), host, export);
    err = Imp_Mount(host, export, &login, location, path, sizeof(path), vol, sizeof(vol));
    memset(&login, 0, sizeof(login));
    if (err == ERROR_OBJECT_EXISTS)
    {
        Printf(_(MSG_STATUS_ALREADY), dev);
        PutStr("\n");
        return RETURN_WARN;
    }
    if (err)
    {
        char reason[100];
        if (err == IMP_ERR_REFUSED)
        {
            char tmp[64];
            strncpy(reason, Imp_ErrorText(err, tmp, sizeof(tmp)), sizeof(reason) - 1);
            reason[sizeof(reason) - 1] = '\0';
        }
        else
            Fault(err, NULL, reason, sizeof(reason));
        Printf(_(MSG_STATUS_MOUNTFAIL), dev, reason);
        PutStr("\n");
        return RETURN_ERROR;
    }
    Printf(_(MSG_STATUS_MOUNTED), vol, dev);
    PutStr("\n");
    return RETURN_OK;
}

int main(void)
{
    IPTR args[ARG_COUNT] = { 0 };
    struct RDArgs *rda;
    int rc = RETURN_FAIL;

    Locale_Initialize();
    if (!(rda = ReadArgs("HOST/K,USER/K,PASSWORD/K,LIST/S,EXPORT/K,LOCATION/K,MOUNT/S,PUBSCREEN/K", args, NULL)))
    {
        PrintFault(IoErr(), "FilesystemImports");
        Locale_Deinitialize();
        return RETURN_FAIL;
    }
    NIPCBase = OpenLibrary("nipc.library", 50);
    ServicesBase = OpenLibrary("services.library", 50);
    AccountsBase = OpenLibrary("accounts.library", 50);
    EnvoyBase = OpenLibrary("envoy.library", 50);
    IconBase = OpenLibrary("icon.library", 0);

    if (!NIPCBase || !ServicesBase)
    {
        Printf(_(MSG_ERR_NOLIB), NIPCBase ? "services.library" : "nipc.library");
        PutStr("\n");
    }
    else if (args[ARG_LIST] || args[ARG_MOUNT])
    {
        CONST_STRPTR host = (CONST_STRPTR)args[ARG_HOST];
        LONG location = ParseLocation((CONST_STRPTR)args[ARG_LOCATION]);

        if (!host || !host[0])
        {
            PutStr("FilesystemImports: HOST is required\n");
            rc = RETURN_ERROR;
        }
        else if (args[ARG_LIST])
            rc = CliList(host, (CONST_STRPTR)args[ARG_USER], (CONST_STRPTR)args[ARG_PASSWORD]);
        else if (!args[ARG_EXPORT])
        {
            PutStr("FilesystemImports: MOUNT needs EXPORT\n");
            rc = RETURN_ERROR;
        }
        else if (location < 0)
        {
            PutStr("FilesystemImports: LOCATION is TEMPORARY, PERMANENT or STORAGE\n");
            rc = RETURN_ERROR;
        }
        else
            rc = CliMount(host, (CONST_STRPTR)args[ARG_EXPORT], (CONST_STRPTR)args[ARG_USER],
                          (CONST_STRPTR)args[ARG_PASSWORD], location);
    }
    else
        rc = Gui_Run((CONST_STRPTR)args[ARG_HOST], (CONST_STRPTR)args[ARG_USER], (CONST_STRPTR)args[ARG_PUBSCREEN]);

    if (args[ARG_PASSWORD])
        memset((APTR)args[ARG_PASSWORD], 0, strlen((CONST_STRPTR)args[ARG_PASSWORD]));
    FreeArgs(rda);
    if (IconBase) CloseLibrary(IconBase);
    if (EnvoyBase) CloseLibrary(EnvoyBase);
    if (AccountsBase) CloseLibrary(AccountsBase);
    if (ServicesBase) CloseLibrary(ServicesBase);
    if (NIPCBase) CloseLibrary(NIPCBase);
    Locale_Deinitialize();
    return rc;
}
