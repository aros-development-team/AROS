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

int __forceerrorrequester __attribute__((weak)) = 0;

void __showerror(char *format, const IPTR *args)
{
    struct IntuitionBase *IntuitionBase;
    struct DosLibrary *DOSBase = NULL;
    const char *name = FindTask(NULL)->tc_Node.ln_Name;

    if
    (
        !__forceerrorrequester                                                 &&
        (DOSBase = (struct DosLibrary *)OpenLibrary("dos.library", 0)) != NULL &&
	Cli() != NULL
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

        CloseLibrary((struct Library *)DOSBase);
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
        if (name)
            kprintf("%s: ", name);

        if (args)
        {
            RawDoFmt(format, (APTR)args, (VOID_FUNC)RAWFMTFUNC_SERIAL, NULL);
            kprintf("\n");
        }
        else
            kprintf("%s\n", format);
    }
}
