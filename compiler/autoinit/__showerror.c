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
    struct Process *me     = (struct Process *)FindTask(0);
    char            *pname  = __getprogramname();

    va_list args;
    va_start(args, format);

    if (me->pr_CLI && !__forceerrorrequester)
    {
        if (pname)
	{
            PutStr(pname);
            PutStr(": ");
	}
#warning This next line might break on bizarre architectures.
        VPrintf(format, (IPTR *)args);
        PutStr("\n");
    }
    else
    {
     	if ((IntuitionBase = (struct IntuitionBase *)OpenLibrary("intuition.library", 0)))
	{
    	    struct EasyStruct es =
    	    {
		sizeof(struct EasyStruct),
		0,
		pname,
		format,
		"Exit"
	    };

	    EasyRequestArgs(NULL, &es, NULL, (APTR)args);

	    CloseLibrary((struct Library *)IntuitionBase);
	}
    }

    va_end(args);

    if (pname)
        FreeVec(pname);
}

