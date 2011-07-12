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

DECLARESET(LIBS)
DEFINESET(LIBREQ)

AROS_MAKE_ASM_SYM(int, dummy, __includelibrarieshandling, 0);
AROS_EXPORT_ASM_SYM(__includelibrarieshandling);

static inline int int_strcmp(const char *a, const char *b)
{
    for (;*a && *b && (*a == *b);a++, b++);

    return *a-*b;
}
                             
int set_open_libraries_list(const void * const list[])
{
    int pset, preq;
    struct libraryset *set;
    struct libraryreq *req;

#if 0
    /* If list was an unresolved weak alias, then
     * it'll end up at address 0.
     */
    if (&list[0] == NULL)
        return 1;
#endif
    
    ForeachElementInSet(list, 1, pset, set)
    {
	BOOL do_not_fail = FALSE;
	int minversion = set->version;

	if (minversion < 0) {
	    do_not_fail = TRUE;
	    minversion = -(minversion + 1);
        }

        /* See what the maximum requested version is for this
         * library
         */
        ForeachElementInSet(SETNAME(LIBREQ), 1, preq, req) {
            if (int_strcmp(req->name, set->name) != 0)
                continue;
            if (req->version > minversion) {
                minversion = req->version;
            }
        }

        *set->baseptr = OpenLibrary(set->name, minversion);
	
	if (!do_not_fail && *set->baseptr == NULL)
	{
	    __showerror
	    (
	        "Could not open version %ld or higher of library \"%s\".",
		(const IPTR []){set->version, (IPTR)set->name}
	    );

	    return 0;
	}
    }

    return 1;
}

void set_close_libraries_list(const void * const list[])
{
    int pos;
    struct libraryset *set;
   
#if 0
    /* If list was an unresolved weak alias, then
     * it'll end up at address 0.
     */
    if (&list[0] == NULL)
        return;
#endif

     ForeachElementInSet(SETNAME(LIBS), 1, pos, set)
    {
	if (*set->baseptr)
        {
	    CloseLibrary(*set->baseptr);
            *set->baseptr = NULL;
        }
    }
}


