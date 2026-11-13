/*
    Copyright 2026, The AROS Development Team. All rights reserved.
*/

#include <linux/radix-tree.h>
#include <linux/slab.h>
#include <linux/errno.h>
#include <linux/bitops.h>

static unsigned long radix_max_index(int height)
{
    unsigned int bits = height * RADIX_TREE_MAP_SHIFT;

    if (bits >= sizeof(unsigned long) * 8)
        return ~0UL;
    return (1UL << bits) - 1;
}

static unsigned int radix_pos(unsigned long index, int level)
{
    return (index >> (level * RADIX_TREE_MAP_SHIFT)) & RADIX_TREE_MAP_MASK;
}

static struct radix_tree_node *radix_node_alloc(const struct radix_tree_root *root)
{
    return kzalloc(sizeof(struct radix_tree_node), root->gfp_mask ? root->gfp_mask : GFP_KERNEL);
}

static bool radix_node_any_tag(const struct radix_tree_node *node, unsigned int tag)
{
    int i;

    for (i = 0; i < RADIX_TREE_TAG_LONGS; i++)
        if (node->tags[tag][i])
            return true;
    return false;
}

void *radix_tree_lookup(const struct radix_tree_root *root, unsigned long index)
{
    struct radix_tree_node *node = root->rnode;
    int level;

    if (!node || index > radix_max_index(root->height))
        return NULL;
    for (level = root->height - 1; level > 0 && node; level--)
        node = node->slots[radix_pos(index, level)];
    return node ? node->slots[radix_pos(index, 0)] : NULL;
}

int radix_tree_insert(struct radix_tree_root *root, unsigned long index, void *item)
{
    struct radix_tree_node *node, *child;
    unsigned int pos, tag;
    int level;

    if (!item)
        return -EINVAL;

    if (!root->rnode)
    {
        root->rnode = radix_node_alloc(root);
        if (!root->rnode)
            return -ENOMEM;
        root->height = 1;
        while (index > radix_max_index(root->height))
            root->height++;
    }

    while (index > radix_max_index(root->height))
    {
        node = radix_node_alloc(root);
        if (!node)
            return -ENOMEM;
        node->slots[0] = root->rnode;
        node->count = 1;
        for (tag = 0; tag < RADIX_TREE_MAX_TAGS; tag++)
            if (radix_node_any_tag(root->rnode, tag))
                __set_bit(0, node->tags[tag]);
        root->rnode = node;
        root->height++;
    }

    node = root->rnode;
    for (level = root->height - 1; level > 0; level--)
    {
        pos = radix_pos(index, level);
        child = node->slots[pos];
        if (!child)
        {
            child = radix_node_alloc(root);
            if (!child)
                return -ENOMEM;
            node->slots[pos] = child;
            node->count++;
        }
        node = child;
    }

    pos = radix_pos(index, 0);
    if (node->slots[pos])
        return -EEXIST;
    node->slots[pos] = item;
    node->count++;
    return 0;
}

int radix_tree_store(struct radix_tree_root *root, unsigned long index, void **ppitem)
{
    void *old = radix_tree_lookup(root, index);

    if (!old)
        return radix_tree_insert(root, index, *ppitem);
    radix_tree_delete(root, index);
    return radix_tree_insert(root, index, *ppitem);
}

static void radix_root_retag(struct radix_tree_root *root)
{
    unsigned int tag;

    for (tag = 0; tag < RADIX_TREE_MAX_TAGS; tag++)
        root->tags[tag] = root->rnode && radix_node_any_tag(root->rnode, tag);
}

void *radix_tree_delete(struct radix_tree_root *root, unsigned long index)
{
    struct radix_tree_node *path[RADIX_TREE_MAX_HEIGHT];
    struct radix_tree_node *node = root->rnode;
    unsigned int tag, pos;
    void *item;
    int level;

    if (!node || index > radix_max_index(root->height))
        return NULL;

    for (level = root->height - 1; level >= 0; level--)
    {
        path[level] = node;
        if (level == 0)
            break;
        node = node->slots[radix_pos(index, level)];
        if (!node)
            return NULL;
    }

    pos = radix_pos(index, 0);
    item = node->slots[pos];
    if (!item)
        return NULL;

    for (level = 0; level < root->height; level++)
    {
        node = path[level];
        pos = radix_pos(index, level);
        if (level == 0)
        {
            node->slots[pos] = NULL;
            node->count--;
            for (tag = 0; tag < RADIX_TREE_MAX_TAGS; tag++)
                __clear_bit(pos, node->tags[tag]);
        }
        else
        {
            struct radix_tree_node *child = path[level - 1];

            if (child->count == 0)
            {
                kfree(child);
                node->slots[pos] = NULL;
                node->count--;
                for (tag = 0; tag < RADIX_TREE_MAX_TAGS; tag++)
                    __clear_bit(pos, node->tags[tag]);
            }
            else
            {
                for (tag = 0; tag < RADIX_TREE_MAX_TAGS; tag++)
                    if (!radix_node_any_tag(child, tag))
                        __clear_bit(pos, node->tags[tag]);
            }
        }
    }

    if (root->rnode->count == 0)
    {
        kfree(root->rnode);
        root->rnode = NULL;
        root->height = 0;
    }
    radix_root_retag(root);
    return item;
}

