/*
 * Copyright (C) 2011, The AROS Development Team.  All rights reserved.
 * Author: Jason S. McMullan <jason.mcmullan@gmail.com>
 *
 * Licensed under the AROS PUBLIC LICENSE (APL) Version 1.1
 */
#include <proto/exec.h>

char const CDVD_version[] = "\0$VER: CDVDFS 1.4 (16-Jun-2008)";

#ifdef __AROS__
#define AbsExecBase SysBase
#else
#ifndef __startup
#define __startup
#endif
struct Library const *AbsExecBase = (void *)4;
#endif

extern void CDVD_handler(struct ExecBase *sysbase);

__startup LONG SAVEDS Main(void)
{
    return CDVD_handler(AbsExecBase);
}


