/*
    (C) 1995-96 AROS - The Amiga Replacement OS
    $Id$

    Desc:
    Lang: english
*/
#include "dos_intern.h"

/*****************************************************************************

    NAME */
#include <proto/dos.h>

	AROS_LH3(struct Segment *, FindSegment,

/*  SYNOPSIS */
	AROS_LHA(STRPTR          , name, D1),
	AROS_LHA(struct Segment *, seg, D2),
	AROS_LHA(LONG            , system, D3),

/*  LOCATION */
	struct DosLibrary *, DOSBase, 130, Dos)

/*  FUNCTION

    INPUTS

    RESULT

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

    HISTORY
	27-11-96    digulla automatically created from
			    dos_lib.fd and clib/dos_protos.h

*****************************************************************************/
{
    AROS_LIBFUNC_INIT
    AROS_LIBBASE_EXT_DECL(struct DosLibrary *,DOSBase)
    extern void aros_print_not_implemented (char *);

    aros_print_not_implemented ("FindSegment");

    return NULL;
    AROS_LIBFUNC_EXIT
} /* FindSegment */
