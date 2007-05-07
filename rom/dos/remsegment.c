/*
    Copyright © 1995-2007, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Remove a segment from the system list.
    Lang: English
*/

#include "dos_intern.h"
#include <proto/exec.h>

/*****************************************************************************

    NAME */
#include <dos/dosextens.h>
#include <proto/dos.h>

	AROS_LH1(LONG, RemSegment,

/*  SYNOPSIS */
	AROS_LHA(struct Segment *, seg, D1),

/*  LOCATION */
	struct DosLibrary *, DOSBase, 131, Dos)

/*  FUNCTION
	Remove the segment seg from the DOS resident command list.

	The segment to be removed should be in the list, and should
	have a usercount of 0. System or internal segment cannot be
	removed (although they can be replaced).

    INPUTS
	seg		- Segment to remove.

    RESULT
	!= 0	Segment was removed
	== 0	Segment was not removed (not in list, or not free).

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO
	AddSegment(), FindSegment()

    INTERNALS

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    /* Make sure segment is freeable */
#if AROS_FLAVOUR & AROS_FLAVOUR_BINCOMPAT
    if (seg->seg_UC == 0 || seg->seg_UC == 1)
#else
    if (seg->seg_UC == 0)
#endif
    {
	struct Segment *next, *prev;
	prev = NULL;
	next = BADDR(DOSBase->dl_ResList);
	while (next != NULL)
	{
	    if (next == seg)
	    {
		if (prev)
		{
		    prev->seg_Next = next->seg_Next;
		}
		else
		{
		    DOSBase->dl_ResList = MKBADDR(next->seg_Next);
		}

		return DOSTRUE;
	    }

	    prev = next;
	    next = BADDR(next->seg_Next);
	}
    }

    return DOSFALSE;

    AROS_LIBFUNC_EXIT
} /* RemSegment */
