/*
    Copyright © 1995-2004, The AROS Development Team. All rights reserved.
    $Id$
*/

#include "iffparse_intern.h"

/*****************************************************************************

    NAME */
#include <proto/iffparse.h>

	AROS_LH1(void, CloseIFF,

/*  SYNOPSIS */
	AROS_LHA(struct IFFHandle *, iff, A0),

/*  LOCATION */
	struct Library *, IFFParseBase, 8, IFFParse)

/*  FUNCTION
	Completes a read or write session by closing the IFF handle.
	The IFFHandle struct is ready for reuse in another session,
	it's just to open it again with OpenIFF(). This function
	also automatically cleans up if a read or write fails halfway through.

    INPUTS
	iff - Pointer to an IFFhandle struct previously opened with OpenIFF()

    RESULT

    NOTES
	This function tells the custom stream handler to clean up
	by sending it a IFFCMD_CLEANUP IFFStreamCmd.

    EXAMPLE

    BUGS

    SEE ALSO
	OpenIFF(), InitIFF()

    INTERNALS
	This function checks that buffers for buffered streams
	have been freed. This is not very elegant and should have been
	done at an earlier stadium. It is not a real bug though.

*****************************************************************************/
{
    AROS_LIBFUNC_INIT
    AROS_LIBBASE_EXT_DECL(struct Library *,IFFParseBase)

    struct IFFStreamCmd    cmd;

    if (iff != NULL)
    {
	/* clear the IFFF_OPEN bit to mark IFF stream closed */
	if (!(iff->iff_Flags & IFFF_OPEN) )
	{
	    return;
	}

	iff->iff_Flags &= ~IFFF_OPEN;

	/* Pop of all contextnodes so that only the default one is remaining */

	/*
	for (count = iff->iff_Depth; count; count -- )
	{
	    PopContextNode(iff, IPB(IFFParseBase));
	}
	*/
	while (iff->iff_Depth)
	{
	    PopChunk(iff);
	}

	/* This is for safety:
	  It might be in PopChunk that seeking or writing to streams failed.
	  In that case the memory for eventual buffered setreams
	  were not freed, so we should free it here.

	  (Yes, it is is a kludge !)
	*/
	if ( GetIntIH(iff)->iff_BufferStartDepth)
	{
	    FreeBuffer((struct BufferList*)iff->iff_Stream, IPB(IFFParseBase));

	    GetIntIH(iff)->iff_BufferStartDepth = 0;
	}

	/* Tell the custom stream to cleanup */
	cmd.sc_Command = IFFCMD_CLEANUP;
	CallHookPkt
	(
	    GetIntIH(iff)->iff_StreamHandler,
	    iff,
	    &cmd
	);
    }
    
    AROS_LIBFUNC_EXIT
} /* CloseIFF */
