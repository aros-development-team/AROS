/*
    (C) 1995-96 AROS - The Amiga Replacement OS
    $Id$

    Desc: DOS stream handler. Used in InitIFFasDOS.
    Lang: English.
*/
#include "iffparse_intern.h"

/********************/
/* DosStreamHandler */
/********************/

ULONG DOSStreamHandler
(
    struct Hook 	* hook,
    struct IFFHandle	* iff,
    struct IFFStreamCmd * cmd,
    struct IFFParseBase * IFFParseBase
)
{

    LONG error;

    switch (cmd->sc_Command)
    {
	case IFFCMD_READ:

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
