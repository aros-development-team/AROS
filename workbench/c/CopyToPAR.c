/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Copy file to parallel.device
    Lang: English
*/

#include <exec/io.h>
#include <dos/dos.h>
#include <devices/parallel.h>
#include <proto/exec.h>
#include <proto/dos.h>

#include <stdlib.h>
#include <stdio.h>

/****************************************************************************************/

#define ARG_TEMPLATE 	    "FILE/A,QUIET/S"
#define ARG_FILE    	    0
#define ARG_QUIET   	    1
#define NUM_ARGS    	    2

#define BUFSIZE     	    4096

/****************************************************************************************/

struct MsgPort	*ParMP;
struct IOStdReq *ParIO;
BOOL	    	 ParOpen;
BPTR	    	 fh;
struct RDArgs 	*myargs;
IPTR	         args[NUM_ARGS];
UBYTE	         s[256];
UBYTE	    	 buf[BUFSIZE];

/****************************************************************************************/

static void cleanup(char *msg, ULONG retcode)
{
    if (msg && !args[ARG_QUIET]) 
    {
    	fprintf(stderr, "CopyToPAR: %s\n", msg);
    }
    
    if (fh) Close(fh);
    if (myargs) FreeArgs(myargs);
    
    if (ParOpen) CloseDevice((struct IORequest *)ParIO);
    if (ParIO) DeleteIORequest((struct IORequest *)ParIO);
    if (ParMP) DeleteMsgPort(ParMP);
    
    exit(retcode);
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

static void openpar(void)
{
    ParMP = CreateMsgPort();
    if (!ParMP) cleanup("Failed to create msgport", RETURN_ERROR);
    
    ParIO = (struct IOStdReq *)CreateIORequest(ParMP, sizeof(struct IOExtPar));
    if (!ParIO) cleanup("Failed to create IO request", RETURN_ERROR);
    
    if (OpenDevice("parallel.device", 0, (struct IORequest *)ParIO, 0))
    {
    	cleanup("Failed to open parallel.device", RETURN_ERROR);
    }
    
    ParOpen = TRUE;        
}

/****************************************************************************************/

static void openfile(void)
{
    fh = Open((STRPTR)args[ARG_FILE], MODE_OLDFILE);
    if (!fh)
    {
    	Fault(IoErr(), 0, s, 255);
	cleanup(s, RETURN_FAIL);
    }
}

/****************************************************************************************/

static BOOL WritePAR(APTR buf, ULONG size)
{
    ParIO->io_Command = CMD_WRITE;
    ParIO->io_Data    = buf;
    ParIO->io_Length  = size;
    
    return (DoIO((struct IORequest *)ParIO) == 0) ? TRUE : FALSE;
}

/****************************************************************************************/

static void docopy(void)
{
    LONG size;
    
    do
    {
    	size = Read(fh, buf, BUFSIZE);
	if (size == -1)
	{
	    Fault(IoErr(), 0, s, 255);
	    cleanup(s, RETURN_FAIL);
	}
	
	if (!WritePAR(buf, size))
	{
	    cleanup("Error writing to parallel.device", RETURN_FAIL);
    	}
		
    } while (size == BUFSIZE);
        
}

/****************************************************************************************/

int main(void)
{
    getarguments();
    openpar();
    openfile();
    docopy();
    cleanup(NULL, 0);
    
    return 0;
}
