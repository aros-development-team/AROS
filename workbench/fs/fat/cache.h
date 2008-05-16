/*
 * Disk buffer cache
 *
 * Copyright © 2007 Robert Norris
 * Copyright © 2008 Pavel Fedin
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the same terms as AROS itself.
 *
 * $Id$
 */

#ifndef CACHE_H
#define CACHE_H 1

#include <exec/types.h>
#include <exec/io.h>

struct cache_block {
    struct cache_block  *hash_next;     /* next block in this hash bucket */
    struct cache_block  *hash_prev;     /* previous block in this hash bucket */

    struct cache_block  *free_next;     /* next block on free list */
    struct cache_block  *free_prev;     /* previous block on free list */

    struct cache_block  *dirty_next;    /* next block on dirty list */

    ULONG               use_count;      /* number of users of this block */
    BOOL                is_dirty;       /* does the block need to be written? */

    ULONG               num;            /* block number */

    UBYTE               *data;          /* actual block data */
};

struct cache {
    ULONG               hash_size;      /* size of hash table */
    ULONG               hash_mask;      /* mask applied to block number to find correct hash bucket */

    ULONG               num_blocks;     /* number of blocks allocated */
    ULONG               block_size;     /* size of block */

    ULONG               flags;          /* cache options */

    struct cache_block  **blocks;       /* big list of all the blocks */
    ULONG               num_in_use;     /* number of blocks currently in use (ie not on the free list) */

    struct cache_block  **hash;         /* the hash table itself */

    struct cache_block  *free_head;     /* first block in the free list */
    struct cache_block  *free_tail;     /* last block in the free list */

    struct cache_block  *dirty;         /* list of dirty blocks */

    ULONG               hits;           /* number of hits, for stats */
    ULONG               misses;         /* number of misses */
};

/* flags for cache_new */
#define CACHE_WRITETHROUGH  (1<<0)      /* write immediately */
#define CACHE_WRITEBACK     (1<<1)      /* defer writes */

struct cache *cache_new(ULONG hash_size, ULONG num_blocks, ULONG block_size, ULONG flags);
void cache_free(struct cache *c);

ULONG cache_get_block(struct cache *c, ULONG num, ULONG flags, struct cache_block **rb);
ULONG cache_put_block(struct cache *c, struct cache_block *b, ULONG flags);

ULONG cache_get_blocks(struct cache *c, ULONG num, ULONG nblocks, ULONG flags, struct cache_block **rb);
ULONG cache_put_blocks(struct cache *c, struct cache_block **b, ULONG nblocks, ULONG flags);

ULONG cache_mark_block_dirty(struct cache *c, struct cache_block *b);
ULONG cache_mark_blocks_dirty(struct cache *c, struct cache_block **b, ULONG nblocks);

ULONG cache_flush(struct cache *c);

void cache_stats(struct cache *c);

#endif
