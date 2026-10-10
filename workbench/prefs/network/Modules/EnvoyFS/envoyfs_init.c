/*
    Copyright (C) 2009-2026, The AROS Development Team. All rights reserved.

    envoyfs_init.c - Envoy Filesystem mounted-share module initialisation.
    Loaded as envoyfs.netprefs by the Network prefs editor.

    The handler is ALWAYS registered, even when no Envoy library can be
    opened ("show but degrade"): existing Envoy mountfiles stay listed and
    hand-editable, and the editor's stale-mount cleanup must keep treating
    them as ours.  Only discovery (host requester, export listing) and
    password hashing degrade with the missing libraries.
*/

#include <aros/debug.h>

#include <proto/exec.h>
#include <proto/netprefs.h>

#include <aros/libcall.h>
#include <aros/asmcall.h>

#include "envoyfs_intern.h"
#include "protocols.h"
#include "locale.h"

struct Library *NIPCBase     = NULL;
struct Library *ServicesBase = NULL;
struct Library *AccountsBase = NULL;
struct Library *EnvoyBase    = NULL;

/* Forward declarations from envoyfs_win.c */
extern BOOL EFSWin_InitClass(struct MUI_CustomClass *PAWinCl);
extern void EFSWin_FreeClass(void);
extern struct MUI_CustomClass *EFSWinClass;

/* Forward declarations from envoyfs_mount.c */
extern BOOL EFS_ReadMount(const struct NetPrefsMountInfo *mi,
                          struct MountedShare *ms);
extern BOOL EFS_WriteMount(FILE *f, const struct MountedShare *ms);
extern void EFS_InitShare(struct MountedShare *ms, CONST_STRPTR domain);

/* -----------------------------------------------------------------------
 * EFSStartup — called after all modules are loaded.
 * ----------------------------------------------------------------------- */
static void EFSStartup(struct NetPrefsBase *NetPrefsBase)
{
    struct NetPrefsEFSBase *EFSBase =
        (struct NetPrefsEFSBase *)GetBase("EnvoyFS.Module");

    D(bug("[envoyfs.netprefs] %s: EFSBase @ %p\n", __func__, EFSBase));

    /* All optional - NULL bases just disable the dependent gadgets */
    NIPCBase     = OpenLibrary("nipc.library", 50);
    ServicesBase = OpenLibrary("services.library", 50);
    AccountsBase = OpenLibrary("accounts.library", 50);
    EnvoyBase    = OpenLibrary("envoy.library", 50);

    struct MUI_CustomClass *PAWinCl =
        (struct MUI_CustomClass *)GetBase("PAWin.Class");

    if (PAWinCl && EFSWin_InitClass(PAWinCl))
        EFSBase->npe_WinClass = EFSWinClass;

    RegisterFSHandler(_(MSG_SERVICETYPE_ENVOY), 60, EFSBase->npe_WinClass,
                      EFS_ReadMount, EFS_WriteMount, EFS_InitShare, NULL);
}

/* -----------------------------------------------------------------------
 * EFSShutdown — called on app exit.
 * ----------------------------------------------------------------------- */
static void EFSShutdown(struct NetPrefsBase *NetPrefsBase)
{
    D(bug("[envoyfs.netprefs] %s()\n", __func__));

    EFSWin_FreeClass();

    if (EnvoyBase)    { CloseLibrary(EnvoyBase);    EnvoyBase    = NULL; }
    if (AccountsBase) { CloseLibrary(AccountsBase); AccountsBase = NULL; }
    if (ServicesBase) { CloseLibrary(ServicesBase); ServicesBase = NULL; }
    if (NIPCBase)     { CloseLibrary(NIPCBase);     NIPCBase     = NULL; }
}

/* -----------------------------------------------------------------------
 * ModuleInit — library vector 5.
 * Called immediately after the module is loaded via OpenLibrary().
 * ----------------------------------------------------------------------- */
AROS_LH1(void, ModuleInit,
         AROS_LHA(struct NetPrefsBase *, NetPrefsBase, A0),
         struct NetPrefsEFSBase *, EFSBase, 5, Envoyfs)
{
    AROS_LIBFUNC_INIT

    D(bug("[envoyfs.netprefs] %s(%p)\n", __func__, NetPrefsBase));

    EFSBase->npe_NetPrefsBase = NetPrefsBase;

    EFSBase->npe_Module.npm_Node.ln_Name = "EnvoyFS.Module";
    EFSBase->npe_Module.npm_Node.ln_Pri  = 60;
    EFSBase->npe_Module.npm_Startup      = EFSStartup;
    EFSBase->npe_Module.npm_Shutdown     = EFSShutdown;

    RegisterProtoModule(&EFSBase->npe_Module, EFSBase);

    AROS_LIBFUNC_EXIT
}
