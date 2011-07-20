/*
    Copyright © 1995-2011, The AROS Development Team. All rights reserved.
    $Id$

    Desc: DOS startup-sequence starter, iOS version
    Lang: english
*/

#include <aros/debug.h>
#include <dos/dos.h>
#include <utility/tagitem.h>
#include <proto/dos.h>

#include "dosboot_intern.h"

void __dosboot_Boot(struct DosLibrary *DOSBase, ULONG Flags)
{
    LONG rc;
    BPTR cis = BNULL;

    /*
     * This is just an enlightened version of the native booter.
     * On iOS we won't run Poseidon and we don't have
     * a console so we don't need emergency shell code.
     */

    D(bug("[DOSBoot] __dosboot_Boot()\n"));

    cis = Open("CON:20/20///Boot Shell/AUTO", MODE_OLDFILE);
    if (cis)
    {
        BPTR sseq = BNULL;

        struct TagItem tags[] =
            {
                { SYS_Asynch,      TRUE       }, /* 0 */
                { SYS_Background,  FALSE      }, /* 1 */
                { SYS_Input,       (IPTR)cis  }, /* 2 */
                { SYS_Output,      0          }, /* 3 */
                { SYS_Error,       0          }, /* 4 */
                { TAG_DONE,        0          }, /* 5 */
                { TAG_DONE,        0          }
            };

        D(bug("[DOSBoot] __dosboot_Boot: Open Startup Sequence = %d\n", opensseq));

        if (!(Flags & BF_NO_STARTUP_SEQUENCE))
        {
            sseq = Open("S:Startup-Sequence", MODE_OLDFILE);
            tags[5].ti_Tag = SYS_ScriptInput;
            tags[5].ti_Data = (IPTR)sseq;
        }

        rc = SystemTagList("", tags);
        if (rc != -1)
        {
            cis  = BNULL;
            sseq = BNULL;
        }
        else
        {
	    D(bug("[DOSBoot] __dosboot_Boot: Cannot run boot shell\n"));
            Alert(AT_DeadEnd|AN_BootError);
        }

        if (sseq != BNULL)
            Close(sseq);
    } else
	kprintf("Cannot open boot console\n");

    /* We get here when the Boot Shell Window is left with EndShell/EndCli.
       There's no RemTask() here, otherwise the process cleanup routines
       are not called. And that would for example mean, that the
       Boot Process (having a CLI) is not removed from the rootnode.
       --> Dead stuff in there -> Crash
    */
}
