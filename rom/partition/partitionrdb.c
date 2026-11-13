/*
    Copyright (C) 1995-2026, The AROS Development Team. All rights reserved.

*/

#include <aros/config.h>
#include <proto/dos.h>
#include <proto/exec.h>
#include <proto/partition.h>
#include <proto/utility.h>

#include <devices/hardblocks.h>
#include <exec/memory.h>
#include <exec/types.h>
#include <libraries/partition.h>
#include <resources/filesysres.h>

#include "partitionrdb.h"
#include "partition_types.h"
#include "platform.h"

#ifndef DEBUG
#define DEBUG 0
#endif
#include "debug.h"

#include <string.h>

static ULONG PartitionRDBFileSysPayload(struct FileSysNode *fn, ULONG block)
{
    return (AROS_BE2LONG(PartitionRDBFileSysBlock(fn, block)->lsb_SummedLongs) - 5) * sizeof(ULONG);
}

struct FileSysReader
{
    ULONG count;
    ULONG offset;
    ULONG position;
    struct FileSysNode *fsn;
};

static AROS_UFH4(LONG, ReadFunc,
        AROS_UFHA(BPTR, file,   D1),
        AROS_UFHA(APTR, buffer, D2),
        AROS_UFHA(LONG, length, D3),
        AROS_UFHA(struct Library *, DOSBase, A6))
{
    AROS_USERFUNC_INIT

    struct FileSysReader *fsr = (struct FileSysReader*)file;
    ULONG outsize = 0;
    UBYTE *outbuf = buffer;

    while (length > 0 && fsr->count < fsr->fsn->fsblocks) {
        ULONG size = length;
        ULONG payload = PartitionRDBFileSysPayload(fsr->fsn, fsr->count);

        if (size > payload - fsr->offset)
            size = payload - fsr->offset;
        if (size > 0) {
            UBYTE *inbuf = (UBYTE*)PartitionRDBFileSysBlock(fsr->fsn, fsr->count)->lsb_LoadData + fsr->offset;
            CopyMem(inbuf, outbuf, size);
        }

        outsize += size;
        fsr->position += size;
        fsr->offset += size;
        length -= size;
        outbuf += size;

        if (fsr->offset == payload) {
            fsr->offset = 0;
            fsr->count++;
            if (fsr->count == fsr->fsn->fsblocks)
                break;
        }

    }

    return outsize;

    AROS_USERFUNC_EXIT
}

static AROS_UFH4(LONG, SeekFunc,
        AROS_UFHA(BPTR, file,   D1),
        AROS_UFHA(LONG, pos,    D2),
        AROS_UFHA(LONG, mode,   D3),
        AROS_UFHA(struct Library *, DOSBase, A6))
{
    AROS_USERFUNC_INIT

    struct FileSysReader *fsr = (struct FileSysReader*)file;
    ULONG size = fsr->fsn->filesize;
    LONG oldpos = fsr->position;
    ULONG base, newpos;

    switch (mode) {
    case OFFSET_BEGINNING: base = 0; break;
    case OFFSET_END:       base = size; break;
    case OFFSET_CURRENT:   base = oldpos; break;
    default: return -1;
    }

    /* Filesize is capped at LONG_MAX when loading. Check before adding so
     * signed offsets, including LONG_MIN, need no 64-bit arithmetic. */
    if (pos < 0)
    {
        ULONG back = 0 - (ULONG)pos;
        if (back > base)
            return -1;
        newpos = base - back;
    }
    else
    {
        if ((ULONG)pos > size - base)
            return -1;
        newpos = base + pos;
    }

    fsr->position = newpos;
    fsr->count = 0;
    fsr->offset = newpos;
    while (fsr->count < fsr->fsn->fsblocks)
    {
        ULONG payload = PartitionRDBFileSysPayload(fsr->fsn, fsr->count);

        if (fsr->offset < payload)
            break;
        fsr->offset -= payload;
        fsr->count++;
    }

    return oldpos;

    AROS_USERFUNC_EXIT
}

