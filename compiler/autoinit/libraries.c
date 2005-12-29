/*
    Copyright © 1995-2005, The AROS Development Team. All rights reserved.
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

AROS_MAKE_ASM_SYM(int, dummy, __includelibrarieshandling, 0);
AROS_EXPORT_ASM_SYM(__includelibrarieshandling);
                             
int set_open_libraries_list(const void *list[])
{
    int pos;
    struct libraryset *set;
    
    ForeachElementInSet(list, 1, pos, set)
    {
        LONG version = *set->versionptr;
	BOOL do_not_fail = 0;
	
	if (version < 0)
	{
	    version = -(version + 1); 
	    do_not_fail = 1;
	}
	
        *set->baseptr = OpenLibrary(set->name, version);
	
	if (!do_not_fail && *set->baseptr == NULL)
	{
	    __showerror
	    (
	        "Could not open version %ld or higher of library \"%s\".",
		version, set->name
	    );

	    return 0;
	}
    }

    return 1;
}

void set_close_libraries_list(const void *list[])
{
    int pos;
    struct libraryset *set;
    
    ForeachElementInSet(SETNAME(LIBS), 1, pos, set)
    {
	if (*set->baseptr)
	    CloseLibrary(*set->baseptr);
    }
}


