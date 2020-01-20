/*
    Copyright © 2020, The AROS Development Team. All rights reserved.
    $Id$

    Desc:
    Lang: english
*/

#include <aros/debug.h>

#include <proto/exec.h>

#include <dos/dos.h>
#include <dos/dostags.h>
#include <exec/types.h>
#include <exec/lists.h>

#include "dos_intern.h"
#include "internalloadseg.h"

/*****************************************************************************

    NAME */
#include <proto/dos.h>

        AROS_LH2(ULONG, GetSegListInfo,

/*  SYNOPSIS */
        AROS_LHA(BPTR, seglist, D0),
        AROS_LHA(const struct TagItem *, taglist, A0),

/*  LOCATION */
        struct DosLibrary *, DOSBase, 196, Dos)

/*  FUNCTION
        returns information about a loaded seglist.

    INPUTS
        seglist - The segment list.

    RESULT
        returns number of tags aknowledged.

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO
        LoadSeg()

    INTERNALS

*****************************************************************************/
{
    AROS_LIBFUNC_INIT
    struct Node *segfound = NULL;
    ULONG count = 0;

    if ((seglist) && (taglist))
    {
        struct Node *segnode;

        ObtainSemaphoreShared(&((struct IntDosBase *)DOSBase)->segsem);
        ForeachNode(&((struct IntDosBase *)DOSBase)->segdata, segnode)
        {
            if (segnode->ln_Name == seglist)
            {
                segfound = segnode;
                break;
            }
        }
        if (segfound)
        {
            struct TagItem *tags  = (struct TagItem *)taglist;
            struct TagItem  *tag;

            D(bug("[DOS] %s: seglist info @ 0x%p\n", __func__, segfound);)
            while ((tag = NextTagItem(&tags)) != NULL)
            {
                switch(tag->ti_Tag)
                {
                    case GSLI_ElfHandle:
                        if ((tag->ti_Data) && (segfound->ln_Type == SEGTYPE_ELF))
                        {
                            IPTR *storage = (IPTR *)tag->ti_Data;
                            *storage = (IPTR)segfound->ln_Name;
                            count++;
                        }
                        break;

                    case GSLI_68KHUNK:
                        if ((tag->ti_Data) && (segfound->ln_Type == SEGTYPE_HUNK))
                        {
                            IPTR *storage = (IPTR *)tag->ti_Data;
                            *storage = (IPTR)segfound->ln_Name;
                            count++;
                        }
                        break;
                }
            }

        }
        ReleaseSemaphore(&((struct IntDosBase *)DOSBase)->segsem);
    }

    return count;

    AROS_LIBFUNC_EXIT
} /* GetSegListInfo */
