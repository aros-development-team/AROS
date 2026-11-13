/*
    Copyright (C) 1995-2010, The AROS Development Team. All rights reserved.

    Desc: autoinit library - support function for showing errors to the user
*/

#include <exec/rawfmt.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/intuition.h>
#include <aros/autoinit.h>
#include <aros/debug.h>

#include <stdarg.h>

const int __forceerrorrequester __attribute__((weak)) = 0;

static AROS_UFH2(IPTR, RawPutc,
        AROS_UFHA(UBYTE, c, D0),
        AROS_UFHA(struct ExecBase *, SysBase, A3))
{
    AROS_USERFUNC_INIT
    if (c)
        RawPutChar(c);
    return c;
    AROS_USERFUNC_EXIT
}

void ___showerror(struct ExecBase *SysBase, const char *format, ...)
{
    struct IntuitionBase *IntuitionBase;
    struct DosLibrary *DOSBase = NULL;
    const char *name = FindTask(NULL)->tc_Node.ln_Name;

    AROS_SLOWSTACKFORMAT_PRE(format);

    if
    (
        !__forceerrorrequester                                                 &&
        (DOSBase = (struct DosLibrary *)OpenLibrary("dos.library", 0)) != NULL &&
        Cli() != NULL &&
        Output() != BNULL
    )
    {
        if (name)
        {
            PutStr(name);
            PutStr(": ");
        }

        VPrintf(format, AROS_SLOWSTACKFORMAT_ARG(format));

        PutStr("\n");

    }
    else
    if ((IntuitionBase = (struct IntuitionBase *)OpenLibrary("intuition.library", 0)))
    {
        struct EasyStruct es =
        {
            sizeof(struct EasyStruct),
            0,
            name,
            format,
            "Exit"
        };

        EasyRequestArgs(NULL, &es, NULL, AROS_SLOWSTACKFORMAT_ARG(format));

        CloseLibrary((struct Library *)IntuitionBase);
    }
    else
    {
        if (name) {
            RawDoFmt("%s: ", (RAWARG)&name, (VOID_FUNC)RawPutc, SysBase);
        }

        RawDoFmt(format, AROS_SLOWSTACKFORMAT_ARG(format), (VOID_FUNC)RawPutc, SysBase);
        RawPutChar('\n');
    }

    AROS_SLOWSTACKFORMAT_POST(format);

    CloseLibrary((struct Library *)DOSBase);
}
