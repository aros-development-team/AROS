/*
    Copyright (C) 1995-2026, The AROS Development Team. All rights reserved.

*/

#define RDB_WRITE 1
#define IDNAME_BOOT 0x424f4f54

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

#include "partition_support.h"
#include "platform.h"

#ifndef DEBUG
#define DEBUG 0
#endif
#include "debug.h"

#include <string.h>

struct RDBData {
    UBYTE rdbblock; /* the block rdb was read from */
    LONG readerror; /* partial discovery must never be rewritten */
    BOOL newtable;  /* invalidate an earlier header only on initial save */
    struct List badblocklist;
    struct List fsheaderlist;
    struct FileSysNode *driveinit;
    struct FileSysNode *bootcode;
    struct RigidDiskBlock rdb; /* last: allocation includes the rest of the sector */
};

struct BadBlockNode {
    struct Node ln;
    struct BadBlockBlock bbb;
};

struct FileSysNode
{
    struct FileSysHandle h;

    struct LoadSegBlock *filesystem; /* the FS in LSEG blocks */
    ULONG fsblocks;                  /* nr of LSEG blocks for FS */
    ULONG blockbytes;                /* size of each stored LSEG block */
    ULONG segmentid;                 /* LSEG, or BOOT for the OS4 boot chain */
    ULONG filesize;                  /* sum of the encoded LSEG payload sizes */
    struct FileSysHeaderBlock fhb; /* last: allocation includes the rest of the sector */
};

static struct LoadSegBlock *PartitionRDBFileSysBlock(struct FileSysNode *fn, ULONG block)
{
    return (struct LoadSegBlock *)((UBYTE *)fn->filesystem + (IPTR)block * fn->blockbytes);
}

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
    QUAD newpos = pos;

    switch (mode) {
    case OFFSET_BEGINNING: break;
    case OFFSET_END:       newpos += size; break;
    case OFFSET_CURRENT:   newpos += oldpos; break;
    default: return -1;
    }

    /* The DOS seek callback returns a signed LONG position. */
    if (newpos < 0 || newpos > size || newpos > 0x7fffffff)
        return -1;

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

static ULONG calcChkSum(ULONG *ptr, ULONG size)
{
  ULONG i;
  ULONG sum=0;

  for (i=0;i<size;i++)
  {
    sum += AROS_BE2LONG(*ptr);
    ptr++;
  }
  return sum;
}

static BOOL PartitionRDBValidChecksum(const void *buffer, ULONG minimum, ULONG bytes)
{
    const ULONG *words = buffer;
    ULONG count = AROS_BE2LONG(words[1]);

    return count >= minimum && count <= bytes / sizeof(ULONG) &&
        calcChkSum((ULONG *)buffer, count) == 0;
}

/* Keep geometry consistent for newly created, edited and reopened partitions. */
static void PartitionRDBUpdateGeometry(struct PartitionHandle *ph)
{
    ph->dg.dg_DeviceType = DG_DIRECT_ACCESS;
    ph->dg.dg_SectorSize = ph->de.de_SizeBlock << 2;
    ph->dg.dg_Heads = ph->de.de_Surfaces;
    ph->dg.dg_TrackSectors = ph->de.de_BlocksPerTrack;
    ph->dg.dg_CylSectors = ph->dg.dg_Heads * ph->dg.dg_TrackSectors;
    ph->dg.dg_Cylinders = ph->de.de_HighCyl - ph->de.de_LowCyl + 1;
    ph->dg.dg_TotalSectors = ph->dg.dg_CylSectors * ph->dg.dg_Cylinders;
    ph->dg.dg_BufMemType = ph->de.de_BufMemType;
}

LONG PartitionRDBCheckPartitionTable
    (
        struct Library *PartitionBase,
        struct PartitionHandle *root
    )
{
UBYTE i;
struct RigidDiskBlock *rdb = (struct RigidDiskBlock *)root->buffer;
struct PartitionType type;
struct TagItem tags[] = {{PT_TYPE, (IPTR)&type}, {TAG_DONE, 0}};

    if (root->de.de_SizeBlock < 128 ||
        root->de.de_SizeBlock > sizeof(root->buffer) / 4)
        return 0;

    if (root->root != NULL)
    {
        GetPartitionAttrs(root, tags);
        if (
            (root->root->table->type != PHPTT_MBR &&
             root->root->table->type != PHPTT_EBR) ||
            (type.id[0] != 0x30 && type.id[0] != 0x76)
            )
        {
            return 0;
        }
    }
    for (i=0;i<RDB_LOCATION_LIMIT; i++)
    {
        if (readBlock(PartitionBase, root, i, rdb) != 0)
            return 0;
        if (rdb->rdb_ID == AROS_BE2LONG(IDNAME_RIGIDDISK))
            break;
    }
    if (i != RDB_LOCATION_LIMIT)
    {
        if (PartitionRDBValidChecksum(rdb, 64, root->de.de_SizeBlock << 2))
            return 1;
    }
    return 0;
}

