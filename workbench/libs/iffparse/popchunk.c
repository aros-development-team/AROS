/*
    Copyright © 1995-2004, The AROS Development Team. All rights reserved.
    $Id$
*/

#include "iffparse_intern.h"

/*****************************************************************************

    NAME */
#include <proto/iffparse.h>

	AROS_LH1(LONG, PopChunk,

/*  SYNOPSIS */
	AROS_LHA(struct IFFHandle *, iff, A0),

/*  LOCATION */
	struct Library *, IFFParseBase, 15, IFFParse)

/*  FUNCTION
	Pops a context node of the context stack. Usually called
	in write mode to signal the end of a chunk.


    INPUTS
	iff    - pointer to IFFHandle struct.

    RESULT
	error  -  0 if successfull, IFFERR_#? otherwise.

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO
	PushChunk()

    INTERNALS

    Frees a contextnode an all its related LCIs and removes it from the stack-list
    If we are in write mode, we will update the cn_Size in the chunk according
    t cn_Scan, and we will also insert an align byte if necessary


    If the stream was of PushChunked() as IFFSIZE_UNKNOWN, we have to seek backwards
    and write the correct size.
    Since non RSEEK streams are automagically buffered by
    WriteStream, we don't have to fiddle with it here.
    However if the stream is buffered, we MUST see if we are
    about to pop the chunk that started the Buffering (in PushChunk)

*****************************************************************************/
{
    AROS_LIBFUNC_INIT
    AROS_LIBBASE_EXT_DECL(struct Library *,IFFParseBase)


    struct ContextNode	*cn;

    LONG err;

    LONG size;

    UBYTE nullbyte = 0;

    DEBUG_POPCHUNK(dprintf("PopChunk: iff 0x%lx\n", iff));

    /* Get current chunk */
    cn = TopChunk(iff);

    /* Is the IFFHandle opened in Read or Write mode ? */
    if (iff->iff_Flags & IFFF_WRITE)
    {
	/* Write mode. We should update cn_Size *INSIDE the stream,
	if the chunk was pushed with IFFSIZE_UNKNOWN */

	if (cn->cn_Size == IFFSIZE_UNKNOWN)
	{

	    err = SeekStream
	    (
		iff,
		/* minus is for seeking backwards. Remember: evt. chunk types
		for composite chunks are allready in cn_Scan */
		- ( cn->cn_Scan + sizeof(ULONG) ),
		IPB(IFFParseBase)
	    );

	    if (err) return (err);

	    size = cn->cn_Scan;

	    /* Write the chunk size */
	    err = WriteStreamLong
	    (
		iff,
		&size,
		IPB(IFFParseBase)
	    );

	    if (err < 0) return (err);

	    /* Seek towards end of chunk again */
	    err = SeekStream
	    (
		iff,
		size,
		IPB(IFFParseBase)
	    );

	    if (err) return (err);

	}
	else  /* IFFSIZE known at PushChunk() time */
	    size = cn->cn_Size;


	/* Write a pad byte if chunk is not word-aligned */
	if (size % 2)
	{
	    err =  WriteStream
	    (
		iff,
		&nullbyte,
		1,
		IPB(IFFParseBase)
	    );
	    
	    if (err < 0) return (err);
	    size++;
	}

	if
	(
	    GetIntIH(iff)->iff_BufferStartDepth
	==
	    iff->iff_Depth
	)
	{
	    /* a routine that writes the buffer to stream and reinstallss the old streamhandler */

	    err = ExitBufferedStream(iff, IPB(IFFParseBase));
	    if (err) return (err);

	}
    }

    /* Actually pop the top context-node. (Done for both handles in Read & Write mode) */

    PopContextNode(iff, IPB(IFFParseBase));

    /* stegerg: is this okay!? */
    
    if (iff->iff_Flags & IFFF_WRITE)
    {
        cn = TopChunk(iff);

	/* Might work without this check, because there seems to be always at
	   least one contextnode --> see AllocIFF) */
	if (cn->cn_Node.mln_Succ)
	{
	    cn->cn_Scan += size + sizeof(ULONG) + sizeof(ULONG);
	}
    }
    
    return 0;

    AROS_LIBFUNC_EXIT
} /* PopChunk */
