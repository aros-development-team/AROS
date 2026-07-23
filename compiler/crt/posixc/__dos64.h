#ifndef ___DOS64_H
#define ___DOS64_H

/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Internal 64-bit file access helpers for posixc.library. They use
    dos64.library when it is available and transparently fall back to
    the 32-bit dos.library calls when it is not, so posixc keeps
    working on systems without dos64.library. Values the 32-bit
    fallback cannot express fail with IoErr() == ERROR_OBJECT_TOO_LARGE
    (mapped to EOVERFLOW by the callers).
*/

#include <dos/dos.h>
#include <dos/dos64.h>

#include "__fdesc.h"

/* Detect 64-bit filesystem support for an open fcb and cache it in
   the fcb's _FCB_FS64 privflag; the helpers below then pick the
   64-bit or 32-bit path directly, without per-call probing. */
void __dos64_probe(fcb *fcb);

QUAD __dos64_seek(fcb *fcb, QUAD position, LONG mode);
QUAD __dos64_getpos(fcb *fcb);
QUAD __dos64_getsize(fcb *fcb);
QUAD __dos64_setfilesize(fcb *fcb, QUAD size, LONG mode);
QUAD __dos64_read(fcb *fcb, APTR buf, QUAD len);
QUAD __dos64_write(fcb *fcb, CONST_APTR buf, QUAD len);

/* obj is a filehandle for __DOS64_EXAMINE_FH, otherwise a lock */
#define __DOS64_EXAMINE      0
#define __DOS64_EXAMINE_FH   1
#define __DOS64_EXAMINE_NEXT 2
LONG __dos64_examine(BPTR obj, struct FileInfoBlock64 *fib, int what);

LONG __dos64_info(BPTR lock, struct InfoData64 *id);

#endif /* ___DOS64_H */
