/*
    Copyright 2009-2026, The AROS Development Team. All rights reserved.
*/

#ifndef _LINUX_IOSYS_MAP_H_
#define _LINUX_IOSYS_MAP_H_

#include <string.h>
#include <linux/types.h>
#include <linux/io.h>

/*
 * A pointer that may be either system memory or an I/O mapping; the
 * accessors pick the matching copy routine.
 */
struct iosys_map {
    union {
        void __iomem *vaddr_iomem;
        void *vaddr;
    };
    bool is_iomem;
};
#define IOSYS_MAP_INIT_VADDR(vaddr_)        { { .vaddr = (vaddr_) }, .is_iomem = false }
#define IOSYS_MAP_INIT_VADDR_IOMEM(v_)      { { .vaddr_iomem = (v_) }, .is_iomem = true }
#define IOSYS_MAP_INIT_OFFSET(map_, offset_) ({ struct iosys_map copy_ = *(map_); iosys_map_incr(&copy_, offset_); copy_; })

static inline void iosys_map_set_vaddr(struct iosys_map *map, void *vaddr)               { map->vaddr = vaddr; map->is_iomem = false; }
static inline void iosys_map_set_vaddr_iomem(struct iosys_map *map, void __iomem *vaddr) { map->vaddr_iomem = vaddr; map->is_iomem = true; }
static inline bool iosys_map_is_equal(const struct iosys_map *lhs, const struct iosys_map *rhs)
{
    if (lhs->is_iomem != rhs->is_iomem)
        return false;
    return lhs->is_iomem ? lhs->vaddr_iomem == rhs->vaddr_iomem : lhs->vaddr == rhs->vaddr;
}
static inline bool iosys_map_is_null(const struct iosys_map *map)   { return map->is_iomem ? !map->vaddr_iomem : !map->vaddr; }
static inline bool iosys_map_is_set(const struct iosys_map *map)    { return !iosys_map_is_null(map); }
static inline void iosys_map_clear(struct iosys_map *map)           { map->vaddr = NULL; map->is_iomem = false; }
static inline void iosys_map_memcpy_to(struct iosys_map *dst, size_t dst_offset, const void *src, size_t len)
{
    if (dst->is_iomem)
        memcpy_toio((char __iomem *)dst->vaddr_iomem + dst_offset, src, len);
    else
        memcpy((char *)dst->vaddr + dst_offset, src, len);
}
static inline void iosys_map_memcpy_from(void *dst, const struct iosys_map *src, size_t src_offset, size_t len)
{
    if (src->is_iomem)
        memcpy_fromio(dst, (const char __iomem *)src->vaddr_iomem + src_offset, len);
    else
        memcpy(dst, (const char *)src->vaddr + src_offset, len);
}
static inline void iosys_map_incr(struct iosys_map *map, size_t incr)
{
    if (map->is_iomem)
        map->vaddr_iomem = (char __iomem *)map->vaddr_iomem + incr;
    else
        map->vaddr = (char *)map->vaddr + incr;
}
static inline void iosys_map_memset(struct iosys_map *dst, size_t offset, int value, size_t len)
{
    if (dst->is_iomem)
        memset_io((char __iomem *)dst->vaddr_iomem + offset, value, len);
    else
        memset((char *)dst->vaddr + offset, value, len);
}
#define iosys_map_rd(map__, offset__, type__) ({ type__ val; iosys_map_memcpy_from(&val, map__, offset__, sizeof(val)); val; })
#define iosys_map_wr(map__, offset__, type__, val__) ({ type__ val = (val__); iosys_map_memcpy_to(map__, offset__, &val, sizeof(val)); })
#define iosys_map_rd_field(map__, struct_offset__, struct_type__, field__) \
    iosys_map_rd(map__, struct_offset__ + offsetof(struct_type__, field__), typeof(((struct_type__ *)0)->field__))
#define iosys_map_wr_field(map__, struct_offset__, struct_type__, field__, val__) \
    iosys_map_wr(map__, struct_offset__ + offsetof(struct_type__, field__), typeof(((struct_type__ *)0)->field__), val__)

#endif /* _LINUX_IOSYS_MAP_H_ */
