/*
    Copyright 2009-2026, The AROS Development Team. All rights reserved.
*/

#include <aros/debug.h>
#include <proto/exec.h>
#include <exec/memory.h>

#include <stdio.h>
#include <stdarg.h>
#include <string.h>

#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/mm.h>
#include <linux/device.h>
#include <linux/dma-mapping.h>

#include <drm-compat/drm_compat_mem.h>

/*
 * Small allocations come from the driver's memory pool (zeroed by the pool
 * flags, so __GFP_ZERO is free). Page-granular allocations - pages,
 * vmalloc, coherent DMA - are made with AllocMem, page aligned, and the
 * raw pointer is kept in the word below the aligned block so it can be
 * freed again.
 */

void *kmalloc(size_t size, gfp_t flags)
{
    void *p;

    if (size == 0)
        size = 1;

    p = HIDDNouveauAlloc(size);
    return p;
}

void kfree(const void *p)
{
    if (p && p != ZERO_SIZE_PTR)
        HIDDNouveauFree((APTR)p);
}

size_t ksize(const void *p)
{
    if (!p || p == ZERO_SIZE_PTR)
        return 0;
    return HIDDNouveauAllocSize(p);
}

void *krealloc(const void *p, size_t new_size, gfp_t flags)
{
    void *np;
    size_t old;

    if (!p)
        return kmalloc(new_size, flags);
    if (new_size == 0) {
        kfree(p);
        return ZERO_SIZE_PTR;
    }
    old = ksize(p);
    if (new_size <= old)
        return (void *)p;
    np = kmalloc(new_size, flags);
    if (!np)
        return NULL;
    memcpy(np, p, old);
    kfree(p);
    return np;
}

void *kmemdup(const void *src, size_t len, gfp_t gfp)
{
    void *p = kmalloc(len, gfp);
    if (p)
        memcpy(p, src, len);
    return p;
}

char *kstrdup(const char *s, gfp_t gfp)
{
    size_t len;
    char *p;

    if (!s)
        return NULL;
    len = strlen(s) + 1;
    p = kmalloc(len, gfp);
    if (p)
        memcpy(p, s, len);
    return p;
}

char *kstrndup(const char *s, size_t max, gfp_t gfp)
{
    size_t len;
    char *p;

    if (!s)
        return NULL;
    len = strnlen(s, max);
    p = kmalloc(len + 1, gfp);
    if (p) {
        memcpy(p, s, len);
        p[len] = 0;
    }
    return p;
}

char *kvasprintf(gfp_t gfp, const char *fmt, va_list ap)
{
    va_list aq;
    int len;
    char *p;

    va_copy(aq, ap);
    len = vsnprintf(NULL, 0, fmt, aq);
    va_end(aq);

    p = kmalloc(len + 1, gfp);
    if (p)
        vsnprintf(p, len + 1, fmt, ap);
    return p;
}

char *kasprintf(gfp_t gfp, const char *fmt, ...)
{
    va_list ap;
    char *p;

    va_start(ap, fmt);
    p = kvasprintf(gfp, fmt, ap);
    va_end(ap);
    return p;
}

/* kmem caches are only a size to remember */

struct kmem_cache *kmem_cache_create(const char *name, unsigned int size, unsigned int align, unsigned int flags, void (*ctor)(void *))
{
    struct kmem_cache *s = kmalloc(sizeof(*s), GFP_KERNEL);
    if (s) {
        s->name = name;
        s->size = size;
        s->ctor = ctor;
    }
    return s;
}

void kmem_cache_destroy(struct kmem_cache *s)
{
    kfree(s);
}

void *kmem_cache_alloc(struct kmem_cache *s, gfp_t flags)
{
    void *p = kmalloc(s->size, flags);
    if (p && s->ctor)
        s->ctor(p);
    return p;
}

void *kmem_cache_zalloc(struct kmem_cache *s, gfp_t flags)
{
    return kmem_cache_alloc(s, flags | __GFP_ZERO);
}

void kmem_cache_free(struct kmem_cache *s, void *p)
{
    kfree(p);
}

/* devm: nothing ever unbinds, so these are the plain calls */

void *devm_kmalloc(struct device *dev, size_t size, gfp_t gfp)
{
    return kmalloc(size, gfp);
}

void devm_kfree(struct device *dev, const void *p)
{
    kfree(p);
}

char *devm_kasprintf(struct device *dev, gfp_t gfp, const char *fmt, ...)
{
    va_list ap;
    char *p;

    va_start(ap, fmt);
    p = kvasprintf(gfp, fmt, ap);
    va_end(ap);
    return p;
}

char *devm_kstrdup(struct device *dev, const char *s, gfp_t gfp)
{
    return kstrdup(s, gfp);
}

int devm_add_action(struct device *dev, void (*action)(void *), void *data)
{
    return 0;
}

int devm_add_action_or_reset(struct device *dev, void (*action)(void *), void *data)
{
    return 0;
}

void devm_remove_action(struct device *dev, void (*action)(void *), void *data)
{
}

void devm_release_action(struct device *dev, void (*action)(void *), void *data)
{
    action(data);
}

