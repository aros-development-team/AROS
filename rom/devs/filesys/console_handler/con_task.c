/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Filesystem that uses console device for input/output.
    Lang: english
*/


#include <proto/exec.h>
#include <exec/libraries.h>
#include <exec/resident.h>
#include <exec/memory.h>
#include <exec/io.h>
#include <exec/errors.h>
#include <exec/alerts.h>
#include <utility/tagitem.h>
#include <dos/filesystem.h>
#include <dos/exall.h>
#include <dos/dosasl.h>
#include <intuition/intuition.h>
#include <proto/dos.h>
#include <proto/intuition.h>
#include <devices/conunit.h>

#undef SDEBUG
#undef DEBUG
// #define SDEBUG 1
// #define DEBUG 1
#include <aros/debug.h>

#include "con_handler_intern.h"
#include "support.h"
#include "completion.h"

#include <string.h>
#include <stdio.h>

/****************************************************************************************/

#define ioReq(x) ((struct IORequest *)x)

/****************************************************************************************/


#if 0
static const struct TagItem win_tags[] =
{
    {WA_Width,		500},
    {WA_Height,		300},
    {WA_SmartRefresh,	TRUE},
    {WA_DragBar,	TRUE},
    {WA_IDCMP,		0},
    {WA_Title,		(IPTR)"CON:"},
    {WA_Flags,		WFLG_DEPTHGADGET | WFLG_SIZEGADGET | WFLG_DRAGBAR | WFLG_CLOSEGADGET | WFLG_SIZEBRIGHT },
    {TAG_DONE,		0UL}
};
#endif

static const struct NewWindow default_nw =
{
    0,				/* LeftEdge */
    0,				/* TopEdge */
    600,			/* Width */
    300,			/* Height */
    1,				/* DetailPen */
    0,				/* BlockPen */
    0,		    	    	/* IDCMP */
    WFLG_DEPTHGADGET   |
    WFLG_SIZEGADGET    |
    WFLG_DRAGBAR       |
    WFLG_SIZEBRIGHT    |
    WFLG_SMART_REFRESH |
    WFLG_NOCAREREFRESH |
    WFLG_ACTIVATE,
    0,				/* FirstGadget */
    0,				/* CheckMark */
    "CON:",			/* Title */
    0,				/* Screen */
    0,				/* Bitmap */
    100,			/* MinWidth */
    100,			/* MinHeight */
    32767,			/* MaxWidth */
    32767,			/* MaxHeight */
    WBENCHSCREEN		/* type */
};

/****************************************************************************************/

static void close_con(struct conbase *conbase, struct IOFileSys *iofs)
{
    struct filehandle *fh = (struct filehandle *)iofs->IOFS.io_Unit;

    EnterFunc(bug("close_con(fh=%p)\n", fh));
    
    fh->usecount--;
    if (fh->usecount > 0)
    {
	if (iofs->IOFS.io_Message.mn_ReplyPort->mp_SigTask == fh->breaktask)
	{
	    fh->breaktask = NULL;
	}	
	if (iofs->IOFS.io_Message.mn_ReplyPort->mp_SigTask == fh->lastwritetask)
	{
	    fh->lastwritetask = NULL;
	}	
    }
    else
    {
    	fh->breaktask = NULL;
	iofs->IOFS.io_Unit = NULL;
    }

    iofs->io_DosError = 0;
    ReplyMsg(&iofs->IOFS.io_Message);
}

/****************************************************************************************/

static void con_read(struct conbase *conbase, struct IOFileSys *iofs)
{
    struct filehandle *fh = (struct filehandle *)iofs->IOFS.io_Unit;

    if (fh->flags & FHFLG_CANREAD)
    {
        ULONG readlen = (fh->canreadsize < iofs->io_Union.io_READ.io_Length) ? fh->canreadsize :
									       iofs->io_Union.io_READ.io_Length;
	/* we must correct io_READ.io_Length, because answer_read_request
	   would read until fh->inputsize if possible, but here it is allowed only
	   to read max. fh->canreadsize chars */

	iofs->io_Union.io_READ.io_Length = readlen;
	answer_read_request(conbase, fh, iofs);

	fh->canreadsize -= readlen;
	if (fh->canreadsize == 0) fh->flags &= ~FHFLG_CANREAD;
	
    }
    else
    {    
    	if (fh->flags & FHFLG_EOF)
	{
	    iofs->io_Union.io_READ.io_Length = 0;
	    iofs->IOFS.io_Error     	     = 0;
	    iofs->io_DosError 	    	     = 0;

	    ReplyMsg(&iofs->IOFS.io_Message);

	    fh->flags &= ~FHFLG_EOF;
	}
	else
	{
	    AddTail((struct List *)&fh->pendingReads, (struct Node *)iofs);
	    fh->flags |= FHFLG_READPENDING;
	}
    }
}

