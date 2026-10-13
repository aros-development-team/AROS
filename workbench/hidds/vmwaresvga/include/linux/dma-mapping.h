/*
    Copyright 2009-2026, The AROS Development Team. All rights reserved.
*/

#ifndef _LINUX_DMA_MAPPING_H_
#define _LINUX_DMA_MAPPING_H_

#include <linux/types.h>
#include <linux/device.h>
#include <linux/dma-direction.h>
#include <linux/scatterlist.h>
#include <linux/mm.h>
#include <linux/string.h>
#include <linux/err.h>
#include <linux/bug.h>
#include <linux/mem_encrypt.h>

#define DMA_BIT_MASK(n)         (((n) == 64) ? ~0ULL : ((1ULL << (n)) - 1))
#define DMA_MAPPING_ERROR       (~(dma_addr_t)0)
#define DMA_ATTR_WEAK_ORDERING      (1UL << 1)
#define DMA_ATTR_WRITE_COMBINE      (1UL << 2)
#define DMA_ATTR_NO_KERNEL_MAPPING  (1UL << 4)
#define DMA_ATTR_SKIP_CPU_SYNC      (1UL << 5)
#define DMA_ATTR_FORCE_CONTIGUOUS   (1UL << 6)
#define DMA_ATTR_ALLOC_SINGLE_PAGES (1UL << 7)
#define DMA_ATTR_NO_WARN            (1UL << 8)
#define DMA_ATTR_PRIVILEGED         (1UL << 9)

/*
 * RAM is identity mapped and reachable by the card, so a mapping is a
 * translation to a bus address plus - where the platform needs it - a
 * cache maintenance step, which is what compat_dma_sync does.
 */
dma_addr_t compat_dma_map(struct device *dev, void *cpu, size_t size, enum dma_data_direction dir);
void compat_dma_sync(struct device *dev, void *cpu, size_t size, enum dma_data_direction dir, bool for_device);
void *compat_dma_bus_to_cpu(struct device *dev, dma_addr_t addr);
/* push every coherent allocation out of the CPU caches (non-snooping bus) */
void compat_dma_sync_all_coherent(void);
/* cache maintenance shortcuts for shared queues on such a bus */
#define compat_dma_sync_for_cpu(p, n)    compat_dma_sync(NULL, (void *)(p), (n), DMA_FROM_DEVICE, false)
#define compat_dma_sync_for_device(p, n) compat_dma_sync(NULL, (void *)(p), (n), DMA_TO_DEVICE, true)

static inline int dma_mapping_error(struct device *dev, dma_addr_t dma_addr) { return dma_addr == DMA_MAPPING_ERROR; }
static inline int dma_set_mask(struct device *dev, u64 mask)                { dev->dma_mask_storage = mask; dev->dma_mask = &dev->dma_mask_storage; return 0; }
static inline int dma_set_coherent_mask(struct device *dev, u64 mask)       { dev->coherent_dma_mask = mask; return 0; }
static inline int dma_set_mask_and_coherent(struct device *dev, u64 mask)   { dma_set_mask(dev, mask); return dma_set_coherent_mask(dev, mask); }
static inline u64 dma_get_mask(struct device *dev)                          { return dev->dma_mask ? *dev->dma_mask : DMA_BIT_MASK(32); }
static inline u64 dma_get_required_mask(struct device *dev)                 { return DMA_BIT_MASK(64); }
static inline int dma_supported(struct device *dev, u64 mask)               { return 1; }
static inline size_t dma_max_mapping_size(struct device *dev)               { return SIZE_MAX; }
static inline bool dma_addressing_limited(struct device *dev)               { return false; }
static inline unsigned int dma_get_max_seg_size(struct device *dev)         { return UINT_MAX; }
static inline int dma_set_max_seg_size(struct device *dev, unsigned int s)  { return 0; }
static inline int dma_set_seg_boundary(struct device *dev, unsigned long m) { return 0; }
static inline bool dev_is_dma_coherent(struct device *dev)                  { return true; }
static inline bool dma_can_mmap(struct device *dev)                         { return false; }
static inline bool dma_need_sync(struct device *dev, dma_addr_t a)          { return true; }
static inline bool dma_dev_need_sync(const struct device *dev)              { return true; }
#define dma_bits(mask)          ((mask) ? fls64(mask) : 32)

void *dma_alloc_attrs(struct device *dev, size_t size, dma_addr_t *dma_handle, gfp_t gfp, unsigned long attrs);
void  dma_free_attrs(struct device *dev, size_t size, void *cpu_addr, dma_addr_t dma_handle, unsigned long attrs);
static inline void *dma_alloc_coherent(struct device *dev, size_t size, dma_addr_t *dma_handle, gfp_t gfp)
{
    return dma_alloc_attrs(dev, size, dma_handle, gfp, 0);
}
static inline void dma_free_coherent(struct device *dev, size_t size, void *cpu_addr, dma_addr_t dma_handle)
{
    dma_free_attrs(dev, size, cpu_addr, dma_handle, 0);
}
static inline void *dma_alloc_wc(struct device *dev, size_t size, dma_addr_t *dma_addr, gfp_t gfp)
{
    return dma_alloc_attrs(dev, size, dma_addr, gfp, DMA_ATTR_WRITE_COMBINE);
}
static inline void dma_free_wc(struct device *dev, size_t size, void *cpu_addr, dma_addr_t dma_addr)
{
    dma_free_attrs(dev, size, cpu_addr, dma_addr, DMA_ATTR_WRITE_COMBINE);
}
static inline void *dma_alloc_noncoherent(struct device *dev, size_t size, dma_addr_t *dma_handle, enum dma_data_direction dir, gfp_t gfp)
{
    return dma_alloc_attrs(dev, size, dma_handle, gfp, 0);
}
static inline void dma_free_noncoherent(struct device *dev, size_t size, void *cpu_addr, dma_addr_t dma_handle, enum dma_data_direction dir)
{
    dma_free_attrs(dev, size, cpu_addr, dma_handle, 0);
}
static inline struct page *dma_alloc_pages(struct device *dev, size_t size, dma_addr_t *dma_handle, enum dma_data_direction dir, gfp_t gfp)
{
    void *cpu = dma_alloc_attrs(dev, size, dma_handle, gfp, 0);
    return cpu ? virt_to_page(cpu) : NULL;
}
static inline void dma_free_pages(struct device *dev, size_t size, struct page *page, dma_addr_t dma_handle, enum dma_data_direction dir)
{
    dma_free_attrs(dev, size, page_address(page), dma_handle, 0);
}

