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

	AROS_LH3(void, InitIFF,

/*  SYNOPSIS */
	AROS_LHA(struct IFFHandle *, iff, A0),
	AROS_LHA(LONG              , flags, D0),
	AROS_LHA(struct Hook      *, streamHook, A1),

/*  LOCATION */
	struct Library *, IFFParseBase, 38, IFFParse)

/*  FUNCTION
	Initializes an IFFHandle with a custom stream handler and
	flags describing seekability of the stream.

    INPUTS
	iff	    - pointer to IFFHandle struct.
	flags	     -	stream I/O flags for the IFFHandle.
	streamHook  - pointer to a Hook structure initialized with the streamhandler
		      to be called.

    RESULT

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO
	utility/hooks.h

    INTERNALS

    HISTORY
  27-11-96    digulla automatically created from
	  iffparse_lib.fd and clib/iffparse_protos.h

*****************************************************************************/
{
    AROS_LIBFUNC_INIT
    AROS_LIBBASE_EXT_DECL(struct Library *,IFFParseBase)

    /* Change the flags IFF flags to the supplied ones */
    iff->iff_Flags   |=   flags;

    /* Put the pointer to the streamHook into the iffhandle */
    GetIntIH(iff)->iff_StreamHandler  = streamHook;

    return;

    AROS_LIBFUNC_EXIT
} /* InitIFF */
