/*
    Copyright © 1995-2011, The AROS Development Team. All rights reserved.
    $Id$

    Desc:
    Lang: English
*/

#include <aros/debug.h>
#include <dos/dosextens.h>
#include <proto/utility.h>
#include "dos_intern.h"


/*****************************************************************************

    NAME */
#include <proto/dos.h>

        AROS_LH3(struct DosList *, FindDosEntry,

/*  SYNOPSIS */
        AROS_LHA(struct DosList *, dlist, D1),
        AROS_LHA(CONST_STRPTR,     name,  D2),
        AROS_LHA(ULONG,            flags, D3),

/*  LOCATION */
        struct DosLibrary *, DOSBase, 114, Dos)

/*  FUNCTION
        Looks for the next dos list entry with the right name. The list
        must be locked for this. There may be not more than one device
        or assign node of the same name. There are no such restrictions
        on volume nodes.

    INPUTS
        dlist - the value given by LockDosList() or the last call to
                FindDosEntry().
        name  - logical device name without colon. Case insensitive.
        flags - the same flags as given to LockDosList() or a subset
                of them.

    RESULT
        Pointer to dos list entry found or NULL if the are no more entries.

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    static const ULONG flagarray[]=
    { 0, LDF_DEVICES, LDF_ASSIGNS, LDF_VOLUMES, LDF_ASSIGNS, LDF_ASSIGNS };

    /* Determine the size of the name (-1 if the last character is a ':') */
    CONST_STRPTR end = name;
    ULONG size;

    if (!dlist)
        return NULL;

    while (*end++)
        ;

    size = ~(name-end);

    if (size && end[-2] == ':')
    {
        size--;
    }

    /* Follow the list */   
    for (;;)
    {
        /* Get next entry. Return NULL if there is none. */
        dlist = BADDR(dlist->dol_Next);

        if (dlist == NULL)
        {
            return NULL;
        }
        
        D(bug("[FindDosEntry] Found list entry 0x%p, '%b' type %d\n", dlist, dlist->dol_Name, dlist->dol_Type));

        /* Check type and name */
        if (flags & flagarray[dlist->dol_Type + 1] &&
            !CMPNICBSTR(name, dlist->dol_Name, size) &&
            !AROS_BSTR_ADDR(dlist->dol_Name)[size])
        {
            return dlist;
        }
    }
    AROS_LIBFUNC_EXIT
} /* FindDosEntry */
