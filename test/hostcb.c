/*
    Copyright © 1995-2004, The AROS Development Team. All rights reserved.
    $Id$

    Tool to access host clipboard, when using a hosted version of AROS.
*/


#include <dos/dos.h>
#include <libraries/iffparse.h>
#include <datatypes/textclass.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/iffparse.h>
#include <aros/debug.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

/****************************************************************************************/

#define ARG_TEMPLATE 	    "TOSTDOUT=OUT/S,TOAROSCLIP=TOAROS/S,TOFILE=TO/K," 	\
    	    	    	    "FROMAROSCLIP=FROMAROS/S,FROMSTRING=STRING/K,"  	\
			    "FROMFILE=FROM/K,AROSCLIPUNIT=AROSUNIT/N/K,QUIET/S"
			    
#define ARG_TOSTDOUT	    0
#define ARG_TOAROSCLIP      1
#define ARG_TOFILE  	    2
#define ARG_FROMAROSCLIP    3
#define ARG_FROMSTRING	    4
#define ARG_FROMFILE	    5
#define ARG_AROSCLIPUNIT    6
#define ARG_QUIET   	    7
#define NUM_ARGS    	    8

#define CMD(msg)    	((msg)->mn_Node.ln_Pri)
#define PARAM(msg)	((msg)->mn_Node.ln_Name)
#define RETVAL(msg) 	((msg)->mn_Node.ln_Name)
#define SUCCESS(msg) 	((msg)->mn_Node.ln_Pri)

/****************************************************************************************/

struct Library  *IFFParseBase;
struct MsgPort  *replyport;
struct Message   msg;
struct RDArgs 	*myargs;
BPTR	         fh;
ULONG	    	 arosclipunit;
STRPTR	    	 filebuffer, hostbuffer;
IPTR	         args[NUM_ARGS];
UBYTE	         s[256];

/****************************************************************************************/

static void cleanup(char *msg, ULONG retcode)
{
    if (msg && !args[ARG_QUIET]) 
    {
    	fprintf(stderr, "hostcb: %s\n", msg);
    }
    
    if (fh) Close(fh);
    if (hostbuffer) FreeVec(hostbuffer);
    if (filebuffer) FreeVec(filebuffer);
    if (myargs) FreeArgs(myargs);
    
    if (IFFParseBase) CloseLibrary(IFFParseBase);
    if (replyport) DeleteMsgPort(replyport);
    
    exit(retcode);
}

/****************************************************************************************/

static void noport(void)
{
    cleanup("HOST_CLIPBOARD port not found!", RETURN_ERROR);
}

/****************************************************************************************/

static void cmdfailed(void)
{
    cleanup("HOST_CLIPBOARD cmd failure!", RETURN_ERROR);
}

/****************************************************************************************/

static void init(void)
{
    replyport = CreateMsgPort();
    if (!replyport) cleanup("Out of memory", RETURN_ERROR);

    msg.mn_ReplyPort = replyport;
    
    IFFParseBase = OpenLibrary("iffparse.library", 36);
}

/****************************************************************************************/

static void getarguments(void)
{
    if (!(myargs = ReadArgs(ARG_TEMPLATE, args, 0)))
    {
    	Fault(IoErr(), 0, s, 255);
	cleanup(s, RETURN_FAIL);
    }
    
    if (args[ARG_AROSCLIPUNIT]) arosclipunit = *(IPTR *)args[ARG_AROSCLIPUNIT];    
}

/****************************************************************************************/

static BOOL sendclipboardmsg(struct Message *msg, char command, void *param)
{
    struct MsgPort *port;

    Forbid();
    port = FindPort("HOST_CLIPBOARD");
    if (port)
    {
    	CMD(msg) = command;
	PARAM(msg) = param;
	
	PutMsg(port, msg);
    }
    Permit();
    
    return port ? TRUE : FALSE;    
}

/****************************************************************************************/