void *devres_alloc(dr_release_t release, size_t size, gfp_t gfp)
{
    return kzalloc(size, gfp);
}

void devres_add(struct device *dev, void *res)
{
}

void devres_free(void *res)
{
    kfree(res);
}

int devres_release(struct device *dev, dr_release_t release, void *match, void *match_data)
{
    return 0;
}

/* page granular memory */

static void *page_block_alloc(size_t size)
{
    UBYTE *raw, *aligned;

    raw = AllocMem(size + PAGE_SIZE + sizeof(void *), MEMF_PUBLIC | MEMF_CLEAR);
    if (!raw)
        return NULL;

    aligned = (UBYTE *)PAGE_ALIGN((IPTR)raw + sizeof(void *));
    ((void **)aligned)[-1] = raw;
    ((size_t *)raw)[0] = size;
    return aligned;
}

static void page_block_free(void *aligned)
{
    UBYTE *raw;
    size_t size;

    if (!aligned)
        return;
    raw = ((void **)aligned)[-1];
    size = ((size_t *)raw)[0];
    FreeMem(raw, size + PAGE_SIZE + sizeof(void *));
}

struct page *alloc_pages(gfp_t gfp, unsigned int order)
{
    return (struct page *)page_block_alloc(PAGE_SIZE << order);
}

void __free_pages(struct page *page, unsigned int order)
{
    page_block_free(page);
}

unsigned long __get_free_pages(gfp_t gfp, unsigned int order)
{
    return (unsigned long)page_block_alloc(PAGE_SIZE << order);
}

void free_pages(unsigned long addr, unsigned int order)
{
    page_block_free((void *)addr);
}

void *vmalloc(unsigned long size)
{
    return page_block_alloc(PAGE_ALIGN(size));
}

void *vzalloc(unsigned long size)
{
    return vmalloc(size);
}

void vfree(const void *addr)
{
    page_block_free((void *)addr);
}

void *vmap(struct page **pages, unsigned int count, unsigned long flags, pgprot_t prot)
{
    unsigned int i;

    if (!count)
        return NULL;
    for (i = 1; i < count; i++) {
        if ((IPTR)pages[i] != (IPTR)pages[i - 1] + PAGE_SIZE) {
            bug("[nouveau] vmap: %u pages are not contiguous, cannot map\n", count);
            return NULL;
        }
    }
    return page_address(pages[0]);
}

void vunmap(const void *addr)
{
}

struct page **compat_page_array(void *base, unsigned int npages)
{
    struct page **pages = kmalloc_array(npages, sizeof(*pages), GFP_KERNEL);
    unsigned int i;

    if (!pages)
        return NULL;
    for (i = 0; i < npages; i++)
        pages[i] = (struct page *)((UBYTE *)base + (i << PAGE_SHIFT));
    return pages;
}

unsigned long compat_totalram_pages(void)
{
    return AvailMem(MEMF_PUBLIC | MEMF_TOTAL) >> PAGE_SHIFT;
}

void si_meminfo(struct sysinfo *si)
{
    si->totalram = compat_totalram_pages();
    si->freeram = AvailMem(MEMF_PUBLIC) >> PAGE_SHIFT;
    si->totalhigh = 0;
    si->freehigh = 0;
    si->mem_unit = PAGE_SIZE;
}

/*
 * Coherent DMA memory: page aligned RAM plus its bus address. The
 * platform hook compat_dma_map does the translation and, where caches are
 * not snooped, the maintenance.
 */
struct coherent_block {
    struct list_head node;
    void *cpu;
    size_t size;
};
static LIST_HEAD(coherent_blocks);

void *dma_alloc_attrs(struct device *dev, size_t size, dma_addr_t *dma_handle, gfp_t gfp, unsigned long attrs)
{
    struct coherent_block *cb;
    void *cpu = page_block_alloc(PAGE_ALIGN(size));

    if (!cpu)
        return NULL;
    *dma_handle = compat_dma_map(dev, cpu, PAGE_ALIGN(size), DMA_BIDIRECTIONAL);

    cb = kmalloc(sizeof(*cb), GFP_KERNEL);
    if (cb) {
        cb->cpu = cpu;
        cb->size = PAGE_ALIGN(size);
        Forbid();
        list_add(&cb->node, &coherent_blocks);
        Permit();
    }
    return cpu;
}

void dma_free_attrs(struct device *dev, size_t size, void *cpu_addr, dma_addr_t dma_handle, unsigned long attrs)
{
    struct coherent_block *cb;

    Forbid();
    list_for_each_entry(cb, &coherent_blocks, node) {
        if (cb->cpu == cpu_addr) {
            list_del(&cb->node);
            kfree(cb);
            break;
        }
    }
    Permit();
    page_block_free(cpu_addr);
}

/*
 * The card is about to read structures the CPU filled in through cached
 * mappings: on a bus that does not snoop, clean them all first.
 */
void compat_dma_sync_all_coherent(void)
{
    struct coherent_block *cb;

    Forbid();
    list_for_each_entry(cb, &coherent_blocks, node)
        compat_dma_sync(NULL, cb->cpu, cb->size, DMA_TO_DEVICE, true);
    Permit();
}
