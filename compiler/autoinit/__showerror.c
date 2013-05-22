/*
    Copyright � 1995-2010, The AROS Development Team. All rights reserved.
    $Id$

    Desc: autoinit library - support function for showing errors to the user
    Lang: english
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

void ___showerror(char *format, const IPTR *args, struct ExecBase *SysBase)
{
    struct IntuitionBase *IntuitionBase;
    struct DosLibrary *DOSBase = NULL;
    const char *name = FindTask(NULL)->tc_Node.ln_Name;

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

        if (args)
            VPrintf(format, (IPTR*)args);
        else
            PutStr(format);

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

	EasyRequestArgs(NULL, &es, NULL, (APTR)args);

	CloseLibrary((struct Library *)IntuitionBase);
    }
    else
    {
        if (name) {
            IPTR args[] = { (IPTR)name };
            RawDoFmt("%s: ", (APTR)args, (VOID_FUNC)RawPutc, SysBase);
        }

        RawDoFmt(format, (APTR)args, (VOID_FUNC)RawPutc, SysBase);
        RawPutChar('\n');
    }

    CloseLibrary((struct Library *)DOSBase);
}
