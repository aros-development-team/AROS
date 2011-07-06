/*
 * Copyright (C) 2011, The AROS Development Team.  All rights reserved.
 * Author: Jason S. McMullan <jason.mcmullan@gmail.com>
 *
 * Licensed under the AROS PUBLIC LICENSE (APL) Version 1.1
 */
#include <proto/exec.h>

char const CDVD_version[] = "\0$VER: CDVDFS 1.4 (16-Jun-2008)";

#ifdef __AROS__
#ifndef SAVEDS
#define SAVEDS
#endif
#define AbsExecBase SysBase
#else
struct Library const *AbsExecBase = (void *)4;
#endif

extern void CDVD_handler(struct ExecBase *sysbase);

LONG SAVEDS CDVD_Main(void)
{
    CDVD_handler(AbsExecBase);
    return 0;
}


