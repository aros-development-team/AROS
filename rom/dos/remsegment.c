/*
    Copyright (C) 1995-1998 AROS - The Amiga Replacement OS
    $Id$

    Desc: Remove a segment from the system list.
    Lang: english
*/
#include "dos_intern.h"

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

    HISTORY
	27-11-96    digulla automatically created from
			    dos_lib.fd and clib/dos_protos.h

*****************************************************************************/
{
    AROS_LIBFUNC_INIT
    AROS_LIBBASE_EXT_DECL(struct DosLibrary *,DOSBase)

    /* Make sure segment is freeable */
    if( seg->seg_UC == 0 )
    {
	struct Segment *next, *prev;
	prev = NULL;
	next = BADDR(DOSBase->dl_ResList);
	while( next != NULL )
	{
	    if( next == seg )
	    {
		if( prev )
		{
		    prev->seg_Next = next->seg_Next;
		}
		else
		{
		    DOSBase->dl_ResList = MKBADDR(next->seg_Next);
		}
		return TRUE;
	    }
	    prev = next;
	    next = BADDR(next->seg_Next);
	}
    }
    return FALSE;

    AROS_LIBFUNC_EXIT
} /* RemSegment */