/****************************************************************************************/

static void con_write(struct conbase *conbase, struct IOFileSys *iofs)
{
    struct filehandle *fh = (struct filehandle *)iofs->IOFS.io_Unit;

    EnterFunc(bug("con_write(fh=%p, buf=%s)\n", fh, iofs->io_Union.io_WRITE.io_Buffer));

#if 0

    /* stegerg: this seems to be wrong */

    /* Change the task to which CTRL/C/D/E/F signals are sent to
       the task which sent this write request */

    /* PARANOIA ^ 3 */
       
    if (iofs->IOFS.io_Message.mn_ReplyPort)
    if ((iofs->IOFS.io_Message.mn_ReplyPort->mp_Flags & PF_ACTION) == PA_SIGNAL)
    if (iofs->IOFS.io_Message.mn_ReplyPort->mp_SigTask)
    {
    	fh->breaktask = iofs->IOFS.io_Message.mn_ReplyPort->mp_SigTask;
    }
#endif

    /* PARANOIA ^ 3 */
       
    if (iofs->IOFS.io_Message.mn_ReplyPort)
    if ((iofs->IOFS.io_Message.mn_ReplyPort->mp_Flags & PF_ACTION) == PA_SIGNAL)
    if (iofs->IOFS.io_Message.mn_ReplyPort->mp_SigTask)
    {
    	fh->lastwritetask = iofs->IOFS.io_Message.mn_ReplyPort->mp_SigTask;
    }
    
#if !BETTER_WRITE_HANDLING    
    if ((fh->inputsize - fh->inputstart) == 0)
    {
        answer_write_request(conbase, fh, iofs);
    } else {
#endif
    	AddTail((struct List *)&fh->pendingWrites, (struct Node *)iofs);
	fh->flags |= FHFLG_WRITEPENDING;
#if !BETTER_WRITE_HANDLING
    }
#endif
}

/****************************************************************************************/

LONG MakeConWindow(struct filehandle *fh, struct conbase *conbase)
{
    LONG err = 0;

    struct TagItem win_tags [] =
    {
	{WA_PubScreen	,0	    },
	{WA_AutoAdjust	,TRUE       },
	{WA_PubScreenName, NULL     },
	{WA_PubScreenFallBack, TRUE },
	{TAG_DONE                   }
    };

    win_tags[2].ti_Data = (IPTR)fh->screenname;
    fh->window = OpenWindowTagList(&fh->nw, (struct TagItem *)win_tags);

    if (fh->window)
    {
    	D(bug("contask: window opened\n"));
	fh->conreadio->io_Data   = (APTR)fh->window;
	fh->conreadio->io_Length = sizeof (struct Window);

	if (0 == OpenDevice("console.device", CONU_STANDARD, ioReq(fh->conreadio), 0))
	{
	    const UBYTE lf_on[] = {0x9B, 0x32, 0x30, 0x68 }; /* Set linefeed mode    */

	    D(bug("contask: device opened\n"));

    	    fh->flags |= FHFLG_CONSOLEDEVICEOPEN;

	    fh->conwriteio = *fh->conreadio;
	    fh->conwriteio.io_Message.mn_ReplyPort = fh->conwritemp;

	    /* Turn the console into LF+CR mode so that both
	       linefeed and carriage return is done on
	    */
	    fh->conwriteio.io_Command	= CMD_WRITE;
	    fh->conwriteio.io_Data	= (APTR)lf_on;
	    fh->conwriteio.io_Length	= 4;

	    DoIO(ioReq(&fh->conwriteio));

	} /* if (0 == OpenDevice("console.device", CONU_STANDARD, ioReq(fh->conreadio), 0)) */
	else
	{
	    err = ERROR_INVALID_RESIDENT_LIBRARY;
	}
	if (err) CloseWindow(fh->window);

    } /* if (fh->window) */
    else
    {
	err = ERROR_NO_FREE_STORE;
    }
    
    return err;
}

