/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: 
    Lang: English
*/

/*****************************************************************************************/

#include <aros/asmcall.h>
#include <dos/dos.h>
#include <intuition/intuition.h>
#include <intuition/sghooks.h>
#include <intuition/classusr.h>
#include <libraries/iffparse.h>
#include <devices/inputevent.h>
#include <datatypes/textclass.h>
#include <utility/hooks.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/intuition.h>
#include <proto/iffparse.h>
#include <proto/alib.h>
#include <proto/utility.h>

#include <string.h>
#include <stdlib.h>


#undef DEBUG
#define DEBUG 0
#include <aros/debug.h>

/*****************************************************************************************/

#define ARG_TEMPLATE 	"CLIPUNIT=UNIT/N,OFF/S"
#define ARG_CLIPUNIT 	0
#define ARG_OFF		1
#define NUM_ARGS	2

/*****************************************************************************************/

#define CODE_COPY	'C'
#define CODE_PASTE	'V'

struct MyEditHookMsg
{
    struct Message 	msg;
    struct SGWork	*sgw;
    WORD 		code;
};

/*****************************************************************************************/

const STRPTR CONCLIP_TASKNAME = "« ConClip »";
const STRPTR CONCLIP_PORTNAME = "ConClip.rendezvous";

/*****************************************************************************************/

static struct MsgPort 	*progport;
static struct Hook	edithook, *oldedithook;
static struct Task	*progtask;
static ULONG 		clipunit, portmask;
static BOOL 		off;
static UBYTE 		s[256];

/*****************************************************************************************/

static void cleanup(STRPTR msg)
{
    if (msg) Printf("ConClip: %s\n", msg);

    if (oldedithook) SetEditHook(oldedithook);
    if (progport) DeletePort(progport);

    exit(0);
}

/*****************************************************************************************/

static void init(void)
{
    progtask = FindTask(NULL);
    ((struct Process *)progtask)->pr_WindowPtr = (APTR)-1;
}

/*****************************************************************************************/

static void getarguments(void)
{
    struct RDArgs 	*myargs;
    IPTR 		args[NUM_ARGS] = {0, 0};
    
    if (!(myargs = ReadArgs(ARG_TEMPLATE, args, NULL)))
    {
        Fault(IoErr(), 0, s, 255);
	cleanup(s);
    }
    
    if (args[ARG_CLIPUNIT]) clipunit = (ULONG)(*(IPTR *)args[ARG_CLIPUNIT]);
    if (args[ARG_OFF]) off = args[ARG_OFF] ? TRUE : FALSE;
    
    FreeArgs(myargs);
}

/*****************************************************************************************/

static void checkport(void)
{
    Forbid();
    progport = FindPort(CONCLIP_PORTNAME);
    if (progport)
    {
        if (off) Signal(progport->mp_SigTask, SIGBREAKF_CTRL_C);
	Permit();
	progport = NULL;
	cleanup(NULL);
    }
    progport = CreatePort(CONCLIP_PORTNAME, 1);
    Permit();

    if (!progport) cleanup("Could not create MsgPort!");
    
    portmask = 1L << progport->mp_SigBit;
}

/*****************************************************************************************/

