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
 * A value-returning packet did not give us an answer, so the caller
 * should ask again over the 32-bit path.
 *
 * Handlers reply to a packet they will not service with either -1 or
 * the canonical DOSFALSE, so accept both res1 values - but do not go
 * on to insist on a particular error code. Filesystems report refusing
 * these packets in whatever terms suit them (pfs3 answers them with
 * ERROR_NOT_A_DOS_DISK when it has no valid disk, for instance), and a
 * code we failed to anticipate must not be mistaken for a result.
 *
 * Testing the error rather than the result also keeps the genuine
 * answer zero - an empty file, or position zero - distinguishable from
 * DOSFALSE: those come back with no error and are returned as-is.
 *
 * Falling back is always safe here. These packets only ever ask a
 * question or reposition a file; reads and writes never travel this
 * way, so nothing can be done twice by asking again.
 */
#define dos64_UnsupportedPkt(ret, err) \
    (((ret) == -1 || (ret) == DOSFALSE) && ((err) != 0))

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
