/*
    Copyright © 1995-2011, The AROS Development Team. All rights reserved.
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

void __dos_Boot(struct DosLibrary *DOSBase, ULONG Flags)
{
    LONG rc = RETURN_FAIL;
    BPTR cis = BNULL;

    /*  We have been created as a process by DOS, we should now
    	try and boot the system. */

    D(bug("[__dos_Boot] generic boot sequence\n"));

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
    if (!(Flags & BF_NO_DISPLAY_DRIVERS))
    {
        /* Check that it exists first... */
        BPTR seg = LoadSeg("C:LoadMonDrvs");
        if (seg != BNULL) {
            RunCommand(seg, AROS_STACKSIZE, "", 0);
            /* We don't care about the return code */
            UnLoadSeg(seg);
        }
    }

    cis = Open("CON:////Boot Shell/AUTO", MODE_OLDFILE);
    if (cis)
    {
        BPTR sseq = BNULL;
        struct TagItem tags[] =
        {
            { SYS_Asynch,      TRUE      }, /* 0 */
            { SYS_Background,  FALSE     }, /* 1 */
            { SYS_Input,       (IPTR)cis }, /* 2 */
            { SYS_Output,      0	 }, /* 3 */
            { SYS_Error,       0	 }, /* 4 */
            { TAG_DONE,	       0	 }, /* 5 */
            { TAG_DONE,        0         }
        };

        SetConsoleTask(((struct FileHandle*)BADDR(cis))->fh_Type);

        if (!(Flags & BF_NO_STARTUP_SEQUENCE))
        {
            sseq = Open("S:Startup-Sequence", MODE_OLDFILE);
        }

	if (sseq)
	{
            tags[5].ti_Tag  = SYS_ScriptInput;
            tags[5].ti_Data = (IPTR)sseq;

            D(bug("[__dos_Boot] Open Startup Sequence = %d\n", sseq));
        }

        rc = SystemTagList("", tags);
        if (rc == -1)
            Alert(AN_BootError);

	/* Don't need to close sseq here. Shell will take care of it. */
    }
    else
        Alert(AN_NoWindow);

    /* We get here when the Boot Shell Window is left with EndShell/EndCli.
       There's no RemTask() here, otherwise the process cleanup routines
       are not called. And that would for example mean, that the
       Boot Process (having a CLI) is not removed from the rootnode.
       --> Dead stuff in there -> Crash
    */
}
