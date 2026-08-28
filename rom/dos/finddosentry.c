/*
    Copyright (C) 1995-2026, The AROS Development Team. All rights reserved.

    Desc:
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
    struct DosList *prev = dlist;
    for (;;)
    {
        /* Get next entry. Return NULL if there is none. */
        dlist = BADDR(dlist->dol_Next);

        if (dlist == NULL)
        {
            return NULL;
        }

        /*
         * dol_Type indexes flagarray[dol_Type + 1] below. The array only
         * has entries for the valid node types (DLT_DEVICE..DLT_NONBINDING,
         * plus the -1 "never matches" slot), so a node whose dol_Type is
         * out of that range would read past the array and, with a wild
         * value, fault. That only happens if the list itself is damaged -
         * typically a node that was freed while still linked, whose fields
         * now hold allocator data. Stop the walk safely and report the bad
         * node (and the one that pointed at it) rather than crashing, so
         * the culprit can be identified from the log.
         */
        if (dlist->dol_Type < -1 || dlist->dol_Type > DLT_NONBINDING)
        {
            bug("[FindDosEntry] corrupt DosList node 0x%p (type 0x%p,"
                " next 0x%p) linked after 0x%p - stopping walk\n",
                dlist, (APTR)(IPTR)dlist->dol_Type,
                (APTR)(IPTR)dlist->dol_Next, prev);
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

        prev = dlist;
    }
    AROS_LIBFUNC_EXIT
} /* FindDosEntry */
