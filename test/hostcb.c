/*
    Copyright © 1995-2004, The AROS Development Team. All rights reserved.
    $Id$
*/


#include <dos/dos.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <aros/debug.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

/****************************************************************************************/

#define ARG_TEMPLATE 	    "TOSTDOUT/S,TOAROSCLIP/S,TOFILE/K,FROMAROSCLIP/S,FROMSTRING/K,FROMFILE/K"
#define ARG_TOSTDOUT	    0
#define ARG_TOAROSCLIP      1
#define ARG_TOFILE  	    2
#define ARG_FROMAROSCLIP    3
#define ARG_FROMSTRING	    4
#define ARG_FROMFILE	    5
#define NUM_ARGS    	    6

#define CMD(msg)    	((msg)->mn_Node.ln_Pri)
#define PARAM(msg)	((msg)->mn_Node.ln_Name)
#define RETVAL(msg) 	((msg)->mn_Node.ln_Name)
#define SUCCESS(msg) 	((msg)->mn_Node.ln_Pri)

/****************************************************************************************/

struct MsgPort  *replyport;
struct Message   msg;
struct RDArgs 	*myargs;
BPTR	         fh;
STRPTR	    	 filebuffer, hostbuffer;
IPTR	         args[NUM_ARGS];
UBYTE	         s[256];

/****************************************************************************************/

static void cleanup(char *msg, ULONG retcode)
{
    if (msg) fprintf(stderr, "hostcb: %s\n", msg);
    
    if (fh) Close(fh);
    if (hostbuffer) FreeVec(hostbuffer);
    if (filebuffer) FreeVec(filebuffer);
    if (myargs) FreeArgs(myargs);
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

static void initport(void)
{
    replyport = CreateMsgPort();
    if (!replyport) cleanup("Out of memory", RETURN_ERROR);

    msg.mn_ReplyPort = replyport;
}

/****************************************************************************************/

static void getarguments(void)
{
    if (!(myargs = ReadArgs(ARG_TEMPLATE, args, 0)))
    {
    	Fault(IoErr(), 0, s, 255);
	cleanup(s, RETURN_FAIL);
    }
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
    initport();
    getarguments();
    if (args[ARG_TOSTDOUT]) tostdout();
    if (args[ARG_TOFILE]) tofile((char *)args[ARG_TOFILE]);
    if (args[ARG_FROMSTRING]) fromstring((char *)args[ARG_FROMSTRING]);
    if (args[ARG_FROMFILE]) fromfile((char *)args[ARG_FROMFILE]);
    
    cleanup(0, RETURN_OK);
    
    return 0;
}

/****************************************************************************************/
