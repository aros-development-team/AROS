/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Code that loads and initializes necessary HIDDs.
    Lang: english
*/

#include <exec/memory.h>
#include <exec/resident.h>
#include <exec/alerts.h>
#include <exec/io.h>
#include <dos/filesystem.h>
#include <utility/tagitem.h>
#include <utility/hooks.h>
#include <hidd/hidd.h>

#include <proto/exec.h>
#include <proto/oop.h>
#include <proto/utility.h>
#include <proto/dos.h>
#include <proto/intuition.h>
#include <oop/oop.h>
#include <string.h>

#include "devs_private.h"

#ifdef __AROS__
#include <aros/asmcall.h>
#endif /* __AROS__ */

#define SDEBUG 0
#define DEBUG 0
#include <aros/debug.h>

#warning This is just a temporary and hackish way to get the HIDDs up and working

struct initbase
{
    struct ExecBase	*sysbase;
    struct DosLibrary	*dosbase;
    struct Library	*oopbase;
};

#define SysBase	(base->sysbase)
#define DOSBase (base->dosbase)
#define OOPBase (base->oopbase)


static BOOL init_gfx  ( STRPTR gfxclassname,   struct initbase *base);
static BOOL init_device( STRPTR hiddclassname, STRPTR devicename,  struct initbase *base);

/************************************************************************/


#define HIDDPATH "SYS:Hidds/"
#define BUFSIZE 100

#define HIDDPREFSFILE "SYS:S/hidd.prefs"

/* We don't link with c library so I must implement this separately */
#define isblank(c) \
	(c == '\t' || c == ' ')
#define isspace(c) \
	(c == '\t' || c == ' ' || c == '\f' || c == '\n' || c == '\r' || c == '\v')


#include <proto/graphics.h>

BOOL init_hidds(struct ExecBase *sysBase, struct DosLibrary *dosBase)
{
/* This is the initialisation code for InitHIDDs module */


    struct initbase stack_b, *base = &stack_b;
    BOOL success = TRUE;
    UBYTE buf[BUFSIZE];
    UBYTE gfxname[BUFSIZE], kbdname[BUFSIZE], mousename[BUFSIZE];
    BOOL got_gfx = FALSE, got_kbd = FALSE, got_mouse = FALSE, got_library = TRUE;


    base->sysbase = sysBase;
    base->dosbase = dosBase;

    EnterFunc(bug("init_hidds\n"));
    
    OOPBase = OpenLibrary(AROSOOP_NAME, 0);
    if (!OOPBase)
    {
    	success = FALSE;
    }
    else
    {
	BPTR fh;
    
	D(bug("OOP opened\n"));
	
	/* Open the hidd prefsfile */
	
	fh = Open(HIDDPREFSFILE, FMF_READ);
	if (!fh)
	{
	    success = FALSE;
	}
	else
	{
	    STRPTR libname = NULL;
	    
	    D(bug("hiddprefs file opened\n"));
	    while (FGets(fh, buf, BUFSIZE))
	    {
	        STRPTR keyword = buf, arg, end;
		STRPTR s;
		
		s = buf;
		if (*s) {
		    for (; *s; s ++)
		    	;
		    if (s[-1] == 10) {
		    	s[-1] = 0;
		    }
		}
		    
		

		D(bug("Got line\n"));
		D(bug("Line: %s\n", buf));

		  /* Get keyword */
		while ((*keyword != 0) && isspace(*keyword))
		    keyword ++;

		if (*keyword == 0)
		    continue;

		  /* terminate keyword */
		arg = keyword;
		while ((*arg != 0) && (!isblank(*arg)))
		{
		    arg ++;
		}
		if (*arg == 0)
		    continue;

		*arg = 0;

		arg ++;

		  /* Find start of argument */
		D(bug("Find argument at %s\n", arg));
		while ((*arg != 0) && isblank(*arg))
		    arg ++;

		if (*arg == 0)
		    continue;

		D(bug("Terminate argument at %s\n", arg));
		  /* terminate argument */
		end = arg;
		while ( (*end != 0) && (!isblank(*end)))
		    end ++;
		if (*end != 0)
		    *end = 0;

		D(bug("Got keyword \"%s\"\n", keyword));
		D(bug("Got arg \"%s\"\n", arg));

		if (0 == strcmp(keyword, "library"))
		{
		    D(bug("Opening library\n"));
		      /* Open a specified library */
		    libname = arg;
		    if (NULL == OpenLibrary(libname, 0))
		    {
		        success = FALSE;
			got_library = FALSE;
			break;
		    }
		}
		else if (0 == strcmp(keyword, "gfx"))
		{
		    strncpy(gfxname, arg, BUFSIZE - 1);
		    got_gfx = TRUE;
		}
		else if (0 == strcmp(keyword, "mouse"))
		{
		    strncpy(mousename, arg, BUFSIZE - 1);
		    got_mouse = TRUE;
		}
		else if (0 == strcmp(keyword, "kbd"))
		{
		    strncpy(kbdname, arg, BUFSIZE - 1);
		    got_kbd = TRUE;
		}
	    }
	    
	    Close(fh);
	    
	    if (!got_library)
	    {
	    	success = FALSE;
		kprintf("Could not open library %s\n", libname);
		goto end;
	    }
	    
	    if (!got_gfx)
	    {
	        success = FALSE;
	    	kprintf("No configuration for gfx hidd\n");
		goto end;
	    }
	    
	    if (!got_mouse)
	    {
	        success = FALSE;
	    	kprintf("No configuration for mouse hidd\n");
		goto end;
	    }
	
	    if (!got_kbd)
	    {
	        success = FALSE;
	    	kprintf("No configuration for keyboard hidd\n");
		goto end;
	    }
	    
	    if (!init_gfx(gfxname, base))
	    {
	        kprintf("Could not init gfx hidd %s\n", gfxname);
		success = FALSE;
		goto end;
	    }

	    if (!init_device(kbdname, "keyboard.device", base))
	    {
	        kprintf("Could not init keyboard hidd %s\n", kbdname);
		success = FALSE;
		goto end;
	    }

	    if (!init_device(mousename, "gameport.device", base))
	    {
	        kprintf("Could not init mouse hidd %s\n", mousename);
		success = FALSE;
		goto end;
	    }
	}
end:    
	CloseLibrary(OOPBase);
    }
    
    ReturnBool("init_hidds", success);
}

