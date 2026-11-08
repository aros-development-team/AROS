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
#include <dos/exall.h>
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

/*
 * The 64-bit structures must keep their published layout: the MorphOS (tm)
 * V51 offsets on 32-bit systems (64-bit integers 2-byte aligned), natural
 * alignment on 64-bit systems. Fail the build if a compiler moves a field.
 */
#define DOS64_LAYOUT_CHECK(name, cond) typedef char dos64_layout_##name[(cond) ? 1 : -1]
#if (__WORDSIZE == 64)
DOS64_LAYOUT_CHECK(fib_size,   __builtin_offsetof(struct FileInfoBlock, fib_Size) == 128);
DOS64_LAYOUT_CHECK(fib_size64, __builtin_offsetof(struct FileInfoBlock, fib_Size64) == 232);
DOS64_LAYOUT_CHECK(fib_nblk64, __builtin_offsetof(struct FileInfoBlock, fib_NumBlocks64) == 240);
DOS64_LAYOUT_CHECK(fib_actext, __builtin_offsetof(struct FileInfoBlock, fib_ActExtFlags) == 262);
DOS64_LAYOUT_CHECK(fib_total,  sizeof(struct FileInfoBlock) == 264);
DOS64_LAYOUT_CHECK(ead_size64, __builtin_offsetof(struct ExAllData, ed_Size64) == 56);
DOS64_LAYOUT_CHECK(rec_offset, __builtin_offsetof(struct RecordLock64, rec_Offset) == 8);
DOS64_LAYOUT_CHECK(rec_total,  sizeof(struct RecordLock64) == 32);
DOS64_LAYOUT_CHECK(id_nblk64,  __builtin_offsetof(struct InfoData64, id_NumBlocks64) == 48);
#else
DOS64_LAYOUT_CHECK(fib_size,   __builtin_offsetof(struct FileInfoBlock, fib_Size) == 124);
DOS64_LAYOUT_CHECK(fib_size64, __builtin_offsetof(struct FileInfoBlock, fib_Size64) == 228);
DOS64_LAYOUT_CHECK(fib_nblk64, __builtin_offsetof(struct FileInfoBlock, fib_NumBlocks64) == 236);
DOS64_LAYOUT_CHECK(fib_actext, __builtin_offsetof(struct FileInfoBlock, fib_ActExtFlags) == 258);
DOS64_LAYOUT_CHECK(fib_total,  sizeof(struct FileInfoBlock) == 260);
DOS64_LAYOUT_CHECK(ead_size64, __builtin_offsetof(struct ExAllData, ed_Size64) == 40);
DOS64_LAYOUT_CHECK(rec_offset, __builtin_offsetof(struct RecordLock64, rec_Offset) == 4);
DOS64_LAYOUT_CHECK(rec_length, __builtin_offsetof(struct RecordLock64, rec_Length) == 12);
DOS64_LAYOUT_CHECK(rec_total,  sizeof(struct RecordLock64) == 24);
DOS64_LAYOUT_CHECK(id_nblk64,  __builtin_offsetof(struct InfoData64, id_NumBlocks64) == 36);
#endif

/*
 * A MorphOS style packet (results stored through pointers, dp_Res1 a
 * success flag) succeeded. dp_Res2 is cleared before the packet is sent,
 * so a handler that answers an unknown packet with a non-zero dp_Res1
 * still sets an error and is not mistaken for success.
 */
#define dos64_PktOK(ret, err) (((ret) != DOSFALSE) && ((err) == 0))

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
void dos64_FixFIB64(struct FileInfoBlock *fib);
void dos64_PrepFIB64(struct FileInfoBlock *fib);
void dos64_FixSizeFH(struct Dos64Base *DOS64Base, struct FileHandle *fh,
                     struct FileInfoBlock *fib);
void dos64_SetFIBSize(struct FileInfoBlock *fib, UQUAD size);
LONG dos64_Examine32(struct Dos64Base *DOS64Base, BPTR lock,
                     struct FileInfoBlock *fib, LONG action);

#endif /* DOS64_INTERN_H */
