/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Helpfuncs needed when iffparse is used for clipboard handling.
    Lang: English.
*/
#include "iffparse_intern.h"
#include <exec/io.h>

#include <aros/debug.h>

#define DEBUG_STREAM(x)		;

/***********************/
/* Port initialization */
/***********************/

/* Initializes and Closes PRIVATE ports     */
/* Used in OpenClipboard and CloseClipboard */
/* Look at page 501-502 in RKM Libraries     */

BOOL InitPort (struct MsgPort *mp, struct Task *t,
	struct IFFParseBase_intern * IFFParseBase)
{
    LONG sigbit;

    if ((sigbit = AllocSignal(-1L)) == -1) return (FALSE);

    mp->mp_Node.ln_Type = NT_MSGPORT;
    mp->mp_Flags	=  PA_SIGNAL;
    mp->mp_SigBit	 =  sigbit;
    mp->mp_SigTask	=  t;

    NewList(&(mp->mp_MsgList));

    return (TRUE);
}

VOID ClosePort (struct MsgPort *mp,
    struct IFFParseBase_intern * IFFParseBase)
{
    mp->mp_SigTask	    =  (struct Task*)-1;
    mp->mp_MsgList.lh_Head  = (struct Node*)-1;

    FreeSignal( mp->mp_SigBit );

    return;
}


/**********************/
/* ClipStreamHandler  */
/**********************/

#define IFFParseBase IPB(hook->h_Data)

ULONG ClipStreamHandler
(
    struct Hook 	* hook,
    struct IFFHandle	* iff,
    struct IFFStreamCmd * cmd
)
{
      #define CLIPSCANBUFSIZE 10000000 //500
    LONG error = NULL;

    /* Buffer neede for reading rest of clip in IFFCMD_CLEANUP. Eats some stack */
//	UBYTE  buf[CLIPSCANBUFSIZE];

    struct IOClipReq *req;

    req = &( ((struct ClipboardHandle*)iff->iff_Stream)->cbh_Req);

    DEBUG_STREAM(bug("ClipStream: iff %p cmd %d buf %p bytes %d\n",
			 iff, cmd->sc_Command, cmd->sc_Buf, cmd->sc_NBytes));

    switch (cmd->sc_Command)
    {
	case IFFCMD_READ:

	    DEBUG_BUFSTREAMHANDLER(dprintf("ClipStream: IFFCMD_READ...\n"));

	    req->io_Command = CMD_READ;
	    req->io_Data    = cmd->sc_Buf;
	    req->io_Length  =  cmd->sc_NBytes;

	    error = (DoIO((struct IORequest*)req));

	    break;

	case IFFCMD_WRITE:

	    DEBUG_BUFSTREAMHANDLER(dprintf("ClipStream: IFFCMD_WRITE...\n"));

	    req->io_Command = CMD_WRITE;
	    req->io_Data    = cmd->sc_Buf;
	    req->io_Length  =  cmd->sc_NBytes;

	    error = (DoIO((struct IORequest*)req));

	    break;

	case IFFCMD_SEEK:

	    DEBUG_BUFSTREAMHANDLER(dprintf("ClipStream: IFFCMD_SEEK...\n"));

	    req->io_Offset += cmd->sc_NBytes;

	    if (req->io_Offset < 0)
		error = TRUE;

	    break;

	case IFFCMD_INIT:

	    DEBUG_BUFSTREAMHANDLER(dprintf("ClipStream: IFFCMD_INIT...\n"));

	    /* Start reading and writing at offset 0 */
	    req->io_ClipID = 0;
	    req->io_Offset = 0;
	    break;

	case IFFCMD_CLEANUP:

	    DEBUG_BUFSTREAMHANDLER(dprintf("ClipStream: IFFCMD_CLEANUP...\n"));

	    if ((iff->iff_Flags & IFFF_RWBITS) == IFFF_READ)
	    {
		/* Read past end of clip if we are in read mode */
		req->io_Command = CMD_READ;
		req->io_Data    = NULL;
		req->io_Length  =  CLIPSCANBUFSIZE;

		/* Read until there is not more left */
		do
                {
                    DoIO((struct IORequest*)req);
		}
		while (req->io_Actual==req->io_Length);

	    }

	    if ((iff->iff_Flags & IFFF_RWBITS) == IFFF_WRITE)
	    {
	        req->io_Command = CMD_UPDATE;
		DoIO((struct IORequest*)req);
	    }
	    break;

    }

    DEBUG_STREAM(bug("ClipStream: error %ld\n", error));

    return (error);
}
