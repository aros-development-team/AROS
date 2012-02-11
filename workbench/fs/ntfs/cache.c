/*
    Copyright © 2010, The AROS Development Team. All rights reserved.
    $Id: cache.c 33098 2010-04-26 16:58:12Z neil $

    Disk cache.
 */

/* 
 * This is an LRU copyback cache.
 *
 * The cache consists of a fixed number of cache blocks, each of which can
 * hold a contiguous range of disk blocks (each range is of a fixed
 * length). Each range is addressed by its base block number, calculated by
 * applying a mask to the requested block number.
 * 
 * Each cache block contains two Exec list nodes, so it can be part of two
 * lists simultaneously. The first node links a block into a hash table
 * list, while the second links it into either the free list or the dirty
 * list. Initially, all cache blocks are present in the free list, and are
 * absent from the hash table.
 * 
 * When a disk block is requested by the client, the range that contains
 * that disk block is calculated. The range is then sought by looking up
 * the hash table. Each position in the hash table holds a list containing
 * all cache blocks (henceforth referred to as a block: individual disk
 * blocks are not used internally) that map to that hash location. Each
 * list in the hash table is typically very short, so look-up time is
 * quick.
 * 
 * If the requested range is not in the hash table, a cache block is
 * removed from the head of the free list and from any hash table list it
 * is in, and used to read the appropriate range from disk. This block is
 * then added to the hash table at its new location.
 * 
 * Two elements are returned to the client when a requested block is found:
 * an opaque cache block handle, and a pointer to the data buffer.
 * 
 * When a range is freed, it remains in the hash table so that it can be
 * found quickly if needed again. Unless dirty, it is also added to the
 * tail of the free list through its second list node. The block then
 * remains in the hash table until reused for a different range.
 * 
 * If a disk block is marked dirty by the client, the entire containing
 * range is marked dirty and added to the tail of the dirty list through
 * the cache block's second node. The dirty list is flushed periodically
 * (currently once per second), and additionally whenever the free list
 * becomes empty.
 * 
 */

#include <dos/dos.h>

#include <proto/exec.h>
#include <proto/dos.h>
#include <clib/alib_protos.h>

#include "cache.h"
#include "ntfs_fs.h"
#include "ntfs_protos.h"

#include "debug.h"

#define RANGE_SHIFT 5
#define RANGE_SIZE (1 << RANGE_SHIFT)
#define RANGE_MASK (RANGE_SIZE - 1)

#define NODE2(A) \
   ((struct BlockRange *)(((A) != NULL) ? \
   (((BYTE *)(A)) - (IPTR)&((struct BlockRange *)NULL)->node2) : NULL))

APTR Cache_CreateCache(ULONG hash_size, ULONG block_count, ULONG block_size)
{
    struct Cache *c;
    ULONG i;
    BOOL success = TRUE;
    struct BlockRange *b;

    D(bug("[NTFS]: %s()\n", __PRETTY_FUNCTION__));

    /* Allocate cache structure */

    if((c = AllocVec(sizeof(struct Cache), MEMF_PUBLIC | MEMF_CLEAR)) == NULL)
        success = FALSE;

    if(success)
    {
        c->block_size = block_size;
        c->block_count = block_count;
        c->hash_size = hash_size;

        /* Allocate hash table */

        c->hash_table = AllocVec(sizeof(struct MinList) * hash_size,
            MEMF_PUBLIC | MEMF_CLEAR);
        if(c->hash_table == NULL)
            success = FALSE;

        /* Initialise each hash location's list, and free and dirty lists */

        for(i = 0; i < hash_size && success; i++)
            NewList((struct List *)&c->hash_table[i]);
        NewList((struct List *)&c->free_list);
        NewList((struct List *)&c->dirty_list);

        /* Allocate cache blocks and add them to the free list */

        c->blocks = AllocVec(sizeof(APTR) * block_count,
            MEMF_PUBLIC | MEMF_CLEAR);
        if(c == NULL)
            success = FALSE;

        for(i = 0; i < block_count && success; i++)
        {
            b = AllocVec(sizeof(struct BlockRange)
                + (c->block_size << RANGE_SHIFT), MEMF_PUBLIC);
            b->use_count = 0;
            b->state = BS_EMPTY;
            b->num = 0;
            b->data = (UBYTE *)b + sizeof(struct BlockRange);

            if(b != NULL)
                c->blocks[i] = b;
            else
                success = FALSE;

            if(success)
                AddTail((struct List *)&c->free_list,
                    (struct Node *)&b->node2);
        }
    }

    if(!success)
    {
        Cache_DestroyCache(c);
        c = NULL;
    }

    return c;
}

