/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id: aros.c 28764 2008-05-20 19:40:27Z NicJA $

    Desc: This is the "boot code" of AROS when it runs as an emulation.
    Lang: english
*/

#include <aros/debug.h>

#include <dos/dostags.h>
#include <dos/dos.h>
#include <dos/dosextens.h>
#include <dos/filesystem.h>
#include <libraries/expansionbase.h>
#include <proto/dos.h>
#include <proto/intuition.h>
#include <proto/exec.h>

#define CANNOT_LOAD_SHELL	"Unable to load C:shell\n"
#define CANNOT_OPEN_CON		"Cannot open boot console\n"

int submain(struct ExecBase * SysBase, struct DosLibrary * DOSBase)
{
    LONG            rc = RETURN_FAIL;

    BPTR cis  = Open("CON:20/20///Boot Shell/AUTO", FMF_READ);

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
    }
    else
    {
        PutStr(CANNOT_OPEN_CON);
    }

    Close(cis);

    return rc;
}
