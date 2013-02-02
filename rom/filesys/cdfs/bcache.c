/*
 * Copyright (C) 2013, The AROS Development Team
 * All right reserved.
 * Author: Jason S. McMullan <jason.mcmullan@gmail.com>
 *
 * Licensed under the AROS PUBLIC LICENSE (APL) Version 1.1
 */

/* 'BCache' is a trivial write-through cache for block devices.
 *
 * Supports TD_*, CD_*, TD64_*, and NSCMD_* devices
 */

#include <aros/debug.h>

#include <proto/exec.h>

#include <devices/trackdisk.h>
#include <devices/newstyle.h>

#include "bcache.h"

/* From Wikipedia - XORShift RNG, by George Marsaglia
 */
struct XorshiftRNG {
    ULONG w, x, y, z;
};

static VOID XorshiftRNG_Init(struct XorshiftRNG *r)
{
    r->w = 88675123;
    r->x = 123456789;
    r->y = 362436069;
    r->z = 521288629;
}

static ULONG XorshiftRNG(struct XorshiftRNG *r)
{
    ULONG t;
    t = r->x ^ (r->x << 11);
    r->x = r->y; r->y = r->z; r->z = r->w;
    return r->w = r->w ^ (r->w >> 19) ^ (t ^ (t >> 8));
}


struct BCacheEntry {
    struct MinNode be_Node;
    UBYTE         *be_Buffer;   /* Pointer into bp_CacheBlocks */
    ULONG          be_Block;    /* Block address.
                                 * Sufficient for up to:
                                 *   2TB of  512 byte blocks
                                 *   8TB of 2048 byte blocks
                                 *  16TB of 4096 byte blocks
                                 */
};

struct BCachePrivate {
    struct BCache    bp_Public;
    struct ExecBase *bp_SysBase;
    struct IOStdReq *bp_IOStdReq;
    ULONG            bp_ChangeIdx;      /* Last invalidate changenum */
    ULONG            bp_ChangeNum;      /* Last changenum */
    ULONG            bp_BlockBase;
    ULONG            bp_Blocks;
    ULONG            bp_ReadCMD;
    ULONG            bp_WriteCMD;
    struct XorshiftRNG bp_RNG;

    LONG             bp_NumBuffers;
    ULONG            bp_BufMemType;
    UBYTE           *bp_CacheBlocks;    /* Single contiguous buffer area */
    struct BCacheEntry *bp_CacheEntries;
    struct List      bp_CacheValid;     /* Active blocks */
    struct List      bp_CacheInvalid;   /* Inactive entries */
};

/* Create a buffer cache, based off of a FileSysStartupMsg.
 * Note that:
 *  bc_BlockSize will be fsm_Environ[DE_BLOCKSIZE]*4
 *            (default - 512)
 *  bc_Blocks is the number of blocks from DE_LOWCYL to DE_HIGHCYL
 *            (default - total # of blocks on the disk)
 */