static void toarosclip(void)
{
    struct IFFHandle *iff;
    BOOL    	      sent;

    if (!IFFParseBase) cleanup("Failed to open iffparse.library!", RETURN_FAIL);
    
    sent = sendclipboardmsg(&msg, 'R', NULL);    
    if (sent)
    {
    	WaitPort(replyport);
    	GetMsg(replyport);

	if (SUCCESS(&msg) && RETVAL(&msg))
	{
    	    BOOL ok = FALSE;

	    iff = AllocIFF();
	    if (iff)
	    {
    		if ((iff->iff_Stream = (IPTR)OpenClipboard(arosclipunit)))
		{
		    InitIFFasClip(iff);
		    
		    if (!OpenIFF(iff, IFFF_WRITE))
		    {
	    		if (!PushChunk(iff, ID_FTXT, ID_FORM, IFFSIZE_UNKNOWN))
			{
			    if (!PushChunk(iff, 0, ID_CHRS, IFFSIZE_UNKNOWN))
			    {
			    	ULONG len = strlen(RETVAL(&msg));
				
				if (WriteChunkBytes(iff, RETVAL(&msg), len) == len)
				{
				    ok = TRUE;
				}
				PopChunk(iff);
				
			    } /* if (!PushChunk(iff, 0, ID_CHRS, IFFSIZE_UNKNOWN)) */
			    
			    PopChunk(iff);
			    
			} /* if (!PushChunk(iff, ID_FTXT, ID_FORM, IFFSIZE_UNKNOWN)) */
			
			CloseIFF(iff);
			
		    } /* if (!OpenIFF(iff, IFFF_WRITE)) */
		    
		    CloseClipboard((struct ClipboardHandle*)iff->iff_Stream);
		    
		} /* if ((iff->iff_Stream = (IPTR)OpenClipboard(arosclipunit))) */
		
		FreeIFF(iff);
		
	    } /* if (iff) */

	    FreeVec(RETVAL(&msg));

	    if (!ok) cleanup("Error writing to AROS clipboard!", RETURN_ERROR);
	    
	} /* if (SUCCESS(&msg) && RETVAL(&msg)) */
	else
	{
	    cmdfailed();
	}	    
	
    } /* if (sent) */
    else
    {
    	noport(); 
    }
    
}

/****************************************************************************************/

static void fromarosclip(void)
{
    struct IFFHandle *iff;
    BOOL    	      sent, ok = FALSE;

    if (!IFFParseBase) cleanup("Failed to open iffparse.library!", RETURN_FAIL);
    
    iff = AllocIFF();
    if (iff)
    {
    	if ((iff->iff_Stream = (IPTR)OpenClipboard(arosclipunit)))
	{
	    InitIFFasClip(iff);

	    if (!OpenIFF(iff, IFFF_READ))
	    {
	    	if (!StopChunk(iff, ID_FTXT, ID_CHRS))
		{
		    ULONG filebuffer_size = 0;
		    
		    for(;;)
		    {
		    	struct ContextNode *cn;
		    	LONG 	    	    error;
			
		    	error = ParseIFF(iff, IFFPARSE_SCAN);
		    	if ((error != 0) && (error != IFFERR_EOC)) break;
			
			cn = CurrentChunk(iff);
			if (!cn)
			{
			    kprintf(" [ZERO CONTEXTNODE]\n\n");
			    continue;
			}
			
			if ((cn->cn_Type == ID_FTXT) && (cn->cn_ID == ID_CHRS))
			{
			    if (!filebuffer)
			    {
			    	filebuffer = AllocVec(cn->cn_Size + 1, MEMF_ANY);
				if (!filebuffer) break;
				
				ok = TRUE;
			    }
			    else
			    {
			    	STRPTR new_filebuffer;
				
				new_filebuffer = AllocVec(filebuffer_size + cn->cn_Size + 1, MEMF_ANY);
				if (!new_filebuffer)
				{
				    ok = FALSE;
				    break;
				}
				
				CopyMem(filebuffer, new_filebuffer, filebuffer_size);
				FreeVec(filebuffer);
				filebuffer = new_filebuffer;
			    }
			    
			    if (ReadChunkBytes(iff, filebuffer + filebuffer_size, cn->cn_Size) != cn->cn_Size)
			    {
			    	ok = FALSE;
				break;
			    }
			    
			    filebuffer_size += cn->cn_Size;
			    filebuffer[filebuffer_size] = '\0';
			    
			} /* if ((cn->cn_Type == ID_FTXT) && (cn->cn_ID == ID_CHRS)) */

		    } /* for(;;) */
		    
		} /* if (!StopChunk(iff, ID_FTXT, ID_CHRS)) */
					  
		CloseIFF(iff);
		
	    } /* if (!OpenIFF(iff, IFFF_READ)) */
	    
	    CloseClipboard((struct ClipboardHandle*)iff->iff_Stream);
	    
	} /* if ((iff->iff_Stream = (IPTR)OpenClipboard(arosclipunit))) */
	FreeIFF(iff);
	
    } /* if (iff) */
    
    if (!ok)
    {
	cleanup("Error reading from AROS clipboard!", RETURN_ERROR);
    }
    
    sent = sendclipboardmsg(&msg, 'W', filebuffer);    
    if (sent)
    {
    	WaitPort(replyport);
    	GetMsg(replyport);

	if (SUCCESS(&msg))
	{
    	    FreeVec(filebuffer),
    	    filebuffer = 0;
	}
	else
	{
	    cmdfailed();
	}	    
    }
    else
    {
    	noport(); 
    }
        
}

