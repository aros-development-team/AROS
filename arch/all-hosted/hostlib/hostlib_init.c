/*
    Copyright © 1995-2014, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <aros/debug.h>
#include <aros/kernel.h>
#include <aros/symbolsets.h>
#include <utility/tagitem.h>
#include <proto/arossupport.h>
#include <proto/exec.h>
#include <proto/kernel.h>

#include "hostlib_intern.h"

static int HostLib_Init(struct HostLibBase *HostLibBase)
{
    APTR KernelBase;
    struct TagItem *tag;

    KernelBase = OpenResource("kernel.resource");
    D(bug("[hostlib] KernelBase = 0x%08lX\n", KernelBase));
    if (!KernelBase)
	return FALSE;

    tag = LibFindTagItem(KRN_HostInterface, KrnGetBootInfo());
    if (tag)
    {
	HostLibBase->HostIFace = (struct HostInterface *)tag->ti_Data;
    	D(bug("[hostlib] HostIFace = 0x%08lX\n", HostLibBase->HostIFace));

#ifndef USE_FORBID_LOCK
	InitSemaphore(&HostLibBase->HostSem);
#endif
	return TRUE;
    }

    return FALSE;
}

ADD2INITLIB(HostLib_Init, 0)