LONG BCache_Create(struct ExecBase *SysBase, struct FileSysStartupMsg *fssm, struct BCache **bcache)
{
    struct BCachePrivate *bp;
    struct DosEnvec *de = BADDR(fssm->fssm_Environ);
    LONG err;

    if ((bp = AllocVec(sizeof(*bp), MEMF_ANY | MEMF_PUBLIC))) {
        struct MsgPort *mp;
        if ((mp = CreateMsgPort())) {
            struct IOStdReq *io;
            if ((io = (struct IOStdReq*)CreateIORequest(mp, sizeof(*io)))) {
                bp->bp_IOStdReq = io;
                bp->bp_SysBase = SysBase;
                XorshiftRNG_Init(&bp->bp_RNG);

                D(bug("%s: Device %b.%d\n", __func__, fssm->fssm_Device, fssm->fssm_Unit));

                if (0 == OpenDevice(AROS_BSTR_ADDR(fssm->fssm_Device), fssm->fssm_Unit, (struct IORequest *)io, 0)) {
                    struct DriveGeometry dg;
                    struct NSDeviceQueryResult nsq;
                    ULONG  blocksize = 1;
                    ULONG  blockspertrack = 0, lowcyl = 0, cylinders = ~0;
                    ULONG  numbuffers = 0;
                    ULONG  bufmemtype = MEMF_PUBLIC;

                    D(bug("%s: Opened.\n", __func__));

                    /* Get the device geometry */
                    io->io_Command = TD_GETGEOMETRY;
                    io->io_Data = &dg;
                    io->io_Offset = 0;
                    io->io_Length = sizeof(dg);
                    io->io_Actual = 0;
                    if (0 == DoIO((struct IORequest *)io)) {
                        blockspertrack = dg.dg_TrackSectors;
                        lowcyl = 0;
                        cylinders = dg.dg_Cylinders;
                        blocksize = dg.dg_SectorSize;

                        D(bug("%s: Geometry: bs=%d, blocks/track=%d, cyls=%d\n", __func__, blocksize, blockspertrack, cylinders));
                    }

                    if (de->de_TableSize >= DE_SIZEBLOCK)
                        blocksize = de->de_SizeBlock * 4;

                    /* Default to 32-bit read/write */
                    bp->bp_ReadCMD = CMD_READ;
                    bp->bp_WriteCMD = CMD_WRITE;
                    bp->bp_Blocks = (ULONG)(0x100000000ULL/blocksize);

                    /* Probe for TD_READ64 */
                    io->io_Command = TD_READ64;
                    io->io_Data    = NULL;
                    io->io_Length  = 0;
                    io->io_Offset  = 0;
                    io->io_Actual  = 0;
                    if (0 == DoIO((struct IORequest *)io)) {
                        bp->bp_ReadCMD = TD_READ64;
                        bp->bp_WriteCMD = TD_WRITE64;
                        bp->bp_Blocks = (ULONG)(~0ULL/blocksize);
                        D(bug("%s: TD64 works\n", __func__));
                    }

                    /* Determine if it's a NSD device */
                    io->io_Command = NSCMD_DEVICEQUERY;
                    io->io_Data    = &nsq;
                    io->io_Length  = sizeof(nsq);
                    io->io_Offset  = 0;
                    io->io_Actual  = 0;
                    if (0 == DoIO((struct IORequest *)io)) {
                        int i;
                        D(bug("%s: NSCMD_DEVICEQUERY works\n", __func__));
                        for (i = 0; nsq.SupportedCommands[i] != 0; i++) {
                            if (nsq.SupportedCommands[i] == NSCMD_TD_READ64) {
                                bp->bp_ReadCMD  = NSCMD_TD_READ64;
                                bp->bp_WriteCMD = NSCMD_TD_WRITE64;
                                bp->bp_Blocks = (ULONG)(~0ULL/blocksize);
                                D(bug("%s: NSCMD_TD_READ64 works\n", __func__));
                                break;
                            }
                        }
                    }

                    if (de->de_TableSize >= DE_BLKSPERTRACK && de->de_BlocksPerTrack)
                        blockspertrack = de->de_BlocksPerTrack;

                    if (de->de_TableSize >= DE_LOWCYL && de->de_LowCyl)
                        lowcyl = de->de_LowCyl;

                    if (de->de_TableSize >= DE_HIGHCYL && de->de_HighCyl) {
                        if ((de->de_HighCyl+1-lowcyl) < cylinders)
                            cylinders = (de->de_HighCyl+1-lowcyl);
                    }

                    if (de->de_TableSize >= DE_NUMBUFFERS) {
                        numbuffers = de->de_NumBuffers;
                    }
                    if (numbuffers == 0) {
                        if (blockspertrack > 32)
                            numbuffers = 32;
                        else
                            numbuffers = blockspertrack;
                    }

                    if (de->de_TableSize >= DE_BUFMEMTYPE) {
                        bufmemtype = de->de_BufMemType;
                    }

                    if (((lowcyl + cylinders) * blockspertrack) < bp->bp_Blocks) {
                        bp->bp_Public.bc_BlockSize = blocksize;
                        bp->bp_Blocks = cylinders * blockspertrack;
                        bp->bp_BlockBase = lowcyl * blockspertrack;
                        io->io_Command = TD_CHANGENUM;
                        io->io_Data = NULL;
                        io->io_Offset = 0;
                        io->io_Length = 0;
                        io->io_Actual = 0;
                        if (0 == DoIO((struct IORequest *)io)) {
                            D(bug("%s: TD_CHANGENUM works\n", __func__));
                            bp->bp_ChangeNum = io->io_Actual;
                        } else {
                            bp->bp_ChangeNum = ~0;
                        }
                        bp->bp_ChangeIdx = bp->bp_ChangeNum;

                        bp->bp_NumBuffers = 0;
                        bp->bp_BufMemType = bufmemtype;
                        err = BCache_Extend(&bp->bp_Public, numbuffers);
                        if (err == RETURN_OK) {
                            BCache_Invalidate(&bp->bp_Public);
                            *bcache = &bp->bp_Public;
                            D(bug("%s: BCache of %dx%d blocks created for blocks %d-%d\n", __func__, bp->bp_Public.bc_BlockSize, bp->bp_NumBuffers, bp->bp_BlockBase, bp->bp_BlockBase + bp->bp_Blocks-1));
                            return RETURN_OK;
                        } else {
                            err = ERROR_NO_FREE_STORE;
                        }
                    } else {
                        err = ERROR_SEEK_ERROR;
                    }
                    CloseDevice((struct IORequest *)io);
                } else {
                    err = ERROR_OBJECT_NOT_FOUND;
                }
                DeleteIORequest((struct IORequest *)io);
            } else {
                err = ERROR_NO_FREE_STORE;
            }
            DeleteMsgPort(mp);
        } else {
            err = ERROR_NO_FREE_STORE;
        }
        FreeVec(bp);
    } else {
        err = ERROR_NO_FREE_STORE;
    }
    *bcache = NULL;
    return err;
}