/*****************
**  init_gfx()  **
*****************/

static BOOL init_gfx(STRPTR gfxclassname, struct initbase *base)
{
    struct GfxBase *GfxBase;
    BOOL success = FALSE;
    
    EnterFunc(bug("init_gfx(hiddbase=%s)\n", gfxclassname));
    
    GfxBase = (struct GfxBase *)OpenLibrary("graphics.library", 37);
    if (GfxBase)
    {
    	D(bug("gfx.library opened\n"));

	/*  Call private gfx.library call to init the HIDD.
	    Gfx library is responsable for closing the HIDD
	    library (although it will probably not be neccesary).
	*/

    	D(bug("calling private gfx LateGfxInit()\n"));
	if (LateGfxInit(gfxclassname))
	{
	    struct IntuitionBase *IntuitionBase;
	    D(bug("success\n"));
	    
	    /* Now that gfx. is guaranteed to be up & working, let intuition open WB screen */
	    IntuitionBase = (struct IntuitionBase *)OpenLibrary("intuition.library", 37);
	    if (IntuitionBase)
	    {
	    	if (LateIntuiInit(NULL))
	    	{
	    	    success = TRUE;
		}
		CloseLibrary((struct Library *)IntuitionBase);
	    }
	}
	D(bug("Closing gfx\n"));
	
	CloseLibrary((struct Library *)GfxBase);
    }
    ReturnBool ("init_gfxhidd", success);
}


static BOOL init_device( STRPTR hiddclassname, STRPTR devicename,  struct initbase *base)
{
    BOOL success = FALSE;
    struct MsgPort *mp;


    EnterFunc(bug("init_device(classname=%s)\n", hiddclassname));

    mp = CreateMsgPort();
    if (mp)
    {
    	struct IORequest *io;
	io = CreateIORequest(mp, sizeof ( struct IOStdReq));
	{
	    if (0 == OpenDevice(devicename, 0, io, 0))
	    {
		UBYTE *data;

	        /* Allocate message data */
		data = AllocMem(BUFSIZE, MEMF_PUBLIC);
		if (data)
		{
		    #define ioStd(x) ((struct IOStdReq *)x)
		    strcpy(data, hiddclassname);
		    ioStd(io)->io_Command = CMD_HIDDINIT;
		    ioStd(io)->io_Data = data;
		    ioStd(io)->io_Length = strlen(data);

		    /* Let the device init the HIDD */
		    DoIO(io);
		    if (0 == io->io_Error)
		    {
			success = TRUE;
		    }

		    FreeMem(data, BUFSIZE);
		}
		CloseDevice(io);
		
	    }
	    DeleteIORequest(io);
	    
	}
	
	DeleteMsgPort(mp);
    
    }
    
    ReturnBool("init_device", success);
}

