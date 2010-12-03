
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

#include <stddef.h>
#include <string.h>

#include "con_handler_intern.h"
#include "support.h"

#undef SDEBUG
#undef DEBUG
#define SDEBUG 0
#define DEBUG 0
#include <aros/debug.h>

static char *BSTR2C(BSTR srcs)
{
	UBYTE *src = BADDR(srcs);
	char *dst;
	
	dst = AllocVec(src[0] + 1, MEMF_ANY);
	if (!dst)
		return NULL;
	memcpy (dst, src + 1, src[0]);
	dst[src[0]] = 0;
	return dst;
}
static WORD isdosdevicec(CONST_STRPTR s)
{
	UBYTE b = 0;
	while (s[b]) {
		if (s[b] == ':')
			return b;
		b++;
	}
	return -1;
}

#define ioReq(x) ((struct IORequest *)x)

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


static LONG MakeConWindow(struct filehandle *fh)
{
    LONG err = 0;

    struct TagItem win_tags [] =
    {
	{WA_PubScreen	,0	    },
	{WA_AutoAdjust	,TRUE       },
	{WA_PubScreenName, 0        },
	{WA_PubScreenFallBack, TRUE },
	{TAG_DONE                   }
    };

    win_tags[2].ti_Data = (IPTR)fh->screenname;
    D(bug("[contask] Opening window on screen %s, IntuitionBase = 0x%p\n", fh->screenname, IntuitionBase));
    fh->window = OpenWindowTagList(&fh->nw, (struct TagItem *)win_tags);

    if (fh->window)
    {
    	D(bug("contask: window opened\n"));
	fh->conreadio->io_Data   = (APTR)fh->window;
	fh->conreadio->io_Length = sizeof (struct Window);

	if (0 == OpenDevice("console.device", CONU_SNIPMAP, ioReq(fh->conreadio), 0))
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
        D(bug("[contask] Failed to open a window\n"));
	err = ERROR_NO_FREE_STORE;
    }
    
    return err;
}

static BOOL MakeSureWinIsOpen(struct filehandle *fh)
{
    D(bug("[contask] Console window handle: 0x%p\n", fh->window));
    if (fh->window)
    	return TRUE;
    return MakeConWindow(fh) == 0;
}

static void close_con(struct filehandle *fh)
{
	D(bug("exit\n"));
	for(;;);
}

static struct filehandle *open_con(struct DosPacket *dp, LONG *perr)
{
	char *filename, *fn;
	struct filehandle *fh;
	struct DeviceNode *dn;
	LONG err, ok;
	LONG i;
	
	dn = BADDR(dp->dp_Arg3);
	*perr = ERROR_NO_FREE_STORE;
	fh = AllocMem(sizeof(struct filehandle), MEMF_PUBLIC | MEMF_CLEAR);
    	if (!fh)
    		return NULL;

	fh->intuibase = OpenLibrary("intuition.library", 0);
	Forbid();
    	fh->inputbase = (struct Device *)FindName(&SysBase->DeviceList, "input.device");
    	Permit();

	err = 0;
	filename = BSTR2C(dp->dp_Arg1);
	fn = filename;
	i = isdosdevicec(fn);
	if (i >= 0)
		fn += i + 1;

 	fh->breaktask = 0;//param->parentTask;
 	fh->contask = FindTask(0);

	NEWLIST(&fh->pendingReads);
	NEWLIST(&fh->pendingWrites);


    	/* Create msgport for console.device communication
	   and for app <-> contask communication  */
	fh->conreadmp = AllocVec(sizeof (struct MsgPort) * 2, MEMF_PUBLIC|MEMF_CLEAR);
	if (fh->conreadmp)
	{

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


	    fh->conreadio = (struct IOStdReq *)CreateIORequest(fh->conreadmp, sizeof (struct IOStdReq));
	    if (fh->conreadio)
	    {
    	    	D(bug("contask: conreadio created\n"));

		fh->nw = default_nw;

		if (parse_filename(fh, fn, &fh->nw))
		{
                    if (!(fh->flags & FHFLG_AUTO))
                    {
                        err = MakeConWindow(fh);
                        if (!err)
                            ok = TRUE;
                    }
                    else
                    {
                        ok = TRUE;
                    }
                }
		else
		    err = ERROR_BAD_STREAM_NAME;

		if (!ok)
		{
		    DeleteIORequest(ioReq(fh->conreadio));
		}

	    } /* if (fh->conreadio) */
	    else
	    {
	    	err = ERROR_NO_FREE_STORE;
	    }

	} /* if (fh->conreadmp) */
	else
	{
	    err = ERROR_NO_FREE_STORE;
	}

	if (dn->dn_Startup)
		fh->flags |= FHFLG_RAW;

	if (!ok)
		FreeMem(fh, sizeof (struct filehandle));

 	*perr = err;
 	FreeVec(filename);
    	return fh;
}

static void startread(struct filehandle *fh)
{
	fh->conreadio->io_Command = CMD_READ;
	fh->conreadio->io_Data    = fh->consolebuffer;
	fh->conreadio->io_Length  = CONSOLEBUFFER_SIZE;
	SendIO((struct IORequest*)fh->conreadio);
	D(bug("startread\n"));
}

#if (AROS_FLAVOUR & AROS_FLAVOUR_BINCOMPAT)

    /* SegList points here, must be long aligned */
    __attribute__((aligned(4)))

#endif

