/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: dos64.library internal support - 64-bit operations with
          32-bit delegation when the filesystem lacks 64-bit support.
*/

#include <aros/debug.h>
#include <exec/memory.h>
#include <proto/exec.h>

#include <string.h>

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
    /* Single-packet 64-bit seek; returns the previous position */
    ret = dos64_SendPkt(DOS64Base, fh->fh_Type, ACTION_SEEK64,
                        fh->fh_Arg1, position, mode, 0, 0, &err);
    if (!(ret == -1 && dos64_UnsupportedAction(err)))
        return ret;

    /* OS4 style pair: query the old position, then reposition */
    old = dos64_SendPkt(DOS64Base, fh->fh_Type, ACTION_GET_FILE_POSITION64,
                        fh->fh_Arg1, 0, 0, 0, 0, &err);
    if (!(old == -1 && dos64_UnsupportedAction(err)))
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
    if (!(old == -1 && dos64_UnsupportedAction(err)))
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
    struct FileInfoBlock32 *fib;
    SIPTR err = 0;
    QUAD  ret, cur;

#if (__WORDSIZE == 64)
    ret = dos64_SendPkt(DOS64Base, fh->fh_Type, ACTION_GET_FILE_SIZE64,
                        fh->fh_Arg1, 0, 0, 0, 0, &err);
    if (!(ret == -1 && dos64_UnsupportedAction(err)))
        return ret;
#else
    ret = dos64_SendPkt64OS4(DOS64Base, fh->fh_Type, ACTION_GET_FILE_SIZE64,
                             fh->fh_Arg1, 0, 0, &err);
    if (!(ret == -1 && dos64_UnsupportedAction(err)))
        return ret;
#endif

    fib = AllocVec(sizeof(struct FileInfoBlock32), MEMF_PUBLIC | MEMF_CLEAR);
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
    /* Single-packet 64-bit set-size; returns the new size */
    ret = dos64_SendPkt(DOS64Base, fh->fh_Type, ACTION_SET_FILE_SIZE64,
                        fh->fh_Arg1, offset, mode, 0, 0, &err);
    if (!(ret == -1 && dos64_UnsupportedAction(err)))
        return ret;

    ret = dos64_SendPkt(DOS64Base, fh->fh_Type, ACTION_CHANGE_FILE_SIZE64,
                        fh->fh_Arg1, offset, mode, 0, 0, &err);
#else
    ret = dos64_SendPkt64OS4(DOS64Base, fh->fh_Type, ACTION_CHANGE_FILE_SIZE64,
                             fh->fh_Arg1, offset, mode, &err);
#endif
    if (!(ret == DOSFALSE && dos64_UnsupportedAction(err)))
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

static void BSTR2CINLINE(char *s)
{
    UBYTE len = s[0];
    memmove(s, s + 1, len);
    s[len] = 0;
}

/* BSTR -> C string conversion, as dos.library does for FileInfoBlocks */
void dos64_FixFIB64(struct FileInfoBlock64 *fib)
{
    BSTR2CINLINE((char *)fib->fib_FileName);
    BSTR2CINLINE((char *)fib->fib_Comment);
}

void dos64_WidenFIB(const struct FileInfoBlock32 *src, struct FileInfoBlock64 *dst)
{
    dst->fib_DiskKey      = src->fib_DiskKey;
    dst->fib_DirEntryType = src->fib_DirEntryType;
    memcpy(dst->fib_FileName, src->fib_FileName, sizeof(dst->fib_FileName));
    dst->fib_Protection   = src->fib_Protection;
    dst->fib_EntryType    = src->fib_EntryType;
    dst->fib_Size         = (UQUAD)(ULONG)src->fib_Size;
    dst->fib_NumBlocks    = (UQUAD)(ULONG)src->fib_NumBlocks;
    dst->fib_Date         = src->fib_Date;
    memcpy(dst->fib_Comment, src->fib_Comment, sizeof(dst->fib_Comment));
    dst->fib_OwnerUID     = src->fib_OwnerUID;
    dst->fib_OwnerGID     = src->fib_OwnerGID;
    memset(dst->fib_Reserved, 0, sizeof(dst->fib_Reserved));
}

void dos64_NarrowFIB(const struct FileInfoBlock64 *src, struct FileInfoBlock32 *dst)
{
    dst->fib_DiskKey      = src->fib_DiskKey;
    dst->fib_DirEntryType = src->fib_DirEntryType;
    memcpy(dst->fib_FileName, src->fib_FileName, sizeof(dst->fib_FileName));
    dst->fib_Protection   = src->fib_Protection;
    dst->fib_EntryType    = src->fib_EntryType;
    dst->fib_Size         = (LONG)src->fib_Size;
    dst->fib_NumBlocks    = (LONG)src->fib_NumBlocks;
    dst->fib_Date         = src->fib_Date;
    memcpy(dst->fib_Comment, src->fib_Comment, sizeof(dst->fib_Comment));
    dst->fib_OwnerUID     = src->fib_OwnerUID;
    dst->fib_OwnerGID     = src->fib_OwnerGID;
}

/*
 * 32-bit delegation for the Examine64 family: performs the equivalent
 * dos.library call on a temporary 32-bit FileInfoBlock and widens the
 * result. For ACTION_EXAMINE_NEXT the (64-bit) FileInfoBlock's
 * enumeration state is propagated into the temporary block first.
 */
LONG dos64_Examine32(struct Dos64Base *DOS64Base, BPTR obj,
                     struct FileInfoBlock64 *fib, LONG action)
{
    struct FileInfoBlock32 *fib32;
    LONG ret = DOSFALSE;

    fib32 = AllocVec(sizeof(struct FileInfoBlock32), MEMF_PUBLIC | MEMF_CLEAR);
    if (fib32 == NULL)
    {
        SetIoErr(ERROR_NO_FREE_STORE);
        return DOSFALSE;
    }

    switch (action)
    {
    case ACTION_EXAMINE_OBJECT:
        ret = Examine(obj, (struct FileInfoBlock *)fib32);
        break;

    case ACTION_EXAMINE_NEXT:
        dos64_NarrowFIB(fib, fib32);
        ret = ExNext(obj, (struct FileInfoBlock *)fib32);
        break;

    case ACTION_EXAMINE_FH:
        ret = ExamineFH(obj, (struct FileInfoBlock *)fib32);
        break;
    }

    if (ret)
        dos64_WidenFIB(fib32, fib);

    FreeVec(fib32);
    return ret;
}
