/*
 * fat.handler - FAT12/16/32 filesystem handler
 *
 * Copyright © 2006 Marek Szyprowski
 * Copyright © 2007 The AROS Development Team
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the same terms as AROS itself.
 *
 * $Id: startup-aros.c,v 1.1 2008/04/25 10:33:43 sonic_amiga Exp $
 */

#define DEBUG 0

#include <aros/asmcall.h>
#include <exec/execbase.h>
#include <aros/debug.h>

struct ExecBase *SysBase;

extern void handler(void);

AROS_UFH1(__used void, startup,
	  AROS_UFHA(struct ExecBase *, sBase, A6))
{
    AROS_USERFUNC_INIT
    SysBase = sBase;
    D(bug("[FAT] handler started, ExecBase = 0x%08lX\n", sBase));
    handler();
    AROS_USERFUNC_EXIT
}
