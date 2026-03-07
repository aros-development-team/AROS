/*
    Copyright (C) 2009-2026, The AROS Development Team. All rights reserved.

    ipv4_init.c - IPv4 protocol module initialisation.
    Loaded as net4.netprefs by the Network prefs editor.
*/

#include <aros/debug.h>

#include <proto/netprefs.h>

#include <aros/libcall.h>
#include <aros/asmcall.h>

#include "ipv4_intern.h"
#include "protocols.h"

/* Forward declarations from net4.c */
extern BOOL Net4Win_InitClass(struct MUI_CustomClass *PAWinCl);
extern void Net4Win_FreeClass(void);
extern struct MUI_CustomClass *Net4WinClass;
extern void Net4_WriteTokens(FILE *f, struct ProtocolAddress *pa);

/* -----------------------------------------------------------------------
 * IPv4Startup — called after all modules are loaded.
 * Retrieves PAWinClass from the main app and creates Net4WinClass.
 * ----------------------------------------------------------------------- */
static void IPv4Startup(struct NetPrefsBase *NetPrefsBase)
{
    struct NetPrefsIPv4Base *IPv4Base =
        (struct NetPrefsIPv4Base *)GetBase("IPv4.Module");

    D(bug("[net4.netprefs] %s: IPv4Base @ %p\n", __func__, IPv4Base));

    struct MUI_CustomClass *PAWinCl =
        (struct MUI_CustomClass *)GetBase("PAWin.Class");

    if (PAWinCl && Net4Win_InitClass(PAWinCl))
    {
        IPv4Base->npv4_WinClass = Net4WinClass;
        RegisterProtoHandler("IPv4", PROTO_FAMILY_IPV4,
                             Net4WinClass, Net4_WriteTokens);
    }
}

/* -----------------------------------------------------------------------
 * IPv4Shutdown — called on app exit.
 * ----------------------------------------------------------------------- */
static void IPv4Shutdown(struct NetPrefsBase *NetPrefsBase)
{
    D(bug("[net4.netprefs] %s()\n", __func__));
    Net4Win_FreeClass();
}

/* -----------------------------------------------------------------------
 * ModuleInit — library vector 5.
 * Called immediately after the module is loaded via OpenLibrary().
 * ----------------------------------------------------------------------- */
AROS_LH1(void, ModuleInit,
         AROS_LHA(struct NetPrefsBase *, NetPrefsBase, A0),
         struct NetPrefsIPv4Base *, IPv4Base, 5, Net4)
{
    AROS_LIBFUNC_INIT

    D(bug("[net4.netprefs] %s(%p)\n", __func__, NetPrefsBase));

    IPv4Base->npv4_NetPrefsBase = NetPrefsBase;

    IPv4Base->npv4_Module.npm_Node.ln_Name = "IPv4.Module";
    IPv4Base->npv4_Module.npm_Node.ln_Pri  = 90;
    IPv4Base->npv4_Module.npm_Startup      = IPv4Startup;
    IPv4Base->npv4_Module.npm_Shutdown     = IPv4Shutdown;

    RegisterProtoModule(&IPv4Base->npv4_Module, IPv4Base);

    AROS_LIBFUNC_EXIT
}
