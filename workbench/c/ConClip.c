/*
    (C) 1999-2000 AROS - The Amiga Research OS
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
#include <utility/hooks.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/intuition.h>
#include <proto/iffparse.h>
#include <proto/alib.h>
#include <proto/utility.h>

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>

#undef DEBUG
#define DEBUG 1
#include <aros/debug.h>

/*****************************************************************************************/

#define ARG_TEMPLATE 	"CLIPUNIT=UNIT/N, OFF/S"
#define ARG_CLIPUNIT 	0
#define ARG_OFF		1
#define NUM_ARGS	2

/*****************************************************************************************/

#define CODE_COPY	'C'
#define CODE_PASTE	'V'

struct MyEditHookMsg
{
    struct Message 	msg;
    WORD 		code;
    STRPTR 		text;
};

/*****************************************************************************************/

const STRPTR CONCLIP_TASKNAME = "« ConClip »";
const STRPTR CONCLIP_PORTNAME = "ConClip.rendezvous";

/*****************************************************************************************/

struct Library 		*IFFParseBase;
struct UtilityBase	*UtilityBase;
struct IntuitionBase 	*IntuitionBase;

static struct MsgPort 	*progport;
static struct Hook	edithook, *oldedithook;
static struct Task	*progtask;
static ULONG 		clipunit, portmask;
static BOOL 		off;
static UBYTE 		s[256];

/*****************************************************************************************/

static void cleanup(STRPTR msg)
{
    if (msg) printf("ConClip: %s\n", msg);
    
    if (oldedithook) SetEditHook(oldedithook);
    if (progport) DeletePort(progport);
    
    if (IntuitionBase) CloseLibrary((struct Library *)IntuitionBase);
    if (UtilityBase) CloseLibrary((struct Library *)UtilityBase);
    if (IFFParseBase) CloseLibrary(IFFParseBase);
    
    exit(0);
}

/*****************************************************************************************/

static void openlibs(void)
{
    IFFParseBase = OpenLibrary("iffparse.library", 39);
    UtilityBase = (struct UtilityBase *)OpenLibrary("utility.library", 39);
    IntuitionBase = (struct IntuitionBase *)OpenLibrary("intuition.library", 39);
    
    if (!IFFParseBase || !UtilityBase || !IntuitionBase)
    {
        cleanup("Error opening libraries!");
    }
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
		
	        switch(toupper(sgw->Code))
		{
		    case 'C':
		    	if (!sgw->WorkBuffer[0]) break;
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
			    
			    msg.code = toupper(sgw->Code);
			    
			    switch(msg.code)
			    {
			        case CODE_COPY:
				    msg.text = sgw->WorkBuffer;
				    break;
				    
				case CODE_PASTE:
				    msg.text = NULL;
				    break;
			    }
			    
			    SetSignal(0, SIGF_SINGLE);
			    PutMsg(port, &msg.msg);
			    WaitPort(&replyport);
			    
			} /* if ((port = FindPort(CONCLIP_PORTNAME))) */
		
			break;
		    
		} /* switch(toupper(sgw->Code)) */
		
	    } /* if (sgw->IEvent->ie_Qualifier & IEQUALIFIER_RCOMMAND) */
    	    break;    
    }
    
    if (calloldhook) retcode = CallHookPkt(oldedithook, sgw, command);
    
    return retcode;
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

static void savetoclipboard(STRPTR text)
{
    struct IFFHandle *iff;
    
    if((iff = AllocIFF()))
    {
	if((iff->iff_Stream = (IPTR)OpenClipboard(clipunit)))
	{
	    InitIFFasClip(iff);
	    if(!OpenIFF(iff,IFFF_WRITE))
	    {
		if(!PushChunk(iff, MAKE_ID('F','T','X','T'), ID_FORM, IFFSIZE_UNKNOWN))
		{
		    if(!PushChunk(iff, 0, MAKE_ID('C','H','R','S'), IFFSIZE_UNKNOWN))
		    {
		        WriteChunkBytes(iff, text, strlen(text));
			
			PopChunk(iff);
			
		    } /* if(!PushChunk(iff, 0, MAKE_ID('C','H','R','S'), IFFSIZE_UNKNOWN)) */
		    PopChunk(iff);
		    
		} /* if(!PushChunk(iff, MAKE_ID('F','T','X','T'), ID_FORM, IFFSIZE_UNKNOWN)) */
		CloseIFF(iff);
		
	    } /* if(!OpenIFF(iff,IFFF_WRITE)) */
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
			savetoclipboard(msg->text);
			break;
			
		    case CODE_PASTE:	
		    	D(bug("ConClip: Received CODE_PASTE message\n"));
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
    openlibs();
    getarguments();
    checkport();
    installedithook();
    handleall();
    cleanup(NULL);
    return 0;
}

/*****************************************************************************************/
