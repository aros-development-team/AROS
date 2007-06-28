/*
 * Disk buffer cache
 *
 * Copyright © 2007 Robert Norris
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the same terms as AROS itself.
 *
 * $Id$
 */

/* 
 * This is an attempt at a generic buffer cache for filesystems. Its a
 * traditional cache of the type described in Tanenbaum 2nd Ed. Eventually I
 * hope it will be a system-wide facility (eg cache.resource), but for the
 * moment its just part of fat.handler.
 *
 * The idea is that we have pile of blocks (struct cache_block), a hashtable
 * and a free list. Initially all the blocks are free, and reside on the free
 * list (a doubly-linked list). The hashtable is empty.
 *
 * When a request for a block comes in (cache_get_block()), we extract the
 * bottom N bits of the block number, and then scan the linked-list in that
 * cache bucket for the block. If its there, the use count for the block is
 * incremented and the block is returned.
 *
 * If not found, then we check the free list. The free list is stored in order
 * of use time, with the least-recently-used block at the front (head) and the
 * most-recently-used block at the back (tail). We scan the free list from
 * back to front. A block that was used recently is likely to be used again in
 * the future, so this is the fastest way to find it. Once found, its use
 * count is incremented and its re-inserted into the appropriate hash
 * bucket/list, then returned to the caller.
 *
 * If the block isn't found in the free list, then we don't have it anywhere,
 * and have to load it from disk. We take an empty buffer from the beginning
 * of the free list (that is, the oldest unused block available). If its dirty
 * (ie was written to), we write out it and all the other blocks for the
 * device/unit (to improve the odds of the filesystem remaining consistent).
 * Then, we load the block from the disk into the buffer, fiddle with the
 * block metadata, add it to the hash and return it.
 *
 * When the caller is finished with the block, it calls cache_put_block() to
 * return the block to the cache. The blocks' use count is decremented. If its
 * still >0 (ie the block is in use), then the function just returns. If not,
 * it removes the block from the hash and adds it to the end of the freelist.
 *
 * A task periodically scans the free list for dirty buffers, and writes them
 * out to disk.
 */

#include <exec/types.h>
#include <exec/nodes.h>
#include <exec/lists.h>
#include <exec/io.h>
#include <exec/ports.h>
#include <exec/errors.h>
#include <dos/dos.h>
#include <devices/trackdisk.h>

#include <proto/exec.h>

#include "cache.h"

#define DEBUG 0
#include <aros/debug.h>

/* prototype for lowlevel block function */
static ULONG _cache_do_blocks_ll(struct cache *c, BOOL do_write, ULONG num, ULONG nblocks, ULONG block_size, UBYTE *data);

struct cache *cache_new(struct Device *device, struct Unit *unit, ULONG hash_size, ULONG num_blocks, ULONG block_size, ULONG flags) {
    struct cache *c;
    ULONG i;

    D(bug("cache_new: hash_size %ld num_blocks %ld block_size %ld flags 0x%08x\n", hash_size, num_blocks, block_size, flags));

    c = (struct cache *) AllocVec(sizeof(struct cache), MEMF_PUBLIC);

    c->hash_size = hash_size;
    c->hash_mask = hash_size-1;

    c->block_size = block_size;

    if ((flags & (CACHE_WRITETHROUGH | CACHE_WRITEBACK)) == 0)
        flags |= CACHE_WRITETHROUGH;
    c->flags = flags;

    c->num_blocks = num_blocks;
    c->num_in_use = 0;

    c->blocks = (struct cache_block **) AllocVec(sizeof(struct cache_block *) * num_blocks, MEMF_PUBLIC);

    c->device = device;
    c->unit = unit;

    c->hits = c->misses = 0;

    for (i = 0; i < num_blocks; i++) {
        struct cache_block *b;

        b = (struct cache_block *) AllocVec(sizeof(struct cache_block) + block_size, MEMF_PUBLIC);
        b->use_count = 0;
        b->is_dirty = FALSE;
        b->num = 0;
        b->data = (UBYTE *) b + sizeof(struct cache_block);

        b->hash_next = b->hash_prev = NULL;

        c->blocks[i] = b;

        b->free_next = NULL;
        if (i == 0)
            b->free_prev = NULL;
        else {
            b->free_prev = c->blocks[i-1];
            c->blocks[i-1]->free_next = b;
        }
    }

    c->free_head = c->blocks[0];
    c->free_tail = c->blocks[num_blocks-1];

    c->hash = (struct cache_block **) AllocVec(sizeof(struct cache_block *) * hash_size, MEMF_PUBLIC | MEMF_CLEAR);

    return c;
}

void cache_free(struct cache *c) {
    ULONG i;

    /* XXX flush dirty blocks */

    for (i = 0; i < c->num_blocks; i++)
        FreeVec(c->blocks[i]);
    FreeVec(c->blocks);
    FreeVec(c->hash);
    FreeVec(c);
}