void CopyBE2HostDosEnvec(LONG *src, SIPTR *dst, ULONG size) {
ULONG count=0;

    while (count != size)
    {
        *dst++ = AROS_BE2LONG(*src);
        src++;
        count++;
    }
}

void CopyHost2BEDosEnvec(SIPTR *src, ULONG *dst, ULONG size) {

    ULONG count=0;

    while (count != size)
    {
        *dst++ = AROS_LONG2BE(*src);
        src++;
        count++;
    }

}

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
        ph = AllocMem(sizeof(struct PartitionHandle), MEMF_PUBLIC | MEMF_CLEAR);
        if (ph)
        {
            ph->ln.ln_Name = AllocVec(32, MEMF_PUBLIC | MEMF_CLEAR);
            if (ph->ln.ln_Name)
            {
                pblock = AllocMem(root->de.de_SizeBlock << 2, MEMF_PUBLIC);
                if (pblock)
                {
                    CopyMem(buffer, pblock, root->de.de_SizeBlock << 2);
                    ph->root = root;
                    ph->bd = root->bd;
                    ph->data = pblock;
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
                FreeVec(ph->ln.ln_Name);
            }
            FreeMem(ph, sizeof(struct PartitionHandle));
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
            fn->h.ln.ln_Name = fn->fhb.fhb_FileSysName;
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
    UQUAD power;
    UQUAD length;
};

static BOOL PartitionRDBChainBlock(struct PartitionHandle *root, struct RDBChain *chain, ULONG block)
{
    UQUAD blocks = (UQUAD)(root->de.de_HighCyl - root->de.de_LowCyl + 1) *
        root->de.de_Surfaces * root->de.de_BlocksPerTrack;

    if (block >= blocks)
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
            chain->power *= 2;
            chain->length = 0;
        }
    }
    return TRUE;
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
        if (
                (AROS_BE2LONG(buffer->lsb_ID) != (fn->segmentid ? fn->segmentid : IDNAME_LOADSEG)) ||
                (!PartitionRDBValidChecksum(buffer, 5, root->de.de_SizeBlock << 2))
            )
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
        UQUAD bytes = 0;
        struct RDBChain chain = {0};

        block = AROS_BE2LONG(fn->fhb.fhb_SegListBlocks);
        for (i = 0; i < size && block != (ULONG)-1; i++)
        {
            struct LoadSegBlock *lsb = PartitionRDBFileSysBlock(fn, i);

            if (!PartitionRDBChainBlock(root, &chain, block) ||
                readBlock(PartitionBase, root, block, lsb) != 0 ||
                AROS_BE2LONG(lsb->lsb_ID) != (fn->segmentid ? fn->segmentid : IDNAME_LOADSEG) ||
                AROS_BE2LONG(lsb->lsb_SummedLongs) < 5 ||
                AROS_BE2LONG(lsb->lsb_SummedLongs) > fn->blockbytes / 4 ||
                calcChkSum((ULONG *)lsb, AROS_BE2LONG(lsb->lsb_SummedLongs)) != 0)
                break;
            bytes += (AROS_BE2LONG(lsb->lsb_SummedLongs) - 5) * sizeof(ULONG);
            if (bytes > 0x7fffffff)
                break;
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
UBYTE i;

    if (root->de.de_SizeBlock < 128 || root->de.de_SizeBlock > sizeof(root->buffer) / 4)
        return ERROR_BAD_NUMBER;

    buffer = AllocVec(root->de.de_SizeBlock << 2, MEMF_PUBLIC);
    if (!buffer)
        return 1;
    data = AllocMem(sizeof(struct RDBData) + (root->de.de_SizeBlock << 2) - sizeof(struct RigidDiskBlock), MEMF_PUBLIC);
    if (data)
    {
        for (i=0;i<RDB_LOCATION_LIMIT; i++)
        {
            if (readBlock(PartitionBase, root, i, buffer) != 0) {
                FreeMem(data, sizeof(*data) + (root->de.de_SizeBlock << 2) - sizeof(data->rdb));
                FreeVec(buffer);
                return 1;
            }
            CopyMem(buffer, &data->rdb, root->de.de_SizeBlock << 2);
            if (data->rdb.rdb_ID == AROS_BE2LONG(IDNAME_RIGIDDISK))
            {
                if (!PartitionRDBValidChecksum(&data->rdb, 64, root->de.de_SizeBlock << 2))
                {
                    FreeMem(data, sizeof(*data) + (root->de.de_SizeBlock << 2) - sizeof(data->rdb));
                    FreeVec(buffer);
                    return ERROR_NOT_A_DOS_DISK;
                }
                break;
            }
        }
        if (i != RDB_LOCATION_LIMIT)
        {
            ULONG block;
            struct RDBChain chain = {0};

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

            /* read bad blocks */
            block = AROS_BE2LONG(data->rdb.rdb_BadBlockList);
            while (block != (ULONG)-1)
            {
            struct BadBlockNode *bn;

                if (!PartitionRDBChainBlock(root, &chain, block))
                {
                    data->readerror = ERROR_NOT_A_DOS_DISK;
                    break;
                }
                if (readBlock(PartitionBase, root, block, buffer)==0)
                {
                    bn = PartitionRDBNewBadBlock(PartitionBase, root, (struct BadBlockBlock *)buffer);
                    if (bn != NULL)
                    {
                        AddTail(&data->badblocklist, &bn->ln);
                        block = AROS_BE2LONG(bn->bbb.bbb_Next);
                    }
                    else
                    {
                        data->readerror = ERROR_NOT_A_DOS_DISK;
                        break;
                    }
                }
                else
                {
                    data->readerror = ERROR_NOT_A_DOS_DISK;
                    break;
                }
            }
            SetMem(&chain, 0, sizeof(chain));
            /* read partition blocks */
            block = AROS_BE2LONG(data->rdb.rdb_PartitionList);
            while (block != (ULONG)-1)
            {
            struct PartitionHandle *ph;
                if (!PartitionRDBChainBlock(root, &chain, block))
                {
                    data->readerror = ERROR_NOT_A_DOS_DISK;
                    break;
                }
                if (readBlock(PartitionBase, root, block, buffer)==0)
                {
                    ph = PartitionRDBNewHandle(PartitionBase, root, (struct PartitionBlock *)buffer);
                    if (ph != NULL)
                    {
                        AddTail(&root->table->list, &ph->ln);
                        block = AROS_BE2LONG(((struct PartitionBlock *)ph->data)->pb_Next);
                    }
                    else
                    {
                        data->readerror = ERROR_NOT_A_DOS_DISK;
                        break;
                    }
                }
                else
                {
                    data->readerror = ERROR_NOT_A_DOS_DISK;
                    break;
                }
            }
            SetMem(&chain, 0, sizeof(chain));
            /* read filesystem blocks */
            block = AROS_BE2LONG(data->rdb.rdb_FileSysHeaderList);
            while (block != (ULONG)-1)
            {
            struct FileSysNode *fn;

                if (!PartitionRDBChainBlock(root, &chain, block))
                {
                    data->readerror = ERROR_NOT_A_DOS_DISK;
                    break;
                }
                if (readBlock(PartitionBase, root, block, buffer)==0)
                {
                    fn = PartitionRDBNewFileSys(PartitionBase, root, (struct FileSysHeaderBlock *)buffer);
                    if (fn != NULL)
                    {
                        AddTail(&data->fsheaderlist, &fn->h.ln);
                        PartitionRDBReadFileSys(PartitionBase, root, fn, (struct LoadSegBlock *)buffer);
                        block = AROS_BE2LONG(fn->fhb.fhb_Next);
                    }
                    else
                    {
                        data->readerror = ERROR_NOT_A_DOS_DISK;
                        break;
                    }
                }
                else
                {
                    data->readerror = ERROR_NOT_A_DOS_DISK;
                    break;
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
        FreeMem(data, sizeof(*data) + (root->de.de_SizeBlock << 2) - sizeof(data->rdb));
    }
    FreeVec(buffer);
    return 1;
}

void PartitionRDBFreeHandle
    (
        struct Library *PartitionBase,
        struct PartitionHandle *ph
    )
{
    ClosePartitionTable(ph);
    Remove(&ph->ln);
    FreeMem(ph->data, ph->root->de.de_SizeBlock << 2);
    FreeVec(ph->ln.ln_Name);
    FreeMem(ph, sizeof(struct PartitionHandle));
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

LONG PartitionRDBWriteFileSys
    (
        struct Library *PartitionBase,
        struct PartitionHandle *root,
        struct FileSysNode *fn,
        ULONG *nextblock,
        ULONG first
    )
{
ULONG size;
ULONG block = *nextblock;
LONG error;

    if (fn->filesystem)
    {
        size = first;
        while (size != fn->fsblocks)
        {
            struct LoadSegBlock *lsb = PartitionRDBFileSysBlock(fn, size);

            lsb->lsb_Next = (size+1) != fn->fsblocks ? AROS_LONG2BE(block+1) : (ULONG)-1;
            lsb->lsb_ChkSum = 0;
            lsb->lsb_ChkSum = AROS_LONG2BE(0-calcChkSum((ULONG *)lsb, AROS_BE2LONG(lsb->lsb_SummedLongs)));
#if RDB_WRITE
            error = writeBlock(PartitionBase, root, block++, lsb);
            if (error != 0)
                return error;
#else
            kprintf("RDB-write: block=%ld, type=LSEG\n", block);
            block++;
#endif
            size++;
        }
    }
    *nextblock = block;
    return 0;
}

/* Check the complete rewrite before touching any metadata or volume data. */
static LONG PartitionRDBCheckWriteSize(struct PartitionHandle *root)
{
    struct RDBData *data = root->table->data;
    struct BadBlockNode *bn;
    struct PartitionHandle *ph;
    struct FileSysNode *fn;
    struct FileSysNode *code[2] = {data->driveinit, data->bootcode};
    ULONG i;
    UQUAD end = (UQUAD)data->rdbblock + 1;
    UQUAD blocks = (UQUAD)(root->de.de_HighCyl - root->de.de_LowCyl + 1) *
        root->de.de_Surfaces * root->de.de_BlocksPerTrack;
    UQUAD first = (UQUAD)AROS_BE2LONG(data->rdb.rdb_LoCylinder) *
        AROS_BE2LONG(data->rdb.rdb_Heads) * AROS_BE2LONG(data->rdb.rdb_Sectors);
    UQUAD limit = (UQUAD)AROS_BE2LONG(data->rdb.rdb_RDBBlocksHi) + 1;

    if (data->readerror)
        return data->readerror;
    if (data->rdbblock < AROS_BE2LONG(data->rdb.rdb_RDBBlocksLo))
        return ERROR_NOT_A_DOS_DISK;
    if (limit > blocks)
        limit = blocks;
    if (limit > first)
        limit = first;

    for (bn = (struct BadBlockNode *)data->badblocklist.lh_Head;
         bn->ln.ln_Succ; bn = (struct BadBlockNode *)bn->ln.ln_Succ)
        end++;
    for (ph = (struct PartitionHandle *)root->table->list.lh_Head;
         ph->ln.ln_Succ; ph = (struct PartitionHandle *)ph->ln.ln_Succ)
    {
        UQUAD start = (UQUAD)ph->de.de_LowCyl * ph->de.de_Surfaces * ph->de.de_BlocksPerTrack;
        if (limit > start)
            limit = start;
        end++;
    }
    for (fn = (struct FileSysNode *)data->fsheaderlist.lh_Head;
         fn->h.ln.ln_Succ; fn = (struct FileSysNode *)fn->h.ln.ln_Succ)
    {
        if (!fn->filesystem || !fn->fsblocks || fn->blockbytes != root->de.de_SizeBlock * 4)
            return ERROR_NOT_A_DOS_DISK;
        end += (UQUAD)fn->fsblocks + 1;
    }
    for (i = 0; i < 2; i++)
    {
        if (code[i])
        {
            if (!code[i]->filesystem || !code[i]->fsblocks)
                return ERROR_NOT_A_DOS_DISK;
            end += code[i]->fsblocks;
            if (i == 1 && AROS_BE2LONG(data->rdb.rdb_Reserved1[0]) < data->rdbblock)
                end--; /* its head remains before the RDB header */
        }
    }
    return end > limit ? ERROR_DISK_FULL : 0;
}

static LONG PartitionRDBWritePartitionTable(struct Library *PartitionBase, struct PartitionHandle *root)
{
    struct RDBData *data;
    struct PartitionHandle *ph;
    struct PartitionBlock *pblock;
    struct BadBlockNode *bn;
    struct FileSysNode *fn;
    ULONG block;
    LONG error;

    if (root->de.de_SizeBlock < 128 ||
        root->de.de_SizeBlock > sizeof(root->buffer) / 4)
        return ERROR_BAD_NUMBER;

    error = PartitionRDBCheckWriteSize(root);
    if (error != 0)
        return error;

    data = root->table->data;
    block = data->rdbblock+1; /* RDB will be written at the end */

    SetMem(root->buffer, 0, root->de.de_SizeBlock << 2);

    /* Firmware may scan the early sectors rather than follow BootBlockList.
     * Pack BOOT before other metadata, retaining an earlier head in place. */
    if (data->bootcode)
    {
        ULONG head = AROS_BE2LONG(data->rdb.rdb_Reserved1[0]);

        if (head < data->rdbblock)
        {
            struct LoadSegBlock *lsb = PartitionRDBFileSysBlock(data->bootcode, 0);
            ULONG next = block;

            error = PartitionRDBWriteFileSys(PartitionBase, root, data->bootcode, &block, 1);
            if (error != 0)
                return deviceError(error);
            lsb->lsb_Next = data->bootcode->fsblocks > 1 ? AROS_LONG2BE(next) : (ULONG)-1;
            lsb->lsb_ChkSum = 0;
            lsb->lsb_ChkSum = AROS_LONG2BE(0 - calcChkSum((ULONG *)lsb, AROS_BE2LONG(lsb->lsb_SummedLongs)));
#if RDB_WRITE
            error = writeBlock(PartitionBase, root, head, lsb);
            if (error != 0)
                return deviceError(error);
#endif
        }
        else
        {
            data->rdb.rdb_Reserved1[0] = AROS_LONG2BE(block);
            error = PartitionRDBWriteFileSys(PartitionBase, root, data->bootcode, &block, 0);
            if (error != 0)
                return deviceError(error);
        }
    }

    /* write bad blocks */
    bn = (struct BadBlockNode *)data->badblocklist.lh_Head;
    if (bn->ln.ln_Succ)
        data->rdb.rdb_BadBlockList = AROS_LONG2BE(block);
    else
        data->rdb.rdb_BadBlockList = (ULONG)-1;
    while (bn->ln.ln_Succ)
    {
        bn->bbb.bbb_Next = bn->ln.ln_Succ->ln_Succ ? AROS_LONG2BE(block+1) : (ULONG)-1;
        bn->bbb.bbb_ChkSum = 0;
        bn->bbb.bbb_ChkSum = AROS_LONG2BE(0-calcChkSum((ULONG *)&bn->bbb, AROS_BE2LONG(bn->bbb.bbb_SummedLongs)));
        CopyMem(&bn->bbb, root->buffer, root->de.de_SizeBlock << 2);
#if RDB_WRITE
        error = writeBlock(PartitionBase, root, block++, root->buffer);
        if (error != 0)
            return deviceError(error);
#else
        kprintf("RDB-write: block=%ld, type=BADB\n", block);
        block++;
#endif

        bn = (struct BadBlockNode *)bn->ln.ln_Succ;
    }

    /* write partition blocks */
    ph = (struct PartitionHandle *)root->table->list.lh_Head;
    if (ph->ln.ln_Succ)
        data->rdb.rdb_PartitionList = AROS_LONG2BE(block);
    else
        data->rdb.rdb_PartitionList = (ULONG)-1;
    while (ph->ln.ln_Succ)
    {
        pblock = (struct PartitionBlock *)ph->data;
        pblock->pb_Next = ph->ln.ln_Succ->ln_Succ ? AROS_LONG2BE(block+1) : (ULONG)-1;
        pblock->pb_ChkSum = 0;
        pblock->pb_ChkSum = AROS_LONG2BE(0-calcChkSum((ULONG *)pblock, AROS_BE2LONG(pblock->pb_SummedLongs)));
        CopyMem(pblock, root->buffer, root->de.de_SizeBlock << 2);
#if RDB_WRITE
        error = writeBlock(PartitionBase, root, block++, root->buffer);
        if (error != 0)
            return deviceError(error);
#else
        kprintf("RDB-write: block=%ld, type=PART\n", block);
        block++;
#endif
        ph = (struct PartitionHandle *)ph->ln.ln_Succ;
    }

    /* write filesystem blocks */
    fn = (struct FileSysNode *)data->fsheaderlist.lh_Head;
    if (fn->h.ln.ln_Succ)
        data->rdb.rdb_FileSysHeaderList = AROS_LONG2BE(block);
    else
        data->rdb.rdb_FileSysHeaderList = (ULONG)-1;
    while (fn->h.ln.ln_Succ)
    {
    ULONG fshblock;
    struct FileSysHeaderBlock *fsb;

        fshblock = block;
        block++; /* header block will be written later */
        fn->fhb.fhb_SegListBlocks = AROS_LONG2BE(block);
        /* write filesystem LSEG blocks */
        if (!fn->filesystem || !fn->fsblocks)
            return ERROR_NOT_A_DOS_DISK;
        error = PartitionRDBWriteFileSys(PartitionBase, root, fn, &block, 0);
        if (error != 0)
            return deviceError(error);
        fn->fhb.fhb_Next = fn->h.ln.ln_Succ->ln_Succ ? AROS_LONG2BE(block) : (ULONG)-1;
        fn->fhb.fhb_ChkSum = 0;
        CopyMem(&fn->fhb, root->buffer, root->de.de_SizeBlock << 2);
        fsb = (struct FileSysHeaderBlock *)root->buffer;
        fsb->fhb_ChkSum = AROS_LONG2BE(0-calcChkSum((ULONG *)root->buffer, AROS_BE2LONG(fn->fhb.fhb_SummedLongs)));
#if RDB_WRITE
        error = writeBlock(PartitionBase, root, fshblock, root->buffer);
        if (error != 0)
            return deviceError(error);
#else
        kprintf("RDB-write: block=%ld, type=FSHD\n", fshblock);
#endif
        fn = (struct FileSysNode *)fn->h.ln.ln_Succ;
    }
    if (data->driveinit)
    {
        data->rdb.rdb_DriveInit = AROS_LONG2BE(block);
        error = PartitionRDBWriteFileSys(PartitionBase, root, data->driveinit, &block, 0);
        if (error != 0)
            return deviceError(error);
    }
    data->rdb.rdb_HighRDSKBlock = AROS_LONG2BE(block-1);
    data->rdb.rdb_ChkSum = 0;
    data->rdb.rdb_ChkSum = AROS_LONG2BE(0-calcChkSum((ULONG *)&data->rdb, AROS_BE2LONG(data->rdb.rdb_SummedLongs)));
    CopyMem(&data->rdb, root->buffer, root->de.de_SizeBlock << 2);
#if RDB_WRITE
    error = writeBlock(PartitionBase, root, data->rdbblock, root->buffer);
    if (error != 0)
        return deviceError(error);
#else
    kprintf("RDB-write: block=%ld, type=RDSK\n", data->rdbblock);
#endif
    /* A newly created header at block 1 must not be hidden by an old one at
     * block 0. Preserve boot code/MBRs and do not clear the other probe blocks:
     * they already contain the new table's PART/FSHD/LSEG records. */
    if (data->newtable)
    {
        struct RigidDiskBlock *old = (struct RigidDiskBlock *)root->buffer;

        error = readBlock(PartitionBase, root, 0, root->buffer);
        if (error != 0)
            return deviceError(error);
        if (AROS_BE2LONG(old->rdb_ID) == IDNAME_RIGIDDISK)
        {
            old->rdb_ID = 0;
#if RDB_WRITE
            error = writeBlock(PartitionBase, root, 0, root->buffer);
            if (error != 0)
                return deviceError(error);
#endif
        }
        data->newtable = FALSE;
    }
    return 0;
}

LONG PartitionRDBCreatePartitionTable(struct Library *PartitionBase, struct PartitionHandle *ph)
{
    struct RDBData *data;
    ULONG i;
    ULONG rdbsize;

    if (!ph->dg.dg_CylSectors || ph->dg.dg_CylSectors > 0x7fffffff ||
        (UQUAD)ph->dg.dg_Heads * ph->dg.dg_TrackSectors != ph->dg.dg_CylSectors ||
        !ph->dg.dg_Cylinders || ph->de.de_SizeBlock < 128 ||
        ph->de.de_SizeBlock > sizeof(ph->buffer) / 4 ||
        ph->dg.dg_SectorSize != ph->de.de_SizeBlock * 4)
        return ERROR_BAD_NUMBER;

    /* Reserve two cylinders, increasing in whole-cylinder steps to at least
     * 256 blocks for flat LBA containers (Heads == TrackSectors == 1). */
    rdbsize = ph->dg.dg_CylSectors << 1;
    while (rdbsize < 256)
        rdbsize <<= 1;
    if (rdbsize >= (UQUAD)ph->dg.dg_Cylinders * ph->dg.dg_CylSectors)
        return ERROR_DISK_FULL;

    data = AllocMem(sizeof(struct RDBData) + (ph->de.de_SizeBlock << 2) - sizeof(struct RigidDiskBlock), MEMF_PUBLIC | MEMF_CLEAR);
    if (data)
    {
        ph->table->data = data;

        data->rdb.rdb_ID                = AROS_LONG2BE(IDNAME_RIGIDDISK);
        data->rdb.rdb_SummedLongs       = AROS_LONG2BE(sizeof(struct RigidDiskBlock)/4);
        data->rdb.rdb_BlockBytes        = AROS_LONG2BE(ph->dg.dg_SectorSize);
        data->rdb.rdb_BadBlockList      = (ULONG)-1;
        data->rdb.rdb_PartitionList     = (ULONG)-1;
        data->rdb.rdb_FileSysHeaderList = (ULONG)-1;
        data->rdb.rdb_DriveInit         = (ULONG)-1;
        for (i = 0; i < 6; i++)
            data->rdb.rdb_Reserved1[i]  = (ULONG)-1;
        data->rdb.rdb_Cylinders         = AROS_LONG2BE(ph->dg.dg_Cylinders);
        data->rdb.rdb_Sectors           = AROS_LONG2BE(ph->dg.dg_TrackSectors);
        data->rdb.rdb_Heads             = AROS_LONG2BE(ph->dg.dg_Heads);

        data->rdb.rdb_Park              = data->rdb.rdb_Cylinders;
        data->rdb.rdb_WritePreComp      = data->rdb.rdb_Cylinders;
        data->rdb.rdb_ReducedWrite      = data->rdb.rdb_Cylinders;
        /* StepRate */
        data->rdb.rdb_RDBBlocksLo       = AROS_LONG2BE(1); /* leave a block for PC */
        data->rdb.rdb_RDBBlocksHi       = AROS_LONG2BE(rdbsize - 1);
        data->rdb.rdb_LoCylinder        = AROS_LONG2BE(rdbsize / ph->dg.dg_CylSectors);
        data->rdb.rdb_HiCylinder        = AROS_LONG2BE(ph->dg.dg_Cylinders-1);
        data->rdb.rdb_CylBlocks         = AROS_LONG2BE(ph->dg.dg_CylSectors);
        /* AutoParkSeconds */
        /* DiskVendor */
        /* DiskProduct */
        /* DiskRevision */
        /* ControllerVendor */
        /* ControllerProduct */
        /* ControllerRevision */

        data->rdbblock = 1;
        data->newtable = TRUE;
        NEWLIST(&data->badblocklist);
        NEWLIST(&data->fsheaderlist);
        NEWLIST(&ph->table->list);
        return 0;
    }
    return 1;
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

static LONG PartitionRDBSetPartitionAttrs(struct Library *PartitionBase, struct PartitionHandle *ph, const struct TagItem *taglist)
{
    struct TagItem *tag;
    struct PartitionBlock *data = (struct PartitionBlock *)ph->data;

    /* Validate first so an invalid tag does not leave a partially edited entry. */
    const struct TagItem *check = taglist;

    while ((tag = NextTagItem((struct TagItem **)&check)))
    {
        if (tag->ti_Tag == PT_DOSENVEC)
        {
            struct DosEnvec *de = (struct DosEnvec *)tag->ti_Data;

            if (de->de_TableSize < DE_NUMBUFFERS || de->de_TableSize >= sizeof(*de) / sizeof(IPTR))
                return ERROR_BAD_NUMBER;
        }
        if (tag->ti_Tag == PT_NAME && strlen((STRPTR)tag->ti_Data) >= sizeof(data->pb_DriveName))
            return ERROR_BAD_NUMBER;
    }

    while ((tag = NextTagItem((struct TagItem **)&taglist)))
    {
        switch (tag->ti_Tag)
        {
        case PT_DOSENVEC:
            {
            struct DosEnvec *de = (struct DosEnvec *)tag->ti_Data;

                CopyMem(de, &ph->de, (de->de_TableSize+1)*sizeof(IPTR));
                CopyHost2BEDosEnvec((SIPTR *)de, data->pb_Environment, de->de_TableSize+1);
                PartitionRDBUpdateGeometry(ph);
            }
            break;
        case PT_TYPE:
            {
            struct PartitionType *ptype=(struct PartitionType *)tag->ti_Data;
            ULONG dt;

                CopyMem(ptype->id, &dt, 4);
                ph->de.de_DosType = AROS_BE2LONG(dt);
                data->pb_Environment[DE_DOSTYPE] = dt;
            }
            break;
        case PT_NAME:
            {
            STRPTR name = (STRPTR)tag->ti_Data;
            ULONG len = strlen(name);

                CopyMem(name, ph->ln.ln_Name, len+1);
                CopyMem(name, data->pb_DriveName+1, len);
                data->pb_DriveName[0] = len;
            }
            break;
        case PT_BOOTABLE:
            if (tag->ti_Data)
                data->pb_Flags = AROS_LONG2BE(AROS_BE2LONG(data->pb_Flags) | PBFF_BOOTABLE);
            else
                data->pb_Flags = AROS_LONG2BE(AROS_BE2LONG(data->pb_Flags) & ~PBFF_BOOTABLE);
            break;
        case PT_AUTOMOUNT:
            if (tag->ti_Data)
                data->pb_Flags = AROS_LONG2BE(AROS_BE2LONG(data->pb_Flags) & ~PBFF_NOMOUNT);
            else
                data->pb_Flags = AROS_LONG2BE(AROS_BE2LONG(data->pb_Flags) | PBFF_NOMOUNT);
            break;
        }
    }
    return 0;
}

struct PartitionHandle *PartitionRDBAddPartition(struct Library *PartitionBase, struct PartitionHandle *root, const struct TagItem *taglist)
{
    if (FindTagItem(PT_DOSENVEC, taglist) != NULL)
    {
        struct PartitionBlock *pblock;
        struct PartitionHandle *ph;
        struct PartitionHandle *oph;

        ph = AllocMem(sizeof(struct PartitionHandle), MEMF_PUBLIC | MEMF_CLEAR);
        if (ph)
        {
            ph->ln.ln_Name = AllocVec(32, MEMF_PUBLIC | MEMF_CLEAR);
            if (ph->ln.ln_Name)
            {
                pblock = AllocMem(root->de.de_SizeBlock << 2, MEMF_PUBLIC | MEMF_CLEAR);
                if (pblock)
                {
                    ph->root = root;
                    ph->bd = root->bd;
                    ph->data = pblock;
                    pblock->pb_ID = AROS_LONG2BE(IDNAME_PARTITION);
                    pblock->pb_SummedLongs = AROS_LONG2BE(sizeof(struct PartitionBlock)/4);
                    if (PartitionRDBSetPartitionAttrs(PartitionBase, ph, taglist) != 0)
                    {
                        FreeMem(pblock, root->de.de_SizeBlock << 2);
                        FreeVec(ph->ln.ln_Name);
                        FreeMem(ph, sizeof(*ph));
                        return NULL;
                    }
                    oph = (struct PartitionHandle *)root->table->list.lh_Head;
                    while (oph->ln.ln_Succ)
                    {
                        if (ph->de.de_LowCyl<oph->de.de_LowCyl)
                            break;
                        oph = (struct PartitionHandle *)oph->ln.ln_Succ;
                    }
                    if (oph->ln.ln_Succ)
                    {
                        oph = (struct PartitionHandle *)oph->ln.ln_Pred;
                        if (oph->ln.ln_Pred)
                        {
                            Insert(&root->table->list, &ph->ln, &oph->ln);
                        }
                        else
                            AddHead(&root->table->list, &ph->ln);
                    }
                    else
                        AddTail(&root->table->list, &ph->ln);
                    return ph;
                }
                FreeVec(ph->ln.ln_Name);
            }
            FreeMem(ph, sizeof(struct PartitionHandle));
        }
    }
    return NULL;
}

void PartitionRDBDeletePartition
    (
        struct Library *PartitionBase,
        struct PartitionHandle *ph
    )
{

    PartitionRDBFreeHandle(PartitionBase, ph);
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

ULONG PartitionRDBDestroyPartitionTable
    (
        struct Library *PartitionBase,
        struct PartitionHandle *root
    )
{
struct RDBData *data;
struct RigidDiskBlock *rdb;

    if (root->de.de_SizeBlock < 128 ||
        root->de.de_SizeBlock > sizeof(root->buffer) / 4)
            return ERROR_BAD_NUMBER;

    data = root->table->data;
    CopyMem(&data->rdb, root->buffer, root->de.de_SizeBlock << 2);
    rdb = (struct RigidDiskBlock *)root->buffer;
    rdb->rdb_ID = 0;
    return deviceError(writeBlock(PartitionBase, root, data->rdbblock, root->buffer));
}

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
            if (strcmp(fn->fhb.fhb_FileSysName, (char *)nameTag->ti_Data))
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
        *((STRPTR *)tag->ti_Data) = fhb->fhb_FileSysName;
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
    PartitionRDBWritePartitionTable,
    PartitionRDBCreatePartitionTable,
    PartitionRDBAddPartition,
    PartitionRDBDeletePartition,
    PartitionRDBGetPartitionTableAttr,
    NULL,
    PartitionRDBGetPartitionAttr,
    PartitionRDBSetPartitionAttrs,
    PartitionRDBPartitionTableAttrs,
    PartitionRDBPartitionAttrs,
    PartitionRDBDestroyPartitionTable,
    PartitionRDBFindFileSystem
};

const struct FSFunctionTable FilesystemRDB =
{
    PartitionRDBLoadFileSystem,
    PartitionRDBGetFileSystemAttr,
    PartitionRDBFreeFileSystem
};
