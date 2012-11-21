/*
    Copyright © 1995-2012, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Find a resident segment.
    Lang: english
*/

#include <aros/debug.h>
#include "dos_intern.h"
#include <string.h>

/*****************************************************************************

    NAME */
#include <dos/dosextens.h>
#include <proto/dos.h>

        AROS_LH3(struct Segment *, FindSegment,

/*  SYNOPSIS */
        AROS_LHA(CONST_STRPTR    , name, D1),
        AROS_LHA(struct Segment *, seg, D2),
        AROS_LHA(BOOL            , system, D3),

/*  LOCATION */
        struct DosLibrary *, DOSBase, 130, Dos)

/*  FUNCTION
        Search for a resident segment by name and type. FindSegment() will
        return the first segment that exactly matches the name and type.

        You can continue searching by specifying the last returned segment
        as the seg argument.

    INPUTS
        name            - Name of the segment to search for.
        seg             - Start search from this point.
        system          - Search for a system segment.

    RESULT
        Will return the segment structure if a match is found, otherwise
        will return NULL.

    NOTES
        FindSegment() does no locking of the segment list. You should 
        lock yourself. FindSegment() also does not increment the value
        of the seg_UC field. If the value of seg_UC > 0, you MUST 
        perform user counting in order to prevent the segment from being
        unloaded.

    EXAMPLE

    BUGS

    SEE ALSO
        AddSegment(), RemSegment()

    INTERNALS

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    struct DosInfo *dinf = BADDR(DOSBase->dl_Root->rn_Info);

    /* Segment seg was the last match, let's start from the next one */
    if( seg != NULL )
        seg = BADDR(seg->seg_Next);
    else
        seg = BADDR(dinf->di_ResList);

    while( seg != NULL )
    {
        D(bug("[FindSegment] Checking segment '%s'\n",
            AROS_BSTR_ADDR(MKBADDR(&seg->seg_Name[0]))));
        if
        (
            (system || (system == FALSE && (seg->seg_UC >=0)))  &&
            (Stricmp( name, AROS_BSTR_ADDR(MKBADDR(&seg->seg_Name[0]))) == 0)
        )
        {
                /* We have a matching segment */
                return seg;
        }
        seg = BADDR(seg->seg_Next);
    }

    return NULL;
    AROS_LIBFUNC_EXIT
} /* FindSegment */
