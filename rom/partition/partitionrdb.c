/*
    Copyright � 1995-2011, The AROS Development Team. All rights reserved.
    $Id$

*/

#define RDB_WRITE 1

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
#define DEBUG 1
#endif
#include "debug.h"

#include <string.h>

struct RDBData {
    struct RigidDiskBlock rdb;
    UBYTE rdbblock; /* the block rdb was read from */
    struct List badblocklist;
    struct List fsheaderlist;
};

struct BadBlockNode {
    struct Node ln;
    struct BadBlockBlock bbb;
};

struct FileSysNode
{
    struct FileSysHandle h;

    struct FileSysHeaderBlock fhb;
    struct LoadSegBlock *filesystem; /* the FS in LSEG blocks */
    ULONG fsblocks;                  /* nr of LSEG blocks for FS */
};

#define LSEGDATASIZE (123 * sizeof(ULONG))

struct FileSysReader
{
    ULONG count;
    ULONG offset;
    ULONG size;
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

    while (length > 0) {
    	ULONG size = length;

    	if (size + fsr->offset > fsr->size)
    	    size = fsr->size - fsr->offset;
    	if (size > 0) {
    	    UBYTE *inbuf = (UBYTE*)(fsr->fsn->filesystem[fsr->count].lsb_LoadData) + fsr->offset;
	    CopyMemQuick(inbuf, outbuf, size);
	}

	outsize += size;
	fsr->offset += size;
	length -= size;
	outbuf += size;

	if (fsr->offset == fsr->size) {
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
    LONG oldpos = fsr->offset;

    switch (mode) {
    case OFFSET_BEGINNING: break;
    case OFFSET_END:       pos = fsr->size - pos; break;
    case OFFSET_CURRENT:   pos = fsr->offset + pos; break;
    default: return -1;
    }

    if (pos < 0 || pos >= fsr->size)
    	return -1;

    fsr->offset = pos;

    return oldpos;

    AROS_USERFUNC_EXIT
}

/* Load a filesystem into DOS seglist */
static BPTR LoadFS(struct FileSysNode *node, struct DosLibrary *DOSBase)
{
    LONG (*FunctionArray[4])();
    struct FileSysReader fakefile;

#ifndef __mc68000
    /* Prevent loading hunk files on non-m68k */
    if (AROS_BE2LONG(node->filesystem[0].lsb_LoadData[0]) == 0x000003f3)
    	return BNULL;
#endif

    FunctionArray[0] = ReadFunc;
    FunctionArray[1] = __AROS_GETVECADDR(SysBase,33); /* AllocMem() */
    FunctionArray[2] = __AROS_GETVECADDR(SysBase,35); /* FreeMem() */
    FunctionArray[3] = SeekFunc;

    /* Initialize our stream */
    fakefile.count      = 0;
    fakefile.offset     = 4;	/* ??? */
    fakefile.size       = LSEGDATASIZE;
    fakefile.fsn        = node;

    return InternalLoadSeg((BPTR)&fakefile, BNULL, FunctionArray, NULL);
}

/*
 * Insert RDB LSEG filesystem to FileSystem.resource
 * FIXME: this is an obsolete hack. Use new filesystem API
 * in Boot Strap instead.
 */

#if (AROS_FLAVOUR & AROS_FLAVOUR_BINCOMPAT) && defined(__mc68000)

/* We can't use InternalLoadSeg() because DOS isn't initialized
 * at this point. To allow this to be built as a relocatable
 * library, we provide a dummy weak alias here, which will
 * be overridden when linking the ROM image.
 */
static BPTR _InternalLoadSeg_AOS(BPTR fh,
                         BPTR table,
                         SIPTR * funcarray,
                         SIPTR * stack,
                         struct Library * DOSBase)
{
	return BNULL;
}

BPTR InternalLoadSeg_AOS(BPTR fh,
                         BPTR table,
                         SIPTR * funcarray,
                         SIPTR * stack,
                         struct Library * DOSBase)
	__attribute__((weak, alias("_InternalLoadSeg_AOS")));

static void AddFS(struct RDBData *data)
{
    struct FileSysResource *fsr;
    struct FileSysNode *node;
    void (* FunctionArray[4])();
    struct FileSysReader fakefile;

    FunctionArray[0] = (void(*))ReadFunc;
    FunctionArray[1] = __AROS_GETVECADDR(SysBase,33); /* AllocMem() */
    FunctionArray[2] = __AROS_GETVECADDR(SysBase,35); /* FreeMem() */
    FunctionArray[3] = (void(*))SeekFunc;

    fsr = OpenResource("FileSystem.resource");
    if (!fsr)
    	return;

    ForeachNode(&data->fsheaderlist, node) {
    	struct FileSysEntry *fsrnode;
    	ULONG dostype = node->fhb.fhb_DosType;
    	ULONG version = node->fhb.fhb_Version;
    	BOOL newerinstalled = FALSE;
    	ULONG size = LSEGDATASIZE;

    	ForeachNode(&fsr->fsr_FileSysEntries, fsrnode) {
    	    if (fsrnode->fse_DosType == dostype && fsrnode->fse_Version >= version)
    	    	newerinstalled = TRUE;
    	}
    	if (newerinstalled)
    	    continue;
    	fsrnode = AllocVec(sizeof(struct FileSysEntry), MEMF_PUBLIC | MEMF_CLEAR);
    	if (!fsrnode)
    	    break;
    	fakefile.count = 0;
    	fakefile.offset = 4;
    	fakefile.size = size;
    	fakefile.fsn = node;
	if (node->filesystem[0].lsb_LoadData[0] == 0x000003f3) {
	    BPTR seg = InternalLoadSeg_AOS((BPTR)&fakefile, BNULL, (SIPTR*)FunctionArray, NULL, NULL);
	    if (seg) {
    	    	D(bug("RDB fs %08x %d.%d '%s' seg=%08x added\n",
    	    	    dostype, version >> 16, version & 0xffff, &node->fhb.fhb_FileSysName, seg));
    	    	CopyMemQuick(&node->fhb.fhb_DosType, &fsrnode->fse_DosType, sizeof(struct FileSysEntry) - sizeof(struct Node));
    	    	fsrnode->fse_SegList = seg;
    	    	AddHead(&fsr->fsr_FileSysEntries, &fsrnode->fse_Node);
    	    }
    	}
    }
}

#endif

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

LONG PartitionRDBCheckPartitionTable
    (
        struct Library *PartitionBase,
        struct PartitionHandle *root
    )
{
UBYTE i;
UBYTE space[4096];
struct RigidDiskBlock *rdb = (struct RigidDiskBlock *)space;
struct PartitionType type;
struct TagItem tags[] = {{PT_TYPE, (IPTR)&type}, {TAG_DONE, 0}};

    if (sizeof(space) < (root->de.de_SizeBlock << 2))
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
        if (calcChkSum((ULONG *)rdb, AROS_BE2LONG(rdb->rdb_SummedLongs))==0)
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
            (calcChkSum((ULONG *)buffer, AROS_BE2LONG(buffer->bbb_SummedLongs))==0)
        )
    {
        bn = AllocMem(sizeof(struct BadBlockNode), MEMF_PUBLIC | MEMF_CLEAR);
        if (bn)
        {
            CopyMem(buffer, &bn->bbb, sizeof(struct BadBlockBlock));
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
            (calcChkSum((ULONG *)buffer, AROS_BE2LONG(buffer->pb_SummedLongs))==0)
        )
    {
        ph = AllocMem(sizeof(struct PartitionHandle), MEMF_PUBLIC | MEMF_CLEAR);
        if (ph)
        {
            ph->ln.ln_Name = AllocVec(32, MEMF_PUBLIC | MEMF_CLEAR);
            if (ph->ln.ln_Name)
            {
                pblock = AllocMem(sizeof(struct PartitionBlock), MEMF_PUBLIC);
                if (pblock)
                {
                    CopyMem(buffer, pblock, sizeof(struct PartitionBlock));
                    ph->root = root;
                    ph->bd = root->bd;
                    ph->data = pblock;
                    CopyMem(pblock->pb_DriveName+1, ph->ln.ln_Name, pblock->pb_DriveName[0]);
                    ph->ln.ln_Name[pblock->pb_DriveName[0]]=0;
                    CopyBE2HostDosEnvec(pblock->pb_Environment, (SIPTR *)&ph->de, AROS_BE2LONG(((struct DosEnvec *)pblock->pb_Environment)->de_TableSize)+1);
                    ph->dg.dg_DeviceType = DG_DIRECT_ACCESS;
                    ph->dg.dg_SectorSize = ph->de.de_SizeBlock<<2;
                    ph->dg.dg_Heads = ph->de.de_Surfaces;
                    ph->dg.dg_TrackSectors = ph->de.de_BlocksPerTrack;
                    ph->dg.dg_Cylinders = ph->de.de_HighCyl - ph->de.de_LowCyl + 1;
                    ph->dg.dg_BufMemType = ph->de.de_BufMemType;
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
            (calcChkSum((ULONG *)buffer, AROS_BE2LONG(buffer->fhb_SummedLongs))==0)
        )
    {
        fn = AllocMem(sizeof(struct FileSysNode), MEMF_PUBLIC | MEMF_CLEAR);
        if (fn)
        {
            CopyMem(buffer, &fn->fhb, sizeof(struct FileSysHeaderBlock));

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

    if (fn->filesystem)
        FreeVec(fn->filesystem);
    FreeMem(fn, sizeof(struct FileSysNode));
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

    size = 0;
    block = AROS_BE2LONG(fn->fhb.fhb_SegListBlocks);
    while (block != (ULONG)-1)
    {
        size++;
        if (readBlock(PartitionBase, root, block, buffer) !=0)
            return 0;
        if (
                (AROS_BE2LONG(buffer->lsb_ID) != IDNAME_LOADSEG) ||
                (calcChkSum((ULONG *)buffer, AROS_BE2LONG(buffer->lsb_SummedLongs)))
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
    if (size)
    {
        fn->fsblocks = size;
        fn->filesystem = AllocVec(size*512, MEMF_PUBLIC);
        if (fn->filesystem)
        {
            size = 0;
            block = AROS_BE2LONG(fn->fhb.fhb_SegListBlocks);
            while (block != (ULONG)-1)
            {
                if (readBlock(PartitionBase, root, block, &fn->filesystem[size]) !=0)
                    return;
                block = AROS_BE2LONG(fn->filesystem[size].lsb_Next);
                size++;
            }
        }
    }
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

    buffer = AllocVec(root->de.de_SizeBlock << 2, MEMF_PUBLIC);
    if (!buffer)
    	return 1;
    data = AllocMem(sizeof(struct RDBData), MEMF_PUBLIC);
    if (data)
    {
        for (i=0;i<RDB_LOCATION_LIMIT; i++)
        {
            if (readBlock(PartitionBase, root, i, buffer) != 0) {
            	FreeVec(buffer);
                return 1;
            }
            CopyMem(buffer, &data->rdb, sizeof(struct RigidDiskBlock));
            if (data->rdb.rdb_ID == AROS_BE2LONG(IDNAME_RIGIDDISK))
                break;
        }
        if (i != RDB_LOCATION_LIMIT)
        {
        ULONG block;

            data->rdbblock = i;
            NEWLIST(&root->table->list);
            NEWLIST(&data->badblocklist);
            NEWLIST(&data->fsheaderlist);
            root->table->data = data;
            /* take the values of the rdb instead of TD_GEOMETRY */
            root->dg.dg_SectorSize = AROS_BE2LONG(data->rdb.rdb_BlockBytes);
            root->dg.dg_Cylinders = AROS_BE2LONG(data->rdb.rdb_Cylinders);
            root->dg.dg_TrackSectors = AROS_BE2LONG(data->rdb.rdb_Sectors);
            root->dg.dg_Heads = AROS_BE2LONG(data->rdb.rdb_Heads);
            /* read bad blocks */
            block = AROS_BE2LONG(data->rdb.rdb_BadBlockList);
            while (block != (ULONG)-1)
            {
            struct BadBlockNode *bn;

                if (readBlock(PartitionBase, root, block, buffer)==0)
                {
                    bn = PartitionRDBNewBadBlock(PartitionBase, root, (struct BadBlockBlock *)buffer);
                    if (bn != NULL)
                    {
                        AddTail(&data->badblocklist, &bn->ln);
                        block = AROS_BE2LONG(bn->bbb.bbb_Next);
                    }
                    else
                        break;
                }
                else
                    break;
            }
            /* read partition blocks */
            block = AROS_BE2LONG(data->rdb.rdb_PartitionList);
            while (block != (ULONG)-1)
            {
            struct PartitionHandle *ph;
                if (readBlock(PartitionBase, root, block, buffer)==0)
                {
                    ph = PartitionRDBNewHandle(PartitionBase, root, (struct PartitionBlock *)buffer);
                    if (ph != NULL)
                    {
                        AddTail(&root->table->list, &ph->ln);
                        block = AROS_BE2LONG(((struct PartitionBlock *)ph->data)->pb_Next);
                    }
                    else
                        break;
                }
                else
                    break;
            }
            /* read filesystem blocks */
            block = AROS_BE2LONG(data->rdb.rdb_FileSysHeaderList);
            while (block != (ULONG)-1)
            {
            struct FileSysNode *fn;

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
                        break;
                }
                else
                    break;
            }
   	    FreeVec(buffer);
#if (AROS_FLAVOUR & AROS_FLAVOUR_BINCOMPAT) && defined(__mc68000)
   	    AddFS(data);
#endif
            return 0;
        }
        FreeMem(data, sizeof(struct RDBData));
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
    FreeMem(ph->data, sizeof(struct PartitionBlock));
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
        FreeMem(bn, sizeof(struct BadBlockNode));

    while ((fn = (struct FileSysNode *)RemTail(&data->fsheaderlist)))
    {
	/* Do not deallocate filesystem handles which are queued for loading */
    	if (!fn->h.boot)
    	    PartitionRDBFreeFileSystem(&fn->h);
    }
    FreeMem(data, sizeof(struct RDBData));
}

ULONG PartitionRDBWriteFileSys
    (
        struct Library *PartitionBase,
        struct PartitionHandle *root,
        struct FileSysNode *fn,
        ULONG block
    )
{
ULONG size;

    if (fn->filesystem)
    {
        size = 0;
        while (size != fn->fsblocks)
        {
            fn->filesystem[size].lsb_Next = (size+1) != fn->fsblocks ? AROS_LONG2BE(block+1) : (ULONG)-1;
            fn->filesystem[size].lsb_ChkSum = 0;
            fn->filesystem[size].lsb_ChkSum = AROS_LONG2BE(0-calcChkSum((ULONG *)&fn->filesystem[size], AROS_LONG2BE(fn->filesystem[size].lsb_SummedLongs)));
#if RDB_WRITE
            if (writeBlock(PartitionBase, root, block++, &fn->filesystem[size]) != 0)
                return block;
#else
            kprintf("RDB-write: block=%ld, type=LSEG\n", block);
            block++;
#endif
            size++;
        }
    }
    return block;
}

LONG PartitionRDBWritePartitionTable
    (
        struct Library *PartitionBase,
        struct PartitionHandle *root
    )
{
UBYTE buffer[4096];
struct RDBData *data;
struct PartitionHandle *ph;
struct PartitionBlock *pblock;
struct BadBlockNode *bn;
struct FileSysNode *fn;
ULONG block;

    if (sizeof(buffer) < (root->de.de_SizeBlock << 2))
	return 0;
    data = root->table->data;
    block = data->rdbblock+1; /* RDB will be written at the end */
    fillMem((UBYTE *)buffer, root->de.de_SizeBlock << 2, 0);

    /* write bad blocks */
    bn = (struct BadBlockNode *)data->badblocklist.lh_Head;
    if (bn->ln.ln_Succ)
        data->rdb.rdb_BadBlockList = block;
    else
        data->rdb.rdb_BadBlockList = (ULONG)-1;
    while (bn->ln.ln_Succ)
    {
        bn->bbb.bbb_Next = bn->ln.ln_Succ->ln_Succ ? AROS_LONG2BE(block+1) : (ULONG)-1;
        bn->bbb.bbb_ChkSum = 0;
        bn->bbb.bbb_ChkSum = AROS_LONG2BE(0-calcChkSum((ULONG *)&bn->bbb, AROS_BE2LONG(bn->bbb.bbb_SummedLongs)));
        CopyMem(&bn->bbb, buffer, sizeof(struct BadBlockBlock));
#if RDB_WRITE
        writeBlock(PartitionBase, root, block++, buffer);
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
        CopyMem(pblock, buffer, sizeof(struct PartitionBlock));
#if RDB_WRITE
        writeBlock(PartitionBase, root, block++, buffer);
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

        fshblock = block;
        block++; /* header block will be written later */
        fn->fhb.fhb_SegListBlocks = AROS_LONG2BE(block);
        /* write filesystem LSEG blocks */
        block = PartitionRDBWriteFileSys(PartitionBase, root, fn, block);
        fn->fhb.fhb_Next = fn->h.ln.ln_Succ->ln_Succ ? AROS_LONG2BE(block) : (ULONG)-1;
        fn->fhb.fhb_ChkSum = 0;
        CopyMem(&fn->fhb, buffer, sizeof(struct FileSysHeaderBlock));
        ((struct FileSysHeaderBlock *)buffer)->fhb_ChkSum = AROS_LONG2BE(0-calcChkSum((ULONG *)buffer, AROS_BE2LONG(fn->fhb.fhb_SummedLongs)));
#if RDB_WRITE
        writeBlock(PartitionBase, root, fshblock, buffer);
#else
        kprintf("RDB-write: block=%ld, type=FSHD\n", fshblock);
#endif
        fn = (struct FileSysNode *)fn->h.ln.ln_Succ;
    }
    data->rdb.rdb_HighRDSKBlock = AROS_LONG2BE(block-1);
    data->rdb.rdb_ChkSum = 0;
    data->rdb.rdb_ChkSum = AROS_LONG2BE(0-calcChkSum((ULONG *)&data->rdb, AROS_BE2LONG(data->rdb.rdb_SummedLongs)));
    CopyMem(&data->rdb, buffer, sizeof(struct RigidDiskBlock));
#if RDB_WRITE
    writeBlock(PartitionBase, root, data->rdbblock, buffer);
#else
    kprintf("RDB-write: block=%ld, type=RDSK\n", data->rdbblock);
#endif
    return 0;
}

LONG PartitionRDBCreatePartitionTable
    (
        struct Library *PartitionBase,
        struct PartitionHandle *ph
    )
{
struct RDBData *data;
ULONG i;

    data = AllocMem(sizeof(struct RDBData), MEMF_PUBLIC | MEMF_CLEAR);
    if (data)
    {
        ph->table->data = data;
        data->rdb.rdb_ID = AROS_LONG2BE(IDNAME_RIGIDDISK);
        data->rdb.rdb_SummedLongs = AROS_LONG2BE(sizeof(struct RigidDiskBlock)/4);
        data->rdb.rdb_BlockBytes = AROS_LONG2BE(ph->dg.dg_SectorSize);
        data->rdb.rdb_BadBlockList = (ULONG)-1;
        data->rdb.rdb_PartitionList = (ULONG)-1;
        data->rdb.rdb_FileSysHeaderList = (ULONG)-1;
        data->rdb.rdb_DriveInit = (ULONG)-1;
        for (i=0;i<6;i++)
            data->rdb.rdb_Reserved1[i] = (ULONG)-1;
        data->rdb.rdb_Cylinders = AROS_LONG2BE(ph->dg.dg_Cylinders);
        data->rdb.rdb_Sectors = AROS_LONG2BE(ph->dg.dg_TrackSectors);
        data->rdb.rdb_Heads = AROS_LONG2BE(ph->dg.dg_Heads);

        data->rdb.rdb_Park = data->rdb.rdb_Cylinders;
        data->rdb.rdb_WritePreComp = data->rdb.rdb_Cylinders;
        data->rdb.rdb_ReducedWrite = data->rdb.rdb_Cylinders;
        /* StepRate */

        data->rdb.rdb_RDBBlocksLo = AROS_LONG2BE(1); /* leave a block for PC */
        data->rdb.rdb_RDBBlocksHi = AROS_LONG2BE((ph->dg.dg_Heads*ph->dg.dg_TrackSectors*2)-1); /* two cylinders */
        data->rdb.rdb_LoCylinder = AROS_LONG2BE(2);
        data->rdb.rdb_HiCylinder = AROS_LONG2BE(ph->dg.dg_Cylinders-1);
        data->rdb.rdb_CylBlocks = AROS_LONG2BE(ph->dg.dg_Heads*ph->dg.dg_TrackSectors);
        /* AutoParkSeconds */
        /* DiskVendor */
        /* DiskProduct */
        /* DiskRevision */
        /* ControllerVendor */
        /* ControllerProduct */
        /* ControllerRevision */

        data->rdbblock = 1;
        NEWLIST(&data->badblocklist);
        NEWLIST(&data->fsheaderlist);
        NEWLIST(&ph->table->list);
        return 0;
    }
    return 1;
}

LONG PartitionRDBGetPartitionTableAttrs
    (
        struct Library *PartitionBase,
        struct PartitionHandle *root,
        struct TagItem *taglist
    )
{

    while (taglist[0].ti_Tag != TAG_DONE)
    {

        switch (taglist[0].ti_Tag)
        {
        case PTT_TYPE:
            *((LONG *)taglist[0].ti_Data) = root->table->type;
            break;
        case PTT_RESERVED:
            *((LONG *)taglist[0].ti_Data) =
                root->de.de_Surfaces*root->de.de_BlocksPerTrack*2; /* 2 cylinders */
            break;
        }
        taglist++;
    }
    return 0;
}

LONG PartitionRDBSetPartitionTableAttrs
    (
        struct Library *PartitionBase,
        struct PartitionHandle *root,
        struct TagItem *taglist
    )
{

    while (taglist[0].ti_Tag != TAG_DONE)
    {

        switch (taglist[0].ti_Tag)
        {
        }
        taglist++;
    }
    return 0;
}

LONG PartitionRDBGetPartitionAttrs
    (
        struct Library *PartitionBase,
        struct PartitionHandle *ph,
        struct TagItem *taglist
    )
{

    while (taglist[0].ti_Tag != TAG_DONE)
    {
    struct PartitionBlock *data = (struct PartitionBlock *)ph->data;

        switch (taglist[0].ti_Tag)
        {
        case PT_GEOMETRY:
            {
                struct DriveGeometry *dg = (struct DriveGeometry *)taglist[0].ti_Data;
                CopyMem(&ph->dg, dg, sizeof(struct DriveGeometry));
            }
            break;
        case PT_DOSENVEC:
            CopyMem(&ph->de, (struct DosEnvec *)taglist[0].ti_Data, sizeof(struct DosEnvec));
            break;
        case PT_TYPE:
            {
            struct PartitionType *ptype=(struct PartitionType *)taglist[0].ti_Data;
            ULONG dt = AROS_LONG2BE(ph->de.de_DosType);

                CopyMem(&dt, ptype->id, 4);
                ptype->id_len = 4;
            }
            break;
        case PT_NAME:
            CopyMem(ph->ln.ln_Name, (UBYTE *)taglist[0].ti_Data, 32);
            break;
        case PT_BOOTABLE:
            *((LONG *)taglist[0].ti_Data) = (AROS_BE2LONG(data->pb_Flags) & PBFF_BOOTABLE) ? TRUE : FALSE;
            break;
        case PT_AUTOMOUNT:
            *((LONG *)taglist[0].ti_Data) = (AROS_BE2LONG(data->pb_Flags) & PBFF_NOMOUNT) ? FALSE : TRUE;
            break;
        }
        taglist++;
    }
    return 0;
}

LONG PartitionRDBSetPartitionAttrs
    (
        struct Library *PartitionBase,
        struct PartitionHandle *ph,
        struct TagItem *taglist
    )
{

    while (taglist[0].ti_Tag != TAG_DONE)
    {
    struct PartitionBlock *data = (struct PartitionBlock *)ph->data;

        switch (taglist[0].ti_Tag)
        {
        case PT_DOSENVEC:
            {
            struct DosEnvec *de = (struct DosEnvec *)taglist[0].ti_Data;

                CopyMem(de, &ph->de, (de->de_TableSize+1)*sizeof(IPTR));
                CopyHost2BEDosEnvec((SIPTR *)de, data->pb_Environment, de->de_TableSize+1);
            }
            break;
        case PT_TYPE:
            {
            struct PartitionType *ptype=(struct PartitionType *)taglist[0].ti_Data;
            ULONG dt;

                CopyMem(ptype->id, &dt, 4);
                ph->de.de_DosType = AROS_BE2LONG(dt);
                data->pb_Environment[DE_DOSTYPE] = dt;
            }
            break;
        case PT_NAME:
            {
            STRPTR name = (STRPTR)taglist[0].ti_Data;
            ULONG len = strlen(name);

                CopyMem(name, ph->ln.ln_Name, len+1);
                CopyMem(name, data->pb_DriveName+1, len);
                data->pb_DriveName[len+1] = 0;
                data->pb_DriveName[0] = len;
            }
            break;
        case PT_BOOTABLE:
            if (taglist[0].ti_Data)
                data->pb_Flags = AROS_LONG2BE(AROS_BE2LONG(data->pb_Flags) | PBFF_BOOTABLE);
            else
                data->pb_Flags = AROS_LONG2BE(AROS_BE2LONG(data->pb_Flags) & ~PBFF_BOOTABLE);
            break;
        case PT_AUTOMOUNT:
            if (taglist[0].ti_Data)
                data->pb_Flags = AROS_LONG2BE(AROS_BE2LONG(data->pb_Flags) & ~PBFF_NOMOUNT);
            else
                data->pb_Flags = AROS_LONG2BE(AROS_BE2LONG(data->pb_Flags) | PBFF_NOMOUNT);
            break;
        }
        taglist++;
    }
    return 0;
}

struct PartitionHandle *PartitionRDBAddPartition
    (
        struct Library *PartitionBase,
        struct PartitionHandle *root,
        struct TagItem *taglist
    )
{

    if (findTagItem(PT_DOSENVEC, taglist) != NULL)
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
                pblock = AllocMem(sizeof(struct PartitionBlock), MEMF_PUBLIC | MEMF_CLEAR);
                if (pblock)
                {
                    ph->root = root;
                    ph->bd = root->bd;
                    ph->data = pblock;
                    pblock->pb_ID = AROS_LONG2BE(IDNAME_PARTITION);
                    pblock->pb_SummedLongs = AROS_LONG2BE(sizeof(struct PartitionBlock)/4);
                    PartitionRDBSetPartitionAttrs(PartitionBase, ph, taglist);
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
                    if (findTagItem(PT_DOSENVEC, taglist) == NULL)
                    {
                        ph->dg.dg_DeviceType = DG_DIRECT_ACCESS;
                        ph->dg.dg_SectorSize = ph->de.de_SizeBlock<<2;
                        ph->dg.dg_Heads = ph->de.de_Surfaces;
                        ph->dg.dg_TrackSectors = ph->de.de_BlocksPerTrack;
                        ph->dg.dg_Cylinders = ph->de.de_HighCyl - ph->de.de_LowCyl + 1;
                        ph->dg.dg_BufMemType = ph->de.de_BufMemType;
                    }
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
    {PTTA_TYPE,     PLAM_READ},
    {PTTA_RESERVED, PLAM_READ},
    {PTTA_DONE,     0}
};

struct PartitionAttribute *PartitionRDBQueryPartitionTableAttrs(struct Library *PartitionBase)
{
    return (APTR)PartitionRDBPartitionTableAttrs;
}

const struct PartitionAttribute PartitionRDBPartitionAttrs[]=
{
    /* TODO: implement write */
    {PTA_GEOMETRY,  PLAM_READ},
    {PTA_DOSENVEC,  PLAM_READ | PLAM_WRITE},
    {PTA_TYPE,      PLAM_READ | PLAM_WRITE},
    {PTA_NAME,      PLAM_READ | PLAM_WRITE},
    {PTA_BOOTABLE,  PLAM_READ | PLAM_WRITE},
    {PTA_AUTOMOUNT, PLAM_READ | PLAM_WRITE},
    {PTA_DONE,      0}
};

struct PartitionAttribute *PartitionRDBQueryPartitionAttrs(struct Library *PartitionBase)
{
    return (APTR)PartitionRDBPartitionAttrs;
}

ULONG PartitionRDBDestroyPartitionTable
    (
        struct Library *PartitionBase,
        struct PartitionHandle *root
    )
{
struct RDBData *data;
UBYTE buffer[4096];

    if (sizeof(buffer) < (root->de.de_SizeBlock << 2))
    	    return 0;

    data = root->table->data;
    CopyMem(&data->rdb, buffer, sizeof(struct RigidDiskBlock));
    ((struct RigidDiskBlock *)buffer)->rdb_ID = 0;
    if (writeBlock(PartitionBase, root, data->rdbblock, buffer))
        return 1;
    return 0;
}

struct Node *PartitionRDBFindFileSystem(struct Library *PartitionBase, struct PartitionHandle *ph, struct TagItem *tags)
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
	    if (fn->fhb.fhb_ID != idTag->ti_Data)
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
    if (PartitionBase->dosBase)
    	return LoadFS((struct FileSysNode *)fn, (struct DosLibrary *)PartitionBase->dosBase);
    else
	return BNULL;
}

LONG PartitionRDBGetFileSystemAttrs(struct Library *PartitionBase, struct FileSysHandle *fn, const struct TagItem *taglist)
{
    struct TagItem *tag;
    struct FileSysEntry *fse;
    struct FileSysHeaderBlock *fhb = &((struct FileSysNode *)fn)->fhb;

    while ((tag = NextTagItem(&taglist)))
    {
        switch (tag->ti_Tag)
        {
        case FST_ID:
            *((ULONG *)tag->ti_Data) = AROS_BE2LONG(fhb->fhb_DosType);
            break;

        case FST_NAME:
            *((STRPTR *)tag->ti_Data) = fhb->fhb_FileSysName;
            break;

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
	    fse->fse_Type	= AROS_BE2LONG(fhb->fhb_Type);
	    fse->fse_Task	= AROS_BE2LONG(fhb->fhb_Task);
	    fse->fse_Lock	= (BPTR)(SIPTR)AROS_BE2LONG(fhb->fhb_Lock);
	    /* Just for convenience. This is expected to be zero. */
	    fse->fse_Handler	= (BPTR)(SIPTR)AROS_BE2LONG(fhb->fhb_Handler);
	    fse->fse_StackSize  = AROS_BE2LONG(fhb->fhb_StackSize);
	    fse->fse_Priority	= AROS_BE2LONG(fhb->fhb_Priority);
	    fse->fse_Startup	= (BPTR)(SIPTR)AROS_BE2LONG(fhb->fhb_Startup);
	    /* Skip fse_SegList */
	    fse->fse_GlobalVec	= (BPTR)(SIPTR)AROS_BE2LONG(fhb->fhb_GlobalVec);

	    break;

	case FST_VERSION:
	    *((ULONG *)tag->ti_Data) = AROS_BE2LONG(fhb->fhb_Version);
	    break;
        }
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
    PartitionRDBGetPartitionTableAttrs,
    PartitionRDBSetPartitionTableAttrs,
    PartitionRDBGetPartitionAttrs,
    PartitionRDBSetPartitionAttrs,
    PartitionRDBQueryPartitionTableAttrs,
    PartitionRDBQueryPartitionAttrs,
    PartitionRDBDestroyPartitionTable,
    PartitionRDBFindFileSystem
};

const struct FSFunctionTable FilesystemRDB =
{
    PartitionRDBLoadFileSystem,
    PartitionRDBGetFileSystemAttrs,
    PartitionRDBFreeFileSystem
};
