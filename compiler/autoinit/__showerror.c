/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: autoinit library - support function for showing errors to the user
    Lang: english
*/

#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/intuition.h>
#include <aros/autoinit.h>

#include <stdarg.h>

int __forceerrorrequester __attribute__((weak)) = 0;

void __showerror(char *format, ...)
{
    AROS_GET_SYSBASE_OK
    
    va_list args;
    struct DosLibrary *DOSBase = NULL;
    const char *name = FindTask(NULL)->tc_Node.ln_Name;

    va_start(args, format);

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
#warning This next line might break on bizarre architectures.
        VPrintf(format, (IPTR *)args);
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

    va_end(args);
    
    if (DOSBase != NULL)
        CloseLibrary((struct Library *)DOSBase);
    
}

