/*
    Copyright © 1995-2004, The AROS Development Team. All rights reserved.
    $Id$
*/

#include "iffparse_intern.h"

/*****************************************************************************

    NAME */
#include <proto/iffparse.h>

	AROS_LH4(LONG, PushChunk,

/*  SYNOPSIS */
	AROS_LHA(struct IFFHandle *, iff, A0),
	AROS_LHA(LONG              , type, D0),
	AROS_LHA(LONG              , id, D1),
	AROS_LHA(LONG              , size, D2),

/*  LOCATION */
	struct Library *, IFFParseBase, 14, IFFParse)

/*  FUNCTION
	Pushes a new context node onto the context stack. Usually used in write mode.
	In write mode the contextnode will be pushed with the given parameters.
	In Read mode the type, id and size will be read from the installed stream.
	Note that IFFSIZE_UNKNOW can be given for size in write mode. In that case,
	the size of will not be known until you do a PopChunk(). PopChunk()
	will then seek back in the stream and write the correct size.


    INPUTS
	iff    - pointer to IFFHandle struct.
	type  -  chunk type specifier.
	id    -  chunk identifier.
	size  -  size of the new chunk. May be IFFSIZE_UNKNOWN.

    RESULT
	error  -  0 if successfull, IFFERR_#? otherwize.

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO
	PopChunk()

    INTERNALS
	We do different things for Read and Write streams (obviosly enough ;->)

	Write: Write the supplied id, size and evt type into the stream.

	Read: Get a chunk from disk


	For Write mode there are som problems with unknown chunk size and
	non-random-seekable streams:
	4. situations:

	SIZE KNOWN && RSEEK	- Just write the whole header.
	SIZE KNOWN && !RSEEK	 - Write whole header. No RSEEK does not matter, since
				  we don't have to seek back to write size in PopChunk

	SIZE UNKNOWN && RSEEK	- Write whole header. Write size too, just to seek pass it,
				  even if the size value might be meaningless. We will
				  seek back and insert the correct size later.

	SIZE UNKNOWN && !RSEEK	- Here is where the trouble starts. We can not seek back
				  and insert the correct size later, which means that we MUST
				  buffer the contents of the chunk, and don't write ANYTHING to
				  the stream until we know its size.

				  We preserve the old StreamHandler, and inserts a new one
				  that buffers all writes into memory.

*****************************************************************************/
{
    AROS_LIBFUNC_INIT
    AROS_LIBBASE_EXT_DECL(struct Library *,IFFParseBase)

    LONG err;

    LONG byteswritten;

    LONG scan = 0;

    DEBUG_PUSHCHUNK(dprintf("PushChunk: iff 0x%lx type 0x%08lx (%c%c%c%c) id 0x%08lx (%c%c%c%c) size %d\n",
			    iff, type, dmkid(type), id, dmkid(id), size));

    if (iff->iff_Flags & IFFF_WRITE)
    {
      /* Do we have a problem - situation ? */
	if ( (size == IFFSIZE_UNKNOWN)
	    && (!(iff->iff_Flags & IFFF_RSEEK))
	)
	{

	    /* Initialize the buffering streamhandler */
	    err = InitBufferedStream(iff, IPB(IFFParseBase));
	    if (err) return (err);
	}

	byteswritten = WriteStreamLong
	(
	    iff,
	    &id,
	    IPB(IFFParseBase)
	);

	/* IFFFERR_ ..	? */
	    if (byteswritten < 0) return (byteswritten);

	/* The chunk size will be written during PopChunk too, but we write
	 here to seek past it */

	byteswritten = WriteStreamLong
	(
	    iff,
	    &size,
	    IPB(IFFParseBase)
	);
	/* IFFERR_... ? */
	if (byteswritten < 0) return (byteswritten);

	/* If a composite type, then write whole type */
	if
	( id == ID_FORM || id == ID_LIST || id == ID_CAT || id == ID_PROP )
	{
	    byteswritten = WriteStreamLong
	    (
		iff,
		&type,
		IPB(IFFParseBase)
	    );

	    if (byteswritten < 0) return (byteswritten);

	    scan = sizeof(ULONG);

	} /* End of composite */

	err = PushContextNode
	(
	    iff,
	    type,
	    id,
	    size,
	    scan,
	    IPB(IFFParseBase)
	);
    }
    else  /* Read or write mode */
    {
	/* Read mode. Read the chunk header from stream and put a new contextnode into the stack */
	err = GetChunkHeader(iff, IPB(IFFParseBase));
    }  /* End of Write or Read */

    return (err);

    AROS_LIBFUNC_EXIT
} /* PushChunk */
