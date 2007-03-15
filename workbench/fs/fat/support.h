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

#ifndef FAT_HANDLER_SUPPORT_H
#define FAT_HANDLER_SUPPORT_H

void ReturnPacket(struct DosPacket *packet, LONG res1, LONG res2);
struct DosPacket *GetPacket(struct MsgPort *port);
void SendEvent(LONG event);
int ErrorReq (STRPTR text, ULONG args[]);

int ilog2(ULONG data);
#define log2 ilog2

#ifdef __DEBUG__

#define DEBUG 1
#include <aros/debug.h>

#define __DEBUG_ENTRIES__
#define __DEBUG_IO__

void knprints(UBYTE *name, LONG namelen);

#else
#define kprintf( fmt, ... )
#define knprints( path, len )
#endif

#endif

