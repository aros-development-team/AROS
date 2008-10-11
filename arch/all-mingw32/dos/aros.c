/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: This is the "boot code" of AROS when it runs as an emulation.
    Lang: english
*/
#define DEBUG 0

#include <aros/debug.h>
#include <dos/dostags.h>
#include <dos/dos.h>
#include <dos/dosextens.h>
#include <dos/filesystem.h>
#include <libraries/expansionbase.h>
#include <proto/dos.h>
#include <proto/intuition.h>
#include <proto/exec.h>

#define CANNOT_OPEN_CON		"Cannot open boot console\n"

int submain(struct ExecBase * SysBase, struct DosLibrary * DOSBase)
{
    struct TagItem tags[] =
    {
        { SYS_Asynch,      TRUE       }, /* 0 */
        { SYS_Background,  FALSE      }, /* 1 */
        { SYS_ScriptInput, 0          }, /* 2 */
        { SYS_Input,       0          }, /* 3 */
        { SYS_Output,      0          }, /* 4 */
        { SYS_Error,       0          }, /* 5 */
        { TAG_DONE,        0          }
    };
    BPTR sseq = NULL;
    LONG rc = RETURN_FAIL;

    D(bug("[SubMain] Opening boot shell\n"));
    BPTR cis  = Open("CON:20/20///Boot Shell/AUTO", FMF_READ);

    if (cis)
    {
        struct ExpansionBase *ExpansionBase;
        BOOL opensseq = TRUE;

	D(bug("[SubMain] Boot shell opened\n"));
        if ((ExpansionBase = (struct ExpansionBase *)OpenLibrary("expansion.library", 0)) != NULL)
        {
            opensseq = !(ExpansionBase->Flags & EBF_DOSFLAG);
            CloseLibrary(ExpansionBase);
        }

        D(bug("[SubMain] Open Startup Sequence = %d\n", opensseq));

        if (opensseq)
        {
            sseq = Open("S:Startup-Sequence", FMF_READ);
            tags[2].ti_Data = (IPTR)sseq;
        }
        tags[3].ti_Data = (IPTR)cis;
    } else {
        tags[3].ti_Tag = TAG_DONE;
        PutStr("Entering emergency shell\n");
    }
    rc = SystemTagList("", tags);
    if (rc != -1)
    {
        cis  = NULL;
        sseq = NULL;
    }
    else {
        PutStr(CANNOT_OPEN_CON);
        rc = RETURN_FAIL;
    }
    if (sseq)
        Close(sseq);
    if (cis)
        Close(cis);
    return rc;
}
