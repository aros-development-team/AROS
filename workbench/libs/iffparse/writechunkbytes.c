/*
    Copyright © 1995-2004, The AROS Development Team. All rights reserved.
    $Id$
*/

#include "iffparse_intern.h"

/*****************************************************************************

    NAME */
#include <proto/iffparse.h>

	AROS_LH3(LONG, WriteChunkBytes,

/*  SYNOPSIS */
	AROS_LHA(struct IFFHandle *, iff, A0),
	AROS_LHA(APTR              , buf, A1),
	AROS_LHA(LONG              , numBytes, D0),

/*  LOCATION */
	struct Library *, IFFParseBase, 11, IFFParse)

/*  FUNCTION
	Writes given number of bytes in the supplied buffer into the
	current chunk. Attempts to write past the endo of the chunk will
	be truncated.

    INPUTS
	iff	   - pointer to IFFHandle struct.
	buf	   -  buffer with data to write.
	numBytes  - number of bytes to write.

    RESULT
	actual	  -  (positive) number of bytes actually written.
		    (negative) IFFERR_#? indicating unsuccesfull write.
    NOTES

    EXAMPLE

    BUGS

    SEE ALSO
	PushChunk(), PopChunk(), WriteChunkRecords()

    INTERNALS

*****************************************************************************/
{
    AROS_LIBFUNC_INIT
    AROS_LIBBASE_EXT_DECL(struct Library *,IFFParseBase)

    struct ContextNode *cn;

    LONG placeleft;
    LONG byteswritten;

    DEBUG_WRITECHUNKBYTES(dprintf("WriteChunkBytes: iff 0x%lx buf 0x%lx size %d\n",
			    iff, buf, numBytes));

    /* Get the top contextnode */
    cn = TopChunk(iff);

    /* Is the numBytes known for this chunk ? */
    if (cn->cn_Size != IFFSIZE_UNKNOWN)
    {
	/* We must truncate attempts to write larger than the chunksize */
	placeleft = cn->cn_Size - cn->cn_Scan;
	if  (numBytes > placeleft)

	numBytes = placeleft;
    }


    /* Actually write the chunk */
    byteswritten = WriteStream(iff, buf, numBytes, IPB(IFFParseBase));

    if (byteswritten > 0)
    {
	/* No error */
	cn->cn_Scan += byteswritten;
    }

    DEBUG_WRITECHUNKBYTES(dprintf("WriteChunkBytes: return %ld\n", byteswritten));
    return (byteswritten);

    AROS_LIBFUNC_EXIT
} /* WriteChunkBytes */
