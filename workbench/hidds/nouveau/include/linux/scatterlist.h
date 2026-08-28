/*
    Copyright 2009-2026, The AROS Development Team. All rights reserved.
*/

#ifndef _LINUX_SCATTERLIST_H_
#define _LINUX_SCATTERLIST_H_

#include <linux/types.h>
#include <linux/mm.h>
#include <linux/bug.h>
#include <linux/string.h>
#include <linux/types.h>

/*
 * A scatterlist entry describes one run of pages plus, once mapped, the
 * bus address the device sees. Chaining is not needed: tables are one
 * flat array.
 */
struct scatterlist {
    struct page *page;
    unsigned int offset;
    unsigned int length;
    dma_addr_t dma_address;
    unsigned int dma_length;
    unsigned int end:1;
};

struct sg_table {
    struct scatterlist *sgl;
    unsigned int nents;
    unsigned int orig_nents;
};

#define sg_dma_address(sg)      ((sg)->dma_address)
#define sg_dma_len(sg)          ((sg)->dma_length)
#define sg_is_last(sg)          ((sg)->end)
#define sg_is_chain(sg)         (0)
#define sg_dma_is_bus_address(sg) (0)
#define sg_dma_mark_bus_address(sg) do { } while (0)
#define sg_dma_unmark_bus_address(sg) do { } while (0)
#define SG_CHUNK_SIZE           128
#define SG_MAX_SINGLE_ALLOC     (PAGE_SIZE / sizeof(struct scatterlist))

static inline void sg_assign_page(struct scatterlist *sg, struct page *page) { sg->page = page; }
static inline void sg_set_page(struct scatterlist *sg, struct page *page, unsigned int len, unsigned int offset)
{
    sg->page = page;
    sg->offset = offset;
    sg->length = len;
}
static inline void sg_set_buf(struct scatterlist *sg, const void *buf, unsigned int buflen)
{
    sg_set_page(sg, virt_to_page(buf), buflen, offset_in_page(buf));
}
static inline struct page *sg_page(struct scatterlist *sg)   { return sg->page; }
static inline void *sg_virt(struct scatterlist *sg)          { return (char *)page_address(sg->page) + sg->offset; }
static inline dma_addr_t sg_phys(struct scatterlist *sg)     { return page_to_phys(sg->page) + sg->offset; }
static inline struct scatterlist *sg_next(struct scatterlist *sg) { return sg->end ? NULL : sg + 1; }
static inline void sg_mark_end(struct scatterlist *sg)       { sg->end = 1; }
static inline void sg_unmark_end(struct scatterlist *sg)     { sg->end = 0; }
static inline void sg_init_table(struct scatterlist *sgl, unsigned int nents)
{
    memset(sgl, 0, sizeof(*sgl) * nents);
    sg_mark_end(&sgl[nents - 1]);
}
static inline void sg_init_one(struct scatterlist *sg, const void *buf, unsigned int buflen)
{
    sg_init_table(sg, 1);
    sg_set_buf(sg, buf, buflen);
}
static inline struct scatterlist *sg_last(struct scatterlist *sgl, unsigned int nents) { return &sgl[nents - 1]; }
int  sg_alloc_table(struct sg_table *table, unsigned int nents, gfp_t gfp_mask);
void sg_free_table(struct sg_table *table);
int  sg_alloc_table_from_pages(struct sg_table *sgt, struct page **pages, unsigned int n_pages,
        unsigned int offset, unsigned long size, gfp_t gfp_mask);
#define sg_alloc_table_from_pages_segment(sgt, p, n, o, s, m, g) sg_alloc_table_from_pages(sgt, p, n, o, s, g)
#define __sg_alloc_table_from_pages(sgt, p, n, o, s, m, prv, l, g) (sg_alloc_table_from_pages(sgt, p, n, o, s, g) ? ERR_PTR(-ENOMEM) : (sgt)->sgl)
size_t sg_copy_from_buffer(struct scatterlist *sgl, unsigned int nents, const void *buf, size_t buflen);
size_t sg_copy_to_buffer(struct scatterlist *sgl, unsigned int nents, void *buf, size_t buflen);
static inline void sg_dma_mark_swiotlb(struct scatterlist *sg) { }
static inline bool sg_dma_is_swiotlb(struct scatterlist *sg)   { return false; }
#define sg_zero_buffer(sgl, n, len, skip) do { } while (0)

#define for_each_sg(sglist, sg, nr, __i) \
    for (__i = 0, sg = (sglist); __i < (nr); __i++, sg = sg_next(sg))
#define for_each_sgtable_sg(sgt, sg, i)     for_each_sg((sgt)->sgl, sg, (sgt)->orig_nents, i)
#define for_each_sgtable_dma_sg(sgt, sg, i) for_each_sg((sgt)->sgl, sg, (sgt)->nents, i)

/* page-by-page walk of a table, in CPU or DMA space */
struct sg_page_iter {
    struct scatterlist *sg;
    unsigned int sg_pgoffset;
    unsigned int __nents;
    int __pg_advance;
};
struct sg_dma_page_iter {
    struct sg_page_iter base;
};
static inline struct page *sg_page_iter_page(struct sg_page_iter *piter)
{
    return nth_page(sg_page(piter->sg), piter->sg_pgoffset);
}
static inline dma_addr_t sg_page_iter_dma_address(struct sg_dma_page_iter *dma_iter)
{
    return sg_dma_address(dma_iter->base.sg) + ((dma_addr_t)dma_iter->base.sg_pgoffset << PAGE_SHIFT);
}
void __sg_page_iter_start(struct sg_page_iter *piter, struct scatterlist *sglist, unsigned int nents, unsigned long pgoffset);
bool __sg_page_iter_next(struct sg_page_iter *piter);
bool __sg_page_iter_dma_next(struct sg_dma_page_iter *dma_iter);
#define for_each_sg_page(sglist, piter, nents, pgoffset) \
    for (__sg_page_iter_start((piter), (sglist), (nents), (pgoffset)); __sg_page_iter_next(piter);)
#define for_each_sgtable_page(sgt, piter, pgoffset) for_each_sg_page((sgt)->sgl, piter, (sgt)->orig_nents, pgoffset)
#define for_each_sg_dma_page(sglist, dma_iter, dma_nents, pgoffset) \
    for (__sg_page_iter_start(&(dma_iter)->base, sglist, dma_nents, pgoffset); __sg_page_iter_dma_next(dma_iter);)
#define for_each_sgtable_dma_page(sgt, dma_iter, pgoffset) for_each_sg_dma_page((sgt)->sgl, dma_iter, (sgt)->nents, pgoffset)

#endif /* _LINUX_SCATTERLIST_H_ */