VOID Cache_DestroyCache(APTR cache)
{
    struct Cache *c = cache;
    ULONG i;

    D(bug("[NTFS]: %s()\n", __PRETTY_FUNCTION__));

    Cache_Flush(c);

    for(i = 0; i < c->block_count; i++)
        FreeVec(c->blocks[i]);
    FreeVec(c->blocks);
    FreeVec(c->hash_table);
    FreeVec(c);
}

APTR Cache_GetBlock(APTR cache, UQUAD blockNum, UBYTE **data)
{
    struct Cache *c = cache;
    struct BlockRange *b = NULL, *b2;
    ULONG error = 0, data_offset;
    struct MinList *l =
        &c->hash_table[(blockNum >> RANGE_SHIFT) & (c->hash_size - 1)];
    struct MinNode *n;

    D(bug("[NTFS]: %s(%d)\n", __PRETTY_FUNCTION__, blockNum));

    /* Change block number to the start block of a range and get byte offset
     * within range */

    data_offset = (blockNum - (blockNum & ~RANGE_MASK)) * c->block_size;
    blockNum &= ~RANGE_MASK;

    /* Check existing valid blocks first */

    ForeachNode(l, b2)
    {
        if(b2->num == blockNum)
            b = b2;
    }

    if(b != NULL)
    {
        /* Block found, so increment its usage count and remove it from the
         * free list */

        if(b->use_count++ == 0)
        {
            if(b->state != BS_DIRTY)
                Remove((struct Node *)&b->node2);
        }
    }
    else
    {
        /* Get a free buffer to read block from disk */

        n = (struct MinNode *)RemHead((struct List *)&c->free_list);
        if(n == NULL)
        {
            /* No free blocks, so flush dirty list to try and free up some
             * more blocks, then try again */

            Cache_Flush(c);
            n = (struct MinNode *)RemHead((struct List *)&c->free_list);
        }

        if(n != NULL)
        {
            b = (struct BlockRange *)NODE2(n);

            /* Read the block from disk */

            if((error = AccessDisk(FALSE, blockNum, RANGE_SIZE,
                c->block_size, b->data)) == 0)
            {
                /* Remove block from its old position in the hash */

                if(b->state == BS_VALID)
                    Remove((struct Node *)b);

                /* Add it to the hash at the new location */

                AddHead((struct List *)l, (struct Node *)&b->node1);
                b->num = blockNum;
                b->state = BS_VALID;
                b->use_count = 1;
            }
            else
            {
                /* Read failed, so put the block back on the free list */

                b->state = BS_EMPTY;
                AddHead((struct List *)&c->free_list,
                    (struct Node *)&b->node2);
                b = NULL;
            }
        }
        else
            error = ERROR_NO_FREE_STORE;
    }

    /* Set data pointer and error, and return cache block handle */

    *data = b->data + data_offset;
    SetIoErr(error);

    return b;
}

VOID Cache_FreeBlock(APTR cache, APTR block)
{
    struct Cache *c = cache;
    struct BlockRange *b = block;

    D(bug("[NTFS]: %s()\n", __PRETTY_FUNCTION__));

    /* Decrement usage count */

    b->use_count--;

    /* Put an unused block at the end of the free list unless it's dirty */

    if(b->use_count == 0 && b->state != BS_DIRTY)
        AddTail((struct List *)&c->free_list, (struct Node *)&b->node2);

    return;
}

VOID Cache_MarkBlockDirty(APTR cache, APTR block)
{
    struct Cache *c = cache;
    struct BlockRange *b = block;

    D(bug("[NTFS]: %s()\n", __PRETTY_FUNCTION__));

    if(b->state != BS_DIRTY)
    {
        b->state = BS_DIRTY;
        AddTail((struct List *)&c->dirty_list, (struct Node *)&b->node2);
    }

    return;
}

BOOL Cache_Flush(APTR cache)
{
    struct Cache *c = cache;
    ULONG error = 0;
    struct MinNode *n;
    struct BlockRange *b;

    D(bug("[NTFS]: %s()\n", __PRETTY_FUNCTION__));

    while((n = (struct MinNode *)RemHead((struct List *)&c->dirty_list))
        != NULL && error == 0)
    {
        /* Write dirty block range to disk */

        b = NODE2(n);
        error = AccessDisk(TRUE, b->num, RANGE_SIZE, c->block_size, b->data);

        /* Transfer block range to free list if unused, or put back on dirty
         * list upon an error */

        if(error == 0)
        {
            b->state = BS_VALID;
            if(b->use_count == 0)
                AddTail((struct List *)&c->free_list, (struct Node *)&b->node2);
        }
        else
            AddHead((struct List *)&c->dirty_list, (struct Node *)&b->node2);
    }

    SetIoErr(error);
    return error == 0;
}

