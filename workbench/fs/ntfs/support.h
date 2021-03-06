/*
 * ntfs.handler - New Technology FileSystem handler
 *
 * Copyright (C) 2012 The AROS Development Team
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the same terms as AROS itself.
 *
 * $Id $
 */

#ifndef NTFS_HANDLER_SUPPORT_H
#define NTFS_HANDLER_SUPPORT_H

#include <exec/types.h>
#include <dos/dosextens.h>

void SendEvent(LONG event);
void ErrorMessage(CONST_STRPTR fmt, ...);
APTR _AllocVecPooled(APTR mem_pool, ULONG size);
void _FreeVecPooled(APTR mem_pool, APTR vecaddr);

//#define log2 ilog2

#endif