#undef SysBase
#define SysBase bp->bp_SysBase

/* Dispose of the cache, and close the underlying device
 *
 * NOTE: This does *not* call Remove() on bc_Node - do that
 *       before calling this routine!
 */
VOID BCache_Delete(struct BCache *bcache)
{
    struct BCachePrivate *bp = (struct BCachePrivate *)bcache;
    ULONG size = bp->bp_Public.bc_BlockSize * bp->bp_NumBuffers +
                 sizeof(bp->bp_CacheEntries[0]) *  bp->bp_NumBuffers;
   
    CloseDevice((struct IORequest *)&bp->bp_IOStdReq);
    FreeMem(bp->bp_CacheBlocks, size);
    FreeVec(bp);
}

/* Extend/Reduce the cache by 'numbuffers' blocks
 */
LONG BCache_Extend(struct BCache *bcache, LONG numbuffers)
{
    struct BCachePrivate *bp = (struct BCachePrivate *)bcache;
    UBYTE *newcache;
    ULONG newsize;
    ULONG oldsize = bp->bp_Public.bc_BlockSize * bp->bp_NumBuffers +
                 sizeof(bp->bp_CacheEntries[0]) *  bp->bp_NumBuffers;

    /* Minimum size is 1 buffer */
    if (bp->bp_NumBuffers + numbuffers <= 0)
        numbuffers = 1 - bp->bp_NumBuffers;

    if (numbuffers == 0) {
        D(bug("%s: No change to buffer size\n", __func__));
        return RETURN_OK;
    }

    numbuffers += bp->bp_NumBuffers;
    newsize = bp->bp_Public.bc_BlockSize * numbuffers +
              sizeof(bp->bp_CacheEntries[0]) * numbuffers;

    newcache = AllocMem(newsize, bp->bp_BufMemType);
    if (newcache) {
        struct BCacheEntry *newentry = (struct BCacheEntry *)&newcache[bp->bp_Public.bc_BlockSize * numbuffers];
        LONG i, index;

        NEWLIST(&bp->bp_CacheValid);
        NEWLIST(&bp->bp_CacheInvalid);
        for (i = index = 0; i < numbuffers; i++, index += bp->bp_Public.bc_BlockSize) {
            newentry[i].be_Buffer = &newcache[index];
            AddTail(&bp->bp_CacheInvalid, (struct Node *)&newentry[i].be_Node);
        }

        if (bp->bp_NumBuffers)
            FreeMem(bp->bp_CacheBlocks, oldsize);

        bp->bp_NumBuffers = numbuffers;
        bp->bp_CacheBlocks = newcache;
        bp->bp_CacheEntries = newentry;
        D(bug("%s: Cache size now %d entries\n", __func__, numbuffers));
        return RETURN_OK;
    }

    return ERROR_NO_FREE_STORE;
}

/* returns:
 *    RETURN_OK: No change since last invalidate
 *    RETURN_WARN: Disk has changes, but a disk is present
 *    ERROR_NO_DISK: No disk present
 */
LONG BCache_Present(struct BCache *bcache)
{
    struct BCachePrivate *bp = (struct BCachePrivate *)bcache;
    struct IOStdReq *io = bp->bp_IOStdReq;

    /* Determine if medium has changed */
    io->io_Command = TD_CHANGENUM;
    io->io_Data = NULL;
    io->io_Offset = 0;
    io->io_Length = 0;
    io->io_Actual = 0;
    if (0 == DoIO((struct IORequest *)io))
        bp->bp_ChangeNum = io->io_Actual;

    if (bp->bp_ChangeNum != bp->bp_ChangeIdx) {
        /* Determine if a new disk is present */
        io->io_Command = TD_CHANGESTATE;
        io->io_Data = NULL;
        io->io_Offset = 0;
        io->io_Length = 0;
        io->io_Actual = 0;
        /* Even if this fails, io_Actual should be good for us */
        DoIO((struct IORequest *)io);
        return (io->io_Actual ? ERROR_NO_DISK : RETURN_WARN);
    }

    return RETURN_OK;
}


