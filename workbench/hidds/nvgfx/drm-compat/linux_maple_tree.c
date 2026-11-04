/*
    Copyright 2026, The AROS Development Team. All rights reserved.
*/

#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/maple_tree.h>

/* The caller serialises the tree (MT_FLAGS_LOCK_EXTERN); nothing is locked here. */

struct maple_range {
    struct list_head node;
    unsigned long first;
    unsigned long last;
    void *entry;
};

/* the range holding index, else NULL with *gap_prev the range before index */
static struct maple_range *range_at(struct maple_tree *mt, unsigned long index,
    struct maple_range **gap_prev)
{
    struct maple_range *r, *prev = NULL;

    list_for_each_entry(r, &mt->ma_ranges, node) {
        if (r->last < index) {
            prev = r;
            continue;
        }
        if (r->first <= index) {
            if (gap_prev)
                *gap_prev = NULL;
            return r;
        }
        break;
    }
    if (gap_prev)
        *gap_prev = prev;
    return NULL;
}

/*
 * The entry whose range holds mas->index: index/last become that range.
 * Without one they become the empty range around the index, so that the
 * caller can see how much room there is.
 */
void *mas_walk(struct ma_state *mas)
{
    struct maple_range *r, *prev;

    r = range_at(mas->tree, mas->index, &prev);
    if (r) {
        mas->index = r->first;
        mas->last = r->last;
        return r->entry;
    }

    mas->index = prev ? prev->last + 1 : 0;
    if (prev && !list_is_last(&prev->node, &mas->tree->ma_ranges))
        mas->last = list_next_entry(prev, node)->first - 1;
    else if (!prev && !list_empty(&mas->tree->ma_ranges))
        mas->last = list_first_entry(&mas->tree->ma_ranges, struct maple_range, node)->first - 1;
    else
        mas->last = ULONG_MAX;
    return NULL;
}

/* stores entry for [index, last]; ranges that it overlaps are dropped */
int mas_store_gfp(struct ma_state *mas, void *entry, gfp_t gfp)
{
    struct maple_tree *mt = mas->tree;
    struct maple_range *r, *tmp, *nr;

    list_for_each_entry_safe(r, tmp, &mt->ma_ranges, node) {
        if (r->last < mas->index)
            continue;
        if (r->first > mas->last)
            break;
        list_del(&r->node);
        kfree(r);
    }

    if (!entry)
        return 0;

    nr = kzalloc(sizeof(*nr), gfp);
    if (!nr)
        return -ENOMEM;
    nr->first = mas->index;
    nr->last = mas->last;
    nr->entry = entry;

    list_for_each_entry(r, &mt->ma_ranges, node) {
        if (r->first > nr->first) {
            list_add_tail(&nr->node, &r->node);
            return 0;
        }
    }
    list_add_tail(&nr->node, &mt->ma_ranges);
    return 0;
}

/* drops the range holding mas->index and returns its entry */
void *mas_erase(struct ma_state *mas)
{
    struct maple_range *r = range_at(mas->tree, mas->index, NULL);
    void *entry;

    if (!r)
        return NULL;
    mas->index = r->first;
    mas->last = r->last;
    entry = r->entry;
    list_del(&r->node);
    kfree(r);
    return entry;
}

/*
 * The first entry at or after the walk position, not starting beyond max.
 * A find after a find goes on from the range returned.
 */
void *mas_find(struct ma_state *mas, unsigned long max)
{
    unsigned long start = mas->index;
    struct maple_range *r;

    if (mas->active) {
        if (mas->last == ULONG_MAX)
            return NULL;
        start = mas->last + 1;
    }

    list_for_each_entry(r, &mas->tree->ma_ranges, node) {
        if (r->last < start)
            continue;
        if (r->first > max)
            break;
        mas->index = r->first;
        mas->last = r->last;
        mas->active = true;
        return r->entry;
    }

    mas->index = start;
    mas->last = max;
    mas->active = true;
    return NULL;
}

void __mt_destroy(struct maple_tree *mt)
{
    struct maple_range *r, *tmp;

    list_for_each_entry_safe(r, tmp, &mt->ma_ranges, node) {
        list_del(&r->node);
        kfree(r);
    }
}
