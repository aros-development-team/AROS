/*
 * ntfs.handler - New Technology FileSystem handler
 *
 * Copyright © 2012 The AROS Development Team
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the same terms as AROS itself.
 *
 * $Id $
 */

#include <exec/execbase.h>

#include "debug.h"

#ifdef __MORPHOS__
ULONG __abox__ = 1;
#endif

struct ExecBase *SysBase;
extern void handler(void);


#ifdef __AROS__
__startup static AROS_PROCH(startup, argstr, argsize, sysBase)
#else
#define AROS_PROCFUNC_INIT
#define AROS_PROCFUNC_EXIT
#define sysBase (*(struct ExecBase **)4L)
void startup(void)
#endif
{
    AROS_PROCFUNC_INIT

    SysBase = sysBase;

    D(bug("[NTFS]: %s()\n", __PRETTY_FUNCTION__));

    D(bug("[NTFS] %s: handler starting..\n", __PRETTY_FUNCTION__));
    handler();

    return RETURN_OK;

    AROS_PROCFUNC_EXIT
}
