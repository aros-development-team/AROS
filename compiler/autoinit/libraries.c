/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: autoinit library - automatic library opening/closing handling
    Lang: english
*/

#include <proto/exec.h>
#include <dos/dos.h>
#include <aros/symbolsets.h>
#include <aros/autoinit.h>
#include <intuition/intuition.h>
#include <dos/dosextens.h>
#include <proto/intuition.h>
#include <proto/dos.h>
#include <stdlib.h>

DECLARESET(LIBS);

int __includelibrarieshandling;

int set_open_libraries(void)
{
    AROS_GET_SYSBASE_OK
    struct libraryset **set = (struct libraryset **)SETNAME(LIBS);
    int n = 1;

    while(set[n])
    {
	*(set[n]->baseptr) = OpenLibrary(set[n]->name, *set[n]->versionptr);
	if (!*(set[n]->baseptr))
	{
	    __showerror
	    (
	        "Couldn't open version %ld of library \"%s\".",
		*set[n]->versionptr, set[n]->name
	    );

	    return 20;
	}

        n++;
    }

    return 0;
}

void set_close_libraries(void)
{
    AROS_GET_SYSBASE_OK
    struct libraryset **set = (struct libraryset **)SETNAME(LIBS);
    int	n = ((int *)set)[0];

    while (n)
    {
	if (*(set[n]->baseptr))
	    CloseLibrary(*(set[n]->baseptr));

	n--;
    }
}
