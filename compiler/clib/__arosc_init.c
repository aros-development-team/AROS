/*
    Copyright © 2012, The AROS Development Team. All rights reserved.
    $Id$

    Desc: arosc.library - init code
*/
#include <proto/exec.h>
#include <sys/arosc.h>

#include <aros/symbolsets.h>

#include "__arosc_privdata.h"

/* We handle AROStdCBase */
const ULONG const __aros_rellib_base_StdCBase = 0;
SETRELLIBOFFSET(StdCBase, struct aroscbase, StdCBase)

/* We handle StdCIOBase */
const ULONG const __aros_rellib_base_StdCIOBase = 0;
SETRELLIBOFFSET(StdCIOBase, struct aroscbase, StdCIOBase)

/* We handle PosixCBase */
const ULONG const __aros_rellib_base_PosixCBase = 0;
SETRELLIBOFFSET(PosixCBase, struct aroscbase, PosixCBase)

static int __arosc_open(struct aroscbase *aroscbase)
{
    aroscbase->acb_internalpool = CreatePool(MEMF_PUBLIC, 256, 256);

    return aroscbase->acb_internalpool != NULL;
}

static void __arosc_close(struct aroscbase *aroscbase)
{
    DeletePool(aroscbase->acb_internalpool);
}

ADD2OPENLIB(__arosc_open, -50);
ADD2CLOSELIB(__arosc_close, -50);
