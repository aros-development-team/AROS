/*
    Copyright (C) 2009-2026, The AROS Development Team. All rights reserved.

    cifs_init.c - CIFS/SMB mounted-share module initialisation.
    Loaded as cifs.netprefs by the Network prefs editor.
*/

#include <aros/debug.h>

#include <proto/netprefs.h>

#include <aros/libcall.h>
#include <aros/asmcall.h>

#include "cifs_intern.h"
#include "protocols.h"
#include "locale.h"

/* Forward declarations from cifs_win.c */
extern BOOL CIFSWin_InitClass(struct MUI_CustomClass *PAWinCl);
extern void CIFSWin_FreeClass(void);
extern struct MUI_CustomClass *CIFSWinClass;

/* Forward declarations from cifs_mount.c */
extern BOOL CIFS_ReadMount(const struct NetPrefsMountInfo *mi,
                           struct MountedShare *ms);
extern BOOL CIFS_WriteMount(FILE *f, const struct MountedShare *ms);
extern void CIFS_InitShare(struct MountedShare *ms, CONST_STRPTR domain);

/* -----------------------------------------------------------------------
 * CIFSStartup — called after all modules are loaded.
 * Retrieves PAWinClass from the main app and creates CIFSWinClass.
 * ----------------------------------------------------------------------- */
static void CIFSStartup(struct NetPrefsBase *NetPrefsBase)
{
    struct NetPrefsCIFSBase *CIFSBase =
        (struct NetPrefsCIFSBase *)GetBase("CIFS.Module");

    D(bug("[cifs.netprefs] %s: CIFSBase @ %p\n", __func__, CIFSBase));

    struct MUI_CustomClass *PAWinCl =
        (struct MUI_CustomClass *)GetBase("PAWin.Class");

    if (PAWinCl && CIFSWin_InitClass(PAWinCl))
        CIFSBase->npc_WinClass = CIFSWinClass;

    /* Register even without a window class (headless SAVE/USE): parsing and
     * writing mountfiles must work without a display. */
    RegisterFSHandler(_(MSG_SERVICETYPE_CIFS), 70, CIFSBase->npc_WinClass,
                      CIFS_ReadMount, CIFS_WriteMount, CIFS_InitShare, NULL);
}

/* -----------------------------------------------------------------------
 * CIFSShutdown — called on app exit.
 * ----------------------------------------------------------------------- */
static void CIFSShutdown(struct NetPrefsBase *NetPrefsBase)
{
    D(bug("[cifs.netprefs] %s()\n", __func__));
    CIFSWin_FreeClass();
}

/* -----------------------------------------------------------------------
 * ModuleInit — library vector 5.
 * Called immediately after the module is loaded via OpenLibrary().
 * ----------------------------------------------------------------------- */
AROS_LH1(void, ModuleInit,
         AROS_LHA(struct NetPrefsBase *, NetPrefsBase, A0),
         struct NetPrefsCIFSBase *, CIFSBase, 5, Cifs)
{
    AROS_LIBFUNC_INIT

    D(bug("[cifs.netprefs] %s(%p)\n", __func__, NetPrefsBase));

    CIFSBase->npc_NetPrefsBase = NetPrefsBase;

    CIFSBase->npc_Module.npm_Node.ln_Name = "CIFS.Module";
    CIFSBase->npc_Module.npm_Node.ln_Pri  = 70;
    CIFSBase->npc_Module.npm_Startup      = CIFSStartup;
    CIFSBase->npc_Module.npm_Shutdown     = CIFSShutdown;

    RegisterProtoModule(&CIFSBase->npc_Module, CIFSBase);

    AROS_LIBFUNC_EXIT
}