static inline dma_addr_t dma_map_single_attrs(struct device *dev, void *ptr, size_t size, enum dma_data_direction dir, unsigned long attrs)
{
    return compat_dma_map(dev, ptr, size, dir);
}
static inline void dma_unmap_single_attrs(struct device *dev, dma_addr_t addr, size_t size, enum dma_data_direction dir, unsigned long attrs)
{
    compat_dma_sync(dev, compat_dma_bus_to_cpu(dev, addr), size, dir, false);
}
static inline dma_addr_t dma_map_page_attrs(struct device *dev, struct page *page, size_t offset, size_t size, enum dma_data_direction dir, unsigned long attrs)
{
    return compat_dma_map(dev, (char *)page_address(page) + offset, size, dir);
}
static inline void dma_unmap_page_attrs(struct device *dev, dma_addr_t addr, size_t size, enum dma_data_direction dir, unsigned long attrs)
{
    compat_dma_sync(dev, compat_dma_bus_to_cpu(dev, addr), size, dir, false);
}
#define dma_map_single(d, p, s, r)          dma_map_single_attrs(d, p, s, r, 0)
#define dma_unmap_single(d, a, s, r)        dma_unmap_single_attrs(d, a, s, r, 0)
#define dma_map_page(d, p, o, s, r)         dma_map_page_attrs(d, p, o, s, r, 0)
#define dma_unmap_page(d, a, s, r)          dma_unmap_page_attrs(d, a, s, r, 0)
#define dma_map_resource(d, p, s, r, a)     ((dma_addr_t)(p))
#define dma_unmap_resource(d, a, s, r, at)  do { } while (0)
static inline void dma_sync_single_for_cpu(struct device *dev, dma_addr_t addr, size_t size, enum dma_data_direction dir)
{
    compat_dma_sync(dev, compat_dma_bus_to_cpu(dev, addr), size, dir, false);
}
static inline void dma_sync_single_for_device(struct device *dev, dma_addr_t addr, size_t size, enum dma_data_direction dir)
{
    compat_dma_sync(dev, compat_dma_bus_to_cpu(dev, addr), size, dir, true);
}
#define dma_sync_single_range_for_cpu(d, a, o, s, r)    dma_sync_single_for_cpu(d, (a) + (o), s, r)
#define dma_sync_single_range_for_device(d, a, o, s, r) dma_sync_single_for_device(d, (a) + (o), s, r)

int  dma_map_sg_attrs(struct device *dev, struct scatterlist *sg, int nents, enum dma_data_direction dir, unsigned long attrs);
void dma_unmap_sg_attrs(struct device *dev, struct scatterlist *sg, int nents, enum dma_data_direction dir, unsigned long attrs);
int  dma_map_sgtable(struct device *dev, struct sg_table *sgt, enum dma_data_direction dir, unsigned long attrs);
void dma_unmap_sgtable(struct device *dev, struct sg_table *sgt, enum dma_data_direction dir, unsigned long attrs);
void dma_sync_sg_for_cpu(struct device *dev, struct scatterlist *sg, int nelems, enum dma_data_direction dir);
void dma_sync_sg_for_device(struct device *dev, struct scatterlist *sg, int nelems, enum dma_data_direction dir);
#define dma_map_sg(d, s, n, r)              dma_map_sg_attrs(d, s, n, r, 0)
#define dma_unmap_sg(d, s, n, r)            dma_unmap_sg_attrs(d, s, n, r, 0)
#define dma_sync_sgtable_for_cpu(d, t, r)   dma_sync_sg_for_cpu(d, (t)->sgl, (t)->orig_nents, r)
#define dma_sync_sgtable_for_device(d, t, r) dma_sync_sg_for_device(d, (t)->sgl, (t)->orig_nents, r)
struct sg_table *dma_get_sgtable_attrs_stub(void);
#define dma_get_sgtable(d, t, c, a, s)      (-ENXIO)
#define dma_mmap_attrs(d, v, c, a, s, at)   (-ENXIO)
#define dma_mmap_wc(d, v, c, a, s)          (-ENXIO)
#define dma_mmap_coherent(d, v, c, a, s)    (-ENXIO)
#define dmam_alloc_coherent(d, s, h, g)     dma_alloc_coherent(d, s, h, g)
#define dmam_free_coherent(d, s, c, h)      dma_free_coherent(d, s, c, h)
#define dma_pool_create(n, d, s, a, b)      ((struct dma_pool *)NULL)
#define dma_pool_destroy(p)                 do { } while (0)
struct dma_pool;

#endif /* _LINUX_DMA_MAPPING_H_ */