/****************************************************************************************/

static void tostdout(void)
{
    BOOL sent;
    
    sent = sendclipboardmsg(&msg, 'R', NULL);    
    if (sent)
    {
    	WaitPort(replyport);
    	GetMsg(replyport);

	if (SUCCESS(&msg) && RETVAL(&msg))
	{
	    printf("%s", RETVAL(&msg));
	    FreeVec(RETVAL(&msg));
	}
	else
	{
	    cmdfailed();
	}	    
    }
    else
    {
    	noport(); 
    }

}

/****************************************************************************************/

static void tofile(char *filename)
{
    BOOL sent;
    
    sent = sendclipboardmsg(&msg, 'R', NULL);    
    if (sent)
    {
    	WaitPort(replyport);
    	GetMsg(replyport);

	if (SUCCESS(&msg) && RETVAL(&msg))
	{
	    ULONG len;
	    
	    hostbuffer = RETVAL(&msg);
	    len = strlen(hostbuffer);
	    
	    fh = Open(filename, MODE_NEWFILE);
	    if (!fh)
	    {
	    	Fault(IoErr(), 0, s, 255);
		cleanup(s, RETURN_ERROR);
	    }
	    
	    if (Write(fh, hostbuffer, len) != len)
	    {
	    	Fault(IoErr(), 0, s, 255);
		cleanup(s, RETURN_ERROR);	    	
	    }
	    
	    Close(fh);
	    fh = 0;
	    
	    FreeVec(hostbuffer);
	    hostbuffer = 0;
	}
	else
	{
	    cmdfailed();
	}	    
    }
    else
    {
    	noport(); 
    }

}

/****************************************************************************************/

static void fromstring(char *s)
{
    BOOL sent;
    
    sent = sendclipboardmsg(&msg, 'W', s);    
    if (sent)
    {
    	WaitPort(replyport);
    	GetMsg(replyport);

	if (!SUCCESS(&msg))
	{	 
	    cmdfailed();
	}	    
    }
    else
    {
    	noport(); 
    }

}

/****************************************************************************************/

static void fromfile(char *filename)
{
    BOOL sent;
    LONG len;
    
    fh = Open(filename, MODE_OLDFILE);
    if (!fh)
    {
	Fault(IoErr(), 0, s, 255);
	cleanup(s, RETURN_ERROR);
    }

    len = Seek(fh, 0, OFFSET_END);
    if (len != -1) len = Seek(fh, 0, OFFSET_BEGINNING);
    
    if (len == -1)
    {
	Fault(IoErr(), 0, s, 255);
	cleanup(s, RETURN_ERROR);
    }
    
    filebuffer = AllocVec(len + 1, MEMF_ANY);
    if (!filebuffer)
    {
    	cleanup("Out of memory!", RETURN_ERROR);
    }
        
    if (Read(fh, filebuffer, len) != len)
    {
	Fault(IoErr(), 0, s, 255);
	cleanup(s, RETURN_ERROR);	    	
    }
    
    Close(fh);
    fh = 0;

    filebuffer[len] = '\0';
        
    sent = sendclipboardmsg(&msg, 'W', filebuffer);    
    if (sent)
    {
    	WaitPort(replyport);
    	GetMsg(replyport);

	if (!SUCCESS(&msg))
	{
	    cmdfailed();
	}	    
    }
    else
    {
    	noport(); 
    }
    
    FreeVec(filebuffer);
    filebuffer = 0;

}

/****************************************************************************************/

int main(void)
{
    init();
    getarguments();
    if (args[ARG_TOAROSCLIP]) toarosclip();
    if (args[ARG_FROMAROSCLIP]) fromarosclip();
    if (args[ARG_TOSTDOUT]) tostdout();
    if (args[ARG_TOFILE]) tofile((char *)args[ARG_TOFILE]);
    if (args[ARG_FROMSTRING]) fromstring((char *)args[ARG_FROMSTRING]);
    if (args[ARG_FROMFILE]) fromfile((char *)args[ARG_FROMFILE]);
    
    cleanup(0, RETURN_OK);
    
    return 0;
}

/****************************************************************************************/
