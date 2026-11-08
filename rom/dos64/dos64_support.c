/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: dos64.library internal support - 64-bit operations with
          32-bit delegation when the filesystem lacks 64-bit support.
*/

#include <aros/debug.h>
#include <exec/memory.h>
#include <proto/exec.h>

#include "dos64_intern.h"

/*
 * Seek on the underlying filesystem, returning the previous position.
 * Tries the 64-bit packets first and falls back to ACTION_SEEK when
 * the handler does not understand them.
 */
QUAD dos64_Seek(struct Dos64Base *DOS64Base, struct FileHandle *fh,
                QUAD position, LONG mode)
{
    SIPTR err = 0;
    QUAD  ret, old;

#if (__WORDSIZE == 64)
    /*
     * MorphOS style single-packet seek: the new position and the previous
     * position travel by pointer, dp_Res1 is a success flag.
     */
    {
        QUAD pos = position, prev = 0;

        ret = dos64_SendPkt(DOS64Base, fh->fh_Type, ACTION_SEEK64,
                            fh->fh_Arg1, (SIPTR)&pos, mode, (SIPTR)&prev, 0, &err);
        if (dos64_PktOK(ret, err))
            return prev;
    }

    /* OS4 style pair: query the old position, then reposition */
    old = dos64_SendPkt(DOS64Base, fh->fh_Type, ACTION_GET_FILE_POSITION64,
                        fh->fh_Arg1, 0, 0, 0, 0, &err);
    if (!dos64_UnsupportedPkt(old, err))
    {
        if (old == -1)
            return -1;
        ret = dos64_SendPkt(DOS64Base, fh->fh_Type, ACTION_CHANGE_FILE_POSITION64,
                            fh->fh_Arg1, position, mode, 0, 0, &err);
        return ret ? old : -1;
    }
#else
    old = dos64_SendPkt64OS4(DOS64Base, fh->fh_Type, ACTION_GET_FILE_POSITION64,
                             fh->fh_Arg1, 0, 0, &err);
    if (!dos64_UnsupportedPkt(old, err))
    {
        if (old == -1)
            return -1;
        ret = dos64_SendPkt64OS4(DOS64Base, fh->fh_Type, ACTION_CHANGE_FILE_POSITION64,
                                 fh->fh_Arg1, position, mode, &err);
        return ret ? old : -1;
    }
#endif

    /* 32-bit delegation */
    if (position > 0x7FFFFFFFLL || position < -0x80000000LL)
    {
        SetIoErr(ERROR_OBJECT_TOO_LARGE);
        return -1;
    }
    return dos64_SendPkt(DOS64Base, fh->fh_Type, ACTION_SEEK,
                         fh->fh_Arg1, (LONG)position, mode, 0, 0, NULL);
}

/*
 * Seek through a (possibly buffered) filehandle, mirroring the buffer
 * handling of dos.library/Seek().
 */
QUAD dos64_SeekBuffered(struct Dos64Base *DOS64Base, BPTR file,
                        QUAD position, LONG mode)
{
    struct FileHandle *fh = BADDR(file);
    QUAD offset = 0, ret;

    if (fh == NULL)
    {
        SetIoErr(ERROR_INVALID_LOCK);
        return -1;
    }

    /* If the file is in append mode, seeking is not allowed. */
    if (fh->fh_Flags & FHF_APPEND)
        return dos64_Seek(DOS64Base, fh, 0, OFFSET_CURRENT);

    if (fh->fh_Flags & FHF_WRITE)
    {
        /* Write mode: flush pending buffered data first. */
        Flush(file);
    }
    else
    {
        /* Read mode: adjust the offset for unconsumed buffered data. */
        if (fh->fh_Pos < fh->fh_End && mode == OFFSET_CURRENT)
            offset = (QUAD)(fh->fh_Pos - fh->fh_End);
        fh->fh_Pos = fh->fh_End = 0;
    }

    ret = dos64_Seek(DOS64Base, fh, position + offset, mode);
    return (ret == -1) ? -1 : ret + offset;
}

/*
 * Return the size of the file behind fh, using the 64-bit packet when
 * available, then ACTION_EXAMINE_FH, then a seek dance as last resort.
 */
