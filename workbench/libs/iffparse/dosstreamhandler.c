/*
    Copyright © 1995-2004, The AROS Development Team. All rights reserved.
    $Id$

    DOS stream handler. Used in InitIFFasDOS.
*/

#define DEBUG 0
#include <aros/debug.h>
#include "iffparse_intern.h"

/********************/
/* DosStreamHandler */
/********************/

#define IFFParseBase IPB(hook->h_Data)

ULONG DOSStreamHandler
(
    struct Hook 	* hook,
    struct IFFHandle	* iff,
    struct IFFStreamCmd * cmd
)
{
    LONG error = 0;

    DEBUG_DOSSTREAMHANDLER(dprintf("DOSStreamHandler: hook %p iff %p cmd %p\n", hook, iff, cmd));

    switch (cmd->sc_Command)
    {
    case IFFCMD_READ:
	DEBUG_BUFSTREAMHANDLER(dprintf("DOSStreamHandler: IFFCMD_READ...\n"));
	D(bug("   Reading %ld bytes\n", cmd->sc_NBytes));

	error = Read(
		(BPTR)iff->iff_Stream,
		cmd->sc_Buf,
		cmd->sc_NBytes) != cmd->sc_NBytes;

	break;

    case IFFCMD_WRITE:
	DEBUG_BUFSTREAMHANDLER(dprintf("DOSStreamHandler: IFFCMD_WRITE...\n"));
	D(bug("   Writing %ld bytes\n", cmd->sc_NBytes));

	error = Write(
		(BPTR)iff->iff_Stream,
		cmd->sc_Buf,
		cmd->sc_NBytes) != cmd->sc_NBytes;

	break;

    case IFFCMD_SEEK:
	DEBUG_BUFSTREAMHANDLER(dprintf("DOSStreamHandler: IFFCMD_SEEK...\n"));
	D(bug("   Seeking %ld bytes\n", cmd->sc_NBytes));

	error = Seek((BPTR)iff->iff_Stream, cmd->sc_NBytes, OFFSET_CURRENT) == -1;

	break;

    case IFFCMD_INIT:

	DEBUG_BUFSTREAMHANDLER(dprintf("DOSStreamHandler: IFFCMD_INIT...\n"));

	/* Don't need these for dos streams
	*/
	error = 0;
	break;

    case IFFCMD_CLEANUP:

	DEBUG_BUFSTREAMHANDLER(dprintf("DOSStreamHandler: IFFCMD_CLEANUP...\n"));

	/* Force stream to beginning, some applications assume stream is at
	   beginning after failed OpenIFF()'s IFFCMD_CLEANUP. This fixed pbs
	   with multiview and certain jpeg files, for example. - Piru
	*/
	error = Seek((BPTR)iff->iff_Stream, 0, OFFSET_BEGINNING) == -1;
	break;
    }

    DEBUG_DOSSTREAMHANDLER(dprintf("DOSStreamHandler: return %ld\n", error));
    return (error);
}
