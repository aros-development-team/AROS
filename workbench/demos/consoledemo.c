/*
    Copyright © 1995-97, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Demo for the console.device
    Lang: english
*/
#include <proto/exec.h>
#include <proto/intuition.h>
#include <proto/dos.h>
#include <dos/dosextens.h>
#include <exec/io.h>
#include <intuition/intuition.h>
#include <devices/conunit.h>


#define SDEBUG 1
#define DEBUG 1
#include <aros/debug.h>

struct IntuitionBase *IntuitionBase;

struct MsgPort   *con_mp;
struct IORequest *con_io;

struct Window 	 *window;

#define ioStd(x) ((struct IOStdReq *)x)

#define TESTSTR "Test"

BOOL Init();
VOID Cleanup();
VOID HandleEvents(struct Window *);

/*************
**  main()  **
*************/
int main(int argc, char **argv)
{
    BPTR fh;
    SDInit();
    
    D(bug("Opening CON:\n"));
    
    fh = Open("CON:5/5/200/200/Test Console/CLOSE/WAIT", MODE_NEWFILE);
    if (fh)
    {
    	LONG ret;
	ULONG i;
	
    	D(bug("Console file opened\n"));
	for (i = 0; i < 100; i ++)
	{
    	    ret = FPuts(fh, "Test\n");
	    D(bug("Got ret %ld\n", ret));
	    Flush(fh);
	}
	
	Close(fh);
	
    }
    return 0;
    
    
    if (Init())
    {
    	ioStd(con_io)->io_Command = CMD_WRITE;
    	ioStd(con_io)->io_Data    = TESTSTR;
    	ioStd(con_io)->io_Length  = -1L;
    	
	D(bug("Doing IO on console req %p\n", con_io));
    	DoIO(con_io);

	D(bug("IO on console req done\n"));
    	
    	HandleEvents(window);
    
    	Cleanup();
    }
    return (0);
}

/*************
**  Init()  **
*************/
BOOL Init()
{
    IntuitionBase = (struct IntuitionBase *)OpenLibrary("intuition.library", 37);
    if (IntuitionBase)
    {
    	con_mp = CreateMsgPort();
    	if (con_mp)
    	{
    	    con_io = CreateIORequest(con_mp, sizeof (struct IOStdReq));
    	    if (con_io)
    	    {
    	
    	    	window = OpenWindowTags(NULL,
    			WA_Width,		400,
    			WA_Height,		300,
    			WA_SmartRefresh,	TRUE,
    			WA_DragBar,		TRUE,
    			WA_Title,		(IPTR)"Console demo",
    			WA_IDCMP,		IDCMP_REFRESHWINDOW|IDCMP_RAWKEY,
    			TAG_DONE);
    			
    		if (window)
    		{
    	    	    ioStd(con_io)->io_Data   = (APTR)window;
    	    	    ioStd(con_io)->io_Length = sizeof (struct Window);
    	    	    
    	    	    if (0 == OpenDevice("console.device", CONU_STANDARD, con_io, 0))
		    {
		        D(bug("Console device successfully opened\n"));
    	    	    	return (TRUE);
		    }

		    CloseWindow(window);
    	    	}
    	    	DeleteIORequest(con_io);
    	    }
    	    DeleteMsgPort(con_mp);
    	}
    	CloseLibrary((struct Library *)IntuitionBase);
    }
    
    return (FALSE);
}

/****************
**  Cleanup()  **
****************/
VOID Cleanup()
{
    CloseDevice(con_io);
    CloseWindow(window);
    DeleteIORequest(con_io);
    DeleteMsgPort(con_mp);

    CloseLibrary((struct Library *)IntuitionBase);
    
}


/*******************
**  HandleEvents  **
*******************/
VOID HandleEvents(struct Window *win)
{
    struct IntuiMessage *imsg;
    struct MsgPort *port = win->UserPort;
    BOOL terminated = FALSE;
	
    while (!terminated)
    {
	if ((imsg = (struct IntuiMessage *)GetMsg(port)) != NULL)
	{
	    
	    switch (imsg->Class)
	    {
		
	    case IDCMP_REFRESHWINDOW:
	    	BeginRefresh(win);
	    	EndRefresh(win, TRUE);
	    	break;
	    	
	    case IDCMP_RAWKEY:
	    	terminated = TRUE;
	    	break;
		    					
	    } /* switch (imsg->Class) */
	    ReplyMsg((struct Message *)imsg);
	    
	    			
	} /* if ((imsg = GetMsg(port)) != NULL) */
	else
	{
	    Wait(1L << port->mp_SigBit);
	}
    } /* while (!terminated) */
	
    return;
} /* HandleEvents() */


