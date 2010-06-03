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
#include "dosboot_intern.h"

#include <aros/asmcall.h>

/************************************************************************/

#undef GfxBase

#define HIDDPREFSFILE "SYS:S/hidd.prefs"

/* We don't link with c library so I must implement this separately */
#define isblank(c) \
	(c == '\t' || c == ' ')
#define isspace(c) \
	(c == '\t' || c == ' ' || c == '\f' || c == '\n' || c == '\r' || c == '\v')

BOOL __dosboot_InitHidds(struct DosLibrary *dosBase, APTR BootLoaderBase, struct BootConfig *cfg)
{
    /* This is the initialisation code for InitHIDDs module */
    BOOL success = TRUE;
    UBYTE buf[BUFSIZE];
    BOOL def_gfx = TRUE, def_kbd = TRUE, def_mouse = TRUE;
    BPTR fh;

    D(bug("[DOSBoot] __dosboot_InitHidds()\n")); 

    /* Open the hidd prefsfile */	
    fh = Open(HIDDPREFSFILE, FMF_READ);
    if (fh) {
        D(bug("[DOS] __dosboot_InitHidds: hiddprefs file opened\n"));

        while (FGets(fh, buf, BUFSIZE)) {
	    STRPTR keyword = buf, arg, end;
	    STRPTR s;

	    if (*buf == '#')
		continue;

	    s = buf;
	    if (*s) {
		for (; *s; s ++);

		if (s[-1] == 10)
		    s[-1] = 0;
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
		arg ++;
	    if (*arg == 0)
		continue;
	    *arg++ = 0;

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

	    if (0 == strcmp(keyword, "library")) {
		D(bug("Opening library\n"));
		/* Open a specified library */
		OpenLibrary(arg, 0);
	    } else if (0 == strcmp(keyword, "gfx")) {
		strncpy(cfg->defaultgfx.hiddname, arg, BUFSIZE - 1);
		def_gfx = FALSE;
	    } else if (0 == strcmp(keyword, "mouse")) {
		strncpy(cfg->defaultmouse.hiddname, arg, BUFSIZE - 1);
		def_mouse = FALSE;
	    } else if (0 == strcmp(keyword, "kbd")) {
		strncpy(cfg->defaultkbd.hiddname, arg, BUFSIZE - 1);
		def_kbd = FALSE;
	    }
	}
	Close(fh);
    }

    if (BootLoaderBase) {
	struct List *list;
	struct Node *node;

	list = (struct List *)GetBootInfo(BL_Args);
	if (list) {
	    ForeachNode(list,node) {
		if (0 == strncmp(node->ln_Name,"lib=",4)) {
		    D(bug("[DOS] __dosboot_InitHidds: Opening library %s\n",&node->ln_Name[4]));
		    OpenLibrary(&node->ln_Name[4],0L);
		} else if (0 == strncmp(node->ln_Name,"gfx=",4)) {
		    strncpy(cfg->defaultgfx.hiddname, &node->ln_Name[4], BUFSIZE-1);
		    def_gfx = FALSE;
		} else if (0 == strncmp(node->ln_Name,"kbd=",4)) {
		    strncpy(cfg->defaultkbd.hiddname, &node->ln_Name[4], BUFSIZE-1);
		    def_kbd = FALSE;
		} else if (0 == strncmp(node->ln_Name,"mouse=",6)) {
		    strncpy(cfg->defaultmouse.hiddname, &node->ln_Name[6], BUFSIZE-1);
		    def_mouse = FALSE;
		}
	    }
	}
    }

    /* TODO: Run everything from DEVS:Monitors here */

    if (cfg->defaultgfx.hiddname[0]) {
        struct GfxBase *GfxBase;
	BOOL ok;

	if (def_gfx && (!OpenLibrary(cfg->defaultgfx.libname, 0))) {
	    success = FALSE;
	    kprintf("Unable to load graphics hidd %s\n", cfg->defaultgfx.libname);
	    goto end;
	}

	GfxBase = (struct GfxBase *)OpenLibrary("graphics.library", 41);
	if (!GfxBase) {
	    success = FALSE;
	    kprintf("Unable to open graphics.library v41\n");
	    goto end;
	}

	ok = init_gfx(cfg->defaultgfx.hiddname, GfxBase);
	CloseLibrary(&GfxBase->LibNode);
	if (!ok) {
	    success = FALSE;
	    kprintf("Could not init graphics hidd %s\n", cfg->defaultgfx.hiddname);
	    goto end;
	}
    }

    if (cfg->defaultmouse.hiddname[0]) {
        if (def_mouse && (!OpenLibrary(cfg->defaultmouse.libname, 0))) {
	    success = FALSE;
	    kprintf("Unable to load mouse hidd %s\n", cfg->defaultmouse.libname);
	    goto end;
	}
	if (!init_device(cfg->defaultmouse.hiddname, "gameport.device")) {
	    kprintf("Could not init mouse hidd %s\n", cfg->defaultmouse.hiddname);
	    success = FALSE;
	    goto end;
	}
    }

    if (cfg->defaultkbd.hiddname[0]) {
	if (def_kbd && (!OpenLibrary(cfg->defaultkbd.libname, 0))) {
	    success = FALSE;
	    kprintf("Unable to load keyboard hidd %s\n", cfg->defaultkbd.libname);
	    goto end;
	}
	if (!init_device(cfg->defaultkbd.hiddname, "keyboard.device")) {
	    kprintf("Could not init keyboard hidd %s\n", cfg->defaultkbd.hiddname);
	    success = FALSE;
	    goto end;
	}
    }

end:
    ReturnBool("__dosboot_InitHidds", success);
}


BOOL init_gfx(STRPTR gfxclassname, struct GfxBase *GfxBase)
{
    OOP_Object *gfxhidd;
    BOOL success = FALSE;

    D(bug("[BootMenu] init_gfx('%s')\n", gfxclassname));

    gfxhidd = OOP_NewObject(NULL, gfxclassname, NULL);
    if (gfxhidd) {
        if (AddDisplayDriverA(gfxhidd, NULL))
	    OOP_DisposeObject(gfxhidd);
	else
	    success = TRUE;
    }

    ReturnBool ("init_gfxhidd", success);
}

BOOL init_device(STRPTR hiddclassname, STRPTR devicename)
{
    BOOL success = FALSE;
    struct MsgPort *mp = NULL;

    D(bug("[BootMenu] init_device(classname='%s', devicename='%s')\n", hiddclassname, devicename));

    if ((mp = CreateMsgPort()) != NULL)
    {
        struct IORequest *io = NULL;
        if ((io = CreateIORequest(mp, sizeof ( struct IOStdReq))) != NULL)
        {
            if (0 == OpenDevice(devicename, 0, io, 0))
            {
                #define ioStd(x) ((struct IOStdReq *)x)
                ioStd(io)->io_Command = CMD_HIDDINIT;
                ioStd(io)->io_Data = hiddclassname;
                ioStd(io)->io_Length = strlen(hiddclassname);

                /* Let the device init the HIDD */
                DoIO(io);
                if (0 == io->io_Error)
                {
                    success = TRUE;
                }
                CloseDevice(io);
            }
            DeleteIORequest(io); 
        }
        DeleteMsgPort(mp);
    } 
    ReturnBool("init_device", success);
}
