/*
    Copyright  1995-2010, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Disk-resident part of X11 display driver
    Lang: english
*/

#include <aros/debug.h>
#include <dos/dosextens.h>
#include <oop/oop.h>
#include <workbench/startup.h>
#include <workbench/workbench.h>
#include <proto/dos.h>
#include <proto/exec.h>
#include <proto/graphics.h>
#include <proto/oop.h>
#include <proto/icon.h>

#include <stdlib.h>

#include "x11_class.h"

/* Minimum required library version */
#define X11_VERSION 42
/* Default host keymap file */
#define DEF_KEYMAP "DEVS:Keymaps/X11/keycode2rawkey.table"

/************************************************************************/

/*
 * This program is not a driver itself. It is some kind of prefs applicator
 * whose functions are:
 *
 * 1. Load host keymap file
 * 2. Create additional displays (when implemented in the driver)
 *
 * The driver itself should be placed in kickstart in the form of library.
 */

#define ARGS_TEMPLATE "DISPLAYS/N,KEYMAP"

struct MyArgs
{
    IPTR   displays;
    STRPTR keymap;
};

extern struct WBStartup *WBenchMsg;

int __nocommandline = 1;

/*
 * This code loads a specified X11 keymap table into the driver. Without
 * this keyboard may work in a wrong way.
 *
 * The default keymap is created by the build system using this command:
 *     make default-x11keymaptable
 *
 * A user may also generate his own one:
 *     make change-x11keymaptable
 *
 * The default keymaptable probably works with most PCs having a 105 key
 * keyboard if you are using XFree86 as X Server (might also work with
 * others). So try that one first!
 *
 * Since the keymap table will be deleted when you do a "make clean" you
 * might want to make a backup of it. Then you will be able to restore it later:
 *     make backup-x11keymaptable
 *     make restore-x11keymaptable\n"
 *
 * The keymap table will be backed up in your HOME directory.
 *
 * Note that the keymaptable only makes sure that your keyboard looks as
 * much as possible like an Amiga keyboard to AROS. So with the keymaptable
 * alone the keyboard will behave like an Amiga keyboard with American layout.
 * For other layouts you must activate the correct keymap in AROS Locale prefs.
 */

static ULONG LoadKeyCode2RawKeyTable(STRPTR filename)
{
    struct X11Base *X11Base;
    ULONG displays;
    BPTR  fh;

    X11Base = (struct X11Base *)OpenLibrary(X11_LIBNAME, X11_VERSION);
    if (!X11Base) {
        D(bug("[X11] No X11 driver in the system\n"));
        return 0;
    }

    displays = X11Base->library.lib_OpenCnt - 1;

    if ((fh = Open(filename, MODE_OLDFILE)))
    {
	D(bug("[X11] X keymap file handle: %p\n", fh));

	if ((256 == Read(fh, X11Base->keycode2rawkey, 256)))
	{
		D(bug("LoadKeyCode2RawKeyTable: keycode2rawkey.table successfully loaded!\n"));
		X11Base->havetable = TRUE;
	}
	Close(fh);
    }
    
    CloseLibrary(&X11Base->library);

    return displays;
}

/* This function uses library open count as displays count */
static ULONG AddDisplays(ULONG num, ULONG old)
{
    struct Library *X11Base;
    OOP_Object *gfxhidd;
    ULONG i;

    D(bug("[X11] Making %u displays\n", num));
    D(bug("[X11] Current displays count: %u\n", old));

    /* Add displays if needed, open the library once more for every display */
    for (i = old; i < num; i++)
    {
        /* This increments counter */
	X11Base = OpenLibrary(X11_LIBNAME, X11_VERSION);
	if (!X11Base) {
	    D(bug("[X11] Failed to open X11 library!\n"));
	    break;
	}

	gfxhidd = OOP_NewObject(NULL, CLID_Hidd_X11Gfx, NULL);
	D(bug("[X11] Created display object 0x%p\n", gfxhidd));
	if (gfxhidd){
	    if (AddDisplayDriverA(gfxhidd, NULL)) {
		D(bug("[X11] Failed to add display object\n"));
		OOP_DisposeObject(gfxhidd);
		gfxhidd = NULL;
	    }
	}

	/* If driver setup failed, decrement counter back and abort */
	if (!gfxhidd) {
	    CloseLibrary(X11Base);
	    break;
	}
    }
    
    return i;
}

int main(void)
{
    BPTR olddir = BNULL;
    STRPTR myname;
    struct DiskObject *icon;
    struct RDArgs *rdargs = NULL;
    ULONG old_displays;
    int res = RETURN_OK;
    struct MyArgs args =
    {
	1,
	DEF_KEYMAP
    };

    if (WBenchMsg)
    {
        olddir = CurrentDir(WBenchMsg->sm_ArgList[0].wa_Lock);
	myname = WBenchMsg->sm_ArgList[0].wa_Name;
    }
    else
    {
	struct Process *me = (struct Process *)FindTask(NULL);
    
	if (me->pr_CLI)
	{
            struct CommandLineInterface *cli = BADDR(me->pr_CLI);

	    myname = AROS_BSTR_ADDR(cli->cli_CommandName);
	} else
	    myname = me->pr_Task.tc_Node.ln_Name;
    }
    D(bug("[X11] Command name: %s\n", myname));

    icon = GetDiskObject(myname);
    D(bug("[X11] Icon 0x%p\n", icon));

    if (icon) {
        STRPTR str;

	str = FindToolType(icon->do_ToolTypes, "DISPLAYS");
	if (str)
	    args.displays = atoi(str);
	
	str = FindToolType(icon->do_ToolTypes, "KEYMAP");
	if (str)
	    args.keymap = str;
    }

    if (!WBenchMsg) {
        rdargs = ReadArgs(ARGS_TEMPLATE, (IPTR *)&args, NULL);
	D(bug("[X11] RDArgs 0x%p\n", rdargs));
    }

    D(bug("[X11] Keymap: %s\n", args.keymap));

    old_displays = LoadKeyCode2RawKeyTable(args.keymap);

/*
 * TODO: In order for this to work X11 driver needs to be fixed
 *       in the following way:
 *	 - display-specific and window-specific data should be moved from class static data
 *	   to driver instance data.
 *	 - event task should be able to handle several display windows
 *	 - display windows should close when not needed (empty).
 *	 - invent what to do with host clipboard handling. The idea is nice but:
 *	   a) It should be somehow integrated with clipboard.device
 *         b) Several X11 displays might mean several clipboards (if they run on different
 *	      X servers).
 *
    if (old_displays)
        AddDisplays(args.displays, old_displays); */

    if (rdargs)
        FreeArgs(rdargs);
    if (icon)
        FreeDiskObject(icon);
    if (olddir)
        CurrentDir(olddir);

    return res;
}
