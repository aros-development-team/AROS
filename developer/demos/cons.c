/*
 *  $Id$
 *
 *	DISCLAIMER:
 *
 *	This program is provided as a service to the programmer
 *	community to demonstrate one or more features of the Amiga
 *	personal computer.  These code samples may be freely used
 *	for commercial or noncommercial purposes.
 * 
 * 	Commodore Electronics, Ltd ("Commodore") makes no
 *	warranties, either expressed or implied, with respect
 *	to the program described herein, its quality, performance,
 *	merchantability, or fitness for any particular purpose.
 *	This program is provided "as is" and the entire risk
 *	as to its quality and performance is with the user.
 *	Should the program prove defective following its
 *	purchase, the user (and not the creator of the program,
 *	Commodore, their distributors or their retailers)
 *	assumes the entire cost of all necessary damages.  In 
 *	no event will Commodore be liable for direct, indirect,
 *	incidental or consequential damages resulting from any
 *	defect in the program even if it has been advised of the 
 *	possibility of such damages.  Some laws do not allow
 *	the exclusion or limitation of implied warranties or
 *	liabilities for incidental or consequential damages,
 *	so the above limitation or exclusion may not apply.
 *
 */

/* cons.c - a console device demo program with supporting macro routines. */

/* This program is supported by the Amiga C compiler, version 1.1 and beyond.
 * (v1.0 compiler has difficulties if string variables do not have their
 * initial character aligned on a longword boundary.  Compiles acceptably
 * but won't run correctly.)
 *
 * Author:  Rob Peck, 12/1/85
 * 
 * This code may be freely utilized to develop programs for the Amiga.
 *
 * Modifications for AROS, July 2010:
 *  - Window size
 *  - Header required for AROS
 *  - Fixed prototypes
 *  - Added some casts needed by newer compilers
 *
 */

#include <exec/types.h>
#include <exec/io.h>
#include "exec/memory.h"

#define SHORT signed char

#include "graphics/gfx.h"
#include "graphics/gfxmacros.h"
#include "graphics/copper.h"
#include "graphics/view.h"
#include "graphics/gels.h"
#include "graphics/regions.h"
#include "graphics/clip.h"

#include "proto/exec.h"
#include "proto/intuition.h"
#include "proto/dos.h"
#include "proto/alib.h"

#include "exec/exec.h"
#include "graphics/text.h"
#include "graphics/gfxbase.h"

#include "devices/console.h"
#include "devices/keymap.h"

#include "libraries/dos.h"
#include "graphics/text.h"
#include "libraries/diskfont.h"
#include "intuition/intuition.h"

#include <stdlib.h>

UBYTE escdata[] = { 0x9b, '@',  /* insert character */
		    0x9b, 'A',	/* cursor up */
		    0x9b, 'B',  /* cursor down */
		    0x9b, 'C',  /* cursor left */
		    0x9b, 'D',  /* cursor right */
		    0x9b, 'E',  /* cursor next line */
		    0x9b, 'F',  /* cursor prev line */
		    0x9b, 'J',	/* erase to end of display */
		    0x9b, 'K',  /* erase to end of line */
		    0x9b, 'L',  /* insert line */
		    0x9b, 'M',  /* delete line */
		    0x9b, 'P',  /* delete character */
		    0x9b, 'S',  /* scroll up */
		    0x9b, 'T',  /* scroll down */
		    0x1b, 'c',  /* reset to initial state */
		    0x9b, 'q',  /* window status request */
		    0x9b, 'n',  /* device status report */
		    0x9b, ' ', 'p',/* cursor on */
		    0x9b, '0', ' ', 'p', /* cursor off */
		    0x9b, '2', '0', 'h', /* set mode */
		    0x9b, '2', '0', 'l', /* reset mode */
		   };

	/* COVER A SELECTED SUBSET OF THE CONSOLE AVAILABLE FUNCTIONS */

