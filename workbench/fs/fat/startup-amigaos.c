/*
 * fat.handler - FAT12/16/32 filesystem handler
 *
 * Copyright © 2006 Marek Szyprowski
 * Copyright © 2007 The AROS Development Team
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the same terms as AROS itself.
 *
 * $Id$
 */

#define DEBUG 0

#include <exec/execbase.h>
#include "debug.h"

#ifdef __MORPHOS__
ULONG __abox__ = 1;
#endif

struct ExecBase *SysBase;

extern void handler(void);

void startup (void) {
    SysBase = *(struct ExecBase **)4L;
    D(bug("[FAT] handler started, ExecBase = 0x%08lX\n"));
    handler();
}
