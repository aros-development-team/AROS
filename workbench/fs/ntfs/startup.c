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

#ifndef __AROS__
struct ExecBase *SysBase;
#endif

extern void handler(void);

void startup (void) {
#ifndef __AROS__
    SysBase = *(struct ExecBase **)4L;
#endif
    D(bug("[NTFS]: %s()\n", __PRETTY_FUNCTION__));

    D(bug("[NTFS] %s: handler starting..\n", __PRETTY_FUNCTION__));
    handler();
}
