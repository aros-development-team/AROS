/*
    Copyright © 1995-2004, The AROS Development Team. All rights reserved.
    $Id$
*/

#include "iffparse_intern.h"

/*****************************************************************************

    NAME */
#include <proto/iffparse.h>

	AROS_LH4(LONG, ReadChunkRecords,

/*  SYNOPSIS */
	AROS_LHA(struct IFFHandle *, iff, A0),
	AROS_LHA(APTR              , buf, A1),
	AROS_LHA(LONG              , bytesPerRecord, D0),
	AROS_LHA(LONG              , numRecords, D1),

/*  LOCATION */
	struct Library *, IFFParseBase, 12, IFFParse)

/*  FUNCTION
	Read a number of records with the given size from the current chunk
	into a buffer. Attempts to read past the end of the chunk will be truncated.

    INPUTS
	 iff		  - pointer to IFFHandle struct.
	buf		 -  pointer to a buffer into which the data will be placed.
	bytesPerRecord	- number of bytes per record.
	numRecords	-  number of records to read.

    RESULT
	actual -   (positive) the actual number of whole records read.
		  (negative) IFFERR_#? error code if not succesfull.


    NOTES

    EXAMPLE

    BUGS

    SEE ALSO
	ReadChunkBytes(), ParseIFF(), WriteChunkRecords()

    INTERNALS

*****************************************************************************/
{
    AROS_LIBFUNC_INIT
    AROS_LIBBASE_EXT_DECL(struct Library *,IFFParseBase)

    struct ContextNode *cn;

    LONG   bytestoread,
	  lefttoread,
	  bytesread;

    DEBUG_READCHUNKRECORDS(dprintf("ReadChunkRecords: iff %p buf %p bytesPerRecord %ld numRecords %ld\n",
                                   iff, buf, bytesPerRecord, numRecords));

    /* Get pointer to top contextnode */
    cn = TopChunk(iff);

    lefttoread = cn->cn_Size - cn->cn_Scan;

    bytestoread = bytesPerRecord * numRecords;

    /* If bytestoread > lefttoread then we must truncate the readoperation */
    if (bytestoread > lefttoread)
    {
	bytestoread = lefttoread;


	/* See to it that we only read whole records */
	bytestoread -= (lefttoread % bytesPerRecord);
    }

    /* Beware: bytestoread is 0 now if bytesPerRecord > lefttoread */

    bytesread = ReadStream
    (
	iff,
	buf,
	bytestoread,
	IPB(IFFParseBase)
    );


    /* Return number of records  actually read (if no error occured)  */
    if (bytesread < 0)
	/* IFFERR_#? in bytesread */
	numRecords = bytesread;
    else
    {
	/* calculate the actual number of records written */
	numRecords = bytesread / bytesPerRecord;

	/* Update number of bytes read */
	cn->cn_Scan += bytesread;
    }

    DEBUG_READCHUNKRECORDS(dprintf("ReadChunkRecords: return %ld\n", numRecords));
    return (numRecords);

    AROS_LIBFUNC_EXIT
} /* ReadChunkRecords */
