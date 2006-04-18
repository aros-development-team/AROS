/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id: offsets.c 12997 2002-01-13 00:27:32Z bergers $
*/

#include <exec/tasks.h>
#include <exec/types.h>
#include <exec/execbase.h>

ULONG get_offsetof_ThisTask(void)
{
	return (ULONG)&(((struct ExecBase *)0x0)->ThisTask);
}

ULONG get_offsetof_tc_SPLower(void)
{
	return (ULONG)&(((struct Task *)0x0)->tc_SPLower);
}

ULONG get_offsetof_tc_TrapCode(void)
{
	return (ULONG)&(((struct Task *)0x0)->tc_TrapCode);
}