LONG CONMain(void)
{
	struct MsgPort *mp;
	struct DosPacket *dp;
	struct Message *mn;
	struct FileHandle *dosfh;
	LONG error;
	struct MsgPort *timermp = NULL;
    	struct timerequest *timerreq = NULL;
    	struct filehandle *fh;
   	struct DosPacket *waitingdp = NULL;
   	BOOL readstarted = FALSE;
	
	D(bug("[CON] started\n"));
	mp = &((struct Process*)FindTask(NULL))->pr_MsgPort;
	WaitPort(mp);
	dp = (struct DosPacket*)GetMsg(mp)->mn_Node.ln_Name;	
	D(bug("[CON] startup message received. port=%x path='%b'\n", mp, dp->dp_Arg1));

	fh = open_con(dp, &error);
	if (!fh) {
		D(bug("CON: init failed\n"));
		goto end;
	}
	replypkt(dp, DOSTRUE);

    	timermp = CreateMsgPort();
    	timerreq = (struct timerequest*)CreateIORequest(timermp, sizeof(struct timerequest));
   	OpenDevice(TIMERNAME, UNIT_MICROHZ, (struct IORequest *)timerreq, 0);
 
    	for(;;)
    	{
        	ULONG conreadmask = 1L << fh->conreadmp->mp_SigBit;
		ULONG timermask   = 1L << timermp->mp_SigBit;
		ULONG packetmask = 1L << mp->mp_SigBit;
		ULONG sigs;

		sigs = Wait(packetmask | conreadmask | timermask);

		if (sigs & timermask) {
			if (waitingdp) {
				replypkt(waitingdp, DOSFALSE);
				waitingdp = NULL;
			}
		}

		if (sigs & conreadmask) {
			readstarted = FALSE;
			GetMsg(fh->conreadmp);
			if (waitingdp) {
				replypkt(waitingdp, DOSTRUE);
				AbortIO(timerreq);
				WaitIO(timerreq);
				waitingdp = NULL;
			}
			D(bug("IO_READ %d\n", fh->conreadio->io_Actual));
	    		fh->conbuffersize = fh->conreadio->io_Actual;
	    		fh->conbufferpos = 0;
	    		/* terminate with 0 char */
	    		fh->consolebuffer[fh->conbuffersize] = '\0';
			if (fh->flags & FHFLG_RAW)
			{
				LONG inp;
				/* raw mode */
				for(inp = 0; (inp < fh->conbuffersize) && (fh->inputpos <  INPUTBUFFER_SIZE); )
				{
				    fh->inputbuffer[fh->inputpos++] = fh->consolebuffer[inp++];
				}
				fh->inputsize = fh->inputstart = fh->inputpos;
				HandlePendingReads(fh);
			} /* if (fh->flags & FHFLG_RAW) */
			else
			{
			    	/* Cooked mode */
				/* disable output if we're not suppoed to be echoing */
				if (fh->flags & FHFLG_NOECHO)
					fh->flags |= FHFLG_NOWRITE;
				process_input(fh);
				/* re-enable output */
				fh->flags &= ~FHFLG_NOWRITE;
			} /* if (fh->flags & FHFLG_RAW) else ... */
			startread(fh);
			readstarted = TRUE;
		}

		while ((mn = GetMsg(mp))) {
			dp = (struct DosPacket*)mn->mn_Node.ln_Name;	
			dp->dp_Res2 = 0;
			D(bug("[CON] packet %x:%d\n", dp, dp->dp_Type));
			error = 0;
			switch (dp->dp_Type)
			{
				case ACTION_FINDINPUT:
				case ACTION_FINDOUTPUT:
				case ACTION_FINDUPDATE:
					dosfh = BADDR(dp->dp_Arg1);
					dosfh->fh_Port = (struct MsgPort*)DOSTRUE;
					dosfh->fh_Arg1 = (IPTR)fh;
					fh->usecount++;
					replypkt(dp, DOSTRUE);
				break;
				case ACTION_END:
					fh->usecount--;
					if (fh->usecount <= 0)
						goto end;
					replypkt(dp, DOSTRUE);
				break;
				case ACTION_READ:
					if (!MakeSureWinIsOpen(fh))
						goto end;
					D(bug("CON ACTION_READ %x %d\n", dp->dp_Arg2, dp->dp_Arg3));
					if (!readstarted) {
						startread(fh);
						readstarted = TRUE;
					}
					con_read(fh, dp);
				break;
				case ACTION_WRITE:
					D(bug("CON ACTION_WRITE %x %d\n", dp->dp_Arg2, dp->dp_Arg3));
					if (!MakeSureWinIsOpen(fh))
						goto end;
					answer_write_request(fh, dp);
				break;
				case ACTION_SCREEN_MODE:
					replypkt(dp, DOSTRUE);
				break;
				case ACTION_CHANGE_SIGNAL:
				{
					struct Task *old = fh->breaktask;
					if (dp->dp_Arg2)
		    				fh->breaktask = (struct Task*)dp->dp_Arg2;
 					replypkt2(dp, DOSTRUE, (LONG)old);
 				}
				break;
				case ACTION_WAIT_CHAR:
				{
				    if (fh->inputsize > 0)
				    {
				    	replypkt(dp, DOSTRUE);
				    }
				    else
				    {
					LONG timeout = dp->dp_Arg1;
					LONG sec = timeout / 1000000;
					LONG usec = timeout % 1000000;
	
					timerreq->tr_node.io_Command = TR_ADDREQUEST;
					timerreq->tr_time.tv_secs = sec;
					timerreq->tr_time.tv_micro = usec;
					SendIO((struct IORquest*)timerreq);
					waitingdp = dp;
				    }
				}
				break;
				case ACTION_IS_FILESYSTEM:
					replypkt(dp, FALSE);
				break;
				default:
					replypkt2(dp, FALSE, ERROR_NOT_IMPLEMENTED);
				break;
			}
		}
	}
end:
	close_con(fh);
	replypkt(dp, DOSFALSE);
	return 0;

}