#define INSERTCHARSTRING 	&escdata[0] 
#define CURSUPSTRING    	&escdata[0+2] 
#define CURSDOWNSTRING 		&escdata[0+4] 
#define CURSFWDSTRING 		&escdata[0+6] 
#define CURSBAKSTRING 		&escdata[0+8] 
#define CURSNEXTLINE 		&escdata[0+10] 
#define CURSPREVLINE 		&escdata[0+12] 
#define ERASEEODSTRING 		&escdata[0+14] 
#define ERASEEOLSTRING 		&escdata[0+16] 
#define INSERTLINESTRING 	&escdata[0+18] 
#define DELETELINESTRING 	&escdata[0+20] 
#define DELCHARSTRING 		&escdata[0+22] 
#define SCROLLUPSTRING 		&escdata[0+24] 
#define SCROLLDOWNSTRING 	&escdata[0+26] 
#define RESETINITSTRING 	&escdata[0+28]
#define WINDOWSTATSTRING 	&escdata[0+30]
#define DEVSTATSTRING 		&escdata[0+32]
#define CURSONSTRING 		&escdata[0+34] 
#define CURSOFFSTRING 		&escdata[0+37] 
#define SETMODESTRING 		&escdata[0+41]
#define RESETMODESTRING 	&escdata[0+45]

#define BACKSPACE(r) 		ConPutChar(r,0x08)
#define TAB(r) 			ConPutChar(r,0x09)
#define LINEFEED(r) 		ConPutChar(r,0x0a)
#define VERTICALTAB(r) 		ConPutChar(r,0x0b)
#define FORMFEED(r) 		ConPutChar(r,0x0c)
#define CR(r) 			ConPutChar(r,0x0d)
#define SHIFTOUT(r) 		ConPutChar(r,0x0e)
#define SHIFTIN(r) 		ConPutChar(r,0x0f)
#define CLEARSCREEN(r)		ConPutChar(r,0x0c)

#define RESET(r) 		ConWrite(r,RESETINITSTRING,2)
#define INSERT(r) 		ConWrite(r,INSERTCHARSTRING,2)
#define CURSUP(r) 		ConWrite(r,CURSUPSTRING,2)
#define CURSDOWN(r) 		ConWrite(r,CURSDOWNSTRING,2)
#define CURSFWD(r) 		ConWrite(r,CURSFWDSTRING,2)
#define CURSBAK(r) 		ConWrite(r,CURSBAKSTRING,2)
#define CURSNEXTLN(r) 		ConWrite(r,CURSNEXTLINE,2)
#define CURSPREVLN(r) 		ConWrite(r,CURSPREVLINE,2)
#define ERASEEOD(r) 		ConWrite(r,ERASEEODSTRING,2)
#define ERASEEOL(r) 		ConWrite(r,ERASEEOLSTRING,2)
#define INSERTLINE(r) 		ConWrite(r,INSERTLINESTRING,2)
#define DELETELINE(r) 		ConWrite(r,DELETELINESTRING,2)
#define SCROLLUP(r) 		ConWrite(r,SCROLLUPSTRING,2)
#define SCROLLDOWN(r) 		ConWrite(r,SCROLLDOWNSTRING,2)
#define DEVICESTATUS(r) 	ConWrite(r,DEVSTATSTRING,2)
#define WINDOWSTATUS(r) 	ConWrite(r,WINDOWSTATSTRING,2)
#define DELCHAR(r) 		ConWrite(r,DELCHARSTRING,2)
#define CURSORON(r) 		ConWrite(r,CURSONSTRING,3)
#define CURSOROFF(r) 		ConWrite(r,CURSOFFSTRING,4)
#define SETMODE(r) 		ConWrite(r,SETMODESTRING,4)
#define RESETMODE(r) 		ConWrite(r,RESETMODESTRING,4)

#define CloseConsole(r) CloseDevice(r)

 

struct NewWindow nw = {
        10, 10,        /* starting position (left,top) */
        620,400,        /* width, height */
        -1,-1,          /* detailpen, blockpen */
        0,              /* flags for idcmp */
		WFLG_DEPTHGADGET|
		WFLG_SIZEGADGET|
		WFLG_DRAGBAR|
		WFLG_SIMPLE_REFRESH|
		WFLG_ACTIVATE|
		WFLG_GIMMEZEROZERO,
                        /* window gadget flags */
        0,              /* pointer to 1st user gadget */
        NULL,           /* pointer to user check */
        "Console Test", /* title */
        NULL,           /* pointer to window screen */
        NULL,           /* pointer to super bitmap */
        100,45,         /* min width, height */
        640,200,        /* max width, height */
        WBENCHSCREEN};
 
struct Window *w;
struct RastPort *rp;
	
struct IOStdReq *consoleWriteMsg;   /* I/O request block pointer */
struct MsgPort *consoleWritePort;      /* a port at which to receive */	
struct IOStdReq *consoleReadMsg;   /* I/O request block pointer */
struct MsgPort *consoleReadPort;      /* a port at which to receive */	
	