/* Load a filesystem into DOS seglist */
static BPTR LoadFS(struct FileSysNode *node, struct DosLibrary *DOSBase)
{
    LONG_FUNC FunctionArray[4];
    struct FileSysReader fakefile;
    BPTR seglist;

    SetIoErr(ERROR_NOT_EXECUTABLE);

    if (!node->filesystem || !node->fsblocks || node->filesize < sizeof(ULONG))
        return BNULL;


    FunctionArray[0] = (LONG_FUNC)ReadFunc;
    FunctionArray[1] = (LONG_FUNC)__AROS_GETVECADDR(SysBase,33); /* AllocMem() */
    FunctionArray[2] = (LONG_FUNC)__AROS_GETVECADDR(SysBase,35); /* FreeMem() */
    FunctionArray[3] = (LONG_FUNC)SeekFunc;

    /* Initialize our stream */
    fakefile.count      = 0;
    fakefile.offset     = 0;
    fakefile.position   = 0;
    fakefile.fsn        = node;

#ifndef __mc68000
    /* Skip empty payloads when checking the executable's format word. */
    {
        ULONG first = 0;

        while (first < node->fsblocks && PartitionRDBFileSysPayload(node, first) == 0)
            first++;
        if (first == node->fsblocks ||
            AROS_BE2LONG(PartitionRDBFileSysBlock(node, first)->lsb_LoadData[0]) == 0x000003f3)
            return BNULL;
    }
#endif

    /* InternalLoadSeg((BPTR)&fakefile, BNULL, FunctionArray, NULL);
     * with A6 = NULL. Check internalloadseg.c for more information
     */

    seglist = AROS_CALL4(BPTR, __AROS_GETVECADDR(DOSBase, 126),
        AROS_LCA(BPTR, (BPTR)&fakefile, D0),
        AROS_LCA(BPTR, BNULL, A0),
        AROS_LCA(LONG_FUNC*, FunctionArray, A1),
        AROS_LCA(LONG*, NULL, A2),
        struct Library*, NULL);
    if (seglist)
        SetIoErr(0);
    return seglist;
}

static BOOL PartitionRDBValidChecksum(const void *buffer, ULONG minimum, ULONG bytes)
{
    const ULONG *words = buffer;
    ULONG count = AROS_BE2LONG(words[1]);

    return count >= minimum && count <= bytes / sizeof(ULONG) &&
        calcChkSum((ULONG *)buffer, count) == 0;
}

/* Probe and open must select the same first valid header. */
static LONG PartitionRDBFindHeader(struct Library *PartitionBase,
    struct PartitionHandle *root, void *buffer)
{
    struct RigidDiskBlock *rdb = buffer;
    ULONG i;

    for (i = 0; i < RDB_LOCATION_LIMIT; i++)
    {
        if (readBlock(PartitionBase, root, i, buffer) != 0)
            break;
        if (rdb->rdb_ID == AROS_BE2LONG(IDNAME_RIGIDDISK) &&
            PartitionRDBValidChecksum(rdb, 64, root->de.de_SizeBlock << 2))
            return i;
    }
    return -1;
}

LONG PartitionRDBCheckPartitionTable
    (
        struct Library *PartitionBase,
        struct PartitionHandle *root
    )
{
struct PartitionType type;
struct TagItem tags[] = {{PT_TYPE, (IPTR)&type}, {TAG_DONE, 0}};

    if (root->de.de_SizeBlock < 128 ||
        root->de.de_SizeBlock > sizeof(root->buffer) / 4)
        return 0;

    if (root->root != NULL)
    {
        GetPartitionAttrs(root, tags);
        if (root->root->table->type == PHPTT_GPT)
        {
            UQUAD end = 0;
            GetPartitionAttrsTags(root, PT_ENDBLOCK, (IPTR)&end, TAG_DONE);
            if (end >= 0xffffffffULL)
                return 0;
            if (type.id_len != sizeof(GPT_Type_AROS_RDB) ||
                memcmp(type.id, &GPT_Type_AROS_RDB, sizeof(GPT_Type_AROS_RDB)))
                return 0;
        }
        else if ((root->root->table->type != PHPTT_MBR &&
                  root->root->table->type != PHPTT_EBR) ||
                 (type.id[0] != 0x30 && type.id[0] != 0x76))
            return 0;
    }
    return PartitionRDBFindHeader(PartitionBase, root, root->buffer) >= 0;
}

