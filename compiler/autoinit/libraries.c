/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: autoinit library - automatic library opening/closing handling
    Lang: english
*/

#include <proto/exec.h>
#include <dos/dos.h>
#include <aros/symbolsets.h>
#include <intuition/intuition.h>
#include <dos/dosextens.h>
#include <proto/intuition.h>
#include <proto/dos.h>
#include <stdarg.h>

DEFINESET(LIBS);

int __forceerrorrequester __attribute__((weak)) = 0;

int __includelibrarieshandling;

void __showerror(int code, char *title, char *format, ...)
{
    AROS_GET_SYSBASE
    struct Process *me = (struct Process *)FindTask(0);

    va_list args;
    va_start(args, format);


    if (me->pr_CLI && !__forceerrorrequester)
    {
	if (DOSBase)
	{
	    PutStr(title);
	    PutStr(": ");
#warning This next line might break on bizarre architectures.
	    VPrintf(format, args);
	    PutStr("\n");
        }
    }
    else
    {
     	if (IntuitionBase)
	{
    	    struct EasyStruct es =
    	    {
		sizeof(struct EasyStruct),
		0,
		title,
		format,
		"Exit"
	    };

	    EasyRequestArgs(NULL, &es, NULL, (APTR)args);
	}
    }

    if (!IoErr()) SetIoErr(code);

    va_end(args);
}

int set_open_libraries(void)
{
    AROS_GET_SYSBASE
    struct libraryset **set = (struct libraryset **)SETNAME(LIBS);
    int n = 1;
    struct Process *me = (struct Process *)FindTask(0);

    while(set[n])
    {
	*(set[n]->baseptr) = OpenLibrary(set[n]->name, *set[n]->versionptr);
	if (!*(set[n]->baseptr))
	{
	    __showerror(ERROR_INVALID_RESIDENT_LIBRARY,
	                "Library error",
	                "Couldn't open version %ld of library \"%s\".",
		        *set[n]->versionptr, set[n]->name);
	    return 20;
	}

        if (set[n]->postopenfunc)
	{
	    int ret = set[n]->postopenfunc();
	    if (ret)
	    {
	    	__showerror(ERROR_INVALID_RESIDENT_LIBRARY,
		            "Library error",
	                    "Couldn't initialize library \"%s\".",
		            set[n]->name);
	        return ret;
	    }
        }

        n++;
    }
    return 0;
}

void set_close_libraries(void)
{
    AROS_GET_SYSBASE
    struct libraryset **set = (struct libraryset **)SETNAME(LIBS);
    int	n = ((int *)set)[0];

    while (n)
    {
	if (*(set[n]->baseptr))
	{
	    if (set[n]->preclosefunc) set[n]->preclosefunc();
	    CloseLibrary(*(set[n]->baseptr));
        }

	n--;
    }
}
