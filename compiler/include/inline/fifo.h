/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$
*/

#ifndef _INLINE_FIFO_H
#define _INLINE_FIFO_H

#ifndef __INLINE_STUB_H
#include <inline/stubs.h>
#endif
#ifndef __INLINE_MACROS_H
#include <inline/macros.h>
#endif

#define BufSizeFifo(fifo) \
	LP1(0x3c, long, BufSizeFifo, FifoHan, fifo, d0, \
	struct Library *, FifoBase)

#define CloseFifo(fifo, flags) \
	LP2NR(0x24, CloseFifo, FifoHan, fifo, d0, long, flags, d1, \
	struct Library *, FifoBase)

#define OpenFifo(name, bytes, flags) \
	LP3(0x1e, FifoHan, OpenFifo, char *, name, d0, long, bytes, d1, long, flags, a0, \
	struct Library *, FifoBase)

#define ReadFifo(fifo, buf, bytes) \
	LP3(0x2a, long, ReadFifo, FifoHan, fifo, d0, char **, buf, d1, long, bytes, a0, \
	struct Library *, FifoBase)

#define RequestFifo(fifo, msg, req) \
	LP3NR(0x36, RequestFifo, FifoHan, fifo, d0, struct Message *, msg, d1, long, req, a0, \
	struct Library *, FifoBase)

#define WriteFifo(fifo, buf, bytes) \
	LP3(0x30, long, WriteFifo, FifoHan, fifo, d0, char *, buf, d1, long, bytes, a0, \
	struct Library *, FifoBase)

#endif /* _INLINE_FIFO_H */
