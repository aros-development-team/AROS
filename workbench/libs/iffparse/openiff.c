/*
    Copyright © 1995-2004, The AROS Development Team. All rights reserved.
    $Id$
*/

#define DEBUG 0
#include <aros/debug.h>
#include "iffparse_intern.h"

/*****************************************************************************

    NAME */
#include <proto/iffparse.h>

	AROS_LH2(LONG, OpenIFF,

/*  SYNOPSIS */
	AROS_LHA(struct IFFHandle *, iff, A0),
	AROS_LHA(LONG              , rwMode, D0),

/*  LOCATION */
	struct Library *, IFFParseBase, 6, IFFParse)

/*  FUNCTION
	Initializes an IFFHandle struct for a new session of reading or
	writing. The direction of the I/O is determined by the rwMode flags
	supplied (IFFF_READ or IFFF_WRITE).

    INPUTS
	iff - pointer to IFFHandle struct.
	ewMode - IFFF_READ or IFFF_WRITE


    RESULT
	error -  0 if successfull, IFFERR_#? elsewise.

    NOTES
	 This function tells the custom stream handler to initialize
	by sending it a IFFCMD_INIT IFFStreamCmd.

    EXAMPLE

    BUGS

    SEE ALSO
	CloseIFF(), InitIFF()

    INTERNALS

*****************************************************************************/
{
    AROS_LIBFUNC_INIT
    AROS_LIBBASE_EXT_DECL(struct Library *,IFFParseBase)
    LONG  err;
    struct IFFStreamCmd cmd;
    struct ContextNode *cn;

    if (iff == NULL)
    {
	D(bug("openiff error: IFFERR_NOMEM\n"));
	return (IFFERR_NOMEM);
    }

    /* Check that a valid StreamHandler Hook has been supplied */
    if (!( GetIntIH(iff)->iff_StreamHandler) )
    {
	D(bug("openiff error: IFFERR_NOHOOK\n"));
	return (IFFERR_NOHOOK);
    }
     
    /* Tell the custom stream to initialize itself */

    cmd.sc_Command = IFFCMD_INIT;
    err = CallHookPkt (GetIntIH(iff)->iff_StreamHandler, iff, &cmd);

    if (!err)
    {
	/* If we are opend in read mode we should test if we have a valid IFF-File */
	if (rwMode == IFFF_READ)
	{
	    D(bug("testing if it's a valid IFF file\n"));
	    /* Get header of iff-stream */
	    err = GetChunkHeader(iff, IPB(IFFParseBase));

	    /* Valid IFF header ? */
	    if (!err)
	    {
		/* We have now entried the chunk */
		D(bug("entered the chunk\n"));
		GetIntIH(iff)->iff_CurrentState = IFFSTATE_COMPOSITE;

		cn = TopChunk(iff);

		/* We must see if we have a IFF header ("FORM", "CAT" or "LIST") */
		if (GetIntCN(cn)->cn_Composite)
		{
		    D(bug("an iff header\n"));
		    /* Everything went OK */
		    /* Set the acess mode, and mark the stream as opened */
		    iff->iff_Flags &= ~IFFF_RWBITS;
		    iff->iff_Flags |= (rwMode | IFFF_OPEN);

		    err = 0L;
		}
		else
		{
		    err = IFFERR_NOTIFF;
            	    D(bug("OpenIFF: not an IFF header, pop the context node\n"));

		    /* Pop the contextnode */
		    PopContextNode(iff, IPB(IFFParseBase));
		}
	    }
	    else
	    {
		if (err  == IFFERR_MANGLED)
		{
		    err = IFFERR_NOTIFF;
    	    	    D(bug("openiff error: IFFERR_MANGLED\n"));
		}
		
		/* Fail. We should send CLEANUP to the stream */
		cmd.sc_Command = IFFCMD_CLEANUP;
		err = CallHookPkt
		(
		    GetIntIH(iff)->iff_StreamHandler,
		    iff,
		    &cmd
		);
	    }
	} /* IFFF_READ */
	else
	{
	    iff->iff_Flags &= ~IFFF_RWBITS;
	    iff->iff_Flags |= (rwMode | IFFF_OPEN);	
	    err = 0L;
	}
	
    } /* if (!err) */

    D(bug("openiff: return %d\n", err));
    return (err);
    AROS_LIBFUNC_EXIT
} /* OpenIFF */
