/*
    Copyright 2026, The AROS Development Team. All rights reserved.

    Desc: The DRM range allocator, on a sorted list.

    Linux keeps its nodes in augmented interval and hole trees so that a
    large address space can be searched quickly. The two users here - the
    VRAM range manager and the command buffer space pool - hold at most a
    few hundred nodes, so a list ordered by address, walked in full, is
    enough and keeps the augmented tree machinery out of the port. The
    node layout and the semantics the inline helpers in drm_mm.h rely on
    (hole_size, hole_stack, node_list, the head node sentinel) are kept.
*/

#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/list.h>

#include <drm/drm_mm.h>
#include <drm/drm_print.h>

static inline u64 mod64(u64 a, u64 b)
{
    return a - (a / b) * b;
}

static inline u64 hole_start_of(const struct drm_mm_node *node)
{
    return __drm_mm_hole_node_start(node);
}

static inline u64 hole_end_of(const struct drm_mm_node *node)
{
    return __drm_mm_hole_node_end(node);
}

static void hole_track(struct drm_mm *mm, struct drm_mm_node *node)
{
    if (node->hole_size) {
        if (list_empty(&node->hole_stack))
            list_add(&node->hole_stack, &mm->hole_stack);
    } else {
        if (!list_empty(&node->hole_stack))
            list_del_init(&node->hole_stack);
    }
}

/* place node after hole_node, whose hole it takes a piece of */
static void insert_after(struct drm_mm *mm, struct drm_mm_node *hole_node, struct drm_mm_node *node)
{
    u64 hole_end = hole_end_of(hole_node);

    node->mm = mm;
    list_add(&node->node_list, &hole_node->node_list);
    INIT_LIST_HEAD(&node->hole_stack);

    node->hole_size = hole_end - (node->start + node->size);
    hole_node->hole_size = node->start - hole_start_of(hole_node);
    hole_track(mm, node);
    hole_track(mm, hole_node);

    set_bit(DRM_MM_NODE_ALLOCATED_BIT, &node->flags);
}

int drm_mm_reserve_node(struct drm_mm *mm, struct drm_mm_node *node)
{
    struct drm_mm_node *hole;
    u64 end = node->start + node->size;

    if (!node->size || end < node->start)
        return -EINVAL;

    list_for_each_entry(hole, &mm->hole_stack, hole_stack) {
        u64 hs = hole_start_of(hole), he = hole_end_of(hole);
        u64 adj_start = hs, adj_end = he;

        if (mm->color_adjust)
            mm->color_adjust(hole, node->color, &adj_start, &adj_end);

        if (node->start >= adj_start && end <= adj_end) {
            insert_after(mm, hole, node);
            return 0;
        }
    }
    return -ENOSPC;
}

int drm_mm_insert_node_in_range(struct drm_mm *mm, struct drm_mm_node *node,
                                u64 size, u64 alignment, unsigned long color,
                                u64 range_start, u64 range_end,
                                enum drm_mm_insert_mode mode)
{
    struct drm_mm_node *hole, *best_hole = NULL;
    u64 best_start = 0, best_size = 0;
    u64 remainder_mask;

    if (!size || range_end < range_start)
        return -EINVAL;
    if (range_end - range_start < size)
        return -ENOSPC;
    if (alignment <= 1)
        alignment = 0;
    remainder_mask = is_power_of_2(alignment) ? alignment - 1 : 0;

    /*
     * Every node may be followed by a hole, the sentinel included: its
     * hole is the free space before the first real node.
     */
    for (hole = &mm->head_node; ; hole = list_next_entry(hole, node_list)) {
        u64 hs, he, adj_start, adj_end, col_start, col_end;

        if (!hole->hole_size)
            goto next;
        hs = hole_start_of(hole);
        he = hole_end_of(hole);

        col_start = hs;
        col_end = he;
        if (mm->color_adjust)
            mm->color_adjust(hole, color, &col_start, &col_end);

        adj_start = max(col_start, range_start);
        adj_end = min(col_end, range_end);
        if (adj_end < adj_start || adj_end - adj_start < size)
            goto next;

        if (mode == DRM_MM_INSERT_HIGH) {
            adj_start = adj_end - size;
            if (alignment) {
                u64 rem = remainder_mask ? (adj_start & remainder_mask) : mod64(adj_start, alignment);
                if (rem) {
                    if (adj_start < rem)
                        goto next;
                    adj_start -= rem;
                }
            }
        } else if (alignment) {
            u64 rem = remainder_mask ? (adj_start & remainder_mask) : mod64(adj_start, alignment);
            if (rem)
                adj_start += alignment - rem;
        }
        if (adj_start < max(col_start, range_start) || adj_start + size > adj_end)
            goto next;

        if (mode == DRM_MM_INSERT_BEST) {
            if (!best_hole || (he - hs) < best_size) {
                best_hole = hole;
                best_start = adj_start;
                best_size = he - hs;
            }
        } else if (mode == DRM_MM_INSERT_HIGH) {
            /* the last fitting hole wins */
            best_hole = hole;
            best_start = adj_start;
        } else {
            best_hole = hole;
            best_start = adj_start;
            break;
        }
next:
        if (list_next_entry(hole, node_list) == &mm->head_node)
            break;
    }