QUAD dos64_GetFileSize(struct Dos64Base *DOS64Base, struct FileHandle *fh)
{
    struct FileInfoBlock *fib;
    SIPTR err = 0;
    QUAD  ret, cur;

#if (__WORDSIZE == 64)
    ret = dos64_SendPkt(DOS64Base, fh->fh_Type, ACTION_GET_FILE_SIZE64,
                        fh->fh_Arg1, 0, 0, 0, 0, &err);
    if (!dos64_UnsupportedPkt(ret, err))
        return ret;
#else
    ret = dos64_SendPkt64OS4(DOS64Base, fh->fh_Type, ACTION_GET_FILE_SIZE64,
                             fh->fh_Arg1, 0, 0, &err);
    if (!dos64_UnsupportedPkt(ret, err))
        return ret;
#endif

    fib = AllocVec(sizeof(struct FileInfoBlock), MEMF_PUBLIC | MEMF_CLEAR);
    if (fib != NULL)
    {
        ret = dos64_SendPkt(DOS64Base, fh->fh_Type, ACTION_EXAMINE_FH,
                            fh->fh_Arg1, (SIPTR)MKBADDR(fib), 0, 0, 0, &err);
        cur = (QUAD)(ULONG)fib->fib_Size;
        FreeVec(fib);
        if (ret)
            return cur;
        if (!dos64_UnsupportedAction(err))
            return -1;
    }

    /*
     * Seek dance: ACTION_SEEK returns the previous position, so seeking
     * to the end yields the old position, and seeking back to it yields
     * the file size.
     */
    cur = dos64_SendPkt(DOS64Base, fh->fh_Type, ACTION_SEEK,
                        fh->fh_Arg1, 0, OFFSET_END, 0, 0, &err);
    if (cur == -1)
        return -1;
    return dos64_SendPkt(DOS64Base, fh->fh_Type, ACTION_SEEK,
                         fh->fh_Arg1, (SIPTR)cur, OFFSET_BEGINNING, 0, 0, NULL);
}

/*
 * Set the size of the file behind fh. Returns the new size (-1 on
 * error); when wantsize is FALSE, 0 is returned on success instead of
 * querying the resulting size.
 */
QUAD dos64_SetFileSize(struct Dos64Base *DOS64Base, struct FileHandle *fh,
                       QUAD offset, LONG mode, BOOL wantsize)
{
    SIPTR err = 0;
    QUAD  ret;

#if (__WORDSIZE == 64)
    /* MorphOS style: offset and resulting size travel by pointer */
    {
        QUAD off = offset, newsize = 0;

        ret = dos64_SendPkt(DOS64Base, fh->fh_Type, ACTION_SET_FILE_SIZE64,
                            fh->fh_Arg1, (SIPTR)&off, mode, (SIPTR)&newsize, 0, &err);
        if (dos64_PktOK(ret, err))
            return wantsize ? newsize : 0;
    }

    ret = dos64_SendPkt(DOS64Base, fh->fh_Type, ACTION_CHANGE_FILE_SIZE64,
                        fh->fh_Arg1, offset, mode, 0, 0, &err);
#else
    ret = dos64_SendPkt64OS4(DOS64Base, fh->fh_Type, ACTION_CHANGE_FILE_SIZE64,
                             fh->fh_Arg1, offset, mode, &err);
#endif
    if (!dos64_UnsupportedPkt(ret, err))
    {
        if (ret == DOSFALSE)
            return -1;
        return wantsize ? dos64_GetFileSize(DOS64Base, fh) : 0;
    }

    /* 32-bit delegation */
    if (offset > 0x7FFFFFFFLL || offset < -0x80000000LL)
    {
        SetIoErr(ERROR_OBJECT_TOO_LARGE);
        return -1;
    }
    return dos64_SendPkt(DOS64Base, fh->fh_Type, ACTION_SET_FILE_SIZE,
                         fh->fh_Arg1, (LONG)offset, mode, 0, 0, NULL);
}

/* Local copy helpers: kickstart modules must not drag in the C library */
static void dos64_bcopy(const void *src, void *dst, ULONG len)
{
    const UBYTE *s = src;
    UBYTE *d = dst;
    while (len--)
        *d++ = *s++;
}

static void BSTR2CINLINE(char *s)
{
    UBYTE len = s[0];
    dos64_bcopy(s + 1, s, len);   /* forward copy is overlap-safe here */
    s[len] = 0;
}