extern struct MsgPort *CreatePort();
extern struct IOStdReq *CreateStdIO();

char readstring[200];	/* provides a buffer even though using only one char */

int OpenConsole(struct IOStdReq * writerequest, struct IOStdReq * readrequest, struct Window * window);
int QueueRead(struct IOStdReq * request, char * whereto);
int ConWrite(struct IOStdReq * request, char * string, int length);
int ConPutStr(struct IOStdReq * request, char * string);
int ConPutChar(struct IOStdReq * request,char character);

struct Library * DosBase;
struct Library * DiskfontBase;
struct Library * GfxBase;

int main()
{

	SHORT i;
	APTR  status;
	SHORT problem;
	SHORT error;
	problem = 0;

        if((DosBase = OpenLibrary("dos.library", 0)) == NULL) 
		{ problem = 1; goto cleanup1; }
        if((DiskfontBase=OpenLibrary("diskfont.library",0))==NULL) 
		{ problem = 2; goto cleanup2; }
        if((IntuitionBase=(APTR)OpenLibrary("intuition.library",0))==NULL) 
		{ problem = 3; goto cleanup3; }
        if((GfxBase=OpenLibrary("graphics.library",0))==NULL) 
		{ problem = 4; goto cleanup4; }
 
	consoleWritePort = CreatePort("my.con.write",0);
	if(consoleWritePort == 0) 
		{ problem = 5; goto cleanup5; }
	consoleWriteMsg =  CreateStdIO(consoleWritePort);
	if(consoleWritePort == 0)
		{ problem = 6; goto cleanup6; }

	consoleReadPort = CreatePort("my.con.read",0);
	if(consoleReadPort == 0) 
		{ problem = 7; goto cleanup7; }
	consoleReadMsg =  CreateStdIO(consoleReadPort);
	if(consoleReadPort == 0) 
		{ problem = 8; goto cleanup8; }

        w = (struct Window *)OpenWindow(&nw);	/* create a window */
	if(w == NULL)
		{ problem = 9; goto cleanup9; }

        rp = w->RPort;		/* establish its rastport for later */

/* ************************************************************************ */
/* NOW, Begin using the actual console macros defined above.                */	
/* ************************************************************************ */

	error = OpenConsole(consoleWriteMsg,consoleReadMsg,w);	
	if(error != 0)  
		{ problem = 10; goto cleanup10; }
		/* attach a console to this window, initialize
		 * for both write and read */

	QueueRead(consoleReadMsg,&readstring[0]); /* tell console where to 
						   * put a character that
						   * it wants to give me
						   * queue up first read */
	ConWrite(consoleWriteMsg,"Hello, World\r\n",14);

	ConPutStr(consoleWriteMsg,"testing BACKSPACE");
	for(i=0; i<10; i++)
		{ BACKSPACE(consoleWriteMsg); Delay(30); }

	ConPutStr(consoleWriteMsg,"\r\n");

	ConPutStr(consoleWriteMsg,"testing TAB\r");
	for(i=0; i<6; i++)
		{ TAB(consoleWriteMsg); Delay(30); }

	ConPutStr(consoleWriteMsg,"\r\n");

	ConPutStr(consoleWriteMsg,"testing LINEFEED\r");
	for(i=0; i<4; i++)
		{ LINEFEED(consoleWriteMsg); Delay(30); }

	ConPutStr(consoleWriteMsg,"\r\n");

	ConPutStr(consoleWriteMsg,"testing VERTICALTAB\r");
	for(i=0; i<4; i++)
		{ VERTICALTAB(consoleWriteMsg); Delay(30); }

	ConPutStr(consoleWriteMsg,"\r\n");

	ConPutStr(consoleWriteMsg,"testing FORMFEED\r");
	Delay(30);
	for(i=0; i<2; i++)
		{ FORMFEED(consoleWriteMsg); Delay(30); }

	ConPutStr(consoleWriteMsg,"\r\n");

	ConPutStr(consoleWriteMsg,"testing CR");
	Delay(30);
	CR(consoleWriteMsg);
	Delay(60);
	ConPutStr(consoleWriteMsg,"\r\n");

	ConPutStr(consoleWriteMsg,"testing INSERT\r");
	for(i=0; i<4; i++)
		{ INSERT(consoleWriteMsg); Delay(30); }

	ConPutStr(consoleWriteMsg,"\r\n");

	ConPutStr(consoleWriteMsg,"    testing DELCHAR\r");
	CR(consoleWriteMsg);
	for(i=0; i<4; i++)
		{ DELCHAR(consoleWriteMsg); Delay(30); }

	ConPutStr(consoleWriteMsg,"\r\n");

	ConPutStr(consoleWriteMsg,"testing INSERTLINE\r");
	CR(consoleWriteMsg);
	for(i=0; i<3; i++)
		{ INSERTLINE(consoleWriteMsg); Delay(30); }
	ConPutStr(consoleWriteMsg,"\r\n");

	ConPutStr(consoleWriteMsg,"testing DELETELINE\r");
	CR(consoleWriteMsg);
	LINEFEED(consoleWriteMsg);
	Delay(60);
	for(i=0; i<4; i++)
		{ DELETELINE(consoleWriteMsg); Delay(30); }
	ConPutStr(consoleWriteMsg,"\r\n");

	ConPutStr(consoleWriteMsg,"testing CURSUP\r");
	for(i=0; i<4; i++)
		{ CURSUP(consoleWriteMsg); Delay(30); }

	ConPutStr(consoleWriteMsg,"\r\n");

	ConPutStr(consoleWriteMsg,"testing CURSDOWN\r");
	for(i=0; i<4; i++)
		{ CURSDOWN(consoleWriteMsg); Delay(30); }

	ConPutStr(consoleWriteMsg,"\r\n");

	ConPutStr(consoleWriteMsg,"testing CURSFWD\r");
	for(i=0; i<4; i++)
		{ CURSFWD(consoleWriteMsg); Delay(30); }

	ConPutStr(consoleWriteMsg,"\r\n");

	ConPutStr(consoleWriteMsg,"testing CURSBAK");
	for(i=0; i<4; i++)
		{ CURSBAK(consoleWriteMsg); Delay(30); }

	ConPutStr(consoleWriteMsg,"\r\n");

	ConPutStr(consoleWriteMsg,"testing CURSPREVLN");
	for(i=0; i<4; i++)
		{ CURSPREVLN(consoleWriteMsg); Delay(30); }

	ConPutStr(consoleWriteMsg,"\r\n");

	ConPutStr(consoleWriteMsg,"testing CURSNEXTLN");
	for(i=0; i<4; i++)
		{ CURSNEXTLN(consoleWriteMsg); Delay(30); }

	ConPutStr(consoleWriteMsg,"\r\n");

	ConPutStr(consoleWriteMsg,"testing ERASEEOD");
	CURSPREVLN(consoleWriteMsg);
	CURSPREVLN(consoleWriteMsg);
	CURSPREVLN(consoleWriteMsg);
	Delay(60);
	for(i=0; i<4; i++)
		{ ERASEEOD(consoleWriteMsg); Delay(30); }

	ConPutStr(consoleWriteMsg,"\r\n");

	ConPutStr(consoleWriteMsg,"testing ERASEEOL.junk");
	CURSBAK(consoleWriteMsg);
	CURSBAK(consoleWriteMsg);
	CURSBAK(consoleWriteMsg);
	CURSBAK(consoleWriteMsg);
	CURSBAK(consoleWriteMsg);
	Delay(60);
	ERASEEOL(consoleWriteMsg); 
	Delay(30);
	ConPutStr(consoleWriteMsg,"\r\n");

	ConPutStr(consoleWriteMsg,"testing SCROLLUP");
	for(i=0; i<4; i++)
		{ SCROLLUP(consoleWriteMsg); Delay(30); }

	ConPutStr(consoleWriteMsg,"\r\n");
	ConPutStr(consoleWriteMsg,"testing SCROLLDOWN");
	ConPutStr(consoleWriteMsg,"\n\n\n");
	for(i=0; i<4; i++)
		{ SCROLLDOWN(consoleWriteMsg); Delay(30); }

	ConPutStr(consoleWriteMsg,"\r\n");

	ConPutStr(consoleWriteMsg,"testing CURSOROFF");
	CURSOROFF(consoleWriteMsg); 
	ConPutStr(consoleWriteMsg, "printed.with.cursor.off"); 
	Delay(60);
	ConPutStr(consoleWriteMsg,"\r\n");

	CURSORON(consoleWriteMsg); Delay(30);
	ConPutStr(consoleWriteMsg,"testing CURSORON");


/* ************************************************************************ */
	Delay(120);	/* wait 2 seconds (120/60 ticks) */

	status = CheckIO((struct IORequest *)consoleReadMsg);	/* see if console read
						 * anything, abort if not */
	if(status == NULL) AbortIO((struct IORequest *)consoleReadMsg);
	WaitPort(consoleReadPort);	/* wait for abort to complete */
	GetMsg(consoleReadPort);	/* and strip message from port */

	CloseConsole((struct IORequest *)consoleWriteMsg);
   cleanup10:
   cleanup9:
	CloseWindow(w);
   cleanup8:
	DeleteStdIO(consoleReadMsg);
   cleanup7:
	DeletePort(consoleReadPort);
   cleanup6:
	DeleteStdIO(consoleWriteMsg);
   cleanup5:
	DeletePort(consoleWritePort);
   cleanup4:
        CloseLibrary(GfxBase);
   cleanup3:
		CloseLibrary((struct Library *)IntuitionBase);
   cleanup2:
        CloseLibrary(DiskfontBase);
   cleanup1:
        CloseLibrary(DosBase);
	if(problem > 0) exit(problem+1000);
	else
		return(0);

}	/* end of main() */