/****************************************************************************************/

VOID StartAsyncConsoleRead(struct filehandle *fh, struct conbase *conbase)
{
    if (fh->window)
    {
	fh->conreadio->io_Command = CMD_READ;
	fh->conreadio->io_Data    = fh->consolebuffer;
	fh->conreadio->io_Length  = CONSOLEBUFFER_SIZE;

	SendIO(ioReq(fh->conreadio));

	fh->flags |= FHFLG_ASYNCCONSOLEREAD;
    }
}

/****************************************************************************************/

BOOL MakeSureWinIsOpen(struct conbase *conbase, struct IOFileSys *iofs, struct filehandle *fh)
{
    LONG err;
    
    if (fh->window) return TRUE;
    
    err = MakeConWindow(fh, conbase);
    if (err)
    {
    	iofs->io_DosError = err;
    	ReplyMsg(&iofs->IOFS.io_Message);
 	return FALSE;
    }

    /* Send first read request to console.device */

    StartAsyncConsoleRead(fh, conbase);	
    
    return TRUE;
    
}

/****************************************************************************************/

VOID HandlePendingReads(struct conbase *conbase, struct filehandle *fh)
{
    if (fh->flags & FHFLG_READPENDING)
    {			    
	struct IOFileSys *iofs, *next_iofs;

	ForeachNodeSafe(&fh->pendingReads, iofs, next_iofs)
	{
            Remove((struct Node *)iofs);
	    answer_read_request(conbase, fh, iofs);

	    if (fh->inputsize == 0) break;

	} /* ForeachNodeSafe(&fh->pendingReads, iofs, nextiofs) */

	if (IsListEmpty(&fh->pendingReads)) fh->flags &= ~FHFLG_READPENDING;

    }

    if (fh->inputsize)
    {
	fh->flags |= FHFLG_CANREAD;
	fh->canreadsize = fh->inputsize;
    }
}

/****************************************************************************************/

#undef SysBase
#define SysBase sysbase

