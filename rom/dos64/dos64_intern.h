#ifndef DOS64_INTERN_H
#define DOS64_INTERN_H

/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: Internal definitions for dos64.library
*/

#include <aros/debug.h>
#include <exec/execbase.h>
#include <exec/libraries.h>
#include <dos/dos.h>
#include <dos/dosextens.h>
#include <dos/record.h>
#include <dos/dos64.h>

#include <proto/exec.h>
#include <proto/dos.h>

struct Dos64Base
{
    struct Library     d64_Lib;
    struct DosLibrary *d64_DosBase;
};

/*
 * Inside the library functions the base parameter is named DOS64Base;
 * route the dos.library interface through the base we opened at init.
 * (proto/dos.h is included above, so its libbase declaration is not
 * affected by this macro.)
 */
#define DOSBase (DOS64Base->d64_DosBase)

/* Filehandle buffering flags, shared with dos.library */
#include "dos_fhflags.h"

/*
 * Errors that indicate the filesystem does not understand a packet
 * type, meaning we should fall back to the 32-bit operation.
 */
#define dos64_UnsupportedAction(err) \
    ((err) == ERROR_ACTION_NOT_KNOWN || \
     (err) == ERROR_NOT_IMPLEMENTED  || \
     (err) == ERROR_BAD_NUMBER)

/*
 * A value-returning packet failed because the filesystem does not
 * understand the action. Handlers reply to unknown packets with
 * either -1 or the canonical DOSFALSE, so accept both res1 values.
 */
#define dos64_UnsupportedPkt(ret, err) \
    (((ret) == -1 || (ret) == DOSFALSE) && dos64_UnsupportedAction(err))

/* Largest chunk handed to the 32-bit ACTION_READ/ACTION_WRITE packets */
#define DOS64_IOCHUNK 0x40000000

/* dos64_packet.c */
SIPTR dos64_SendPkt(struct Dos64Base *DOS64Base, struct MsgPort *port, LONG action,
                    SIPTR arg1, SIPTR arg2, SIPTR arg3, SIPTR arg4, SIPTR arg5,
                    SIPTR *res2);
#if (__WORDSIZE != 64)
QUAD dos64_SendPkt64OS4(struct Dos64Base *DOS64Base, struct MsgPort *port, LONG action,
                        SIPTR object, QUAD arg64, LONG arg32, SIPTR *res2);
#endif

/* dos64_support.c */
QUAD dos64_Seek(struct Dos64Base *DOS64Base, struct FileHandle *fh,
                QUAD position, LONG mode);
QUAD dos64_SeekBuffered(struct Dos64Base *DOS64Base, BPTR file,
                        QUAD position, LONG mode);
QUAD dos64_SetFileSize(struct Dos64Base *DOS64Base, struct FileHandle *fh,
                       QUAD offset, LONG mode, BOOL wantsize);
QUAD dos64_GetFileSize(struct Dos64Base *DOS64Base, struct FileHandle *fh);
void dos64_FixFIB64(struct FileInfoBlock64 *fib);
void dos64_WidenFIB(const struct FileInfoBlock32 *src, struct FileInfoBlock64 *dst);
void dos64_NarrowFIB(const struct FileInfoBlock64 *src, struct FileInfoBlock32 *dst);
LONG dos64_Examine32(struct Dos64Base *DOS64Base, BPTR lock,
                     struct FileInfoBlock64 *fib, LONG action);

#endif /* DOS64_INTERN_H */
