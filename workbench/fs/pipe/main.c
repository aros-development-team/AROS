/*
 * Copyright (C) 2011, The AROS Development Team.  All rights reserved.
 * Author: Jason S. McMullan <jason.mcmullan@gmail.com>
 *
 * Licensed under the AROS PUBLIC LICENSE (APL) Version 1.1
 */

#include <proto/exec.h>
#include <proto/dos.h>

/* Trivial startup for AROS */
extern void handler(struct DosPacket *dp);

struct ExecBase *SysBase;

#ifdef __AROS__
#include <aros/asmcall.h>

__startup static AROS_PROCH(pipe_main, argstr, argsize, sysBase)
#else
#define AROS_PROCFUNC_INIT
#define AROS_PROCFUNC_EXIT
#define sysBase (*(struct ExecBase **)4L)
void startup(void)
#endif
{
	AROS_PROCFUNC_INIT

        SysBase = sysBase;

	struct DosPacket *dp;
	struct MsgPort *mp;

	mp = &((struct Process *)FindTask(NULL))->pr_MsgPort;

	WaitPort(mp);

	dp = (struct DosPacket *)(GetMsg(mp)->mn_Node.ln_Name);

	handler(dp);

        return RETURN_OK;

	AROS_PROCFUNC_EXIT
}
