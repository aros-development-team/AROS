/*
    Copyright  1995-2010, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Code that loads and initializes necessary HIDDs.
    Lang: english
*/

#define DEBUG 0
#include <aros/debug.h>

#include <aros/bootloader.h>
#include <exec/memory.h>
#include <exec/resident.h>
#include <exec/alerts.h>
#include <exec/io.h>
#include <dos/filesystem.h>
#include <libraries/bootmenu.h>
#include <utility/tagitem.h>
#include <utility/hooks.h>
#include <hidd/hidd.h>

#include <proto/bootloader.h>
#include <proto/exec.h>
#include <proto/oop.h>
#include <proto/utility.h>
#include <proto/dos.h>
#include <proto/graphics.h>
#include <proto/intuition.h>
#include <oop/oop.h>
#include <string.h>

#include "devs_private.h"

#include <aros/asmcall.h>

struct initbase
{
    struct ExecBase	*sysbase;
    struct DosLibrary	*dosbase;
    struct Library	*oopbase;
};

#define DOSBase (base->dosbase)
#define OOPBase (base->oopbase)

static BOOL __dosboot_InitGfx   ( STRPTR gfxclassname,   struct initbase *base);
static BOOL __dosboot_InitDevice( STRPTR hiddclassname, STRPTR devicename,  struct initbase *base);

/************************************************************************/

#define BUFSIZE 100

#define HIDDPREFSFILE "SYS:S/hidd.prefs"

/* We don't link with c library so I must implement this separately */
#define isblank(c) \
	(c == '\t' || c == ' ')
#define isspace(c) \
	(c == '\t' || c == ' ' || c == '\f' || c == '\n' || c == '\r' || c == '\v')

BOOL __dosboot_InitHidds(struct ExecBase *sysBase, struct DosLibrary *dosBase, APTR BootLoaderBase)
{
    /* This is the initialisation code for InitHIDDs module */
    struct initbase stack_b, *base = &stack_b;
    BOOL success = TRUE;
    UBYTE buf[BUFSIZE];
    UBYTE gfxname[BUFSIZE], kbdname[BUFSIZE], mousename[BUFSIZE];
    BOOL def_gfx = TRUE, def_kbd = TRUE, def_mouse = TRUE;
    struct BootMenuBase *BootMenuBase;

    base->sysbase = sysBase;
    base->dosbase = dosBase;

    D(bug("[DOSBoot] __dosboot_InitHidds()\n")); 

    OOPBase = OpenLibrary(AROSOOP_NAME, 0);
    if (!OOPBase)
    {
    	success = FALSE;
    }
    else
    {
	BPTR fh;

	D(bug("[DOSBoot] __dosboot_InitHidds: OOP opened\n"));

/* TODO: graphics.library is not in the kernel on Linux-hosted
	 version, so we can't do this check because we'll fail.
	 As a workaround, we load it explicitly in S:hidd.prefs.

	if ((OpenLibrary("graphics.hidd", 0L)) == NULL)
	{
	    success = FALSE;
	    bug("[DOS] InitHidds: Failed to open graphics.hidd\n");
	    goto end;
	}*/
	OpenLibrary("graphics.hidd", 0);

	BootMenuBase = (struct BootMenuBase *)OpenResource("bootmenu.resource");
	D(bug("[DOS] __dosboot_InitHidds: BootMenuBase = 0x%p\n", BootMenuBase));
	if (BootMenuBase) {
	    strcpy(gfxname, BootMenuBase->bm_BootConfig.defaultgfx.hiddname);
	    strcpy(kbdname, BootMenuBase->bm_BootConfig.defaultkbd.hiddname);
	    strcpy(mousename, BootMenuBase->bm_BootConfig.defaultmouse.hiddname);
	}

	/* Open the hidd prefsfile */	
	fh = Open(HIDDPREFSFILE, FMF_READ);
	if (fh)
	{    
            D(bug("[DOS] __dosboot_InitHidds: hiddprefs file opened\n"));
            while (FGets(fh, buf, BUFSIZE))
	    {
	        STRPTR keyword = buf, arg, end;
		STRPTR s;

		if (*buf == '#')
		    continue;

		s = buf;
		if (*s) {
		    for (; *s; s ++)
		    	;
		    if (s[-1] == 10) {
		    	s[-1] = 0;
		    }
		}		    

		D(bug("[DOS] __dosboot_InitHidds: Got line: %s\n", buf));

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
		D(bug("[DOS] __dosboot_InitHidds: Find argument at %s\n", arg));
		while ((*arg != 0) && isblank(*arg))
		    arg ++;

		if (*arg == 0)
		    continue;

		D(bug("[DOS] __dosboot_InitHidds: Terminate argument at %s\n", arg));
		  /* terminate argument */
		end = arg;
		while ( (*end != 0) && (!isblank(*end)))
		    end ++;
		if (*end != 0)
		    *end = 0;

		D(bug("[DOS] __dosboot_InitHidds: Got keyword \"%s\" arg \"%s\"\n", keyword, arg));

		if (0 == strcmp(keyword, "library"))
		{
		    D(bug("Opening library\n"));
		    /* Open a specified library */
		    OpenLibrary(arg, 0);
		}
		else if (0 == strcmp(keyword, "gfx"))
		{
		    strncpy(gfxname, arg, BUFSIZE - 1);
		    def_gfx = FALSE;
		}
		else if (0 == strcmp(keyword, "mouse"))
		{
		    strncpy(mousename, arg, BUFSIZE - 1);
		    def_mouse = FALSE;
		}
		else if (0 == strcmp(keyword, "kbd"))
		{
		    strncpy(kbdname, arg, BUFSIZE - 1);
		    def_kbd = FALSE;
		}
	    }
	    Close(fh);
	}

	if (BootLoaderBase)
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
			D(bug("[DOS] __dosboot_InitHidds: Using %s as graphics driver\n",&node->ln_Name[4]));
			strncpy(gfxname,&(node->ln_Name[4]),BUFSIZE-1);
			def_gfx = FALSE;
		    }
		    if (0 == strncmp(node->ln_Name,"lib=",4))
		    {
			D(bug("[DOS] __dosboot_InitHidds: Opening library %s\n",&node->ln_Name[4]));
			OpenLibrary(&node->ln_Name[4],0L);
		    }
		    if (0 == strncmp(node->ln_Name,"kbd=",4))
		    {
			strncpy(kbdname, &node->ln_Name[4], BUFSIZE-1);
			def_kbd = FALSE;
		    }
		    if (0 == strncmp(node->ln_Name,"mouse=",6))
		    {
			strncpy(mousename, &node->ln_Name[6], BUFSIZE-1);
			def_mouse = FALSE;
		    }
		}
	    }
	}

	if ((!BootMenuBase) && (def_gfx || def_mouse || def_kbd)) {
	    success = FALSE;
	    kprintf("You must specify drivers for this system!\n");
	    goto end;
	}

	if (def_gfx)
	{
	    if (!OpenLibrary(BootMenuBase->bm_BootConfig.defaultgfx.libname, 0)) {
	        success = FALSE;
	    	kprintf("Unable to load gfx hidd %s\n",
			BootMenuBase->bm_BootConfig.defaultgfx.libname);
		goto end;
	    }
	}
	if (def_mouse && mousename[0])
	{
	    if (!OpenLibrary(BootMenuBase->bm_BootConfig.defaultmouse.libname, 0)) {
	        success = FALSE;
	    	kprintf("Unable to load mouse hidd %s\n",
			BootMenuBase->bm_BootConfig.defaultmouse.libname);
		goto end;
	    }
	}
	if (def_kbd && kbdname[0])
	{
	    if (!OpenLibrary(BootMenuBase->bm_BootConfig.defaultkbd.libname, 0)) {
	        success = FALSE;
	    	kprintf("Unable to load keyboard hidd %s\n", BootMenuBase->bm_BootConfig.defaultkbd.libname);
		goto end;
	    }
	}

	if (!__dosboot_InitGfx(gfxname, base))
	{
	    kprintf("Could not init gfx hidd %s\n", gfxname);
	    success = FALSE;
	    goto end;
	}
	if (!__dosboot_InitDevice(kbdname, "keyboard.device", base))
	{
	    kprintf("Could not init keyboard hidd %s\n", kbdname);
	    success = FALSE;
	    goto end;
	}
	if (!__dosboot_InitDevice(mousename, "gameport.device", base))
	{
	    kprintf("Could not init mouse hidd %s\n", mousename);
	    success = FALSE;
	    goto end;
	}
