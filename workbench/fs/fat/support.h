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

#ifndef __EXT2_HANDLER_SUPPORT_H
#define __EXT2_HANDLER_SUPPORT_H

void ReturnPacket(struct DosPacket *packet, LONG res1, LONG res2);
struct DosPacket *GetPacket(struct MsgPort *port);
void SendEvent(LONG event);
int ErrorReq (STRPTR text, ULONG args[]);

#ifdef __AROS__
int ilog2(ULONG data);
#define log2 ilog2
#else
int log2(ULONG data);
#endif

#ifdef __AROS__
#define LE16(x) AROS_LE2WORD(x)
#define LE32(x) AROS_LE2LONG(x)
#else
#define LE16(x) \
({ \
        UWORD __x = (x); \
        ((UWORD)( \
                (((UWORD)(__x) & (UWORD)0x00ffU) << 8) | \
                (((UWORD)(__x) & (UWORD)0xff00U) >> 8) )); \
})

#define LE32(x) \
({ \
        ULONG __x = (x); \
        ((ULONG)( \
                (((ULONG)(__x) & (ULONG)0x000000ffUL) << 24) | \
                (((ULONG)(__x) & (ULONG)0x0000ff00UL) <<  8) | \
                (((ULONG)(__x) & (ULONG)0x00ff0000UL) >>  8) | \
                (((ULONG)(__x) & (ULONG)0xff000000UL) >> 24) )); \
})
#endif

#ifdef __DEBUG__

#ifdef __AROS__
#define DEBUG 1
#include <aros/debug.h>

#define __DEBUG_ENTRIES__
#define __DEBUG_IO__

#else
void KPrintFArgs( UBYTE* fmt, ULONG* args );

#define kprintf( fmt, ... )        \
({                                 \
  ULONG _args[] = { __VA_ARGS__ }; \
  KPrintFArgs( (fmt), _args );     \
})
#endif

void knprints(UBYTE *name, LONG namelen);

#else
#define kprintf( fmt, ... )
#define knprints( path, len )
#endif


#endif

