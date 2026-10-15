/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: pam.library - logging through log.resource

          Modules' PamLog() messages and the library's own (an unusable
          service file, a module that is refused) are posted to
          log.resource as provider "pam.library", sub-component the
          service name. Without log.resource they only go to the debug
          output. Control characters from callers' strings are replaced so
          that a user name cannot forge further lines.
*/

#include <proto/exec.h>
#include <proto/log.h>
#include <resources/log.h>
#include <string.h>

#include "pam_intern.h"

#define LOGLEVEL        (50)

struct LogBuf
{
    STRPTR  Ptr;
    ULONG   Left;
};

AROS_UFH2(static void, LogPutCh,
    AROS_UFHA(UBYTE, c, D0),
    AROS_UFHA(struct LogBuf *, buf, A3))
{
    AROS_USERFUNC_INIT

    if (buf->Left > 1)
    {
        *buf->Ptr++ = c;
        buf->Left--;
    }
    else if (buf->Left == 1)
        *buf->Ptr = '\0';

    AROS_USERFUNC_EXIT
}

void PamInitLog(struct PamBase *PamBase)
{
    InitSemaphore(&PamBase->LogSem);
    PamBase->LogResBase = OpenResource("log.resource");
    PamBase->LogHandle = NULL;
}

static void PostLine(struct PamBase *PamBase, struct PamHandle *h, ULONG type, STRPTR text)
{
    APTR LogResBase = PamBase->LogResBase;
    UBYTE *p;
    IPTR args[1];

    for (p = (UBYTE *)text; *p; p++)
    {
        if (*p < 0x20 || (*p >= 0x7f && *p < 0xa0))
            *p = '?';
    }
    D(bug(DEBUG_NAME_STR " %s: %s\n", (h && h->Service) ? h->Service : (STRPTR)"-", text);)
    if (!LogResBase)
        return;

    ObtainSemaphore(&PamBase->LogSem);
    if (!PamBase->LogHandle)
    {
        PamBase->LogNode.ln_Name = (STRPTR)PAMNAME;
        PamBase->LogHandle = logInitialise(&PamBase->LogNode);
    }
    if (PamBase->LogHandle)
    {
        args[0] = (IPTR)text;
        logAddEntryA(type | LOGLEVEL, PamBase->LogHandle, (STRPTR)((h && h->Service) ? h->Service : (STRPTR)""),
                     (STRPTR)"", 0, (STRPTR)"%s", (RAWARG)args);
    }
    ReleaseSemaphore(&PamBase->LogSem);
}

/* RawDoFmt-style format with an array of IPTR arguments */
void PamLogA(struct PamBase *PamBase, struct PamHandle *h, ULONG type, CONST_STRPTR fmt, IPTR *args)
{
    char text[256];
    struct LogBuf out = { text, sizeof(text) };

    RawDoFmt(fmt, (RAWARG)args, (VOID_FUNC)LogPutCh, &out);
    text[sizeof(text) - 1] = '\0';
    PostLine(PamBase, h, type, text);
}

/*****************************************************************************

    NAME */
        AROS_LH3(void, PamLog,

/*  SYNOPSIS */
        AROS_LHA(struct PamHandle *, handle, A0),
        AROS_LHA(LONG, level, D0),
        AROS_LHA(CONST_STRPTR, text, A1),

/*  LOCATION */
        struct PamBase *, PamBase, 22, Pam)

/*  FUNCTION
        Module side: record a message. It is posted to log.resource
        (provider "pam.library", the service as sub-component) together
        with the user, when the resource is present, and to the debug
        output.

    INPUTS
        level - PAM_LOG_ERROR, PAM_LOG_WARNING, PAM_LOG_INFO, PAM_LOG_DEBUG.
        text  - the message; control characters are replaced.

******************************************************************************/
{
    AROS_LIBFUNC_INIT

    static const ULONG types[] = { LOGF_Flag_Type_Error, LOGF_Flag_Type_Warn, LOGF_Flag_Type_Information, LOGF_Flag_Type_Debug };
    struct PamHandle *h = handle;
    char line[256];
    ULONG n;

    if (!text)
        return;
    n = 0;
    if (h && h->User)
    {
        strncpy(line, h->User, 64);
        line[64] = '\0';
        n = strlen(line);
        line[n++] = ':';
        line[n++] = ' ';
    }
    strncpy(line + n, text, sizeof(line) - n - 1);
    line[sizeof(line) - 1] = '\0';
    PostLine(PamBase, h, types[(level >= 0 && level <= 3) ? level : 3], line);

    AROS_LIBFUNC_EXIT
}
