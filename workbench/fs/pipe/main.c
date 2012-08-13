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

__startup static AROS_ENTRY(int, pipe_main,
	  AROS_UFHA(char *, argstr, A0),
	  AROS_UFHA(ULONG, argsize, D0),
	  struct ExecBase *, sysBase)
#else
#define AROS_USERFUNC_INIT
#define AROS_USERFUNC_EXIT
#define sysBase (*(struct ExecBase **)4L)
void startup(void)
#endif
{
	AROS_USERFUNC_INIT

        SysBase = sysBase;

	struct DosPacket *dp;
	struct MsgPort *mp;

	mp = &((struct Process *)FindTask(NULL))->pr_MsgPort;

	WaitPort(mp);

	dp = (struct DosPacket *)(GetMsg(mp)->mn_Node.ln_Name);

	handler(dp);

        return RETURN_OK;

	AROS_USERFUNC_EXIT
}