    if (!best_hole)
        return -ENOSPC;

    node->start = best_start;
    node->size = size;
    node->color = color;
    insert_after(mm, best_hole, node);
    return 0;
}

void drm_mm_remove_node(struct drm_mm_node *node)
{
    struct drm_mm *mm = node->mm;
    struct drm_mm_node *prev = list_prev_entry(node, node_list);

    prev->hole_size += node->size + node->hole_size;
    if (!list_empty(&node->hole_stack))
        list_del_init(&node->hole_stack);
    list_del(&node->node_list);
    hole_track(mm, prev);

    clear_bit(DRM_MM_NODE_ALLOCATED_BIT, &node->flags);
    node->hole_size = 0;
}

struct drm_mm_node *__drm_mm_interval_first(const struct drm_mm *mm, u64 start, u64 last)
{
    struct drm_mm_node *node;

    drm_mm_for_each_node(node, mm) {
        if (node->start <= last && node->start + node->size > start)
            return node;
    }
    return (struct drm_mm_node *)&mm->head_node;
}

void drm_mm_init(struct drm_mm *mm, u64 start, u64 size)
{
    mm->color_adjust = NULL;
    INIT_LIST_HEAD(&mm->hole_stack);
    mm->scan_active = 0;

    /* the sentinel: its "hole" is the whole range */
    memset(&mm->head_node, 0, sizeof(mm->head_node));
    INIT_LIST_HEAD(&mm->head_node.node_list);
    INIT_LIST_HEAD(&mm->head_node.hole_stack);
    mm->head_node.mm = mm;
    mm->head_node.start = start + size;
    mm->head_node.size = start - mm->head_node.start;
    mm->head_node.hole_size = size;
    hole_track(mm, &mm->head_node);
}

void drm_mm_takedown(struct drm_mm *mm)
{
    if (WARN(!drm_mm_clean(mm), "Memory manager not clean during takedown.\n"))
        return;
}

void drm_mm_print(const struct drm_mm *mm, struct drm_printer *p)
{
    const struct drm_mm_node *entry;
    u64 total_used = 0, total_free = 0;

    if (mm->head_node.hole_size) {
        drm_printf(p, "%#018llx-%#018llx: %llu: free\n",
                   hole_start_of(&mm->head_node), hole_end_of(&mm->head_node), mm->head_node.hole_size);
        total_free += mm->head_node.hole_size;
    }
    drm_mm_for_each_node(entry, mm) {
        drm_printf(p, "%#018llx-%#018llx: %llu: used\n", entry->start, entry->start + entry->size, entry->size);
        total_used += entry->size;
        if (entry->hole_size) {
            drm_printf(p, "%#018llx-%#018llx: %llu: free\n",
                       hole_start_of(entry), hole_end_of(entry), entry->hole_size);
            total_free += entry->hole_size;
        }
    }
    drm_printf(p, "total: %llu, used %llu free %llu\n", total_used + total_free, total_used, total_free);
}

/* the eviction roaster is not used by either range manager here */
void drm_mm_scan_init_with_range(struct drm_mm_scan *scan, struct drm_mm *mm, u64 size, u64 alignment,
                                 unsigned long color, u64 start, u64 end, enum drm_mm_insert_mode mode)
{
    memset(scan, 0, sizeof(*scan));
    scan->mm = mm;
}

bool drm_mm_scan_add_block(struct drm_mm_scan *scan, struct drm_mm_node *node)
{
    return false;
}

bool drm_mm_scan_remove_block(struct drm_mm_scan *scan, struct drm_mm_node *node)
{
    return false;
}

struct drm_mm_node *drm_mm_scan_color_evict(struct drm_mm_scan *scan)
{
    return NULL;
}
