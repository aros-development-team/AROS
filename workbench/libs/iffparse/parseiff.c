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

	AROS_LH2(LONG, ParseIFF,

/*  SYNOPSIS */
	AROS_LHA(struct IFFHandle *, iff, A0),
	AROS_LHA(LONG              , mode, D0),

/*  LOCATION */
	struct Library *, IFFParseBase, 7, IFFParse)

/*  FUNCTION
	This function is the parser itself. It has three control modes.
	    IFFPARSE_SCAN - the parser will go through the file invoking
		entry and exit handlers on its way.
		When it returns it might be for 3 different reasons:

		- It invoked a Stop entry/exit handler ( Installed by StopChunk[s] or
		  StopOnExit )

		- An error occured.
		  (return value will be negative.)

		- The parser reached EOF and returns IFFERR_EOF.

	    IFFPARSE_STEP  -  The parser steps through the file, returning to the
		user each time it enters (returns NULL) and each time it exits
		(return (IFFERR_EOC) a chunk.
		It will also invoke entry/exit - handlers.

	    IFFPARSE_RAWSTEP - same as IFFPARSE_STEP except that in this mode
		the parse won't invoke any handlers.


    INPUTS
	iff - pointer to IFFHandle struct.
	mode - IFFPARSE_SCAN, IFFPARSE_STEP or IFFPARSE_RAWSTEP.

    RESULT
	0 if successfull or IFFERR_#?

    NOTES


    EXAMPLE

    BUGS

    SEE ALSO
	PushChunk(), PopChunk(), EntryHandler(), ExitHandler(), PropChunk[s](),
	CollectionChunk[s](), StopChunk[s](), StopOnExit()


    INTERNALS

*****************************************************************************/
{
    AROS_LIBFUNC_INIT
    AROS_LIBBASE_EXT_DECL(struct Library *,IFFParseBase)
    struct ContextNode *cn;

    LONG size;
    LONG err = 0;
    LONG toseek; /* To hold number of bytes left to seek in a chunk */
    BOOL done = FALSE;

    D(bug("ParseIFF (iff=%p, mode=%d)\n", iff, mode));

    /* Cannot parse iff when it is not opened yet */
    if (!(iff->iff_Flags & IFFF_OPEN))
    {
	D(bug("ParseIFF: syntax error (open)\n"));
	ReturnInt ("ParseIFF",LONG,IFFERR_SYNTAX);
    }

    /* Cannot parse iff file, when it is opened in write-mode */
    if (iff->iff_Flags & IFFF_WRITE)
    {
	D(bug("ParseIFF: syntax error (write)\n"));
	ReturnInt ("ParseIFF",LONG,IFFERR_SYNTAX);
    }

    /* Main loop for reading the iff file

    This works as a Finite State Automaton  where we between the states might return to the
    user, and get called by the user again. InternalIFFHandle->CurrentState holds
    the current state */

    while (!done)
    {
	D(bug("ParseIFF: state %d\n", GetIntIH(iff)->iff_CurrentState));
        switch ( GetIntIH(iff)->iff_CurrentState )
	{
	case IFFSTATE_COMPOSITE:

	    /* We are inside a FORM, LIST, CAT or PROP
		and expect a new chunk header to be found,
		but first we must invoke the handlers on the current one */

	    /* Where to go next */
	    GetIntIH(iff)->iff_CurrentState = IFFSTATE_PUSHCHUNK;

	    /* Invoke the handlers */
	    err = InvokeHandlers(iff, mode, IFFLCI_ENTRYHANDLER, IPB(IFFParseBase));

	    if (err)
	    {
		if (err == IFF_RETURN2CLIENT)
		    err = 0;
		done = TRUE;
	    }



	    break;

	case IFFSTATE_PUSHCHUNK:

	    /*	What we want to do now is to get the next chunk, make a context-node of it,
		and put it in top of the stack */

	    err = GetChunkHeader(iff, IPB(IFFParseBase));
	    if (err)
	    {
		done = TRUE;
		break;
	    }


	    /* Determine if top chunk is composite or atomic */
	    cn = TopChunk(iff);

	    if ( GetIntCN(cn)->cn_Composite)
		GetIntIH(iff)->iff_CurrentState = IFFSTATE_COMPOSITE;
	    else
		GetIntIH(iff)->iff_CurrentState = IFFSTATE_ATOMIC;


	    break;


	case IFFSTATE_ATOMIC:

	    /* We have entried an atomic chunk. Its contextnode has
	    allready been laid onto the stack */

	    GetIntIH(iff)->iff_CurrentState = IFFSTATE_SCANEXIT;

	    err = InvokeHandlers(iff, mode, IFFLCI_ENTRYHANDLER, IPB(IFFParseBase));
	    if (err)
	    {
		if (err == IFF_RETURN2CLIENT)
		    err = 0L;

		done = TRUE;
	    }
	    break;

	case IFFSTATE_SCANEXIT:

	    /* We have done the needed entry stuff inside the ATOMIC chunk scan towards
	    the end of it */

	    GetIntIH(iff)->iff_CurrentState = IFFSTATE_EXIT;

	    cn = TopChunk(iff);

	    toseek = cn->cn_Size - cn->cn_Scan;

	    /* If cn_Size is not wordaligned we must seek one more byte */
	    if (cn->cn_Size & 1)
		toseek ++;

	    if (toseek)
	    {
		err = SeekStream(iff, toseek, IPB(IFFParseBase));
		if (err)
		    done = TRUE;
	    }

	    break;

	case IFFSTATE_EXIT:

	    /* We are at the end of the chunk, should scan for exithandlers */
	    GetIntIH(iff)->iff_CurrentState = IFFSTATE_POPCHUNK;

	    err = InvokeHandlers(iff, mode, IFFLCI_EXITHANDLER, IPB(IFFParseBase));
	    if (err)
	    {
		if (err == IFF_RETURN2CLIENT)
		    err = 0;


		done = TRUE;

	    }
	    break;

	case IFFSTATE_POPCHUNK:
	    /* Before we pop the top node, get its size */
	    cn	   = TopChunk(iff);
	    size  = cn->cn_Size;

	    /* Align the size */
	    if (size % 2)
		size +=1;

	    /* Add size of chunk header */
	    if ( GetIntCN(cn)->cn_Composite )
		size += 12;
	    else
		size += 8;

	    /* Pop the top node */
	    PopContextNode(iff, IPB(IFFParseBase));

	    /* The underlying node must get updated its scancount */
	    cn	= TopChunk(iff);
	    cn->cn_Scan += size;

	    /* The outer node is ALWAYS a composite one
	    Now we might have 3 different situations :

	    1. There are more chunks inside this composite chunk, and we should
		enter the PUSHCHUNK state to get them.

	    2. There are no more chunks in this composite, but we are not in the outer-most
		composite, so we just want to enter the EXIT state.

	    3. We are at the end of and about to leave the outermost composite chunk,
		and should therefore return IFFERR_EOF.

		*/

	    /* Nr. 1 */
	    if (cn->cn_Scan < cn->cn_Size)
		GetIntIH(iff)->iff_CurrentState = IFFSTATE_PUSHCHUNK;

	    else
	    {
		/* Nr. 3 */
		if (!iff->iff_Depth )
		{
		    err = IFFERR_EOF;
		    done = TRUE;
		}
		else
		    /* Nr. 2 */
		    GetIntIH(iff)->iff_CurrentState = IFFSTATE_EXIT;

	    }
	    break;


	}  /* End of switch */


    }  /* End of while */

    D(bug("ParseIFF: return %ld\n", err));

    ReturnInt ("ParseIFF",LONG,err);
    AROS_LIBFUNC_EXIT
} /* ParseIFF */
