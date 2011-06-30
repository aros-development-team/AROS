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

__startup void _main(void)
{
	struct DosPacket *dp;
	struct MsgPort *mp;

	mp = &((struct Process *)FindTask(NULL))->pr_MsgPort;

	WaitPort(mp);

	dp = (struct DosPacket *)(GetMsg(mp)->mn_Node.ln_Name);

	handler(dp);
}
