/*
    Copyright © 1995-2004, The AROS Development Team. All rights reserved.
    $Id$
*/

#include "iffparse_intern.h"

/*****************************************************************************

    NAME */
#include <proto/iffparse.h>

	AROS_LH1(void, InitIFFasDOS,

/*  SYNOPSIS */
	AROS_LHA(struct IFFHandle *, iff, A0),

/*  LOCATION */
	struct Library *, IFFParseBase, 39, IFFParse)

/*  FUNCTION
	Initializes the given IFFHandle to be a DOS stream. It installs a
	"custom" stream handler (via InitIFF) to handle DOS streams.
	The iff_Stream field of the iffhandle will still need
	to be initializes with a filehandle struct as returned from
	dos.library/Open(). The iff_Flags may be changed to
	change the seekability of the stream after this function is called,
	but before OpenIFF() is called. Seekability for dos files
	default to IFFF_RSEEK (random seekable).



    INPUTS
	iff  - pointer to an IFFHandle struct.

    RESULT

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

*****************************************************************************/
{
    AROS_LIBFUNC_INIT
    AROS_LIBBASE_EXT_DECL(struct Library *,IFFParseBase)

    DEBUG_INITIFFAS(dprintf("InitIFFasDOS: entry\n"));

    /* Initialize the DOS stream handler hook */
    InitIFF(iff, IFFF_RSEEK, &IPB(IFFParseBase)->doshook);

    DEBUG_INITIFFAS(dprintf("InitIFFasDOS: done\n"));

    AROS_LIBFUNC_EXIT
} /* InitIFFasDOS */
