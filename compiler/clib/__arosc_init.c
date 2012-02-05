/*
    Copyright © 2012, The AROS Development Team. All rights reserved.
    $Id$

    Desc: arosc.library - init code
*/
#include <proto/exec.h>

#include <aros/symbolsets.h>

#include "__arosc_privdata.h"

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