ULONG cache_get_block(struct cache *c, ULONG num, ULONG flags, struct cache_block **rb) {
    struct cache_block *b;
    ULONG err;

    D(bug("cache_get_block: looking for block %d, flags 0x%08x\n", num, flags));

    /* first check the in-use blocks */
    for (b = c->hash[num & c->hash_mask]; b != NULL; b = b->hash_next)
        if (b->num == num) {
            b->use_count++;

            D(bug("cache_get_block: found in-use block, use count is now %d\n", b->use_count));
            c->hits++;

            *rb = b;
            return 0;
        }

    D(bug("cache_get_block: in-use block not found, checking free list\n"));

    /* not there, check the recently-freed list. recently used stuff is placed
     * at the back, so we start there */
    for (b = c->free_tail; b != NULL; b = b->free_prev) {
        if (b->num == num) {
            D(bug("cache_get_block: found it in the free list\n"));
            c->hits++;

            /* remove it from the free list */
            if (b->free_prev != NULL)
                b->free_prev->free_next = b->free_next;
            else
                c->free_head = b->free_next;

            if (b->free_next != NULL)
                b->free_next->free_prev = b->free_prev;
            else
                c->free_tail = b->free_prev;

            b->free_prev = b->free_next = NULL;
            
            /* place it at the front of the hash */
            b->hash_prev = NULL;
            b->hash_next = c->hash[num & c->hash_mask];
            if (b->hash_next != NULL)
                b->hash_next->hash_prev = b;
            c->hash[num & c->hash_mask] = b;

            if (b->is_dirty) {
                /* XXX flush dirty blocks */
            }

            b->use_count = 1;
            c->num_in_use++;

            D(bug("cache_get_block: total %d blocks in use\n", c->num_in_use));

            *rb = b;
            return 0;
        }
    }

    D(bug("cache_get_block: not found, loading from disk\n"));
    c->misses++;

    /* gotta read it from disk. get an empty buffer */
    b = c->free_head;
    if (b == NULL) {
        D(bug("cache_get_block: no free blocks left\n"));
        return ERROR_NO_FREE_STORE;
    }

    /* detach it */
    if (c->free_head == c->free_tail)
        c->free_head = c->free_tail = NULL;
    else {
        c->free_head = b->free_next;
        if (c->free_head != NULL)
            c->free_head->free_prev = NULL;
    }

    b->free_prev = b->free_next = NULL;

    /* write it out if its dirty */
    if (b->is_dirty) {
        D(bug("cache_get_block: writing dirty block %d before reusing it\n", b->num));

        if ((err = _cache_do_blocks_ll(c, TRUE, num, 1, c->block_size, b->data)) != 0) {
            /* write failed. this is bad. */
            /* XXX what is the sane thing to do here? */
            *rb = NULL;
            return err;
        }
    }

    /* read the block from disk */
    if ((err = _cache_do_blocks_ll(c, FALSE, num, 1, c->block_size, b->data)) != 0) {
        /* read failed, put the block back on the free list */
        b->free_next = c->free_head;
        c->free_head = b;

        /* and bail */
        *rb = NULL;
        return err;
    }

    /* setup the rest of it */
    b->num = num;
    b->is_dirty = FALSE;

    /* add it to the hash */
    b->hash_next = c->hash[num & c->hash_mask];
    if (b->hash_next != NULL)
        b->hash_next->hash_prev = b;
    c->hash[num & c->hash_mask] = b;

    b->use_count = 1;
    c->num_in_use++;

    D(bug("cache_get_block: loaded from disk, total %d blocks in use\n", c->num_in_use));

    *rb = b;
    return 0;
}

ULONG cache_put_block(struct cache *c, struct cache_block *b, ULONG flags) {
    D(bug("cache_put_block: returning block %d, flags 0x%08x\n", b->num, flags));

    /* if its still in use, then we've got it easy */
    b->use_count--;
    if (b->use_count != 0) {
        D(bug("cache_put_block: new use count is %d, nothing else to do\n", b->use_count));
        return 0;
    }

    /* no longer in use, remove it from its hash */
    if (b->hash_prev != NULL)
        b->hash_prev->hash_next = b->hash_next;
    else
        c->hash[b->num & c->hash_mask] = b->hash_next;

    if (b->hash_next != NULL)
        b->hash_next->hash_prev = b->hash_prev;

    b->hash_prev = b->hash_next = NULL;
            
    /* put it at the end of the free list */
    b->free_next = NULL;
    b->free_prev = c->free_tail;
    if (b->free_prev != NULL)
        b->free_prev->free_next = b;
    c->free_tail = b;
    if (c->free_head == NULL)
        c->free_head = b;
    
    /* one down */
    c->num_in_use--;

    D(bug("cache_put_block: no longer in use, moved to free list, total %ld blocks in use\n", c->num_in_use));

    return 0;
}

ULONG cache_get_blocks(struct cache *c, ULONG num, ULONG nblocks, ULONG flags, struct cache_block **rb) {
    ULONG err, i;

    D(bug("cache_get_blocks: loading %d blocks starting at %d, flags 0x%08x\n", nblocks, num, flags));

    /* XXX optimise this to get contiguous blocks in one hit */
    for (i = 0; i < nblocks; i++) {
        if ((err = cache_get_block(c, num+i, flags, &rb[i])) != 0) {
            D(bug("cache_get_blocks: block load failed, freeing everything we got so far\n"));

            for (; i >= 0; i--)
                cache_put_block(c, rb[i], 0);

            return err;
        }
    
    }

    return 0;
}

