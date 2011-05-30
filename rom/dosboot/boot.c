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
#include <dos/filesystem.h>
#include <utility/tagitem.h>
#include <proto/exec.h>
#include <proto/dos.h>

#include "dosboot_intern.h"

void __dosboot_Boot(struct DosLibrary *DOSBase, ULONG Flags)
{
    LONG rc = RETURN_FAIL;
    BPTR cis = BNULL;

    /*  We have been created as a process by DOS, we should now
    	try and boot the system. */

    
    D(bug("[DOSBoot] __dosboot_Boot()\n"));

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
            sseq = Open("S:Startup-Sequence", MODE_OLDFILE);

	if (sseq)
	{
            tags[5].ti_Tag  = SYS_ScriptInput;
            tags[5].ti_Data = (IPTR)sseq;

            D(bug("[DOSBoot] __dosboot_Boot: Open Startup Sequence = %d\n", sseq));
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