/* Drop all cache
 * returns:
 *   RETURN_OK     - Cache flushed, disk present
 *   ERROR_NO_DISK - Cache flushed, no disk present
 */
LONG BCache_Invalidate(struct BCache *bcache)
{
    struct BCachePrivate *bp = (struct BCachePrivate *)bcache;
    struct BCacheEntry *be;
    int i;

    while ((be = (struct BCacheEntry *)REMHEAD(&bp->bp_CacheValid)))
        ADDHEAD(&bp->bp_CacheInvalid, be);

    if (BCache_Present(bcache) == ERROR_NO_DISK)
        return ERROR_NO_DISK;

    /* We have a disk - mark it as 'present' for this cache */
    bp->bp_ChangeNum = bp->bp_ChangeIdx;

    /* Stir the RNG */
    for (i = 0; i < (bp->bp_ChangeNum & 0xf); i++)
        XorshiftRNG_Init(&bp->bp_RNG);

    return RETURN_OK;
}

static LONG BCache_IO(struct BCache *bcache, ULONG cmd, ULONG block, ULONG blocks, APTR buffer)
{
    struct BCachePrivate *bp = (struct BCachePrivate *)bcache;
    struct IOStdReq *io = bp->bp_IOStdReq;
    LONG err;
    UQUAD offset, length;

    D(bug("%s: cmd=%d, blocks %d (%d)\n", __func__, cmd, block, blocks));

    err = BCache_Present(bcache);
    if (err != RETURN_OK) {
        D(bug("%s: Disk changed or not present\n", __func__));
        return err;
    }

    /* Validate the offset + length */
    if ((block + blocks) > bp->bp_Blocks) {
        D(bug("%s: Block 0x%08x exceeds max 0x%08x\n", __func__, (ULONG)(block + blocks), (ULONG)bp->bp_Blocks));
        return ERROR_SEEK_ERROR;
    }

    /* Read the data into the buffer */
    offset = (UQUAD)block * bp->bp_Public.bc_BlockSize;
    length = (UQUAD)blocks * bp->bp_Public.bc_BlockSize;

    if (length >= (1ULL<<32)) {
        D(bug("%s: Block range exceeds ULONG size\n", __func__));
        return ERROR_OBJECT_TOO_LARGE;
    }

    io->io_Command = cmd;
    io->io_Data = buffer;
    io->io_Length = (ULONG)length;
    io->io_Offset = (ULONG)offset;
    io->io_Actual = (ULONG)(offset >> 32);
    if (0 == DoIO((struct IORequest *)io)) {
        /* Stir the RNG */
        XorshiftRNG(&bp->bp_RNG);
        D(bug("%s: io_Error %d\n", __func__, io->io_Error));
        return io->io_Error ? RETURN_ERROR : RETURN_OK;
    }

    D(bug("%s: Invalid command?\n", __func__));
    return RETURN_FAIL;
}

/* Read block from disk to the buffer
 * returns:
 *   RETURN_OK     - Disk present
 *                   Data read from disk
 *   RETURN_WARN   - Disk has changed since last BCache_Invalidate();
 *                   No data read from disk
 *   RETURN_ERROR  - Disk read error
 *                   Partial data read from disk
 *   ERROR_NO_DISK - No disk present
 *                   No data read from disk
 */
