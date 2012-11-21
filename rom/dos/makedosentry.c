/*
    Copyright © 1995-2007, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Creates an entry for the dos list.
    Lang: english
*/
#include <exec/memory.h>
#include <proto/exec.h>

#include <string.h>

#include "dos_intern.h"

/*****************************************************************************

    NAME */
#include <proto/dos.h>

        AROS_LH2(struct DosList *, MakeDosEntry,

/*  SYNOPSIS */
        AROS_LHA(CONST_STRPTR, name, D1),
        AROS_LHA(LONG,         type, D2),

/*  LOCATION */
        struct DosLibrary *, DOSBase, 116, Dos)

/*  FUNCTION
        Create an entry for the dos list. Depending on the type this may
        be a device, a volume or an assign node.

    INPUTS
        name  --  pointer to name
        type  --  type of list entry to create

    RESULT

        The new device entry, or NULL if it couldn't be created.

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    AddDosEntry(), RemDosEntry(), FindDosEntry(), LockDosList(),
    NextDosEntry(), FreeDosEntry()

    INTERNALS

    This call should be replaced by a value for AllocDosObject().

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    ULONG len = strlen(name);
    STRPTR s2;
    struct DosList *dl;

    dl = (struct DosList *)AllocVec(sizeof(struct DosList),
                                    MEMF_PUBLIC | MEMF_CLEAR);

    if (dl != NULL)
    {
#ifdef AROS_FAST_BPTR
        s2 = (STRPTR)AllocVec(len+1, MEMF_PUBLIC | MEMF_CLEAR);
        dl->dol_Name = MKBADDR(s2);
#else
        /* Binary compatibility for BCPL string.
         * First byte is the length then comes the string.
         * For ease of use a zero is put at the end so it can be used as a
         * C string
         */
        s2 = (STRPTR)AllocVec(len+2, MEMF_PUBLIC | MEMF_CLEAR);
        dl->dol_Name = MKBADDR(s2);
        if (s2 != NULL)
            *s2++ = (UBYTE)(len > 255 ? 255 : len);
#endif
        if (s2 != NULL)
        {
            strcpy(s2, name);
            dl->dol_Type = type;
            return dl;
        }
        else
        {
            SetIoErr(ERROR_NO_FREE_STORE);
        }
        
        FreeVec(dl);
    }
    else
    {
        SetIoErr(ERROR_NO_FREE_STORE);
    }

    return NULL;

    AROS_LIBFUNC_EXIT
} /* MakeDosEntry */
