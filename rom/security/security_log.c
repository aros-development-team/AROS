/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: security.library - log.resource provider

          Everything the library logs (login, password and verification
          results, warnings, start-up) is also posted to log.resource as
          provider "security.library", sub-component "Security.server",
          "Security.auth" or "" and a type that matches the event. The
          Security.log file in the configuration directory is still written
          as before; log.resource is where a system logger or a viewer
          picks the events up without reading that file.

          log.resource is in ROM ahead of this library, but its service task
          may not run yet when we start: the provider handle is created on
          first use. Producers share one handle, whose pool is not
          protected, so every entry is added under LogSem.
*/

#include <proto/exec.h>
#include <proto/log.h>

#include "security_intern.h"
#include "security_log.h"

#define LOGLEVEL        (50)

static APTR GetLogHandle(struct SecurityBase *secBase)
{
    APTR LogResBase;

    if (!secBase->sec_LogHandle && (LogResBase = secBase->sec_LogResBase))
    {
        secBase->sec_LogNode.ln_Name = (STRPTR)"security.library";
        secBase->sec_LogHandle = logInitialise(&secBase->sec_LogNode);
    }
    return secBase->sec_LogHandle;
}

void InitLogging(struct SecurityBase *secBase)
{
    secBase->sec_LogResBase = OpenResource("log.resource");
    secBase->sec_LogHandle = NULL;
}

void VSecLog(struct SecurityBase *secBase, ULONG type, CONST_STRPTR sub, CONST_STRPTR fmt, SIPTR *argv)
{
    APTR LogResBase = secBase->sec_LogResBase;
    APTR handle;

    if (!LogResBase || !fmt)
        return;
    ObtainSemaphore(&secBase->LogSem);
    if ((handle = GetLogHandle(secBase)))
        logAddEntryA(type | LOGLEVEL, handle, (STRPTR)(sub ? sub : (CONST_STRPTR)""), (STRPTR)"", 0, (STRPTR)fmt, (RAWARG)argv);
    ReleaseSemaphore(&secBase->LogSem);
}
