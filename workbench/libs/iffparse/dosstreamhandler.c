/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: DOS stream handler. Used in InitIFFasDOS.
    Lang: English.
*/
#define DEBUG 0
#include <aros/debug.h>
#include "iffparse_intern.h"

/********************/
/* DosStreamHandler */
/********************/

#undef DOSBase
#define DOSBase    (IPB(hook->h_Data)->dosbase)

ULONG DOSStreamHandler
(
    struct Hook 	* hook,
    struct IFFHandle	* iff,
    struct IFFStreamCmd * cmd
)
{

    LONG error = 0;

    switch (cmd->sc_Command)
    {
    case IFFCMD_READ:
	D(bug("   Reading %ld bytes\n", cmd->sc_NBytes));

	error =
	(
	    Read
	    (
		(BPTR)iff->iff_Stream,
		cmd->sc_Buf,
		cmd->sc_NBytes
	    )
	!=
	    cmd->sc_NBytes
	);

	break;

    case IFFCMD_WRITE:

	error =
	(
	    Write
	    (
		(BPTR)iff->iff_Stream,
		cmd->sc_Buf,
		cmd->sc_NBytes
	    )
	!=
	    cmd->sc_NBytes
	);

	break;

    case IFFCMD_SEEK:
	D(bug("   Seeking %ld bytes\n", cmd->sc_NBytes));

	error =
	(
	    Seek
	    (
		(BPTR)iff->iff_Stream,
		cmd->sc_NBytes,
		OFFSET_CURRENT
	    )

	==
	    -1
	);

	break;

    case IFFCMD_INIT:
    case IFFCMD_CLEANUP:
	/* Don't need these for dos streams
	*/
	error = NULL;
	break;
    }

    return (error);

}
