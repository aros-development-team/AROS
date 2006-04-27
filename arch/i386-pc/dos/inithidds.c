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
#include <exec/lists.h>
#include <dos/filesystem.h>
#include <utility/tagitem.h>
#include <utility/hooks.h>
#include <hidd/hidd.h>
#include <aros/bootloader.h>

#include <proto/exec.h>
#include <proto/oop.h>
#include <proto/utility.h>
#include <proto/dos.h>
#include <proto/bootloader.h>
#include <proto/intuition.h>
#include <oop/oop.h>
#include <string.h>

#warning Fix this in a better way. It will break if things move around.
#include "../../rom/devs/devs_private.h"

#include <aros/asmcall.h>

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

#define BUFSIZE 100

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
    BOOL success = TRUE, vga = FALSE;
    UBYTE gfxname[BUFSIZE];
    struct BootLoaderBase *BootLoaderBase;

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
	/* Initialize these HIDDs by default */
	if (!(OpenLibrary("mouse.hidd",0L)))
	{
	    success = FALSE;
	    bug("[DOS] InitHidds: Failed to open mouse.hidd\n");
	}

	if (!(OpenLibrary("kbd.hidd",0L)))
	{
	    success = FALSE;
	    bug("[DOS] InitHidds: Failed to open kbd.hidd\n");
	}
	if (!(OpenLibrary("graphics.hidd",0L)))
	{
	    success = FALSE;
	    bug("[DOS] InitHidds: Failed to open graphics.hidd\n");
	}

	/* Prepare the VGA hidd as a fallback */
	strncpy(gfxname,"hidd.gfx.vga",BUFSIZE-1);
	if ((BootLoaderBase = OpenResource("bootloader.resource")))
	{
	    struct List *list;
	    struct Node *node;

	    list = (struct List *)GetBootInfo(BL_Args);
	    if (list)
	    {
		ForeachNode(list,node)
		{
		    if (0 == strncmp(node->ln_Name,"gfx=",4))
		    {
			bug("[DOS] InitHidds: Using %s as graphics driver\n",&node->ln_Name[4]);
			strncpy(gfxname,&(node->ln_Name[4]),BUFSIZE-1);
		    }
		    if (0 == strncmp(node->ln_Name,"lib=",4))
		    {
			bug("[DOS] InitHidds: Opening library %s\n",&node->ln_Name[4]);
			if (!(OpenLibrary(&node->ln_Name[4],0L)))
			    bug("[DOS] InitHidds: Failed to open %s\n",&node->ln_Name[4]);
			if (0 == strcmp(&node->ln_Name[4],"hidd.gfx.vga"))
			    vga = TRUE;
		    }
		}
	    }
	}

	/* See if vesa.hidd loaded. If so, verify that we actually got a vesa mode from
	 * the bootloader */
	if (0 == strcmp(gfxname,"hidd.gfx.vesa"))
	{
	    struct VesaInfo *vi;

	    if ((vi = GetBootInfo(BL_Video)))
	    {
		if (vi->ModeNumber == 3)
		{
		    /* Nope, bootloader failed to set vesa mode */
		    strcpy(gfxname,"hidd.gfx.vga");
		    bug("[DOS] InitHidds: VESA graphics requested, but not found. Falling back to VGA\n");
		}
	    }
	}

	/* If we got no gfx hidd on the commandline, and did not load vga.hidd,
	 * we will do that now. */
	if (0 == strcmp(gfxname,"hidd.gfx.vga") && vga == FALSE)
	{
	    OpenLibrary("vgah.hidd",0L);
	}

	/* Set up the graphics HIDD system */
	if (!init_gfx(gfxname, base))
	{
	    bug("[DOS] InitHidds: Could not init gfx hidd %s\n", gfxname);
	    success = FALSE;
	}

	/* And finally keyboard and mouse */
	if (!init_device("hidd.kbd.hw", "keyboard.device", base))
	{
	    bug("[DOS] InitHidds: Could not init keyboard hidd\n");
	    success = FALSE;
	}

	if (!init_device("hidd.bus.mouse", "gameport.device", base))
	{
	    bug("[DOS] InitHidds: Could not init mouse hidd\n");
	    success = FALSE;
	}
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

