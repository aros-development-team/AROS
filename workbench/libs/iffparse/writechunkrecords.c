/*
    Copyright © 1995-2004, The AROS Development Team. All rights reserved.
    $Id$
*/

#include "iffparse_intern.h"

/*****************************************************************************

    NAME */
#include <proto/iffparse.h>

	AROS_LH4(LONG, WriteChunkRecords,

/*  SYNOPSIS */
	AROS_LHA(struct IFFHandle *, iff, A0),
	AROS_LHA(APTR              , buf, A1),
	AROS_LHA(LONG              , bytesPerRecord, D0),
	AROS_LHA(LONG              , numRecords, D1),

/*  LOCATION */
	struct Library *, IFFParseBase, 13, IFFParse)

/*  FUNCTION
	Write numRecods records of bytesPerRecord bytes to the current chunk.
	Attempts to write past the end of the chunk will be truncated.

    INPUTS
	 iff		  - pointer to IFFHandle struct.
	buf		 -  pointer to a buffer containig the data to be written.
	bytesPerRecord	- number of bytes per record.
	numRecords	-  number of records to write.

    RESULT
	actual -   (positive) the actual number of whole records written.
		  (negative) IFFERR_#? error code if not succesfull.

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO
	WriteChunkBytes()

    INTERNALS

*****************************************************************************/
{
    AROS_LIBFUNC_INIT
    AROS_LIBBASE_EXT_DECL(struct Library *,IFFParseBase)

    struct ContextNode *cn;

    /* Number of bytes possible to write in the chunk */
    LONG lefttowrite;
    /* Number of bytes actually written */
    LONG byteswritten;
    /* Number of bytes to write into the chunk */
    LONG bytestowrite;

    DEBUG_WRITECHUNKRECORDS(dprintf("WriteChunkRecords: iff %p buf %p bytesPerRecord %ld numRecords %ld\n",
                                    iff, buf, bytesPerRecord, numRecords));

    /* Calculate number of bytes to write */
    bytestowrite = bytesPerRecord * numRecords;

    /* Get the top contextnode */
    cn = TopChunk(iff);

    /* Is the numBytes known for this chunk ? */
    if (cn->cn_Size != IFFSIZE_UNKNOWN)
    {
	/* We must truncate attempts to write larger than the chunksize */
	lefttowrite = cn->cn_Size - cn->cn_Scan;
	if  (bytestowrite > lefttowrite)
	{
	    bytestowrite = lefttowrite;

	    /* See to it that we only write whole records */
	    bytestowrite -= (lefttowrite % bytesPerRecord);
	}
    }


    /* Actually write the chunk */
    byteswritten = WriteStream(iff, buf, bytestowrite, IPB(IFFParseBase));

    if (byteswritten < 0)
	/* IFFERR_#? returned by WriteStream() */
	numRecords = byteswritten;
    else
    {
	/* No error */
	cn->cn_Scan += byteswritten;

	numRecords = (byteswritten / bytesPerRecord);
    }

    DEBUG_WRITECHUNKRECORDS(dprintf("WriteChunkRecords: return %ld\n", numRecords));
    return (numRecords);

    AROS_LIBFUNC_EXIT
} /* WriteChunkRecords */
