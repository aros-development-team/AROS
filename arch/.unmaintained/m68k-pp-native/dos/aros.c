/*
    Copyright � 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: This is the "boot code" of AROS when it runs as an emulation.
    Lang: english
*/
#include <dos/dostags.h>
#include <dos/dos.h>
#include <dos/dosextens.h>
#include <dos/filesystem.h>
#include <proto/dos.h>
#include <proto/intuition.h>
#include <proto/exec.h>

#include <asm/registers.h>

#define DEBUG 0
#include <aros/debug.h>

#define CANNOT_LOAD_SHELL	"Unable to load C:shell\n"
#define CANNOT_OPEN_CON		"Cannot open boot console\n"

int main(struct ExecBase * SysBase, struct Library * DOSBase)
{
	LONG            rc = RETURN_FAIL;


	BPTR sseq = Open("S:Startup-Sequence", FMF_READ);
	BPTR cis = Open("CON:20/20///Boot Shell/AUTO", FMF_READ);

	if (cis) {
		struct TagItem tags[] =
		{
			{ SYS_Asynch,      TRUE       },
			{ SYS_Background,  FALSE      },
			{ SYS_Input,       (IPTR)cis  },
			{ SYS_Output,      (IPTR)NULL },
			{ SYS_Error,       (IPTR)NULL },
			{ SYS_ScriptInput, (IPTR)sseq },
			{ TAG_DONE,       0           }
		};

		rc = SystemTagList("", tags);
		if (rc != -1) {
			cis  = NULL;
			sseq = NULL;
		} else
			rc = RETURN_FAIL;
	} else {
		PutStr(CANNOT_OPEN_CON);
	}
	Close(cis);
	Close(sseq);
	
	return rc;
}