LONG BCache_Read(struct BCache *bcache, ULONG block, UBYTE **bufferp)
{
    struct BCachePrivate *bp = (struct BCachePrivate *)bcache;
    struct BCacheEntry *be;
    LONG err;
    ULONG depth = 0;
    ULONG index, run;
    int i;

    D(bug("%s: Block %d\n", __func__, block));
    if (block > bp->bp_Blocks)
        return ERROR_SEEK_ERROR;

    /* Scan to see if this block was cached */
    ForeachNode(&bp->bp_CacheValid, be) {
        if (be->be_Block == block) {
            /* Move to the front of the list, if it is
             * more than 1/8th of the way into the
             * cache.
             */
            D(bug("%s: Cached block found at depth %d\n", __func__, depth));
            if (depth > (bp->bp_NumBuffers / 8)) {
                D(bug("%s: Moving to head of Valid list..\n", __func__));
                REMOVE(be);
                ADDHEAD(&bp->bp_CacheValid, be);
            }
            *bufferp = be->be_Buffer;
            return RETURN_OK;
        }
        depth++;
    }

    /* Ok, no cached block. Let's pick a random spot
     * and random length out of the buffer cache.
     */
    if (bp->bp_NumBuffers == 1) {
        index = 0;
        run = 1;
    } else {
        if (bp->bp_NumBuffers < 16)
            run = 1 + (XorshiftRNG(&bp->bp_RNG) % (bp->bp_NumBuffers - 1));
        else
            run = 8 + (XorshiftRNG(&bp->bp_RNG) & 7);
        index = XorshiftRNG(&bp->bp_RNG) % (bp->bp_NumBuffers - run);
    }

    ASSERT(run > 0);
    ASSERT(index + run <= bp->bp_NumBuffers);
    ASSERT(block + run <= bp->bp_Blocks);

    /* First, shorten the run to prevent duplications in the cache */
    D(bug("%s: Caching blocks %d (%d)\n", __func__, block, run));
    ForeachNode(&bp->bp_CacheValid, be) {
        if (be->be_Block > block && be->be_Block < (block + run)) {
            /* Shorten the run */
            D(bug("%s: Shorten run: cached block %d in (%d - %d)\n",
                        __func__, be->be_Block, block, block + run - 1));
            run = be->be_Block - block;
        }
    }

    ASSERT(run > 0);

    /* Steal the entries for these buffers */
    D(bug("%s: Steal indexes %d (%d)\n", __func__, index, run));
    for (i = 0, be = &bp->bp_CacheEntries[index]; i < run; i++, be++) {
        REMOVE(&be->be_Node);
    }

    be = &bp->bp_CacheEntries[index];

    /* Read the run */
    err = BCache_IO(bcache, bp->bp_ReadCMD, block, run, be->be_Buffer);
    if (err == RETURN_OK) {
        D(bug("%s: Adding to Valid cache...\n", __func__));
        for (i = 0; i < run; i++, be++) {
            be->be_Block = block + i;
            ADDHEAD(&bp->bp_CacheValid, &be->be_Node);
        }
        *bufferp = bp->bp_CacheEntries[index].be_Buffer;
    } else {
        D(bug("%s: IO failed, returning %d (%d) to Invalid list\n", __func__, index, run));
        for (i = 0; i < run; i++, be++) {
            ADDHEAD(&bp->bp_CacheInvalid, &be->be_Node);
        }
    }

    D(bug("Valid:"));
    ForeachNode(&bp->bp_CacheValid, be) {
        D(bug(" %d (%d)", be - bp->bp_CacheEntries, be->be_Block));
    }
    D(bug("\nInval"));
    ForeachNode(&bp->bp_CacheInvalid, be) {
        D(bug(" %d", be - bp->bp_CacheEntries));
    }
    D(bug("\n"));

    return err;
}

/* Write buffer to blocks on the disk.
 * returns:
 *   RETURN_OK     - Disk present
 *                   Data written to disk
 *   RETURN_WARN   - Disk has changed since last BCache_Invalidate();
 *                   No data written to disk
 *   RETURN_ERROR  - Disk write error
 *                   Partial data written to disk
 *   ERROR_NO_DISK - No disk present
 *                   No data written to disk
 */
LONG BCache_Write(struct BCache *bcache, ULONG block, CONST UBYTE *buffer)
{
    struct BCachePrivate *bp = (struct BCachePrivate *)bcache;
    struct BCacheEntry *be;
    ULONG depth = 0;

    if (block > bp->bp_Blocks)
        return ERROR_SEEK_ERROR;

    /* Scan to see if this block was cached */
    ForeachNode(&bp->bp_CacheValid, be) {
        if (be->be_Block == block) {
            /* Move to the front of the list, if it is
             * more than 1/8th of the way into the
             * cache.
             */
            if (depth > (bp->bp_NumBuffers / 8)) {
                REMOVE(be);
                ADDHEAD(&bp->bp_CacheValid, be);
            }
            /* Synchronize the cache with the data to write */
            CopyMem(buffer, be->be_Buffer, bp->bp_Public.bc_BlockSize);
            break;
        }
        depth++;
    }

    return BCache_IO(bcache, bp->bp_WriteCMD, block, 1, (APTR)buffer);
}
