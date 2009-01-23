/*
    Copyright � 1995-2001, The AROS Development Team. All rights reserved.
    $Id: boot.c 21768 2004-06-17 21:59:56Z hkiel $

    Desc: Boot your operating system.
    Lang: english
*/

#include <exec/types.h>
#include <exec/alerts.h>
#include <exec/libraries.h>
#include <exec/devices.h>
#include <exec/execbase.h>
#include <aros/libcall.h>
#include <aros/asmcall.h>
#include <dos/dosextens.h>
#include <dos/filesystem.h>
#include <libraries/expansionbase.h>
#include <utility/tagitem.h>
#include <aros/debug.h>

#include <proto/exec.h>
#include <proto/dos.h>

void boot(struct ExecBase *SysBase, BOOL hidds_ok)
{
    LONG rc = RETURN_FAIL;
    BPTR cis = NULL;

    /*  We have been created as a process by DOS, we should now
    	try and boot the system. */

    struct DosLibrary *DOSBase;

    DOSBase = (struct DosLibrary *)OpenLibrary("dos.library", 0);
    if( DOSBase == NULL )
    {
    	/* BootStrap couldn't open dos.library */
    	Alert(AT_DeadEnd | AN_BootStrap | AG_OpenLib | AO_DOSLib );
    }

    if (hidds_ok)
	cis = Open("CON:20/20///Boot Shell/AUTO", FMF_READ);
    else
	kprintf("Failed to load system HIDDs\n");
    if (cis)
    {
        struct ExpansionBase *ExpansionBase;
        BPTR sseq = NULL;
        BOOL opensseq = TRUE;

        struct TagItem tags[] =
            {
                { SYS_Asynch,      TRUE       }, /* 0 */
                { SYS_Background,  FALSE      }, /* 1 */
                { SYS_Input,       (IPTR)cis  }, /* 2 */
                { SYS_Output,      (IPTR)NULL }, /* 3 */
                { SYS_Error,       (IPTR)NULL }, /* 4 */
                { SYS_ScriptInput, (IPTR)NULL }, /* 5 */
                { TAG_DONE,       0           }
            };

        if ((ExpansionBase = (struct ExpansionBase *)OpenLibrary("expansion.library", 0)) != NULL)
        {
            opensseq = !(ExpansionBase->Flags & EBF_DOSFLAG);
            CloseLibrary(ExpansionBase);
        }

        D(bug("[SubMain] Open Startup Sequence = %d\n", opensseq));

        if (opensseq)
        {
            sseq = Open("S:Startup-Sequence", FMF_READ);
            tags[5].ti_Data = (IPTR)sseq;
        }

        rc = SystemTagList("", tags);
        if (rc != -1)
        {
            cis  = NULL;
            sseq = NULL;
        }
        else
            rc = RETURN_FAIL;
        if (sseq != NULL)
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
