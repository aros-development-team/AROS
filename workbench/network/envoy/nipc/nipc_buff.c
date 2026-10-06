/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: nipc.library - NIPCBuff scatter/gather buffers and the checksum
*/

#include <proto/exec.h>
#include <string.h>

#include <proto/nipc.h>

#include "nipc_intern.h"

struct PrivBuff
{
    struct NIPCBuff     Pub;
    LONG                Locks;
};

struct PrivEntry
{
    struct NIPCBuffEntry Pub;
    BOOL                 OwnsData;
};

UWORD InetChecksum(const UBYTE *data, ULONG len, ULONG sum)
{
    while (len > 1)
    {
        sum += ((ULONG)data[0] << 8) | data[1];
        data += 2;
        len -= 2;
    }
    if (len)
        sum += (ULONG)data[0] << 8;
    while (sum >> 16)
        sum = (sum & 0xFFFF) + (sum >> 16);
    return (UWORD)sum;
}

/*****************************************************************************

    NAME */
        AROS_LH1(struct NIPCBuff *, AllocNIPCBuff,

/*  SYNOPSIS */
        AROS_LHA(ULONG, entries, D0),

/*  LOCATION */
        struct NIPCBase *, NIPCBase, 39, NIPC)

/*  FUNCTION
        Allocate a buffer with the given number of (empty) entries.

******************************************************************************/
{
    AROS_LIBFUNC_INIT

    struct PrivBuff *b;

    if (!(b = AllocVec(sizeof(struct PrivBuff), MEMF_CLEAR | MEMF_PUBLIC)))
        return NULL;
    NEWLIST(&b->Pub.nbuff_Entries);
    while (entries--)
    {
        struct NIPCBuffEntry *e = AllocNIPCBuffEntry();
        if (!e)
        {
            FreeNIPCBuff(&b->Pub);
            return NULL;
        }
        AddTail((struct List *)&b->Pub.nbuff_Entries, (struct Node *)e);
    }
    return &b->Pub;

    AROS_LIBFUNC_EXIT
}

/*****************************************************************************

    NAME */
        AROS_LH0(struct NIPCBuffEntry *, AllocNIPCBuffEntry,

/*  LOCATION */
        struct NIPCBase *, NIPCBase, 40, NIPC)

/*  FUNCTION
        Allocate an empty buffer entry; the caller supplies nbe_Data.

******************************************************************************/
{
    AROS_LIBFUNC_INIT

    return AllocVec(sizeof(struct PrivEntry), MEMF_CLEAR | MEMF_PUBLIC);

    AROS_LIBFUNC_EXIT
}

/*****************************************************************************

    NAME */
        AROS_LH1(struct NIPCBuffEntry *, AllocNIPCBuffEntryWithMem,

/*  SYNOPSIS */
        AROS_LHA(ULONG, size, D0),

/*  LOCATION */
        struct NIPCBase *, NIPCBase, 50, NIPC)

/*  FUNCTION
        Allocate an entry together with size bytes of data memory, freed
        with the entry.

******************************************************************************/
{
    AROS_LIBFUNC_INIT

    struct PrivEntry *e;

    if (!(e = AllocVec(sizeof(struct PrivEntry) + size, MEMF_CLEAR | MEMF_PUBLIC)))
        return NULL;
    e->Pub.nbe_Data = (UBYTE *)(e + 1);
    e->Pub.nbe_PhysicalLength = size;
    e->Pub.nbe_Length = size;
    e->OwnsData = TRUE;
    return &e->Pub;

    AROS_LIBFUNC_EXIT
}

/*****************************************************************************

    NAME */
        AROS_LH1(void, FreeNIPCBuffEntry,

/*  SYNOPSIS */
        AROS_LHA(struct NIPCBuffEntry *, entry, A0),

/*  LOCATION */
        struct NIPCBase *, NIPCBase, 45, NIPC)

/*  FUNCTION
        Free an entry (not its data unless AllocNIPCBuffEntryWithMem()
        provided it). The entry must not be in a buffer.

******************************************************************************/
{
    AROS_LIBFUNC_INIT

    if (entry)
        FreeVec(entry);

    AROS_LIBFUNC_EXIT
}

/*****************************************************************************

    NAME */
        AROS_LH1(void, FreeNIPCBuff,

/*  SYNOPSIS */
        AROS_LHA(struct NIPCBuff *, buff, A0),

/*  LOCATION */
        struct NIPCBase *, NIPCBase, 44, NIPC)

