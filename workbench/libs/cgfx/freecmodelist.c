/*
    Copyright © 1995-2013, The AROS Development Team. All rights reserved.
    $Id$

    Desc:
    Lang: english
*/

#include <cybergraphx/cybergraphics.h>
#include <proto/exec.h>
#include <proto/graphics.h>

#include "cybergraphics_intern.h"

/*****************************************************************************

    NAME */
#include <clib/cybergraphics_protos.h>

	AROS_LH1(void, FreeCModeList,

/*  SYNOPSIS */
	AROS_LHA(struct List *, modeList, A0),

/*  LOCATION */
	struct Library *, CyberGfxBase, 13, Cybergraphics)

/*  FUNCTION
        Frees a list of RTG modes returned by AllocCModeListTagList().

    INPUTS
        modeList - a list of RTG modes returned by AllocCModeListTagList().

    RESULT

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO
        AllocCModeListTagList()

    INTERNALS

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    struct CyberModeNode *node, *safe;

    ForeachNodeSafe(modeList, node, safe) {
	Remove((struct Node *)node);
	FreeMem(node, sizeof (struct CyberModeNode));
    }

    FreeMem(modeList, sizeof (struct List));

    AROS_LIBFUNC_EXIT
} /* FreeCModeList */