/* Open a console device */

	/* this function returns a value of 0 if the console 
	 * device opened correctly and a nonzero value (the error
	 * returned from OpenDevice) if there was an error.
	 */
	
int OpenConsole(struct IOStdReq * writerequest, struct IOStdReq * readrequest, struct Window * window)
	{	
		int error;	
        	writerequest->io_Data = (APTR) window;
        	writerequest->io_Length = sizeof(*window);
        	error = OpenDevice("console.device", 0, (struct IORequest *)writerequest, 0);
		readrequest->io_Device = writerequest->io_Device;
		readrequest->io_Unit   = writerequest->io_Unit;
			/* clone required parts of the request */
		return(error);	
	}

/* Output a single character to a specified console */ 

int ConPutChar(struct IOStdReq * request,char character)
	{
        	request->io_Command = CMD_WRITE;
        	request->io_Data = (APTR)&character;
        	request->io_Length = 1;
        	DoIO((struct IORequest *)request);
		/* command works because DoIO blocks until command is
		 * done (otherwise pointer to the character could become
		 * invalid in the meantime).
		 */
		return(0);
	}
 
/* Output a stream of known length to a console */ 

int ConWrite(struct IOStdReq * request, char * string, int length)
	{
        	request->io_Command = CMD_WRITE;
        	request->io_Data = (APTR)string;
        	request->io_Length = length; 
        	DoIO((struct IORequest *)request);
		/* command works because DoIO blocks until command is
		 * done (otherwise pointer to string could become
		 * invalid in the meantime).
		 */
		return(0);
	}