AROS_UFH3(ULONG, conclipeditfunc,
    AROS_UFHA(struct Hook *,		hook,		A0),
    AROS_UFHA(struct SGWork *,		sgw,		A2),
    AROS_UFHA(ULONG *, 			command,	A1)
)
{
    AROS_USERFUNC_INIT

    struct MsgPort 	 *port, replyport;
    struct MyEditHookMsg msg;
    BOOL		 calloldhook = TRUE;
    ULONG 		 retcode = 0;
        
    switch (*command)
    {
	case SGH_KEY:
	    D(bug("ConClip/conclipeditfunc: is SGH_KEY\n"));
	    
	    if (sgw->IEvent->ie_Qualifier & IEQUALIFIER_RCOMMAND)
	    {
	        D(bug("ConClip/conclipeditfunc: qualifier RCOMMAND okay\n"));

	        switch(ToUpper(sgw->Code))
		{
		    case 'C':
		    	if (!sgw->NumChars) break;
			/* fall through */

		    case 'V':
	    		D(bug("ConClip/conclipeditfunc: key = %c\n", toupper(sgw->Code)));

		        if ((port = FindPort(CONCLIP_PORTNAME)))
			{
			    calloldhook = FALSE;

			    replyport.mp_Node.ln_Type	= NT_MSGPORT;
			    replyport.mp_Node.ln_Name 	= NULL;
			    replyport.mp_Node.ln_Pri 	= 0;
			    replyport.mp_Flags 		= PA_SIGNAL;
			    replyport.mp_SigBit 	= SIGB_SINGLE;
			    replyport.mp_SigTask 	= FindTask(NULL);
			    NewList(&replyport.mp_MsgList);
			    
			    msg.msg.mn_Node.ln_Type 	= NT_MESSAGE;
			    msg.msg.mn_ReplyPort 	= &replyport;
			    msg.msg.mn_Length 		= sizeof(msg);
			    
			    msg.code = ToUpper(sgw->Code);
			    msg.sgw  = sgw;			    
			    
			    if ((msg.code == CODE_COPY) || (sgw->NumChars < sgw->StringInfo->MaxChars - 1))
			    {
				SetSignal(0, SIGF_SINGLE);
				PutMsg(port, &msg.msg);
				WaitPort(&replyport);
			    }
			    
			    if (msg.code == CODE_PASTE)
			    {
			        WORD len = strlen(sgw->WorkBuffer);
				
				if (len != sgw->NumChars)
				{
				    sgw->NumChars = len;
				    sgw->EditOp   = EO_BIGCHANGE;
				    sgw->Actions  = SGA_USE | SGA_REDISPLAY;
				    
				    retcode = 1;
				}
				
			    } /* if (msg.code == CODE_COPY) */
			    
			} /* if ((port = FindPort(CONCLIP_PORTNAME))) */
		
			break;
		    
		} /* switch(ToUpper(sgw->Code)) */
		
	    } /* if (sgw->IEvent->ie_Qualifier & IEQUALIFIER_RCOMMAND) */
    	    break;    
	    
    } /* switch (*command) */

    if (calloldhook) retcode = CallHookPkt(oldedithook, sgw, command);

    return retcode;

    AROS_USERFUNC_EXIT
}

/*****************************************************************************************/

static void installedithook(void)
{
    edithook.h_Entry = (HOOKFUNC)AROS_ASMSYMNAME(conclipeditfunc);
    edithook.h_SubEntry = NULL;
    edithook.h_Data = NULL;
    
    oldedithook = SetEditHook(&edithook);  
}

/*****************************************************************************************/
/*****************************************************************************************/
/*****************************************************************************************/

static void savetoclipboard(struct SGWork *sgw)
{
    struct IFFHandle *iff;
    
    if((iff = AllocIFF()))
    {
	if((iff->iff_Stream = (IPTR)OpenClipboard(clipunit)))
	{
	    InitIFFasClip(iff);
	    if(!OpenIFF(iff,IFFF_WRITE))
	    {
		if(!PushChunk(iff, ID_FTXT, ID_FORM, IFFSIZE_UNKNOWN))
		{
		    if(!PushChunk(iff, ID_FTXT, ID_CHRS, IFFSIZE_UNKNOWN))
		    {
		        WriteChunkBytes(iff, sgw->WorkBuffer, sgw->NumChars);
			
			PopChunk(iff);
			
		    } /* if(!PushChunk(iff, ID_FTXT, ID_CHRS, IFFSIZE_UNKNOWN)) */
		    PopChunk(iff);
		    
		} /* if(!PushChunk(iff, ID_FTXT, ID_FORM, IFFSIZE_UNKNOWN)) */
		CloseIFF(iff);
		
	    } /* if(!OpenIFF(iff,IFFF_WRITE)) */
	    CloseClipboard((struct ClipboardHandle*)iff->iff_Stream);
	    
	} /* if((iff->iff_Stream = (IPTR)OpenClipboard(clipunit))) */
	FreeIFF(iff);
	
    } /* if((iff = AllocIFF()))) */
}

/*****************************************************************************************/

