/*
    Copyright © 1995-2007, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Add a segment to the resident list.
    Lang: english
*/
#include "dos_intern.h"
#include <string.h>
#include <proto/exec.h>

/*****************************************************************************

    NAME */
#include <dos/dosextens.h>
#include <proto/dos.h>

	AROS_LH3(BOOL, AddSegment,

/*  SYNOPSIS */
	AROS_LHA(CONST_STRPTR, name, D1),
	AROS_LHA(BPTR  , seg, D2),
	AROS_LHA(LONG  , type, D3),

/*  LOCATION */
	struct DosLibrary *, DOSBase, 129, Dos)

/*  FUNCTION
	Adds a program segment to the system resident list. You can later
	use these segments to run programs.

	The name field should refer to a NULL terminated strings, which
	will be copied. The type field determines the type of resident
	program. Normal programs should have type >= 0, system segments
	should have type == CMD_SYSTEM.

	Note that all other values of type are reserved.

    INPUTS
	name		- Name of the segment. This is used by FindSegment().
	seg		- Segment to add.
	type		- What type of segment (initial use count).

    RESULT
	Segment will have been added to the DOS resident list.

	!= 0	success
	== 0	failure

    NOTES

    EXAMPLE

    BUGS
	Uses Forbid() based locking.

    SEE ALSO
	FindSegment(), RemSegment()

    INTERNALS

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    struct Segment *sptr;
    int namelen = strlen(name)+1;

    sptr = AllocVec(sizeof(struct Segment) + namelen - 4,
		    MEMF_CLEAR | MEMF_PUBLIC);

    if( sptr != NULL )
    {
	sptr->seg_UC = type;
	sptr->seg_Seg = seg;

#ifdef AROS_FAST_BPTR
	CopyMem(name, sptr->seg_Name, namelen);
#else
	CopyMem(name, &sptr->seg_Name[1], namelen);
	*sptr->seg_Name = namelen;
#endif

	/* Sigh, we just add the segment to the start of the list */
	Forbid();

	sptr->seg_Next = DOSBase->dl_ResList;
	DOSBase->dl_ResList = MKBADDR(sptr);

	Permit();

	return TRUE;
    }

    return FALSE;

    AROS_LIBFUNC_EXIT
} /* AddSegment */