#if AROS_BIG_ENDIAN && (__WORDSIZE == 32)
/* Same layout and byte order: a plain copy */
#define CopyBE2HostDosEnvec(src, dst, size) CopyMem((src), (dst), (size) * sizeof(ULONG))
#else
static void CopyBE2HostDosEnvec(LONG *src, SIPTR *dst, ULONG size) {
ULONG count=0;

    while (count != size)
    {
        *dst++ = AROS_BE2LONG(*src);
        src++;
        count++;
    }
}
#endif

struct BadBlockNode *PartitionRDBNewBadBlock
    (
        struct Library *PartitionBase,
        struct PartitionHandle *root,
        struct BadBlockBlock *buffer
    )
{
struct BadBlockNode *bn;

    if (
            (AROS_BE2LONG(buffer->bbb_ID) == IDNAME_BADBLOCK) &&
            (PartitionRDBValidChecksum(buffer, 6, root->de.de_SizeBlock << 2))
        )
    {
        bn = AllocMem(sizeof(struct BadBlockNode) + (root->de.de_SizeBlock << 2) - sizeof(struct BadBlockBlock), MEMF_PUBLIC | MEMF_CLEAR);
        if (bn)
        {
            CopyMem(buffer, &bn->bbb, root->de.de_SizeBlock << 2);
            return bn;
        }
    }
    return NULL;
}

struct PartitionHandle *PartitionRDBNewHandle
    (
        struct Library *PartitionBase,
        struct PartitionHandle *root,
        struct PartitionBlock *buffer
    )
{
struct PartitionBlock *pblock;
struct PartitionHandle *ph;

    if (
            (AROS_BE2LONG(buffer->pb_ID) == IDNAME_PARTITION) &&
            (PartitionRDBValidChecksum(buffer, 64, root->de.de_SizeBlock << 2))
        )
    {
        ph = PartitionRDBAllocHandle(root);
        if (ph)
        {
                    pblock = ph->data;
                    CopyMem(buffer, pblock, root->de.de_SizeBlock << 2);
                    ULONG nameLength = pblock->pb_DriveName[0];
                    if (nameLength >= sizeof(pblock->pb_DriveName))
                        nameLength = sizeof(pblock->pb_DriveName) - 1;
                    CopyMem(pblock->pb_DriveName+1, ph->ln.ln_Name, nameLength);
                    ph->ln.ln_Name[nameLength]=0;
                    /* Keep the full disk block, but expose only fields AROS knows. */
                    ULONG tableSize = AROS_BE2LONG(pblock->pb_Environment[DE_TABLESIZE]);
                    if (tableSize >= sizeof(ph->de) / sizeof(IPTR))
                        tableSize = sizeof(ph->de) / sizeof(IPTR) - 1;
                    CopyBE2HostDosEnvec(pblock->pb_Environment, (SIPTR *)&ph->de, tableSize + 1);
                    ph->de.de_TableSize = tableSize;
                    if (ph->de.de_TableSize >= DE_BOOTPRI)
                        ph->de.de_BootPri = (LONG)AROS_BE2LONG(pblock->pb_Environment[DE_BOOTPRI]);
                    PartitionRDBUpdateGeometry(ph);
                    return ph;
        }
    }
    return NULL;
}

