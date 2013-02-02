/*
 * Copyright (C) 2013, The AROS Development Team
 * All right reserved.
 * Author: Jason S. McMullan <jason.mcmullan@gmail.com>
 *
 * Licensed under the AROS PUBLIC LICENSE (APL) Version 1.1
 */

/* 'BCache' is a trivial write-through cache for block devices.
 */

#ifndef BCACHE_H
#define BCACHE_H

#include <exec/io.h>
#include <dos/filehandler.h>

/* DO NOT ALLOCATE NOR FREE THIS STRUCTURE MANUALLY!
 *
 * Feel free to add to other lists - bc_Node is for your use!
 */
struct BCache {
    struct Node    bc_Node;
    /* The following are for quick reference only - DO NOT MODIFY! */
    ULONG          bc_BlockSize;    /* In bytes */
    ULONG          bc_Blocks;       /* # of blocks in the FSSM range */
    /* The rest is internally used by BCache */
};

/* Create a buffer cache, based off of a FileSysStartupMsg.
 * Note that:
 *  bc_BlockSize will be fsm_Environ[DE_BLOCKSIZE]*4
 *            (default - 512)
 *  bc_Blocks is the number of blocks from DE_LOWCYL to DE_HIGHCYL
 *            (default - total # of blocks on the disk)
 */
LONG BCache_Create(struct ExecBase *sysBase, struct FileSysStartupMsg *fssm, struct BCache **bcache);

/* Dispose of the cache, and close the underlying device
 *
 * NOTE: This does *not* call Remove() on bc_Node - do that
 *       before calling this routine!
 */
VOID BCache_Delete(struct BCache *bcache);

/* Extend the cache by 'numbuffers' blocks
 */
LONG BCache_Extend(struct BCache *bcache, LONG numbuffers);

/* Drop all cache
 * returns:
 *   RETURN_OK     - Cache flushed, disk present
 *   ERROR_NO_DISK - Cache flushed, no disk present
 */
LONG BCache_Invalidate(struct BCache *bcache);

/* Determine state of underlying media
 *    RETURN_OK     - No change since last invalidate
 *    RETURN_WARN   - Disk has changed, but a disk is present
 *    ERROR_NO_DISK - No disk present
 */
LONG BCache_Present(struct BCache *bcache);

/* Read block from disk, give pointer to the cached buffer
 * returns:
 *   RETURN_OK     - Disk present
 *                   Data read from disk
 *   RETURN_WARN   - Disk has changed since last BCache_Invalidate();
 *                   No data read from disk
 *   ERROR_NO_DISK - No disk present
 *                   No data read from disk
 *   any other     - Disk read error
 *                   Partial data read from disk
 */
LONG BCache_Read(struct BCache *bcache, ULONG block, UBYTE **buffer);

/* Write buffer to blocks on the disk.
 * returns:
 *   RETURN_OK     - Disk present
 *                   Data written to disk
 *   RETURN_WARN   - Disk has changed since last BCache_Invalidate();
 *                   No data written to disk
 *   ERROR_NO_DISK - No disk present
 *                   No data written to disk
 *   any other     - Disk write error
 *                   Partial data written to disk
 */
LONG BCache_Write(struct BCache *bcache, ULONG block, CONST UBYTE *buffer);

#endif /* BLOCKCACHE_H */
