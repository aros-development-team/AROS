/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc:
    Lang: english
*/
#include "iffparse_intern.h"

/*****************************************************************************

    NAME */
#include <proto/iffparse.h>

	AROS_LH1(void, InitIFFasClip,

/*  SYNOPSIS */
	AROS_LHA(struct IFFHandle *, iff, A0),

/*  LOCATION */
	struct Library *, IFFParseBase, 40, IFFParse)

/*  FUNCTION
	Initializes the given IFFHandle to be a clipboard stream. It installs a
	"custom" stream handler (via InitIFF) to handle clipboard streams.
	The iff_Stream field of the iffhandle will still need
	to be initializes with a ClipboardHandle struct returned from
	OpenClipboard().

    INPUTS
	iff  - pointer to an IFFHandle struct.

    RESULT

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO
	OpenClipboard()

    INTERNALS

    HISTORY
  27-11-96    digulla automatically created from
	  iffparse_lib.fd and clib/iffparse_protos.h

*****************************************************************************/
{
    AROS_LIBFUNC_INIT
    AROS_LIBBASE_EXT_DECL(struct Library *,IFFParseBase)

    /* Initialize the DOS stream handler hook */
    /* Clipboard streams are ALWAYS random seekable */
    InitIFF(iff, IFFF_RSEEK, &(IPB(IFFParseBase)->cliphook));

    AROS_LIBFUNC_EXIT
} /* InitIFFasClip */