end:    
	CloseLibrary(OOPBase);
    }    
    ReturnBool("__dosboot_InitHidds", success);
}

/*****************
**  __dosboot_InitGfx()  **
*****************/

static BOOL __dosboot_InitGfx(STRPTR gfxclassname, struct initbase *base)
{
    struct GfxBase *GfxBase;
    BOOL success = FALSE;

    D(bug("[DOSBoot] __dosboot_InitGfx(hiddbase='%s')\n", gfxclassname)); 

    GfxBase = (struct GfxBase *)OpenLibrary("graphics.library", 37);
    if (GfxBase)
    {
    	D(bug("[DOS] __dosboot_InitGfx: gfx.library opened\n"));

	/*  Call private gfx.library call to init the HIDD.
	    Gfx library is responsable for closing the HIDD
	    library (although it will probably not be neccesary).
	*/

    	D(bug("[DOS] __dosboot_InitGfx: calling private gfx LateGfxInit() ...\n"));
	if (LateGfxInit(gfxclassname))
	{
	    struct IntuitionBase *IntuitionBase;
            D(bug("[DOS] __dosboot_InitGfx:    ... success\n"));

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
	D(bug("[DOS] __dosboot_InitGfx: Closing gfx\n"));

	CloseLibrary((struct Library *)GfxBase);
    }
    ReturnBool ("__dosboot_InitGfx", success);
}


static BOOL __dosboot_InitDevice( STRPTR hiddclassname, STRPTR devicename,  struct initbase *base)
{
    BOOL success = FALSE;
    struct MsgPort *mp;

    D(bug("[DOSBoot] __dosboot_InitDevice(classname='%s')\n", hiddclassname)); 

    if (!hiddclassname[0])
        return TRUE;
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
    ReturnBool("__dosboot_InitDevice", success);
}