/*  FUNCTION
        Free a buffer and its entries. A buffer locked with LockNIPCBuff()
        survives one FreeNIPCBuff() per lock.

******************************************************************************/
{
    AROS_LIBFUNC_INIT

    struct PrivBuff *b = (struct PrivBuff *)buff;
    struct NIPCBuffEntry *e;

    if (!b)
        return;
    if (b->Locks > 0)
    {
        b->Locks--;
        return;
    }
    while ((e = (struct NIPCBuffEntry *)RemHead((struct List *)&b->Pub.nbuff_Entries)))
        FreeNIPCBuffEntry(e);
    FreeVec(b);

    AROS_LIBFUNC_EXIT
}

/*****************************************************************************

    NAME */
        AROS_LH1(void, LockNIPCBuff,

/*  SYNOPSIS */
        AROS_LHA(struct NIPCBuff *, buff, A0),

/*  LOCATION */
        struct NIPCBase *, NIPCBase, 49, NIPC)

/*  FUNCTION
        Defer the next FreeNIPCBuff() of this buffer.

******************************************************************************/
{
    AROS_LIBFUNC_INIT

    if (buff)
        ((struct PrivBuff *)buff)->Locks++;

    AROS_LIBFUNC_EXIT
}

/*****************************************************************************

    NAME */
        AROS_LH1(ULONG, NIPCBuffLength,

/*  SYNOPSIS */
        AROS_LHA(struct NIPCBuff *, buff, A0),

/*  LOCATION */
        struct NIPCBase *, NIPCBase, 46, NIPC)

/*  FUNCTION
        Total data length of a buffer.

******************************************************************************/
{
    AROS_LIBFUNC_INIT

    struct NIPCBuffEntry *e;
    ULONG len = 0;

    if (!buff)
        return 0;
    ForeachNode(&buff->nbuff_Entries, e)
        len += e->nbe_Length;
    return len;

    AROS_LIBFUNC_EXIT
}

/*****************************************************************************

    NAME */
        AROS_LH2(void, AppendNIPCBuff,

/*  SYNOPSIS */
        AROS_LHA(struct NIPCBuff *, first, A0),
        AROS_LHA(struct NIPCBuff *, second, A1),

/*  LOCATION */
        struct NIPCBase *, NIPCBase, 47, NIPC)

/*  FUNCTION
        Move the entries of second to the end of first; second is left
        empty (and still to be freed).

******************************************************************************/
{
    AROS_LIBFUNC_INIT

    struct NIPCBuffEntry *e;
    ULONG off;

    if (!first || !second)
        return;
    off = NIPCBuffLength(first);
    while ((e = (struct NIPCBuffEntry *)RemHead((struct List *)&second->nbuff_Entries)))
    {
        e->nbe_Offset = off;
        off += e->nbe_Length;
        AddTail((struct List *)&first->nbuff_Entries, (struct Node *)e);
    }

    AROS_LIBFUNC_EXIT
}

/*****************************************************************************

    NAME */
        AROS_LH3(UBYTE *, NIPCBuffPointer,

/*  SYNOPSIS */
        AROS_LHA(struct NIPCBuff *, buff, A0),
        AROS_LHA(struct NIPCBuffEntry **, beptr, A1),
        AROS_LHA(ULONG, offset, D0),

/*  LOCATION */
        struct NIPCBase *, NIPCBase, 48, NIPC)

/*  FUNCTION
        Address of the byte at offset within the buffer; *beptr receives
        the entry that holds it. NULL if the offset is beyond the data.

******************************************************************************/
{
    AROS_LIBFUNC_INIT

    struct NIPCBuffEntry *e;
    ULONG pos = 0;

    if (!buff)
        return NULL;
    ForeachNode(&buff->nbuff_Entries, e)
    {
        if (offset < pos + e->nbe_Length)
        {
            if (beptr)
                *beptr = e;
            return e->nbe_Data + (offset - pos);
        }
        pos += e->nbe_Length;
    }
    if (beptr)
        *beptr = NULL;
    return NULL;

    AROS_LIBFUNC_EXIT
}

/*****************************************************************************

    NAME */
        AROS_LH4(ULONG, CopyFromNIPCBuff,

/*  SYNOPSIS */
        AROS_LHA(struct NIPCBuff *, src_buff, A0),
        AROS_LHA(UBYTE *, dest_data, A1),
        AROS_LHA(ULONG, srcoffset, D0),
        AROS_LHA(ULONG, length, D1),

/*  LOCATION */
        struct NIPCBase *, NIPCBase, 43, NIPC)