struct FileSysNode *PartitionRDBNewFileSys
    (
        struct Library *PartitionBase,
        struct PartitionHandle *root,
        struct FileSysHeaderBlock *buffer
    )
{
struct FileSysNode *fn;

    if (
            (AROS_BE2LONG(buffer->fhb_ID) == IDNAME_FILESYSHEADER) &&
            (PartitionRDBValidChecksum(buffer, 20, root->de.de_SizeBlock << 2))
        )
    {
        fn = AllocMem(sizeof(struct FileSysNode) + (root->de.de_SizeBlock << 2) - sizeof(struct FileSysHeaderBlock), MEMF_PUBLIC | MEMF_CLEAR);
        if (fn)
        {
            fn->blockbytes = root->de.de_SizeBlock << 2;
            CopyMem(buffer, &fn->fhb, fn->blockbytes);

            /* Fill in common part of the handle */
            CopyMem(fn->fhb.fhb_FileSysName, fn->name, sizeof(fn->fhb.fhb_FileSysName));
            fn->name[sizeof(fn->fhb.fhb_FileSysName)] = 0;
            fn->h.ln.ln_Name = fn->name;
            fn->h.ln.ln_Pri  = 0;
            fn->h.handler    = &FilesystemRDB;

            return fn;
        }
    }
    return NULL;
}

static void PartitionRDBFreeFileSystem(struct FileSysHandle *fsh)
{
    struct FileSysNode *fn = (struct FileSysNode *)fsh;

    FreeVec(fn->filesystem);
    FreeMem(fn, sizeof(*fn) + fn->blockbytes - sizeof(fn->fhb));
}

/* Track one chain in constant space, without rereading blocks from the device. */
struct RDBChain
{
    ULONG anchor;
    ULONG power;
    ULONG length;
    UQUAD blocks;
};

static BOOL PartitionRDBChainBlock(struct PartitionHandle *root, struct RDBChain *chain, ULONG block)
{
    if (!chain->power)
        chain->blocks = PartitionMulQ(root->de.de_HighCyl - root->de.de_LowCyl + 1,
            root->de.de_Surfaces, root->de.de_BlocksPerTrack);
    if (block >= chain->blocks)
        return FALSE;
    if (!chain->power)
    {
        chain->anchor = block;
        chain->power = 1;
    }
    else
    {
        if (block == chain->anchor)
            return FALSE;
        if (++chain->length == chain->power)
        {
            chain->anchor = block;
            /* A chain has at most ULONG_MAX non-terminal block addresses. */
            chain->power = chain->power <= 0x7fffffff ? chain->power * 2 : (ULONG)-1;
            chain->length = 0;
        }
    }
    return TRUE;
}

/* LSEG (or BOOT) block of fn's chain with a valid checksum */
static BOOL PartitionRDBValidLSEG(struct FileSysNode *fn, struct LoadSegBlock *lsb, ULONG bytes)
{
    return AROS_BE2LONG(lsb->lsb_ID) == (fn->segmentid ? fn->segmentid : IDNAME_LOADSEG) &&
        PartitionRDBValidChecksum(lsb, 5, bytes);
}

ULONG PartitionRDBCalcFSSize
    (
        struct Library *PartitionBase,
        struct PartitionHandle *root,
        struct FileSysNode *fn,
        struct LoadSegBlock *buffer
    )
{
ULONG size;
ULONG block;
struct RDBChain chain = {0};

    size = 0;
    block = AROS_BE2LONG(fn->fhb.fhb_SegListBlocks);
    while (block != (ULONG)-1)
    {
        if (!PartitionRDBChainBlock(root, &chain, block))
            return 0;
        size++;
        if (readBlock(PartitionBase, root, block, buffer) !=0)
            return 0;
        if (!PartitionRDBValidLSEG(fn, buffer, root->de.de_SizeBlock << 2))
            return 0;
        block = AROS_BE2LONG(buffer->lsb_Next);
    }
    return size;
}

