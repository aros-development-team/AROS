/*
    Copyright � 1995-2007, The AROS Development Team. All rights reserved.
    $Id$

    Desc: autoinit library - support function for showing errors to the user
    Lang: english
*/

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
            VPrintf(format, args);
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

	EasyRequestArgs(NULL, &es, NULL, args);

	CloseLibrary((struct Library *)IntuitionBase);
    }
    else
    {
        if (name)
            kprintf("%s: ", name);

//        if (args) {
//            vkprintf(format, args);
//            kprintf("\n");
//        }
//        else
            kprintf("%s\n", format);
    }

    if (DOSBase != NULL)
        CloseLibrary((struct Library *)DOSBase);

}