ULONG cache_put_blocks(struct cache *c, struct cache_block **b, ULONG nblocks, ULONG flags) {
    ULONG i;

    D(bug("cache_put_blocks: returning %d blocks starting at %d, flags 0x%08x\n", nblocks, b[0]->num, flags));

    for (i = 0; i < nblocks; i++)
        cache_put_block(c, b[i], 0);

    return 0;
}

ULONG cache_mark_block_dirty(struct cache *c, struct cache_block *b) {
    ULONG err;

    if (c->flags & CACHE_WRITEBACK) {
        D(bug("cache_mark_block_dirty: block %d is now dirty\n", b->num));
        b->is_dirty = 1;
        return 0;
    }

    D(bug("cache_mark_block_dirty: writing dirty block %d\n", b->num));

    if ((err = _cache_do_blocks_ll(c, TRUE, b->num, 1, c->block_size, b->data)) != 0) {
        D(bug("cache_mark_block_dirty: write failed, leaving block marked dirty\n"));
        b->is_dirty = 1;
        return err;
    }

    return 0;
}

ULONG cache_mark_blocks_dirty(struct cache *c, struct cache_block **b, ULONG nblocks) {
    ULONG err, i;

    if (c->flags & CACHE_WRITEBACK) {
        for (i = 0; i < nblocks; i++) {
            D(bug("cache_mark_blocks_dirty: block %d is now dirty\n", b[i]->num));
            b[i]->is_dirty = 1;
        }
        return 0;
    }

    /* XXX optimise this to do contiguous blocks in one hit */
    for (i = 0; i < nblocks; i++) {
        D(bug("cache_mark_blocks_dirty: writing dirty block %d\n", b[i]->num));
        if ((err = _cache_do_blocks_ll(c, TRUE, b[i]->num, 1, c->block_size, b[i]->data)) != 0) {
            D(bug("cache_mark_blocks_dirty: write failed, leaving this and remaining blocks marked dirty\n"));
            b[i]->is_dirty = 1;
            for (; i < nblocks; i++) {
                D(bug("cache_mark_blocks_dirty: block %d is now dirty\n", b[i]->num));
                b[i]->is_dirty = 1;
            }
        }
    }


    return 0;
}

void cache_stats(struct cache *c) {
    struct cache_block *b;
    ULONG count, i;

    kprintf("[cache] statistics for cache 0x%08x\n", c);

    kprintf("    %ld hash buckets (mask 0x%x)\n", c->hash_size, c->hash_mask);
    kprintf("    block size: %ld bytes\n", c->block_size);
    kprintf("    total blocks: %ld\n", c->num_blocks);
    kprintf("    flags:%s%s\n", c->flags & CACHE_WRITETHROUGH ? " CACHE_WRITETHROUGH" : "",
                                c->flags & CACHE_WRITEBACK    ? " CACHE_WRITEBACK"    : "");
    kprintf("    total blocks in use: %ld\n", c->num_in_use);

    count = 0;
    for (i = 0; i < c->hash_size; i++)
        for (b = c->hash[i]; b != NULL; b = b->hash_next)
            count++;

    if (count != c->num_in_use)
        kprintf("WARNING: in-use count (%ld) doesn't match hash contents (%ld)\n", c->num_in_use, count);

    count = 0;
    for (b = c->free_head; b != NULL; b = b->free_next)
        count++;

    kprintf("    blocks on free list: %ld\n", count);

    kprintf("    hits: %ld    misses: %ld\n", c->hits, c->misses);
}

/* lowlevel block functions */

static ULONG _cache_do_blocks_ll(struct cache *c, BOOL do_write, ULONG num, ULONG nblocks, ULONG block_size, UBYTE *data) {
    struct MsgPort *port;
    struct IOExtTD req;
    ULONG err;

    D(bug("_cache_do_blocks_ll: request to %s %ld blocks starting from %ld (block_size %ld)\n", do_write ? "write" : "read", nblocks, num, block_size));

    port = CreateMsgPort();

    req.iotd_Req.io_Message.mn_Node.ln_Type = NT_REPLYMSG;
    req.iotd_Req.io_Message.mn_ReplyPort = port;
    req.iotd_Req.io_Message.mn_Length = sizeof(struct IOExtTD);

    req.iotd_Req.io_Command = do_write ? CMD_WRITE : CMD_READ;
    req.iotd_Req.io_Offset = num * block_size;
    req.iotd_Req.io_Length = nblocks * block_size;
    req.iotd_Req.io_Data = data;
    req.iotd_Req.io_Flags = IOF_QUICK;

    req.iotd_Req.io_Device = c->device;
    req.iotd_Req.io_Unit = c->unit;

    DoIO((struct IORequest *) &req);

    err = req.iotd_Req.io_Error;

    DeleteMsgPort(port);

    D(bug("_cache_do_blocks_ll: returning error %ld\n", err));

    return err;
}
