/*
    Copyright (C) 2009-2026, The AROS Development Team. All rights reserved.

    ipv6_init.c - IPv6 protocol module initialisation.
    Loaded as net6.netprefs by the Network prefs editor.
*/

#include <aros/debug.h>

#include <proto/netprefs.h>

#include <aros/libcall.h>
#include <aros/asmcall.h>

#include "ipv6_intern.h"
#include "protocols.h"

/* Forward declarations from net6.c */
extern BOOL Net6Win_InitClass(struct MUI_CustomClass *PAWinCl);
extern void Net6Win_FreeClass(void);
extern struct MUI_CustomClass *Net6WinClass;
extern void Net6_WriteTokens(FILE *f, struct ProtocolAddress *pa);

/* -----------------------------------------------------------------------
 * IPv6Startup — called after all modules are loaded.
 * Retrieves PAWinClass from the main app and creates Net6WinClass.
 * ----------------------------------------------------------------------- */
static void IPv6Startup(struct NetPrefsBase *NetPrefsBase)
{
    struct NetPrefsIPv6Base *IPv6Base =
        (struct NetPrefsIPv6Base *)GetBase("IPv6.Module");

    D(bug("[net6.netprefs] %s: IPv6Base @ %p\n", __func__, IPv6Base));

    struct MUI_CustomClass *PAWinCl =
        (struct MUI_CustomClass *)GetBase("PAWin.Class");

    if (PAWinCl && Net6Win_InitClass(PAWinCl))
    {
        IPv6Base->npv6_WinClass = Net6WinClass;
        RegisterProtoHandler("IPv6", PROTO_FAMILY_IPV6,
                             Net6WinClass, Net6_WriteTokens);
    }
}

/* -----------------------------------------------------------------------
 * IPv6Shutdown — called on app exit.
 * ----------------------------------------------------------------------- */
static void IPv6Shutdown(struct NetPrefsBase *NetPrefsBase)
{
    D(bug("[net6.netprefs] %s()\n", __func__));
    Net6Win_FreeClass();
}

/* -----------------------------------------------------------------------
 * ModuleInit — library vector 5.
 * Called immediately after the module is loaded via OpenLibrary().
 * ----------------------------------------------------------------------- */
AROS_LH1(void, ModuleInit,
         AROS_LHA(struct NetPrefsBase *, NetPrefsBase, A0),
         struct NetPrefsIPv6Base *, IPv6Base, 5, Net6)
{
    AROS_LIBFUNC_INIT

    D(bug("[net6.netprefs] %s(%p)\n", __func__, NetPrefsBase));

    IPv6Base->npv6_NetPrefsBase = NetPrefsBase;

    IPv6Base->npv6_Module.npm_Node.ln_Name = "IPv6.Module";
    IPv6Base->npv6_Module.npm_Node.ln_Pri  = 80;
    IPv6Base->npv6_Module.npm_Startup      = IPv6Startup;
    IPv6Base->npv6_Module.npm_Shutdown     = IPv6Shutdown;

    RegisterProtoModule(&IPv6Base->npv6_Module, IPv6Base);

    AROS_LIBFUNC_EXIT
}