void PartitionRDBReadFileSys
    (
        struct Library *PartitionBase,
        struct PartitionHandle *root,
        struct FileSysNode *fn,
        struct LoadSegBlock *buffer
    )
{
ULONG size;
ULONG block;

    size = PartitionRDBCalcFSSize(PartitionBase, root, fn, buffer);
    fn->blockbytes = root->de.de_SizeBlock << 2;
    if (!size || size > (ULONG)-1 / fn->blockbytes)
        return;

    fn->filesystem = AllocVec(size * fn->blockbytes, MEMF_PUBLIC);
    if (fn->filesystem)
    {
        ULONG i;
        ULONG bytes = 0;
        struct RDBChain chain = {0};

        block = AROS_BE2LONG(fn->fhb.fhb_SegListBlocks);
        for (i = 0; i < size && block != (ULONG)-1; i++)
        {
            struct LoadSegBlock *lsb = PartitionRDBFileSysBlock(fn, i);
            ULONG payload;

            if (!PartitionRDBChainBlock(root, &chain, block) ||
                readBlock(PartitionBase, root, block, lsb) != 0 ||
                !PartitionRDBValidLSEG(fn, lsb, fn->blockbytes))
                break;
            payload = (AROS_BE2LONG(lsb->lsb_SummedLongs) - 5) * sizeof(ULONG);
            if (payload > 0x7fffffff - bytes)
                break;
            bytes += payload;
            block = AROS_BE2LONG(lsb->lsb_Next);
        }
        if (i == size && block == (ULONG)-1)
        {
            fn->fsblocks = size;
            fn->filesize = bytes;
            return;
        }
        FreeVec(fn->filesystem);
        fn->filesystem = NULL;
    }
}

/* Drive initialisation and OS4 boot code use the same chained block layout,
 * but are not filesystems to register with DOS. Preserve them on a rewrite. */
static struct FileSysNode *PartitionRDBReadCode(struct Library *PartitionBase,
    struct PartitionHandle *root, ULONG block, ULONG id, void *buffer)
{
    struct RDBData *data = root->table->data;
    struct FileSysNode *fn;
    ULONG bytes = root->de.de_SizeBlock << 2;

    if (block == (ULONG)-1)
        return NULL;
    /* Older tools leave these optional pointers zero. Block zero can also
     * genuinely contain boot code, so inspect its ID before ignoring it. */
    if (block == 0)
    {
        LONG error = readBlock(PartitionBase, root, 0, buffer);
        if (error)
        {
            data->readerror = ERROR_NOT_A_DOS_DISK;
            return NULL;
        }
        if (AROS_BE2LONG(((struct LoadSegBlock *)buffer)->lsb_ID) != id)
            return NULL;
    }
    fn = AllocMem(sizeof(*fn) + bytes - sizeof(fn->fhb), MEMF_PUBLIC | MEMF_CLEAR);
    if (!fn)
    {
        data->readerror = ERROR_NO_FREE_STORE;
        return NULL;
    }
    fn->blockbytes = bytes;
    fn->segmentid = id;
    fn->fhb.fhb_SegListBlocks = AROS_LONG2BE(block);
    PartitionRDBReadFileSys(PartitionBase, root, fn, buffer);
    return fn;
}

