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

int __includelibrarieshandling = 0;
                             
int set_open_libraries(void)
{
    AROS_GET_SYSBASE_OK
    
    int pos;
    struct libraryset *set;
    
    ForeachElementInSet(SETNAME(LIBS), 1, pos, set)
    {
        *set->baseptr = OpenLibrary(set->name, *set->versionptr);
	if (!*set->baseptr)
	{
	    __showerror
	    (
	        "Couldn't open version %ld of library \"%s\".",
		*set->versionptr, set->name
	    );

	    return 0;
	}
    }

    return 1;
}

void set_close_libraries(void)
{
    AROS_GET_SYSBASE_OK
    
    int pos;
    struct libraryset *set;
    
    ForeachElementInSet(SETNAME(LIBS), 1, pos, set)
    {
	if (*set->baseptr)
	    CloseLibrary(*set->baseptr);
    }
}


