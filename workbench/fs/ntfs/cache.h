/*
    Copyright © 2010, The AROS Development Team. All rights reserved.
    $Id$

    Disk cache.
*/

#ifndef CACHE_H
#define CACHE_H

#include <exec/types.h>
#include <exec/lists.h>

struct BlockRange
{
    struct MinNode node1;    /* links into hash table */
    struct MinNode node2;    /* links into free and dirty lists */
    ULONG use_count;    /* number of users of this block */
    UWORD state;
    ULONG num;    /* start block number */
    UBYTE *data;    /* actual block data */
};

struct Cache
{
    ULONG block_count;    /* number of blocks allocated */
    ULONG block_size;    /* size of a disk block */
    ULONG hash_size;    /* size of hash table */
    struct BlockRange **blocks;    /* array of pointers to all blocks */
    struct MinList *hash_table;    /* hash table of all valid cache blocks */
    struct MinList dirty_list;    /* the dirty list */
    struct MinList free_list;    /* the free list */
};

/* Block states */

#define BS_EMPTY 0
#define BS_VALID 1
#define BS_DIRTY 2

/* Prototypes */

APTR Cache_CreateCache(ULONG hash_size, ULONG block_count, ULONG block_size);
VOID Cache_DestroyCache(APTR cache);
APTR Cache_GetBlock(APTR cache, UQUAD blockNum, UBYTE **data);
VOID Cache_FreeBlock(APTR cache, APTR block);
VOID Cache_MarkBlockDirty(APTR cache, APTR block);
BOOL Cache_Flush(APTR cache);

#endif