LONG PartitionRDBOpenPartitionTable
    (
        struct Library *PartitionBase,
        struct PartitionHandle *root
    )
{
UBYTE *buffer;
struct RDBData *data;
LONG i;

    if (root->de.de_SizeBlock < 128 || root->de.de_SizeBlock > sizeof(root->buffer) / 4)
        return ERROR_BAD_NUMBER;

    buffer = AllocVec(root->de.de_SizeBlock << 2, MEMF_PUBLIC);
    if (!buffer)
        return 1;
    i = PartitionRDBFindHeader(PartitionBase, root, buffer);
    if (i < 0)
    {
        FreeVec(buffer);
        return 1;
    }
    data = AllocMem(sizeof(struct RDBData) + (root->de.de_SizeBlock << 2) - sizeof(struct RigidDiskBlock), MEMF_PUBLIC);
    if (data)
    {
        CopyMem(buffer, &data->rdb, root->de.de_SizeBlock << 2);
        {
            ULONG block;
            int kind;

            data->rdbblock = i;
            data->newtable = FALSE;
            data->readerror = 0;
            data->driveinit = NULL;
            data->bootcode = NULL;
            NEWLIST(&root->table->list);
            NEWLIST(&data->badblocklist);
            NEWLIST(&data->fsheaderlist);
            root->table->data = data;

            /* take the values of the rdb instead of TD_GEOMETRY */
            root->dg.dg_SectorSize   = AROS_BE2LONG(data->rdb.rdb_BlockBytes);
            root->dg.dg_Cylinders    = AROS_BE2LONG(data->rdb.rdb_Cylinders);
            root->dg.dg_TrackSectors = AROS_BE2LONG(data->rdb.rdb_Sectors);
            root->dg.dg_Heads        = AROS_BE2LONG(data->rdb.rdb_Heads);
            /*
             * Before v3.1 partition.library left rdb_CylBlocks uninitialized, so don't rely on it here.
             * Otherwise you'll get problem reading drives partitioned with earlier partition.library.
             */
            root->dg.dg_CylSectors   = root->dg.dg_TrackSectors * root->dg.dg_Heads;
            root->dg.dg_TotalSectors = root->dg.dg_CylSectors * root->dg.dg_Cylinders;

            /* read bad block, partition and filesystem header chains */
            for (kind = 0; kind < 3; kind++)
            {
                struct RDBChain chain = {0};

                block = AROS_BE2LONG((&data->rdb.rdb_BadBlockList)[kind]);
                while (block != (ULONG)-1)
                {
                    struct Node *n = NULL;
                    ULONG next;

                    if (PartitionRDBChainBlock(root, &chain, block) &&
                        readBlock(PartitionBase, root, block, buffer) == 0)
                    {
                        /* every RDB record keeps its Next link in longword 4 */
                        next = AROS_BE2LONG(((ULONG *)buffer)[4]);
                        if (kind == 0)
                            n = (struct Node *)PartitionRDBNewBadBlock(PartitionBase, root, (struct BadBlockBlock *)buffer);
                        else if (kind == 1)
                            n = (struct Node *)PartitionRDBNewHandle(PartitionBase, root, (struct PartitionBlock *)buffer);
                        else
                            n = (struct Node *)PartitionRDBNewFileSys(PartitionBase, root, (struct FileSysHeaderBlock *)buffer);
                        if (n)
                        {
                            AddTail(kind == 0 ? &data->badblocklist : kind == 1 ? &root->table->list : &data->fsheaderlist, n);
                            if (kind == 2)
                                PartitionRDBReadFileSys(PartitionBase, root, (struct FileSysNode *)n, (struct LoadSegBlock *)buffer);
                        }
                    }
                    if (!n)
                    {
                        data->readerror = ERROR_NOT_A_DOS_DISK;
                        break;
                    }
                    block = next;
                }
            }
            data->driveinit = PartitionRDBReadCode(PartitionBase, root,
                AROS_BE2LONG(data->rdb.rdb_DriveInit), IDNAME_LOADSEG, buffer);
            /* rdb_Reserved1[0] is rdb_BootBlockList on OS4/Parthenope disks. */
            data->bootcode = PartitionRDBReadCode(PartitionBase, root,
                AROS_BE2LONG(data->rdb.rdb_Reserved1[0]), IDNAME_BOOT, buffer);
            FreeVec(buffer);
            return 0;
        }
    }
    FreeVec(buffer);
    return 1;
}

