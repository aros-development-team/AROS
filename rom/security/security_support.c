/*
    Copyright (C) 2002-2026, The AROS Development Team. All rights reserved.

    Desc: security.library support functions (warnings, fatal errors)
*/

#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/intuition.h>

#include <intuition/intuition.h>

#include "security_intern.h"
#include "security_support.h"

struct FmtCtx
{
    STRPTR      dst;
    ULONG       left;
};

AROS_UFH2(static void, FmtPutCh,
    AROS_UFHA(UBYTE, ch, D0),
    AROS_UFHA(struct FmtCtx *, ctx, A3))
{
    AROS_USERFUNC_INIT

    if (ctx->left > 1 && ch)
    {
        *ctx->dst++ = ch;
        ctx->left--;
    }
    *ctx->dst = '\0';

    AROS_USERFUNC_EXIT
}

void FormatString(CONST_STRPTR fmt, SIPTR *args, STRPTR dst, ULONG dstsize)
{
    struct FmtCtx ctx = { dst, dstsize };
    char fixed[256];

    FixFormat(fmt, fixed, sizeof(fixed));
    dst[0] = '\0';
    RawDoFmt(fixed, (RAWARG)args, (VOID_FUNC)FmtPutCh, &ctx);
}

/*
 * Post a warning. The text is formatted from a RawDoFmt style format
 * string and a mem stream of SIPTR arguments. An EasyRequest is used when a
 * screen exists, otherwise the message only goes to the debug output.
 */
void Warn(struct SecurityBase *secBase, CONST_STRPTR fmt, SIPTR *args)
{
    char text[512];
    struct LocaleInfo li;

    FormatString(fmt, args, text, sizeof(text));

    bug(DEBUG_NAME_STR " WARNING: %s\n", text);

    if (secBase->sec_AfterDOSDone && IntuitionBase && IntuitionBase->FirstScreen)
    {
        struct EasyStruct es;
        SIPTR eargs[1];

        OpenLoc(secBase, &li);
        es.es_StructSize = sizeof(struct EasyStruct);
        es.es_Flags = 0;
        es.es_Title = (STRPTR)GetLocS(secBase, &li, MSG_WARNING_GUI);
        es.es_TextFormat = "%s";
        es.es_GadgetFormat = (STRPTR)GetLocS(secBase, &li, MSG_RESUME);
        eargs[0] = (SIPTR)text;
        EasyRequestArgs(NULL, &es, NULL, (RAWARG)eargs);
        CloseLoc(secBase, &li);
    }
}

/*
 * Fatal error. Marks the library as violated so that all credential queries
 * return "nobody", shows an alert and returns.
 */
void Die(struct SecurityBase *secBase, CONST_STRPTR msg, ULONG alertcode)
{
    if (msg)
        bug(DEBUG_NAME_STR " FATAL: %s\n", msg);
    else
        bug(DEBUG_NAME_STR " FATAL: alert 0x%08lx\n", (unsigned long)alertcode);

    if (secBase)
        secBase->SecurityViolation = TRUE;

    if (alertcode)
        Alert(alertcode);
    else if (msg && secBase && secBase->sec_AfterDOSDone && IntuitionBase && IntuitionBase->FirstScreen)
    {
        struct EasyStruct es;
        SIPTR args[1];

        es.es_StructSize = sizeof(struct EasyStruct);
        es.es_Flags = 0;
        es.es_Title = "Security Fatal Error";
        es.es_TextFormat = "%s";
        es.es_GadgetFormat = "OK";
        args[0] = (SIPTR)msg;
        EasyRequestArgs(NULL, &es, NULL, (RAWARG)args);
    }
}