void *radix_tree_tag_set(struct radix_tree_root *root, unsigned long index, unsigned int tag)
{
    struct radix_tree_node *node = root->rnode;
    int level;

    if (!node || index > radix_max_index(root->height) || tag >= RADIX_TREE_MAX_TAGS)
        return NULL;
    if (!radix_tree_lookup(root, index))
        return NULL;
    for (level = root->height - 1; level >= 0; level--)
    {
        __set_bit(radix_pos(index, level), node->tags[tag]);
        if (level)
            node = node->slots[radix_pos(index, level)];
    }
    root->tags[tag] = 1;
    return node->slots[radix_pos(index, 0)];
}

void *radix_tree_tag_clear(struct radix_tree_root *root, unsigned long index, unsigned int tag)
{
    struct radix_tree_node *path[RADIX_TREE_MAX_HEIGHT];
    struct radix_tree_node *node = root->rnode;
    void *item;
    int level;

    if (!node || index > radix_max_index(root->height) || tag >= RADIX_TREE_MAX_TAGS)
        return NULL;
    item = radix_tree_lookup(root, index);
    if (!item)
        return NULL;

    for (level = root->height - 1; level >= 0; level--)
    {
        path[level] = node;
        if (level)
            node = node->slots[radix_pos(index, level)];
    }
    for (level = 0; level < root->height; level++)
    {
        if (level && radix_node_any_tag(path[level - 1], tag))
            break;
        __clear_bit(radix_pos(index, level), path[level]->tags[tag]);
    }
    radix_root_retag(root);
    return item;
}

int radix_tree_tagged(const struct radix_tree_root *root, unsigned int tag)
{
    return tag < RADIX_TREE_MAX_TAGS && root->tags[tag] != 0;
}

static bool radix_find_from(struct radix_tree_node *node, int level, unsigned long base,
                            unsigned long start, int tag, unsigned long *found, void ***pslot)
{
    unsigned long span = 1UL << (level * RADIX_TREE_MAP_SHIFT);
    unsigned int pos;

    for (pos = 0; pos < RADIX_TREE_MAP_SIZE; pos++)
    {
        unsigned long lo = base + pos * span;
        unsigned long hi = lo + span - 1;

        if (!node->slots[pos] || hi < start)
            continue;
        if (tag >= 0 && !test_bit(pos, node->tags[tag]))
            continue;
        if (level == 0)
        {
            *found = lo;
            *pslot = &node->slots[pos];
            return true;
        }
        if (radix_find_from(node->slots[pos], level - 1, lo, start, tag, found, pslot))
            return true;
    }
    return false;
}

bool radix_tree_iter_find(const struct radix_tree_root *root, struct radix_tree_iter *iter,
                          void ***pppslot, int flags)
{
    int tag = (flags & RADIX_TREE_ITER_TAGGED) ? (flags & RADIX_TREE_ITER_TAG_MASK) : -1;
    unsigned long found;

    if (!root->rnode || iter->index > radix_max_index(root->height))
        return false;
    if (!radix_find_from(root->rnode, root->height - 1, 0, iter->index, tag, &found, pppslot))
        return false;
    iter->index = found;
    return true;
}

void radix_tree_iter_delete(struct radix_tree_root *root, struct radix_tree_iter *iter, void **slot)
{
    radix_tree_delete(root, iter->index);
}

static unsigned int radix_gang(const struct radix_tree_root *root, void **results,
                               unsigned long first_index, unsigned int max_items, int flags)
{
    struct radix_tree_iter iter = { .index = first_index };
    unsigned int n = 0;
    void **slot;

    while (n < max_items && radix_tree_iter_find(root, &iter, &slot, flags))
    {
        results[n++] = *slot;
        if (iter.index == ~0UL)
            break;
        iter.index++;
    }
    return n;
}

unsigned int radix_tree_gang_lookup(const struct radix_tree_root *root, void **results,
                                    unsigned long first_index, unsigned int max_items)
{
    return radix_gang(root, results, first_index, max_items, 0);
}

unsigned int radix_tree_gang_lookup_tag(const struct radix_tree_root *root, void **results,
                                        unsigned long first_index, unsigned int max_items, unsigned int tag)
{
    return radix_gang(root, results, first_index, max_items, RADIX_TREE_ITER_TAGGED | tag);
}