/* Output a NULL-terminated string of characters to a console */ 

int ConPutStr(struct IOStdReq * request, char * string)
	{
        	request->io_Command = CMD_WRITE;
        	request->io_Data = (APTR)string;
        	request->io_Length = -1;  /* tells console to end when it
					   * sees a terminating zero on
					   * the string. */
        	DoIO((struct IORequest *)request);
		return(0);
	}
	
	/* queue up a read request to a console, show where to
	 * put the character when ready to be returned.  Most
	 * efficient if this is called right after console is
	 * opened */

int QueueRead(struct IOStdReq * request, char * whereto)
	{
        	request->io_Command = CMD_READ;
        	request->io_Data = (APTR)whereto;
        	request->io_Length = 1;
        	SendIO((struct IORequest *)request);
		return(0);
	}

	/* see if there is a character to read.  If none, don't wait, 
	 * come back with a value of -1 */

	int 
ConMayGetChar(request,requestPort, whereto)
	struct IOStdReq *request;
	struct MsgPort * requestPort;
	char *whereto;
	{
        	int temp;
 
        	if ( GetMsg(requestPort) == NULL ) return(-1);
        	temp = *whereto;
        	QueueRead(request,whereto);
        	return(temp);
	}
 
	/* go and get a character; put the task to sleep if
	 * there isn't one present */

	UBYTE
ConGetChar(consolePort,request,whereto)
	struct IOStdReq *request; 
	struct MsgPort *consolePort;   
        char *whereto;
	{
		register UBYTE temp;
		while((GetMsg(consolePort) == NULL)) WaitPort(consolePort);
		temp = *whereto;	/* get the character */
		QueueRead(request,whereto);
		return(temp);
	}
			