/*  FUNCTION
        Copy bytes out of a buffer into memory.

    RESULT
        Bytes copied.

******************************************************************************/
{
    AROS_LIBFUNC_INIT

    struct NIPCBuffEntry *e;
    ULONG pos = 0, done = 0;

    if (!src_buff || !dest_data)
        return 0;
    ForeachNode(&src_buff->nbuff_Entries, e)
    {
        if (done >= length)
            break;
        if (srcoffset + done < pos + e->nbe_Length)
        {
            ULONG start = srcoffset + done - pos;
            ULONG n = e->nbe_Length - start;
            if (n > length - done)
                n = length - done;
            CopyMem(e->nbe_Data + start, dest_data + done, n);
            done += n;
        }
        pos += e->nbe_Length;
    }
    return done;

    AROS_LIBFUNC_EXIT
}

/*****************************************************************************

    NAME */
        AROS_LH4(ULONG, CopyToNIPCBuff,

/*  SYNOPSIS */
        AROS_LHA(UBYTE *, src_data, A0),
        AROS_LHA(struct NIPCBuff *, dest_buff, A1),
        AROS_LHA(ULONG, dstoffset, D0),
        AROS_LHA(ULONG, length, D1),

/*  LOCATION */
        struct NIPCBase *, NIPCBase, 42, NIPC)

/*  FUNCTION
        Copy memory into a buffer's existing entries (within their
        nbe_PhysicalLength; nbe_Length grows as needed).

    RESULT
        Bytes copied.

******************************************************************************/
{
    AROS_LIBFUNC_INIT

    struct NIPCBuffEntry *e;
    ULONG pos = 0, done = 0;

    if (!src_data || !dest_buff)
        return 0;
    ForeachNode(&dest_buff->nbuff_Entries, e)
    {
        if (done >= length)
            break;
        if (dstoffset + done < pos + e->nbe_PhysicalLength)
        {
            ULONG start = dstoffset + done - pos;
            ULONG n = e->nbe_PhysicalLength - start;
            if (n > length - done)
                n = length - done;
            CopyMem(src_data + done, e->nbe_Data + start, n);
            if (start + n > e->nbe_Length)
                e->nbe_Length = start + n;
            done += n;
        }
        pos += e->nbe_PhysicalLength;
    }
    return done;

    AROS_LIBFUNC_EXIT
}

/*****************************************************************************

    NAME */
        AROS_LH5(ULONG, CopyNIPCBuff,

/*  SYNOPSIS */
        AROS_LHA(struct NIPCBuff *, src_buff, A0),
        AROS_LHA(struct NIPCBuff *, dest_buff, A1),
        AROS_LHA(ULONG, srcoffset, D0),
        AROS_LHA(ULONG, dstoffset, D1),
        AROS_LHA(ULONG, length, D2),

/*  LOCATION */
        struct NIPCBase *, NIPCBase, 41, NIPC)

/*  FUNCTION
        Copy between buffers.

    RESULT
        Bytes copied.

******************************************************************************/
{
    AROS_LIBFUNC_INIT

    UBYTE tmp[512];
    ULONG done = 0;

    while (done < length)
    {
        ULONG n = length - done > sizeof(tmp) ? sizeof(tmp) : length - done;
        ULONG got = CopyFromNIPCBuff(src_buff, tmp, srcoffset + done, n);
        if (!got)
            break;
        got = CopyToNIPCBuff(tmp, dest_buff, dstoffset + done, got);
        done += got;
        if (got < n)
            break;
    }
    return done;

    AROS_LIBFUNC_EXIT
}

/*****************************************************************************

    NAME */
        AROS_LH3(UWORD, CalcNIPCChecksum,

/*  SYNOPSIS */
        AROS_LHA(APTR, data, A0),
        AROS_LHA(ULONG, length, D0),
        AROS_LHA(UWORD, startsum, D1),

/*  LOCATION */
        struct NIPCBase *, NIPCBase, 51, NIPC)

/*  FUNCTION
        16-bit one's complement sum of a byte range, continuing from
        startsum (not complemented).

******************************************************************************/
{
    AROS_LIBFUNC_INIT

    return InetChecksum(data, length, startsum);

    AROS_LIBFUNC_EXIT
}