void PartitionRDBClosePartitionTable
    (
        struct Library *PartitionBase,
        struct PartitionHandle *root
    )
{
struct PartitionHandle *ph;
struct BadBlockNode *bn;
struct FileSysNode *fn;
struct RDBData *data;

    while ((ph = (struct PartitionHandle *)RemTail(&root->table->list)))
        PartitionRDBFreeHandle(PartitionBase, ph);
    data = (struct RDBData *)root->table->data;
    while ((bn = (struct BadBlockNode *)RemTail(&data->badblocklist)))
        FreeMem(bn, sizeof(*bn) + (root->de.de_SizeBlock << 2) - sizeof(bn->bbb));

    while ((fn = (struct FileSysNode *)RemTail(&data->fsheaderlist)))
    {
        /* Do not deallocate filesystem handles which are queued for loading */
        if (!fn->h.boot)
            PartitionRDBFreeFileSystem(&fn->h);
    }
    if (data->driveinit)
        PartitionRDBFreeFileSystem(&data->driveinit->h);
    if (data->bootcode)
        PartitionRDBFreeFileSystem(&data->bootcode->h);
    FreeMem(data, sizeof(*data) + (root->de.de_SizeBlock << 2) - sizeof(data->rdb));
}

static LONG PartitionRDBGetPartitionTableAttr(struct Library *PartitionBase,
    struct PartitionHandle *root, const struct TagItem *tag)
{
    struct RDBData *data = root->table->data;

    switch (tag->ti_Tag)
    {
    case PTT_RESERVED:
        *((LONG *)tag->ti_Data) = AROS_BE2LONG(data->rdb.rdb_RDBBlocksHi) + 1; /* Reserved blocks count starts from 0, so ignore rdb_RDBBlocksLo */
        return TRUE;
    }

    return 0;
}

static LONG PartitionRDBGetPartitionAttr(struct Library *PartitionBase,
    struct PartitionHandle *ph, const struct TagItem *tag)
{
    struct PartitionBlock *data = (struct PartitionBlock *)ph->data;

    switch (tag->ti_Tag)
    {
    case PT_TYPE:
        *((ULONG *)tag->ti_Data) = AROS_LONG2BE(ph->de.de_DosType);
        PTYPE(tag->ti_Data)->id_len = 4;
        return TRUE;

    case PT_BOOTABLE:
        *((LONG *)tag->ti_Data) = (AROS_BE2LONG(data->pb_Flags) & PBFF_BOOTABLE) ? TRUE : FALSE;
        return TRUE;

    case PT_AUTOMOUNT:
        *((LONG *)tag->ti_Data) = (AROS_BE2LONG(data->pb_Flags) & PBFF_NOMOUNT) ? FALSE : TRUE;
        return TRUE;
    }

    return 0;
}

const struct PartitionAttribute PartitionRDBPartitionTableAttrs[]=
{
    {PTT_TYPE,     PLAM_READ},
    {PTT_RESERVED, PLAM_READ},
    {TAG_DONE,     0}
};

const struct PartitionAttribute PartitionRDBPartitionAttrs[]=
{
    /* TODO: implement write */
    {PT_GEOMETRY,  PLAM_READ},
    {PT_DOSENVEC,  PLAM_READ | PLAM_WRITE},
    {PT_TYPE,      PLAM_READ | PLAM_WRITE},
    {PT_NAME,      PLAM_READ | PLAM_WRITE},
    {PT_BOOTABLE,  PLAM_READ | PLAM_WRITE},
    {PT_AUTOMOUNT, PLAM_READ | PLAM_WRITE},
    {TAG_DONE,      0}
};

struct Node *PartitionRDBFindFileSystem(struct Library *PartitionBase, struct PartitionHandle *ph, const struct TagItem *tags)
{
    struct RDBData *data = (struct RDBData *)ph->table->data;
    struct FileSysNode *fn;
    struct TagItem *idTag   = FindTagItem(FST_ID  , tags);
    struct TagItem *nameTag = FindTagItem(FST_NAME, tags);