static void readfromclipboard(struct SGWork *sgw)
{
    struct IFFHandle    *iff;
    struct ContextNode  *cn;
    
    if((iff = AllocIFF()))
    {
	D(bug("ConClip/conclipeditfunc: AllocIFF okay\n"));
	
	if((iff->iff_Stream = (IPTR)OpenClipboard(clipunit)))
	{
	    D(bug("ConClip/conclipeditfunc: OpenClipboard okay\n"));
	    
	    InitIFFasClip(iff);
	    if(!OpenIFF(iff, IFFF_READ))
	    {
		D(bug("ConClip/conclipeditfunc: OpenIff okay\n"));
		
		if (!(StopChunk(iff, ID_FTXT, ID_CHRS)))
		{
		    D(bug("ConClip/conclipeditfunc: StopChunk okay\n"));
		    
		    if (!ParseIFF(iff, IFFPARSE_SCAN))
		    {
			D(bug("ConClip/conclipeditfunc: ParseIFF okay\n"));
			
		        cn = CurrentChunk(iff);
               		if ((cn->cn_Type == ID_FTXT) && (cn->cn_ID == ID_CHRS) && (cn->cn_Size > 0))
			{
			    WORD readsize;
			    
    			    D(bug("ConClip: readfromclipboard: Found FTXT CHRS Chunk\n"));
			    D(bug("ConClip: readfromclipboard: Old text = \"%s\"\n", sgw->WorkBuffer));

			    readsize = sgw->StringInfo->MaxChars - 1 - sgw->BufferPos;
			    if (cn->cn_Size < readsize) readsize = cn->cn_Size;
			    if (readsize > 0)
			    {
			        memmove(sgw->WorkBuffer + sgw->BufferPos + readsize,
					sgw->WorkBuffer + sgw->BufferPos,
					sgw->StringInfo->MaxChars - sgw->BufferPos - readsize);
			        ReadChunkBytes(iff, sgw->WorkBuffer + sgw->BufferPos, readsize);
    				
				D(bug("ConClip: readfromclipboard: New text = \"%s\"\n", sgw->WorkBuffer));
				
				sgw->BufferPos += readsize;
			    }
			    
			} /* if ((cn->cn_Type == ID_FTXT) && (cn->cn_ID == ID_CHRS) && (cn->cn_Size > 0)) */
			
		    } /* if (!ParseIFF(iff, IFFPARSE_SCAN)) */
		    
		} /* if (!(StopChunk(iff, ID_FTXT, ID_CHRS))) */
		
		CloseIFF(iff);
		
	    } /* if(!OpenIFF(iff, IFFF_READ)) */
	    CloseClipboard((struct ClipboardHandle*)iff->iff_Stream);
	    
	} /* if((iff->iff_Stream = (IPTR)OpenClipboard(clipunit))) */
	FreeIFF(iff);
	
    } /* if((iff = AllocIFF()))) */
}

/*****************************************************************************************/

static void handleall(void)
{
    BOOL quitme = FALSE;
    ULONG sigs;
    
    while(!quitme)
    {
        sigs = Wait(SIGBREAKF_CTRL_C | portmask);
	
	if (sigs & portmask)
	{
	    struct MyEditHookMsg *msg;
	    
	    while((msg = (struct MyEditHookMsg *)GetMsg(progport)))
	    {
	        switch(msg->code)
		{
		    case CODE_COPY:
		        D(bug("ConClip: Received CODE_COPY message\n"));
			savetoclipboard(msg->sgw);
			break;
			
		    case CODE_PASTE:	
		    	D(bug("ConClip: Received CODE_PASTE message\n"));
			readfromclipboard(msg->sgw);
			break;
			
		} /* switch(msg->code) */
		
	        ReplyMsg(&msg->msg);
		
	    } /* while((msg = (struct MyEditHookMsg *)GetMsg(progport))) */
	    
	} /* if (sigs & portmask) */

	if (sigs & SIGBREAKF_CTRL_C) quitme = TRUE;
	
    } /* while(!quitme) */
}

/*****************************************************************************************/

int main(void)
{
    init();
    getarguments();
    checkport();
    installedithook();
    handleall();
    cleanup(NULL);
    return 0;
}

/*****************************************************************************************/
