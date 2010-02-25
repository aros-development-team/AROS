/*
    Copyright © 1995-2010, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Change some internal options of cybergraphics.library.
    Lang: English
*/
/*****************************************************************************

    NAME

        GfxControl

    SYNOPSIS

        PREVENT_DIRECT_BITMAP_ACCESS=PDBA/S,
	ALLOW_DIRECT_BITMAP_ACCESS=ADBA/S,
	DUMP/S

    LOCATION

        C:

    FUNCTION

        Change some internal options of cybergraphics.library
        
    INPUTS

        PREVENT_DIRECT_BITMAP_ACCESS   --  Causes LockBitMapTagList() calls to
	                                   always fail

        ALLOW_DIRECT_BITMAP_ACCESS     --  Allow LocKBitMapTagList() to go to
	                                   gfx driver which may or may not
					   support it. (default)

    	DUMP	    	    	       --  Show current settings
	
    RESULT

        Standard DOS return codes.

    NOTES
    	By default 
    BUGS

    INTERNALS

******************************************************************************/

#include <dos/dosextens.h>
#include <proto/alib.h>
#include <proto/exec.h>
#include <proto/dos.h>

/****************************************************************************************/

#define PORT_NAME "GfxControl"

#define ARG_TEMPLATE 	    "PREVENT_DIRECT_BITMAP_ACCESS=PDBA/S,ALLOW_DIRECT_BITMAP_ACCESS=ADBA/S,DUMP/S"
#define ARG_PDBA   	    0
#define ARG_ADBA   	    1
#define ARG_DUMP            2
#define NUM_ARGS    	    3

/****************************************************************************************/

AROS_UFH3(APTR, MyLockBitMapTagList,
	  AROS_UFHA(struct BitMap *, bitmap, A0),
	  AROS_UFHA(struct TagItem *, tags, A1),
	  AROS_UFHA(struct Library *, CyberGfxBase, A6))
{
    AROS_USERFUNC_INIT

    return NULL;

    AROS_USERFUNC_EXIT
}

/****************************************************************************************/

AROS_UFH3(LONG, PatchTask,
	  AROS_UFHA(char *, argstr, A0),
	  AROS_UFHA(ULONG, argsize, D0),
	  AROS_UFHA(struct ExecBase *, SysBase, A6))
{
    AROS_USERFUNC_INIT

    struct Library *CyberGfxBase;
    struct MsgPort *port;
    APTR orig_func;

    /* Our process opens the library by itself in order to prevent it from being expunged */
    CyberGfxBase = OpenLibrary("cybergraphics.library", 0);
    if (!CyberGfxBase)
        return 0;

    port = CreateMsgPort();
    if (port) {
        port->mp_Node.ln_Name = PORT_NAME;
        AddPort(port);
        orig_func = SetFunction(CyberGfxBase, -28*LIB_VECTSIZE, MyLockBitMapTagList);
	
	/* Just wait for a signal from the port. There's no need to look at message contents */
	WaitPort(port);
	SetFunction(CyberGfxBase, -28*LIB_VECTSIZE, orig_func);
	RemPort(port);
	DeleteMsgPort(port);
    }

    CloseLibrary(CyberGfxBase);
    return 0;

    AROS_USERFUNC_EXIT
}

/****************************************************************************************/

AROS_UFH3(__startup static int, Start,
	  AROS_UFHA(char *, argstr, A0),
	  AROS_UFHA(ULONG, argsize, D0),
	  AROS_UFHA(struct ExecBase *, SysBase, A6))
{
    AROS_USERFUNC_INIT

    struct RDArgs *myargs;
    IPTR args[NUM_ARGS] = {0};
    int	rc = RETURN_OK;
    struct MsgPort *port;
    struct CommandLineInterface *cli;
    struct DosLibrary *DOSBase;
    
    DOSBase = (struct DosLibrary *)OpenLibrary("dos.library", 36);
    if (!DOSBase)
        return RETURN_FAIL;

    cli = Cli();

    /* TODO: Is it possible to run without CLI? For example
       if we were started via "Run command..."? */
    if (!cli) {
        PutStr("This program must be run from CLI\n");
	CloseLibrary((struct Library *)DOSBase);
	return RETURN_FAIL;
    }

    myargs = ReadArgs(ARG_TEMPLATE, args, 0);
    if (myargs) {
	port = FindPort(PORT_NAME);

	if (args[ARG_PDBA]) {
	    if (port)
	        PutStr("Direct bitmap access already disabled");
	    else {
		if (CreateNewProcTags(NP_Seglist, cli->cli_Module, NP_Entry, PatchTask, NP_Name, "GfxControl patch", TAG_DONE))
		    cli->cli_Module = NULL;
		else {
		    PrintFault(IoErr(), "GfxControl");
		    rc = RETURN_FAIL;
		}
	    }
	}

	if (args[ARG_ADBA]) {
	    if (port) {
		/* We do a very basic thing: just send a message. The message has no additional data
		   and therefore does not need to be freed. We even don't look at its contents in the
		   patch process */
	        struct Message msg;

		PutMsg(port, &msg);
	    } else
	        PutStr("Direct bitmap access already enabled");
	}

	if (args[ARG_DUMP])
	    Printf("Prevent Direct BitMap Access: %s\n", port ? "YES" : "NO");

        FreeArgs(myargs);
    } else {
    	PrintFault(IoErr(), "GfxControl");
	rc = RETURN_FAIL;
    }

    CloseLibrary((struct Library *)DOSBase);
    return rc;

    AROS_USERFUNC_EXIT
}
