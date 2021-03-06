/*
    Copyright (C) 2010-2015, The AROS Development Team. All rights reserved.

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
    struct ExecBase *sys_base;
    struct DosLibrary *dos_base;
    APTR  priv;         /* private data sent to AccessDisk() */
    ULONG block_count;  /* number of blocks allocated */
    ULONG block_size;   /* size of a disk block */
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

APTR Cache_CreateCache(APTR priv, ULONG hash_size, ULONG block_count,
    ULONG block_size, struct ExecBase *sys_base, struct DosLibrary *dos_base);
VOID Cache_DestroyCache(APTR cache);
APTR Cache_GetBlock(APTR cache, ULONG blockNum, UBYTE **data);
VOID Cache_FreeBlock(APTR cache, APTR block);
VOID Cache_MarkBlockDirty(APTR cache, APTR block);
BOOL Cache_Flush(APTR cache);

LONG AccessDisk(BOOL do_write, ULONG num, ULONG nblocks, ULONG block_size,
    UBYTE *data, APTR priv);

#endif
