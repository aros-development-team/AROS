/*
    Copyright © 1995-2004, The AROS Development Team. All rights reserved.
    $Id$
*/

#include "iffparse_intern.h"

/*****************************************************************************

    NAME */
#include <proto/iffparse.h>

	AROS_LH1(struct ClipboardHandle *, OpenClipboard,

/*  SYNOPSIS */
	AROS_LHA(LONG, unitNumber, D0),

/*  LOCATION */
	struct Library *, IFFParseBase, 41, IFFParse)

/*  FUNCTION
	Opens the clipboard.device with the specified unit.
	Allocates and initializes a ClipboardHandle struct which should
	be put into the iff_Stream field of the IFFHandle when the
	handle is initialized with InitIFFasClip().


    INPUTS
	unitNumber  - a clipboard device unit number (usually PRIMARY_CLIP).

    RESULT
	ch	    -  pointer to ClipboarHandle struct or NULL if unsuccessfull.

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO
	InitIFFasClip(), CloseClipboard()

    INTERNALS

*****************************************************************************/
{
    AROS_LIBFUNC_INIT
    AROS_LIBBASE_EXT_DECL(struct Library *,IFFParseBase)
    struct ClipboardHandle * ch;
    struct IOClipReq	   * req;
    struct Task 	   * thistask;

    /* Allocate a ClipBoardHandle */

    ch = AllocMem
    (
	sizeof (struct ClipboardHandle),
	MEMF_ANY | MEMF_CLEAR | MEMF_PUBLIC
    );
    if (ch);
    {
	/* Get a ponter to the ioClipReq, so we
	  don't need all that type casting.
	*/
	req = &(ch->cbh_Req);


	thistask = FindTask(0L);

	if (InitPort( &(ch->cbh_CBport), thistask, IPB(IFFParseBase)))
	{
	    if (InitPort( &(ch->cbh_SatisfyPort), thistask, IPB(IFFParseBase)))
	    {
		/* Initialize the IORequest structure.
		Basically CreateIORequest without memory allocation.
		*/
		req->io_Message.mn_ReplyPort = &(ch->cbh_CBport);
		req->io_Message.mn_Length = sizeof(struct IOClipReq);

		if
		(!OpenDevice
		    (
			"clipboard.device",
			unitNumber,
			(struct IORequest*)req,
			0L
		    )
		)
		{

		    return (ch);
		}

		ClosePort( &(ch->cbh_SatisfyPort), IPB(IFFParseBase));
	    }

	    ClosePort( &(ch->cbh_CBport), IPB(IFFParseBase));
	}
	FreeMem(ch, sizeof (struct ClipboardHandle));
    }
    return (FALSE);


    AROS_LIBFUNC_EXIT
} /* OpenClipboard */
