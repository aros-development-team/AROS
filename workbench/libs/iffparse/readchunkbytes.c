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

    AROS_LH3(LONG, ReadChunkBytes,

/*  SYNOPSIS */
	AROS_LHA(struct IFFHandle *, iff, A0),
	AROS_LHA(APTR              , buf, A1),
	AROS_LHA(LONG              , numBytes, D0),

/*  LOCATION */
	struct Library *, IFFParseBase, 10, IFFParse)

/*  FUNCTION
	Read a number of bytes from the current chunk into a buffer.
	Attempts to read past the end of the chunk will be truncated.

    INPUTS
	iff	 - pointer to IFFHandle struct.
	buf	 -  pointer to a buffer into which the data will be placed.
	numBtes  - number of bytes to read.

    RESULT
	actual -   (positive) the actual number of bytes read.
		  (negative) IFFERR_#? error code if not succesfull.

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO
	ReadChunkRecords(), ParseIFF(), WriteChunkBytes()

    INTERNALS

    HISTORY
  27-11-96    digulla automatically created from
	  iffparse_lib.fd and clib/iffparse_protos.h

*****************************************************************************/
{
    AROS_LIBFUNC_INIT
    AROS_LIBBASE_EXT_DECL(struct Library *,IFFParseBase)

    struct ContextNode *cn;

    LONG   lefttoread,
	  bytesread;


    /* Get pointer to current contextnode */
    cn = TopChunk(iff);

    lefttoread = cn->cn_Size - cn->cn_Scan;

    /* If numBytes > lefttoread then we must truncate the readoperation */
    if (numBytes > lefttoread)
	numBytes = lefttoread;

    bytesread = ReadStream
    (
	iff,
	buf,
	numBytes,
	IPB(IFFParseBase)
    );

    /* No error */
    if (bytesread > 0)
	cn->cn_Scan += bytesread;

    /* Return number of bytes actually read  (or error )*/
    return (bytesread);

    AROS_LIBFUNC_EXIT
} /* ReadChunkBytes */
