/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: 
    Lang: english
*/

#include <proto/exec.h>
#include <proto/boopsi.h>
#include <proto/intuition.h>
#include <proto/alib.h>
#include <exec/memory.h>
#include <exec/interrupts.h>
#include <intuition/intuition.h>
#include <intuition/intuitionbase.h>
#include <intuition/gadgetclass.h>
#include <intuition/cghooks.h>
#include <intuition/sghooks.h>
#include "../../intuition/inputhandler.h"
#include "input_intern.h"


#undef DEBUG
#define DEBUG 1
#include <aros/debug.h>


/***************
**  InitIIH   **
***************/

struct Interrupt *InitIIH(struct inputbase *InputDevice)
{
    struct Interrupt *iihandler;
    struct IIHData *iihdata;

    D(bug("InitIIH(InputBase=%p)\n", InputDevice));

    iihandler = AllocMem(sizeof (struct Interrupt), MEMF_PUBLIC|MEMF_CLEAR);
    if (iihandler)
    {
	iihdata = AllocMem(sizeof (struct IIHData), MEMF_ANY|MEMF_CLEAR);
	if (iihdata)
	{
	    iihdata->IntuiReplyPort = CreateMsgPort();
	    if (iihdata->IntuiReplyPort)
	    {
		iihandler->is_Code = (APTR)AROS_ASMSYMNAME(IntuiInputHandler);
		iihandler->is_Data = iihdata;
		iihandler->is_Node.ln_Pri	= 100;
		iihandler->is_Node.ln_Name	= "Intuition InputHandler";

		iihdata->IntuitionBase = IntuitionBase;

		ReturnPtr ("InitIIH", struct Interrupt *, iihandler);
	    }
	    DeleteMsgPort(iihdata->IntuiReplyPort);
	}
	FreeMem(iihandler, sizeof (struct Interrupt));
    }
    ReturnPtr ("InitIIH", struct Interrupt *, NULL);
}

/****************
** CleanupIIH  **
****************/

VOID CleanupIIH(struct Interrupt *iihandler, struct inputbase *InputDevice)
{
    DeleteMsgPort(((struct IIHData *)iihandler->is_Data)->IntuiReplyPort);
    FreeMem(iihandler->is_Data, sizeof (struct IIHData));
    FreeMem(iihandler, sizeof (struct Interrupt));

    return;
}