AROS_UFH3(VOID, conTaskEntry,
    AROS_UFHA(STRPTR, argstr, A0),
    AROS_UFHA(ULONG, arglen, D0),
    AROS_UFHA(struct ExecBase *, sysbase, A6)
)
{
    struct conTaskParams *param = (struct conTaskParams *)FindTask(NULL)->tc_UserData;

#undef SysBase
#define SysBase conbase->sysbase

    struct conbase  	*conbase = param->conbase;

    struct filehandle 	*fh;
    struct IOFileSys 	*iofs = param->iofs;
#if DEBUG
    STRPTR  	    	filename = iofs->io_Union.io_OPEN.io_Filename;
#endif
    LONG    	    	err = 0;

    BOOL    	    	ok = FALSE;

    D(bug("conTaskEntry: taskparams = %x  conbase = %x  iofs = %x  filename = \"%s\"\n",
    			param, conbase, iofs, filename));

    fh = AllocMem(sizeof (struct filehandle), MEMF_PUBLIC | MEMF_CLEAR);
    if (fh)
    {
    	D(bug("contask: fh allocated\n"));

	fh->usecount  = 1;

	/* if iofs->IOFS.io_Unit equals 1 it means that we have to start in RAW mode */
	if (iofs->IOFS.io_Unit)
	    fh->flags |= FHFLG_RAW;

        fh->contask   = FindTask(NULL);
	fh->breaktask = param->parentTask;

	NEWLIST(&fh->pendingReads);
	NEWLIST(&fh->pendingWrites);

    	/* Create msgport for console.device communication
	   and for app <-> contask communication  */
	fh->conreadmp = AllocMem(sizeof (struct MsgPort) * 3, MEMF_PUBLIC|MEMF_CLEAR);
	if (fh->conreadmp)
	{

    	    D(bug("contask: mem for conreadmp, conwritemp and contaskmp allocated\n"));

	    fh->conreadmp->mp_Node.ln_Type = NT_MSGPORT;
	    fh->conreadmp->mp_Flags = PA_SIGNAL;
	    fh->conreadmp->mp_SigBit = AllocSignal(-1);
	    fh->conreadmp->mp_SigTask = fh->contask;
	    NEWLIST(&fh->conreadmp->mp_MsgList);

	    fh->conwritemp = fh->conreadmp + 1;

	    fh->conwritemp->mp_Node.ln_Type = NT_MSGPORT;
	    fh->conwritemp->mp_Flags = PA_SIGNAL;
	    fh->conwritemp->mp_SigBit = AllocSignal(-1);
	    fh->conwritemp->mp_SigTask = fh->contask;
	    NEWLIST(&fh->conwritemp->mp_MsgList);

	    fh->contaskmp = fh->conwritemp + 1;

	    fh->contaskmp->mp_Node.ln_Type = NT_MSGPORT;
	    fh->contaskmp->mp_Flags = PA_SIGNAL;
	    fh->contaskmp->mp_SigBit = AllocSignal(-1);
	    fh->contaskmp->mp_SigTask = fh->contask;
	    NEWLIST(&fh->contaskmp->mp_MsgList);

	    fh->conreadio = (struct IOStdReq *)CreateIORequest(fh->conreadmp, sizeof (struct IOStdReq));
	    if (fh->conreadio)
	    {
    	    	D(bug("contask: conreadio created\n"));

		fh->nw = default_nw;

		parse_filename(conbase, fh, iofs, &fh->nw);

    	    	if (!(fh->flags & FHFLG_AUTO))
		{
		    err = MakeConWindow(fh, conbase);
		    if (!err) ok = TRUE;
		}
		else
		{
		    ok = TRUE;
		}

		if (ok)
		{
		   iofs->IOFS.io_Unit = (struct Unit *)fh;
		}
		else
		{
		    DeleteIORequest( ioReq(fh->conreadio) );
		}

	    } /* if (fh->conreadio) */
	    else
	    {
	    	err = ERROR_NO_FREE_STORE;
	    }

	    if (!ok) FreeMem(fh->conreadmp, sizeof(struct MsgPort) * 3);

	} /* if (fh->conreadmp) */
	else
	{
	    err = ERROR_NO_FREE_STORE;
	}

	if (!ok) FreeMem(fh, sizeof (struct filehandle));

    } /* if (fh) */
    else
    	err = ERROR_NO_FREE_STORE;

    iofs->io_DosError = err;

    Signal(param->parentTask, param->initSignal);
    if (err)
    {
        D(bug("con task: initialization failed. waiting for parent task to kill me.\n"));
        /* parent Task will kill us */
        Wait(0);
    }

    D(bug("con task: initialization okay. entering main loop.\n"));

    /* Main Loop */

    /* Send first read request to console.device */

    StartAsyncConsoleRead(fh, conbase);

    for(;;)
    {
        ULONG conreadmask = 1L << fh->conreadmp->mp_SigBit;
	ULONG contaskmask = 1L << fh->contaskmp->mp_SigBit;
	ULONG sigs;

    	if (fh->usecount == 0)
	{
	    if ((fh->flags & FHFLG_EOF) ||
	    	(fh->window == NULL) ||
		(!(fh->flags & FHFLG_WAIT)))
	    {
	    	break;
	    }
	}
	
#if BETTER_WRITE_HANDLING
	/* Dont wait if a write is pending and a write really can be done */
	if ((fh->flags & FHFLG_WRITEPENDING) &&
	    (fh->inputpos == fh->inputstart) &&
	    ((fh->inputsize - fh->inputstart) == 0))
	{
	    sigs = CheckSignal(conreadmask | contaskmask);
	}
	else
#endif
	{
	    D(bug("contask: waiting for sigs %x\n",conreadmask | contaskmask));
	    sigs = Wait(conreadmask | contaskmask);
	}

	if (sigs & contaskmask)
	{
	    /* FSA mesages */
	    D(bug("contask: recevied contask signal\n"));
	    while((iofs = (struct IOFileSys *)GetMsg(fh->contaskmp)))
	    {
		switch(iofs->IOFS.io_Command)
		{
		    case FSA_CLOSE:
	    		close_con(conbase, iofs);
	    		break;

		    case FSA_READ:
		    	if (!MakeSureWinIsOpen(conbase, iofs, fh)) break;
 			con_read(conbase, iofs);
			break;

		    case FSA_WRITE:
		    	if (!MakeSureWinIsOpen(conbase, iofs, fh)) break;
			con_write(conbase, iofs);
			break;

    	    	    case FSA_CHANGE_SIGNAL:
		    	fh->breaktask = iofs->io_Union.io_CHANGE_SIGNAL.io_Task;
    	    	    	ReplyMsg(&iofs->IOFS.io_Message);
			break;

    	    	    case FSA_CONSOLE_MODE:
			{
			    UWORD newmode = iofs->io_Union.io_CONSOLE_MODE.io_ConsoleMode ? 1 : 0;
			    UWORD oldmode = (fh->flags & FHFLG_RAW) ? 1 : 0;

			    if (newmode != oldmode)
			    {
			    	if (newmode)
				{
				    /* Switching from CON: mode to RAW: mode */

				    fh->flags |= FHFLG_RAW;

				    fh->inputstart = fh->inputsize;
				    fh->inputpos   = fh->inputsize;

				    HandlePendingReads(conbase, fh);
				}
				else
				{
				    /* Switching from RAW: mode to CON: mode */

				    fh->flags &= ~FHFLG_RAW;
 				}

			    } /*  if (newmode != oldmode) */

			}
    	    	    	ReplyMsg(&iofs->IOFS.io_Message);
		    	break;

		} /* switch(iofs->IOFS.io_Command) */

	    } /* while((iofs = (struct IOFileSys *)GetMsg(fh->contaskmp))) */

	} /* if (sigs & contaskmask) */

	if (sigs & conreadmask)
	{
	    UBYTE c;
	    WORD inp;

	    /* console.device read request completed */
	    D(bug("contask: received console device signal\n"));

	    GetMsg(fh->conreadmp);

	    fh->conbuffersize = fh->conreadio->io_Actual;
	    fh->conbufferpos = 0;

	    /* terminate with 0 char */
	    fh->consolebuffer[fh->conbuffersize] = '\0';

    	    if (fh->flags & FHFLG_RAW)
	    {
	    	/* raw mode */

		for(inp = 0; (inp < fh->conbuffersize) && (fh->inputpos <  INPUTBUFFER_SIZE); )
		{
		    fh->inputbuffer[fh->inputpos++] = fh->consolebuffer[inp++];
		}

	    	fh->inputsize = fh->inputstart = fh->inputpos;
		
		HandlePendingReads(conbase, fh);
		
	    } /* if (fh->flags & FHFLG_RAW) */
	    else
	    {
	    	/* Cooked mode */
		
		while((inp = scan_input(conbase, fh, &c)) != INP_DONE)
		{
		    D(bug("Input Code: %d\n",inp));

		    switch(inp)
		    {
			case INP_CURSORLEFT:
			    if (fh->inputpos > fh->inputstart)
			    {
				fh->inputpos--;
				do_movecursor(conbase, fh, CUR_LEFT, 1);
			    }
			    break;

			case INP_SHIFT_CURSORLEFT: /* move to beginning of line */
			case INP_HOME:
			    if (fh->inputpos > fh->inputstart)
			    {
				do_movecursor(conbase, fh, CUR_LEFT, fh->inputpos - fh->inputstart);
				fh->inputpos = fh->inputstart;
			    }
			    break;

			case INP_CURSORRIGHT:
			    if (fh->inputpos < fh->inputsize)
			    {
				fh->inputpos++;
				do_movecursor(conbase, fh, CUR_RIGHT, 1);
			    }
			    break;

			case INP_SHIFT_CURSORRIGHT: /* move to end of line */
			case INP_END:
			    if (fh->inputpos != fh->inputsize)
			    {
				do_movecursor(conbase, fh, CUR_RIGHT, fh->inputsize - fh->inputpos);
				fh->inputpos = fh->inputsize;
			    }
			    break;

			case INP_CURSORUP: /* walk through cmd history */
			case INP_CURSORDOWN:
			case INP_SHIFT_CURSORUP:
			case INP_SHIFT_CURSORDOWN:
			    history_walk(conbase, fh, inp);
		    	    break;

			case INP_BACKSPACE:
			    if (fh->inputpos > fh->inputstart)
			    {
				do_movecursor(conbase, fh, CUR_LEFT, 1);

				if (fh->inputpos == fh->inputsize)
				{
				    do_deletechar(conbase, fh);

				    fh->inputsize--;
				    fh->inputpos--;
				} else {
				    WORD chars_right = fh->inputsize - fh->inputpos;

				    fh->inputsize--;
				    fh->inputpos--;

				    do_cursorvisible(conbase, fh, FALSE);
				    do_write(conbase, fh, &fh->inputbuffer[fh->inputpos + 1], chars_right);
				    do_deletechar(conbase, fh);
				    do_movecursor(conbase, fh, CUR_LEFT, chars_right);
				    do_cursorvisible(conbase, fh, TRUE);

				    memmove(&fh->inputbuffer[fh->inputpos], &fh->inputbuffer[fh->inputpos + 1], chars_right);

				}
			    }
			    break;

			case INP_SHIFT_BACKSPACE:
		            if (fh->inputpos > fh->inputstart)
			    {
				do_movecursor(conbase, fh, CUR_LEFT, fh->inputpos - fh->inputstart);
				if (fh->inputpos == fh->inputsize)
				{
				    do_eraseinline(conbase, fh);

			    	    fh->inputpos = fh->inputsize = fh->inputstart;
				} else {
			            WORD chars_right = fh->inputsize - fh->inputpos;

			            do_cursorvisible(conbase, fh, FALSE);
				    do_write(conbase, fh, &fh->inputbuffer[fh->inputpos], chars_right);
				    do_eraseinline(conbase, fh);
				    do_movecursor(conbase, fh, CUR_LEFT, chars_right);
				    do_cursorvisible(conbase, fh, TRUE);

				    memmove(&fh->inputbuffer[fh->inputstart], &fh->inputbuffer[fh->inputpos], chars_right);

				    fh->inputsize -= (fh->inputpos - fh->inputstart);
				    fh->inputpos = fh->inputstart;
				}
			    }
		    	    break;

			case INP_DELETE:
			    if (fh->inputpos < fh->inputsize)
			    {
				fh->inputsize--;

				if (fh->inputpos == fh->inputsize)
				{
				    do_deletechar(conbase, fh);
				} else {
				    WORD chars_right = fh->inputsize - fh->inputpos;

				    do_cursorvisible(conbase, fh, FALSE);
				    do_write(conbase, fh, &fh->inputbuffer[fh->inputpos + 1], chars_right);
				    do_deletechar(conbase, fh);
				    do_movecursor(conbase, fh, CUR_LEFT, chars_right);
				    do_cursorvisible(conbase, fh, TRUE);

				    memmove(&fh->inputbuffer[fh->inputpos], &fh->inputbuffer[fh->inputpos + 1], chars_right);
				}
			    }
			    break;

			case INP_SHIFT_DELETE:
		            if (fh->inputpos < fh->inputsize)
			    {
				fh->inputsize = fh->inputpos;
				do_eraseinline(conbase, fh);
			    }
		    	    break;

			case INP_CONTROL_X:
			    if ((fh->inputsize - fh->inputstart) > 0)
			    {
				if (fh->inputpos > fh->inputstart)
				{
				    do_movecursor(conbase, fh, CUR_LEFT, fh->inputpos - fh->inputstart);
				}
				do_eraseinline(conbase, fh);

				fh->inputpos = fh->inputsize = fh->inputstart;
			    }
			    break;

    	    	    	case INP_ECHO_STRING:
			    do_write(conbase, fh, &c, 1);
			    break;

			case INP_STRING:
			    if (fh->inputsize < INPUTBUFFER_SIZE)
			    {
				do_write(conbase, fh, &c, 1);

				if (fh->inputpos == fh->inputsize)
				{
				    fh->inputbuffer[fh->inputpos++] = c;
				    fh->inputsize++;
				} else {
				    WORD chars_right = fh->inputsize - fh->inputpos;

				    do_cursorvisible(conbase, fh, FALSE);
				    do_write(conbase, fh, &fh->inputbuffer[fh->inputpos], chars_right);
				    do_movecursor(conbase, fh, CUR_LEFT, chars_right);
				    do_cursorvisible(conbase, fh, TRUE);

				    memmove(&fh->inputbuffer[fh->inputpos + 1], &fh->inputbuffer[fh->inputpos], chars_right);
				    fh->inputbuffer[fh->inputpos++] = c;
				    fh->inputsize++;
				}
			    }
			    break;

    	    		case INP_EOF:
		    	    fh->flags |= FHFLG_EOF;
                            if (fh->flags & FHFLG_AUTO && fh->window)
                            {
                                CloseWindow(fh->window);
                                fh->window = NULL;
                            }

    	    	    	    /* fall through */

			case INP_RETURN:
		            if (fh->inputsize < INPUTBUFFER_SIZE)
			    {
				if (inp != INP_EOF)
				{
		        	    c = '\n';
				    do_write(conbase, fh, &c, 1);

				    add_to_history(conbase, fh);

				    fh->inputbuffer[fh->inputsize++] = '\n';
				}

				fh->inputstart = fh->inputsize;
				fh->inputpos = fh->inputstart;

				if (fh->inputsize) HandlePendingReads(conbase, fh);

				if ((fh->flags & FHFLG_EOF) && (fh->flags & FHFLG_READPENDING))
				{
			    	    struct IOFileSys *iofs = (struct IOFileSys *)RemHead((struct List *)&fh->pendingReads);

				    if (iofs)
				    {
					iofs->io_Union.io_READ.io_Length = 0;
					iofs->IOFS.io_Error     	     = 0;
					iofs->io_DosError 	    	     = 0;

					ReplyMsg(&iofs->IOFS.io_Message);

					fh->flags &= ~FHFLG_EOF;
				    }

			    	    if (IsListEmpty(&fh->pendingReads)) fh->flags &= ~FHFLG_READPENDING;
				}

			    } /* if (fh->inputsize < INPUTBUFFER_SIZE) */
			    break;

			case INP_LINEFEED:
		            if (fh->inputsize < INPUTBUFFER_SIZE)
			    {
				c = '\n';
				do_write(conbase, fh, &c, 1);

				add_to_history(conbase, fh);

				fh->inputbuffer[fh->inputsize++] = c;
				fh->inputstart = fh->inputsize;
				fh->inputpos = fh->inputsize;
			    }
		    	    break;

			case INP_CTRL_C:
			case INP_CTRL_D:
			case INP_CTRL_E:
			case INP_CTRL_F:
		            if (fh->breaktask)
			    {
				Signal(fh->breaktask, 1L << (12 + inp - INP_CTRL_C));
			    }
		    	    break;

    	    	    	case INP_TAB:
			    Completion(conbase, fh);
			    break;
			    
		    } /* switch(inp) */

		} /* while((inp = scan_input(conbase, fh, &c)) != INP_DONE) */

    	    } /* if (fh->flags & FHFLG_RAW) else ... */

	    /* wait for next input from console.device */

	    StartAsyncConsoleRead(fh, conbase);

	} /* if (sigs & conmask) */

	/* pending writes ? */

	if ((fh->flags & FHFLG_WRITEPENDING) &&
	    (fh->inputpos == fh->inputstart) &&
	    ((fh->inputsize - fh->inputstart) == 0))
	{
	    struct IOFileSys *iofs, *iofs_next;

#if BETTER_WRITE_HANDLING
	    ForeachNodeSafe(&fh->pendingWrites, iofs, iofs_next)
	    {
		if (answer_write_request(conbase, fh, iofs) != 0)
		{
		    /* Write was done only partly */
		    break;
		}
		/* Break even here (means execute only one request), so that
		   reads can be handled inbetween */
		break;
	    }

	    if (IsListEmpty(&fh->pendingWrites)) fh->flags &= ~FHFLG_WRITEPENDING;
#else
	    ForeachNodeSafe(&fh->pendingWrites, iofs, iofs_next)
	    {
		Remove((struct Node *)iofs);
		answer_write_request(conbase, fh, iofs);
	    }

	    fh->flags &= ~FHFLG_WRITEPENDING;
#endif
	} /* if ((fh->flags & FHFLG_WRITEPENDING) && (fh->inputpos == fh->inputstart) && (fh->inputsize == 0)) */

    } /* for(;;) */

    if (fh->flags & FHFLG_ASYNCCONSOLEREAD)
    {
	/* Abort all pending requests */
	if (!CheckIO( ioReq(fh->conreadio) ))
    	    AbortIO( ioReq(fh->conreadio) );

	/* Wait for abort */
	WaitIO( ioReq(fh->conreadio) );
    }

    /* Clean up */

    if (fh->flags & FHFLG_CONSOLEDEVICEOPEN)
    	CloseDevice((struct IORequest *)fh->conreadio);

    if (fh->window)
    	CloseWindow(fh->window);
	
    DeleteIORequest( ioReq(fh->conreadio) );
    FreeMem(fh->conreadmp, sizeof (struct MsgPort) * 3);

    if (fh->screenname) FreeVec(fh->screenname);
    if (fh->wintitle) FreeVec(fh->wintitle);

    FreeMem(fh, sizeof (struct filehandle));
}

/****************************************************************************************/
/****************************************************************************************/
/****************************************************************************************/
/****************************************************************************************/
