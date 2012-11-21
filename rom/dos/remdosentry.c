/*
    Copyright © 1995-2010, The AROS Development Team. All rights reserved.
    $Id$

    Desc:
    Lang: english
*/
#include <dos/dosextens.h>
#include <proto/exec.h>

/*****************************************************************************

    NAME */
#include <proto/dos.h>

        AROS_LH1(LONG, RemDosEntry,

/*  SYNOPSIS */
        AROS_LHA(struct DosList *, dlist, D1),

/*  LOCATION */
        struct DosLibrary *, DOSBase, 112, Dos)

/*  FUNCTION
        Removes a given dos list entry from the dos list. Automatically
        locks the list for writing.

    INPUTS
        dlist - pointer to dos list entry.

    RESULT
        !=0 if all went well, 0 otherwise.

    NOTES
        Since anybody who wants to use a device or volume node in the
        dos list has to lock the list, filesystems may be called with
        the dos list locked. So if you want to add a dos list entry
        out of a filesystem don't just wait on the lock but serve all
        incoming requests until the dos list is free instead.

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

*****************************************************************************/
{
    AROS_LIBFUNC_INIT
    struct DosList *dl;

    if(dlist == NULL)
        return 0;

    dl = LockDosList(LDF_ALL | LDF_WRITE);

    while(TRUE)
    {
        struct DosList *dl2 = BADDR(dl->dol_Next);

        if(dl2 == dlist)
        {
            dl->dol_Next = dlist->dol_Next;
            break;
        }

        dl = dl2;
    }

    UnLockDosList(LDF_ALL | LDF_WRITE);

    return 1;

    AROS_LIBFUNC_EXIT
} /* RemDosEntry */
