/*
    Copyright (C) 1995-2026, The AROS Development Team. All rights reserved.

    Desc: security.library - the part of the initialisation that needs
          dos.library. Called from the "security.boot" RTF_AFTERDOS
          resident (security_init.c) out of dos.library's CliInit(), once
          SYS: and the boot assigns exist; the library base itself is
          initialised much earlier so filesystem handlers can open it at
          mount time.
*/

#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/utility.h>
#include <proto/locale.h>

#include <proto/security.h>

#include "security_intern.h"
#include "security_task.h"
#include "security_server.h"
#include "security_memory.h"

#include LC_LIBDEFS_FILE

/*
 * Open the libraries we need, then create and start the server.
 */
BOOL Security_AfterDOS(struct SecurityBase *secBase)
{
    D(bug(DEBUG_NAME_STR " %s(%p)\n", __func__, secBase);)

    if (secBase->sec_AfterDOSDone)
        return TRUE;

    if (!(secBase->sec_DOSBase = TaggedOpenLibrary(TAGGEDOPEN_DOS)) ||
        !(secBase->sec_UtilityBase = TaggedOpenLibrary(TAGGEDOPEN_UTILITY)))
    {
        D(bug(DEBUG_NAME_STR " %s: failed to open dos/utility\n", __func__);)
        return FALSE;
    }
    /* Optional: intuition for requesters, locale for messages */
    secBase->sec_IntuitionBase = TaggedOpenLibrary(TAGGEDOPEN_INTUITION);
    secBase->sec_LocaleBase = OpenLibrary("locale.library", 37);
    secBase->LogInfo.li_LocaleBase = secBase->sec_LocaleBase;
    if (secBase->sec_LocaleBase)
    {
        secBase->LogInfo.li_Catalog = OpenCatalog(NULL, SECURITYCATALOGNAME,
                                                  OC_BuiltInLanguage, "english",
                                                  OC_Version, SECURITYCATALOGVERSION,
                                                  TAG_DONE);
    }

    secBase->sec_AfterDOSDone = TRUE;

    if (!CreateServer(secBase))
    {
        D(bug(DEBUG_NAME_STR " %s: failed to create the server\n", __func__);)
        return FALSE;
    }

    /* The server is owned by the system */
    SetTaskExtOwner(secBase, (struct Task *)secBase->Server, &RootExtOwner);

    if (!StartServer(secBase))
    {
        D(bug(DEBUG_NAME_STR " %s: failed to start the server\n", __func__);)
        return FALSE;
    }

    D(bug(DEBUG_NAME_STR " %s: server running\n", __func__);)

    return TRUE;
}
