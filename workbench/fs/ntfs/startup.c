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
__startup static AROS_ENTRY(int, startup,
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

    D(bug("[NTFS]: %s()\n", __PRETTY_FUNCTION__));

    D(bug("[NTFS] %s: handler starting..\n", __PRETTY_FUNCTION__));
    handler();

    return RETURN_OK;

    AROS_USERFUNC_EXIT
}