    for (fn = (struct FileSysNode *)data->fsheaderlist.lh_Head; fn->h.ln.ln_Succ;
         fn = (struct FileSysNode *)fn->h.ln.ln_Succ)
    {
        if (idTag)
        {
            if (AROS_BE2LONG(fn->fhb.fhb_DosType) != idTag->ti_Data)
                continue;
        }

        if (nameTag)
        {
            if (strcmp(fn->h.ln.ln_Name, (char *)nameTag->ti_Data))
                continue;
        }

        return &fn->h.ln;
    }

    return NULL;
}

BPTR PartitionRDBLoadFileSystem(struct PartitionBase_intern *PartitionBase, struct FileSysHandle *fn)
{
    if (PartitionBase->pb_DOSBase)
        return LoadFS((struct FileSysNode *)fn, (struct DosLibrary *)PartitionBase->pb_DOSBase);
    else
        return BNULL;
}

static LONG PartitionRDBGetFileSystemAttr(struct Library *PartitionBase, struct FileSysHandle *fn, const struct TagItem *tag)
{
    struct FileSysEntry *fse;
    struct FileSysHeaderBlock *fhb = &((struct FileSysNode *)fn)->fhb;

    switch (tag->ti_Tag)
    {
    case FST_ID:
        *((ULONG *)tag->ti_Data) = AROS_BE2LONG(fhb->fhb_DosType);
        return TRUE;

    case FST_NAME:
        *((STRPTR *)tag->ti_Data) = fn->ln.ln_Name;
        return TRUE;

    case FST_FSENTRY:
        fse = (struct FileSysEntry *)tag->ti_Data;

        /* RDB filesystems are not prioritized */
        fse->fse_Node.ln_Pri = 0;

        /*
         * Don't use CopyMem() or something like that.
         * First, you need to deal with endianess.
         * Second, some things are actually pointers, you need
         * to sign-extend them on 64 bits.
         */
        fse->fse_DosType    = AROS_BE2LONG(fhb->fhb_DosType);
        fse->fse_Version    = AROS_BE2LONG(fhb->fhb_Version);
        fse->fse_PatchFlags = AROS_BE2LONG(fhb->fhb_PatchFlags);
        fse->fse_Type       = AROS_BE2LONG(fhb->fhb_Type);
        fse->fse_Task       = AROS_BE2LONG(fhb->fhb_Task);
        fse->fse_Lock       = (BPTR)(SIPTR)AROS_BE2LONG(fhb->fhb_Lock);
        /* Just for convenience. This is expected to be zero. */
        fse->fse_Handler    = (BPTR)(SIPTR)AROS_BE2LONG(fhb->fhb_Handler);
        fse->fse_StackSize  = AROS_BE2LONG(fhb->fhb_StackSize);
        fse->fse_Priority   = AROS_BE2LONG(fhb->fhb_Priority);
        fse->fse_Startup    = (BPTR)(SIPTR)AROS_BE2LONG(fhb->fhb_Startup);
        /* Skip fse_SegList */
        fse->fse_GlobalVec  = (BPTR)(SIPTR)AROS_BE2LONG(fhb->fhb_GlobalVec);
        return TRUE;

    case FST_VERSION:
        *((ULONG *)tag->ti_Data) = AROS_BE2LONG(fhb->fhb_Version);
        return TRUE;
    }

    return 0;
}

const struct PTFunctionTable PartitionRDB =
{
    PHPTT_RDB,
    "RDB",
    PartitionRDBCheckPartitionTable,
    PartitionRDBOpenPartitionTable,
    PartitionRDBClosePartitionTable,
    PartitionRDBGetPartitionTableAttr,
    PartitionRDBGetPartitionAttr,
    PartitionRDBPartitionTableAttrs,
    PartitionRDBPartitionAttrs,
    PartitionRDBFindFileSystem
};

const struct FSFunctionTable FilesystemRDB =
{
    PartitionRDBLoadFileSystem,
    PartitionRDBGetFileSystemAttr,
    PartitionRDBFreeFileSystem
};

