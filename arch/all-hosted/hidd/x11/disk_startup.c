/*
    Copyright  1995-2020, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Disk-resident part of X11 display driver
    Lang: english
*/

#include <aros/debug.h>

#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/graphics.h>
#include <proto/oop.h>
#include <proto/icon.h>
#include <proto/x11gfx.h>

#include <dos/dosextens.h>
#include <oop/oop.h>
#include <workbench/startup.h>
#include <workbench/workbench.h>

#include <stdlib.h>

#include "x11_class.h"

/* Minimum required library version */
#define X11_VERSION 42
/* Default host keymap file */
#define DEF_KEYMAP "DEVS:Keymaps/X11/keycode2rawkey.table"

#define STACK_SIZE 100000

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
    struct Library *X11ClBase;
    ULONG displays;
    BPTR  fh;

    D(bug("[X11:DiskStart] %s()\n", __func__));

    X11ClBase = (struct Library *)OpenLibrary(X11_LIBNAME, X11_VERSION);
    if (!X11ClBase) {
        D(bug("[X11:DiskStart] No X11 driver in the system\n"));
        return 0;
    }
    D(
        bug("[X11:DiskStart] %s: X11ClBase @ 0x%p\n", __func__, X11ClBase);
    )
    displays = X11ClBase->lib_OpenCnt - 1;

    if ((fh = Open(filename, MODE_OLDFILE)))
    {
        UBYTE	   	        keycode2rawkey[256];

        D(bug("[X11:DiskStart] %s: X key table file handle: %p\n", __func__, fh));
        D(bug("[X11:DiskStart] %s: X key table name       : %s\n", __func__, filename));

        if ((256 == Read(fh, keycode2rawkey, 256)))
        {
                x11kdb_LoadkeyTable(keycode2rawkey);
                D(bug("[X11:DiskStart] %s: table loaded\n", __func__));
        }
        Close(fh);
    }

    CloseLibrary(X11ClBase);

    return displays;
}

int main(void)
{
    struct DiskObject *icon;
    struct RDArgs *rdargs = NULL;
    BPTR olddir = BNULL;
    ULONG old_displays;
    STRPTR myname;
    int res = RETURN_OK;
    struct Node *x11entry;

    struct MyArgs args =
    {
        1,
        DEF_KEYMAP
    };

    struct StackSwapStruct sss;
    struct StackSwapArgs ssa;
    UBYTE *stack;

    D(bug("[X11:DiskStart] %s()\n", __func__));

    if ((x11entry = FindName(&SysBase->LibList, X11_LIBNAME)))
    {
        D(bug("[X11:DiskStart] %s: X11gfx has already loaded @ 0x%p\n", __func__, x11entry));
        return res;
    }

    stack = AllocMem(STACK_SIZE, MEMF_ANY);
    if (stack == NULL)
        return RETURN_FAIL;

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
    D(bug("[X11:DiskStart] %s: Command name: %s\n", __func__, myname));

    icon = GetDiskObject(myname);
    if (icon)
    {
        STRPTR str;
        D(bug("[X11:DiskStart] %s: Icon 0x%p\n", __func__, icon));

        str = FindToolType(icon->do_ToolTypes, "DISPLAYS");
        if (str)
            args.displays = atoi(str);
        
        str = FindToolType(icon->do_ToolTypes, "KEYMAP");
        if (str)
            args.keymap = str;
    }

    if (!WBenchMsg) {
        rdargs = ReadArgs(ARGS_TEMPLATE, (IPTR *)&args, NULL);
        D(bug("[X11:DiskStart] %s: RDArgs 0x%p\n", __func__, rdargs));
    }

    D(bug("[X11:DiskStart] %s: Keymap: %s\n", __func__, args.keymap));

    /* Call LoadKeyCode2RawKeyTable() with a new stack: it initialises
       x11gfx.hidd, and some X servers need a larger than normal stack */
    sss.stk_Lower = stack;
    sss.stk_Upper = stack + STACK_SIZE;
    sss.stk_Pointer = sss.stk_Upper;

    ssa.Args[0] = (IPTR)args.keymap;

    old_displays = NewStackSwap(&sss, LoadKeyCode2RawKeyTable, &ssa);

    D(bug("[X11:DiskStart] %s: cleaning up\n", __func__));

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
 */
#if 0
    if (old_displays)
        AddDisplays(args.displays, old_displays);
#else
    (void)old_displays; /* unused */
#endif

    if (rdargs)
        FreeArgs(rdargs);
    if (icon)
        FreeDiskObject(icon);
    if (olddir)
        CurrentDir(olddir);
    FreeMem(stack, STACK_SIZE);

    D(bug("[X11:DiskStart] %s: exiting\n", __func__));

    return res;
}
