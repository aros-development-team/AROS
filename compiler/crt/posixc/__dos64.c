/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Internal 64-bit file access helpers for posixc.library: use
    dos64.library when available, otherwise delegate to the 32-bit
    dos.library calls, widening/narrowing as needed.
*/

#include <string.h>

#include <proto/exec.h>
#include <proto/dos.h>

#define __NOBLIBBASE__

#include "__posixc_intbase.h"
#include "__optionallibs.h"
#include "__dos64.h"

#include <proto/dos64.h>

/* Returns the dos64.library base, or NULL when it is not available */
static struct Library *__dos64_base(void)
{
    struct PosixCIntBase *PosixCBase =
        (struct PosixCIntBase *)__aros_getbase_PosixCBase();

    if (__dos64_available(PosixCBase))
        return PosixCBase->PosixCDOS64Base;

    return NULL;
}

/* The dos64.library base when the fcb's filesystem handles 64-bit
   packets, NULL otherwise (32-bit path) */
static struct Library *__dos64_base_for(fcb *fcb)
{
    if (!(fcb->privflags & _FCB_FS64))
        return NULL;

    return __dos64_base();
}

void __dos64_probe(fcb *fcb)
{
    struct Library *DOS64Base = __dos64_base();

    if (DOS64Base != NULL && fcb->handle != BNULL
        && IsFileSystem64(fcb->handle))
        fcb->privflags |= _FCB_FS64;
}

QUAD __dos64_seek(fcb *fcb, QUAD position, LONG mode)
{
    struct Library *DOS64Base = __dos64_base_for(fcb);
    BPTR fh = fcb->handle;

    if (DOS64Base != NULL)
        return Seek64(fh, mode, position);

    if (position != (QUAD)(LONG)position)
    {
        SetIoErr(ERROR_OBJECT_TOO_LARGE);
        return -1;
    }
    return Seek(fh, (LONG)position, mode);
}

QUAD __dos64_getpos(fcb *fcb)
{
    struct Library *DOS64Base = __dos64_base_for(fcb);
    BPTR fh = fcb->handle;

    if (DOS64Base != NULL)
        return GetFilePosition64(fh);

    return Seek(fh, 0, OFFSET_CURRENT);
}

QUAD __dos64_getsize(fcb *fcb)
{
    struct Library *DOS64Base = __dos64_base_for(fcb);
    BPTR fh = fcb->handle;

    if (DOS64Base != NULL)
        return GetFileSize64(fh);

    /*
     * Seek() returns the previous position: seek to the end to learn
     * nothing yet, then back to where we were - that second call
     * reports the end position, i.e. the size.
     */
    QUAD pos = Seek(fh, 0, OFFSET_END);
    if (pos == -1)
        return -1;
    return Seek(fh, (LONG)pos, OFFSET_BEGINNING);
}

QUAD __dos64_setfilesize(fcb *fcb, QUAD size, LONG mode)
{
    struct Library *DOS64Base = __dos64_base_for(fcb);
    BPTR fh = fcb->handle;

    if (DOS64Base != NULL)
        return SetFileSize64(fh, mode, size);

    if (size != (QUAD)(LONG)size)
    {
        SetIoErr(ERROR_OBJECT_TOO_LARGE);
        return -1;
    }
    return SetFileSize(fh, (LONG)size, mode);
}

QUAD __dos64_read(fcb *fcb, APTR buf, QUAD len)
{
    struct Library *DOS64Base = __dos64_base_for(fcb);
    BPTR fh = fcb->handle;

    if (DOS64Base != NULL)
        return Read64(fh, buf, len);

    /* Partial transfers are permitted, so simply clamp */
    if (len > 0x7FFFF000LL)
        len = 0x7FFFF000LL;
    return Read(fh, buf, (LONG)len);
}

QUAD __dos64_write(fcb *fcb, CONST_APTR buf, QUAD len)
{
    struct Library *DOS64Base = __dos64_base_for(fcb);
    BPTR fh = fcb->handle;

    if (DOS64Base != NULL)
        return Write64(fh, buf, len);

    if (len > 0x7FFFF000LL)
        len = 0x7FFFF000LL;
    return Write(fh, buf, (LONG)len);
}

static void __dos64_widenfib(const struct FileInfoBlock32 *src,
                             struct FileInfoBlock64 *dst)
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

LONG __dos64_examine(BPTR obj, struct FileInfoBlock64 *fib, int what)
{
    struct Library *DOS64Base = __dos64_base();
    struct FileInfoBlock32 *fib32;
    LONG ret = DOSFALSE;

    if (DOS64Base != NULL)
    {
        switch (what)
        {
        case __DOS64_EXAMINE:
            return Examine64(obj, fib, NULL);
        case __DOS64_EXAMINE_FH:
            return ExamineFH64(obj, fib, NULL);
        case __DOS64_EXAMINE_NEXT:
            return ExNext64(obj, fib, NULL);
        }
        return DOSFALSE;
    }

    fib32 = AllocDosObject(DOS_FIB, NULL);
    if (fib32 == NULL)
        return DOSFALSE;

    switch (what)
    {
    case __DOS64_EXAMINE:
        ret = Examine(obj, (struct FileInfoBlock *)fib32);
        break;
    case __DOS64_EXAMINE_FH:
        ret = ExamineFH(obj, (struct FileInfoBlock *)fib32);
        break;
    case __DOS64_EXAMINE_NEXT:
        /* Propagate the enumeration state into the temporary block */
        fib32->fib_DiskKey = fib->fib_DiskKey;
        memcpy(fib32->fib_FileName, fib->fib_FileName,
               sizeof(fib32->fib_FileName));
        ret = ExNext(obj, (struct FileInfoBlock *)fib32);
        break;
    }

    if (ret)
        __dos64_widenfib(fib32, fib);

    FreeDosObject(DOS_FIB, fib32);
    return ret;
}

LONG __dos64_info(BPTR lock, struct InfoData64 *id)
{
    struct Library *DOS64Base = __dos64_base();
    struct InfoData32 id32;
    LONG ret;

    if (DOS64Base != NULL)
        return Info64(lock, id);

    memset(&id32, 0, sizeof(id32));
    ret = Info(lock, (struct InfoData *)&id32);
    if (ret)
    {
        id->id_NumSoftErrors  = id32.id_NumSoftErrors;
        id->id_UnitNumber     = id32.id_UnitNumber;
        id->id_DiskState      = id32.id_DiskState;
        id->id_NumBlocks      = (UQUAD)(ULONG)id32.id_NumBlocks;
        id->id_NumBlocksUsed  = (UQUAD)(ULONG)id32.id_NumBlocksUsed;
        id->id_BytesPerBlock  = id32.id_BytesPerBlock;
        id->id_DiskType       = id32.id_DiskType;
        id->id_VolumeNode     = id32.id_VolumeNode;
        id->id_InUse          = id32.id_InUse;
    }
    return ret;
}
