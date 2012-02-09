/*
    Copyright © 1995-2012, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Boot your operating system.
    Lang: english
*/

#include <aros/debug.h>
#include <exec/alerts.h>
#include <exec/libraries.h>
#include <exec/devices.h>
#include <exec/execbase.h>
#include <aros/libcall.h>
#include <aros/asmcall.h>
#include <dos/dosextens.h>
#include <dos/cliinit.h>
#include <dos/stdio.h>
#include <utility/tagitem.h>
#include <libraries/expansionbase.h>
#include <proto/exec.h>
#include <proto/dos.h>

#include "dos_intern.h"

#ifdef __mc68000
/*
 * Load DEVS:system-configuration only on m68k.
 * Setup pre-2.0 boot disk colors and mouse cursors (for example)
 */
#define USE_SYSTEM_CONFIGURATION

#endif

#ifdef USE_SYSTEM_CONFIGURATION

#include <proto/intuition.h>

static void load_system_configuration(struct DosLibrary *DOSBase)
{
    BPTR fh;
    ULONG len;
    struct Preferences prefs;
    struct Library *IntuitionBase;

    fh = Open("DEVS:system-configuration", MODE_OLDFILE);
    if (!fh)
    	return;
    len = Read(fh, &prefs, sizeof prefs);
    Close(fh);
    if (len != sizeof prefs)
    	return;
    IntuitionBase = TaggedOpenLibrary(TAGGEDOPEN_INTUITION);
    if (IntuitionBase)
	SetPrefs(&prefs, len, FALSE);
    CloseLibrary(IntuitionBase);
}

#else

#define load_system_configuration(DOSBase) do { } while (0)

#endif

extern void BCPL_cliInit(void);

void __dos_Boot(struct DosLibrary *DOSBase, ULONG BootFlags, UBYTE Flags)
{
    BPTR cis = BNULL;

    /*  We have been created as a process by DOS, we should now
    	try and boot the system. */

    D(bug("[__dos_Boot] generic boot sequence, BootFlags 0x%08X Flags 0x%02X\n", BootFlags, Flags));

    /* m68000 uses this to get the default colors and
     * cursors for Workbench
     */
    load_system_configuration(DOSBase);

    /*
     * If needed, run the display drivers loader.
     * In fact the system must have at least one resident driver,
     * which will be used for bootmenu etc. However, it we somehow happen
     * not to have it, this will be our last chance.
     */
    if ((BootFlags & (BF_NO_DISPLAY_DRIVERS | BF_NO_COMPOSITION)) != (BF_NO_DISPLAY_DRIVERS | BF_NO_COMPOSITION))
    {
        /* Check that it exists first... */
        BPTR seg = LoadSeg("C:AROSMonDrvs");

        if (seg != BNULL)
        {
            STRPTR args = "";
            BPTR oldin, oldout;

	    /*
	     * Argument strings MUST contain terminating LF because of ReadItem() bugs.
	     * Their absence causes ReadArgs() crash.
	     */
            if (BootFlags & BF_NO_COMPOSITION)
            	args = "NOCOMPOSITION\n";
            else if (BootFlags & BF_NO_DISPLAY_DRIVERS)
            	args = "ONLYCOMPOSITION\n";

	    D(bug("[__dos_Boot] Running AROSMonDrvs %s\n", args));

            /* RunCommand needs a valid Input() handle
             * for passing in its arguments.
             */
            oldin = SelectInput(Open("NIL:", MODE_OLDFILE));
            oldout= SelectOutput(Open("NIL:", MODE_NEWFILE));
            RunCommand(seg, AROS_STACKSIZE, args, strlen(args));
            SelectInput(oldin);
            SelectOutput(oldout);

            /* We don't care about the return code */
            UnLoadSeg(seg);
        }
    }

    cis = Open("CON:////Initial CLI/AUTO/CLOSE/SMART", MODE_OLDFILE);
    if (cis) {
        BPTR cos = OpenFromLock(DupLockFromFH(cis));
        BYTE const C[] = "AROS - The AROS Research Operating System\n"
                         "Copyright © 1995-2012, The AROS Development Team. "
                         "All rights reserved.\n"
                         "AROS is licensed under the terms of the "
                         "AROS Public License (APL),\n"
                         "a copy of which you should have received "
                         "with this distribution.\n"
                         "Visit http://www.aros.org/ for more information.\n";

        if (cos) {
            BPTR cas = BNULL;
            
            if (!(BootFlags & BF_NO_STARTUP_SEQUENCE))
                cas = Open("S:Startup-Sequence", MODE_OLDFILE);

            /* Inject the banner */
            if (Flags & EBF_SILENTSTART) {
                if (SetVBuf(cos, NULL, BUF_FULL, sizeof(C)) == 0) {
                    FPuts(cos, C);
                    SetVBuf(cos, NULL, BUF_LINE, -1);
                }
            } else {
                FPuts(cos, C);
            }
            
            if (SystemTags(NULL,
                           NP_Name, "Initial CLI",
                           SYS_Asynch, TRUE,
                           SYS_CliType, CLI_BOOT,
                           SYS_Input, cis,
                           SYS_Output, cos,
                           SYS_ScriptInput, cas,
                           TAG_END) == -1) {
                Alert(AT_DeadEnd | AN_BootStrap);
            }
        }
    } else
        Alert(AN_NoWindow);
}