/* BSTR -> C string conversion, as dos.library does for FileInfoBlocks */
void dos64_FixFIB64(struct FileInfoBlock *fib)
{
    BSTR2CINLINE((char *)fib->fib_FileName);
    BSTR2CINLINE((char *)fib->fib_Comment);
}

/*
 * Prepare a FileInfoBlock for an ACTION_EXAMINE_*64 packet: the sender
 * must clear fib_ActExtFlags, and no extensions are requested.
 */
void dos64_PrepFIB64(struct FileInfoBlock *fib)
{
    fib->fib_ActExtFlags = 0;
    fib->fib_ReqExtFlags = 0;
}

/*
 * Store a 64-bit size the way the 64-bit examine packets do: fib_Size64
 * holds it, fib_Size is 0 when it does not fit in 31 bits.
 */
void dos64_SetFIBSize(struct FileInfoBlock *fib, UQUAD size)
{
    fib->fib_Size64 = size;
    fib->fib_Size   = (size > 0x7FFFFFFFULL) ? 0 : (LONG)size;
}

/* Replace a clamped 32-bit size by the 64-bit size of the open file */
void dos64_FixSizeFH(struct Dos64Base *DOS64Base, struct FileHandle *fh,
                     struct FileInfoBlock *fib)
{
    SIPTR err = 0;
#if (__WORDSIZE == 64)
    QUAD size = dos64_SendPkt(DOS64Base, fh->fh_Type, ACTION_GET_FILE_SIZE64,
                              fh->fh_Arg1, 0, 0, 0, 0, &err);
#else
    QUAD size = dos64_SendPkt64OS4(DOS64Base, fh->fh_Type, ACTION_GET_FILE_SIZE64,
                                   fh->fh_Arg1, 0, 0, &err);
#endif

    /* a handler without ACTION_GET_FILE_SIZE64 fails the query: keep the 32-bit size */
    if (size != -1 && err == 0)
        dos64_SetFIBSize(fib, (UQUAD)size);
    SetIoErr(0);
}

/* The size of a file entry that may be clamped; a file that cannot be opened keeps it */
static void dos64_FixSizeEntry(struct Dos64Base *DOS64Base, BPTR obj,
                               struct FileInfoBlock *fib, LONG action)
{
    struct Process *me = (struct Process *)FindTask(NULL);
    BPTR fh;

    if (fib->fib_DirEntryType >= 0 || fib->fib_Size64 < 0x7FFFFFFF ||
        me->pr_Task.tc_Node.ln_Type != NT_PROCESS)
        return;

    if (action == ACTION_EXAMINE_OBJECT)
    {
        BPTR dup = DupLock(obj);

        if (dup == BNULL)
            return;
        fh = OpenFromLock(dup);
        if (fh == BNULL)
        {
            UnLock(dup);
            return;
        }
    }
    else
    {
        BPTR old = CurrentDir(obj);

        fh = Open(fib->fib_FileName, MODE_OLDFILE);
        CurrentDir(old);
        if (fh == BNULL)
            return;
    }

    dos64_FixSizeFH(DOS64Base, BADDR(fh), fib);
    Close(fh);
    SetIoErr(0);
}

/*
 * 32-bit delegation for the Examine64 family. The 64-bit fields share
 * the FileInfoBlock with the standard ones, so the dos.library call
 * fills the caller's block directly; the 64-bit fields are then derived
 * from the 32-bit values (which the handler may have clamped).
 */
LONG dos64_Examine32(struct Dos64Base *DOS64Base, BPTR obj,
                     struct FileInfoBlock *fib, LONG action)
{
    LONG ret = DOSFALSE;

    switch (action)
    {
    case ACTION_EXAMINE_OBJECT:
        ret = Examine(obj, fib);
        break;

    case ACTION_EXAMINE_NEXT:
        ret = ExNext(obj, fib);
        break;

    case ACTION_EXAMINE_FH:
        ret = ExamineFH(obj, fib);
        break;
    }

    if (ret)
    {
        fib->fib_NumBlocks64 = (UQUAD)(ULONG)fib->fib_NumBlocks;
        fib->fib_ActExtFlags = 0;
        dos64_SetFIBSize(fib, (UQUAD)(ULONG)fib->fib_Size);
        if (action != ACTION_EXAMINE_FH)
            dos64_FixSizeEntry(DOS64Base, obj, fib, action);
    }

    return ret;
}
