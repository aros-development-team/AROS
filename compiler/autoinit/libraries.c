/*
    Copyright (C) 1995-2000 AROS - The Amiga Research OS
    $Id$

    Desc: autoinit library - automatic library opening/closing handling
    Lang: english
*/

#include <intuition/intuition.h>
#include <dos/dosextens.h>
#include <proto/intuition.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <aros/symbolsets.h>
#include <stdarg.h>

/*
  Redefine this variable in your program and set
  its value to a value different than 0 to force
  the use of a requester in case of error
*/
int __forceerrorrequester __attribute__((weak)) = 0;

static void showerror(char *title, char *format, ...)
{
    struct Process *me = (struct Process *)FindTask(0);

    va_list args;
    va_start(args, format);


    if (me->pr_CLI && !__forceerrorrequester)
    {
    	if (DOSBase)
	{
	    VPrintf(title, NULL);
	    VPrintf(": ", NULL);
	    VPrintf(format, args);
	    VPrintf("\n", NULL);
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

    va_end(args);
}

int set_open_libraries(struct libraryset *set[])
{
    int n = 1;
    struct Process *me = (struct Process *)FindTask(0);

    while(set[n])
    {
	*(set[n]->baseptr) = OpenLibrary(set[n]->name, *set[n]->versionptr);
	if (!*(set[n]->baseptr))
	{
	    showerror("Library error",
	              "Couldn't open version %ld of library \"%s\".",
		       *set[n]->versionptr, set[n]->name);
	    return 20;
	}

        if (set[n]->postopenfunc)
	{
	    int ret = set[n]->postopenfunc();
	    if (ret)
	    {
	    	showerror("Library error",
	                  "Couldn't initialize library \"%s\".",
		           set[n]->name);
	        return ret;
	    }
        }

        n++;
    }
    return 0;
}

void set_close_libraries(struct libraryset *set[])
{
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
