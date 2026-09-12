/*
    Copyright 2009-2026, The AROS Development Team. All rights reserved.
*/

#ifndef _LINUX_IO_MAPPING_H_
#define _LINUX_IO_MAPPING_H_

#include <linux/io.h>
#include <linux/slab.h>
#include <linux/mm.h>
#include <linux/bug.h>

struct io_mapping {
    resource_size_t base;
    unsigned long size;
    pgprot_t prot;
    void __iomem *iomem;
};

static inline struct io_mapping *io_mapping_init_wc(struct io_mapping *iomap, resource_size_t base, unsigned long size)
{
    iomap->base = base;
    iomap->size = size;
    iomap->iomem = ioremap_wc(base, size);
    if (!iomap->iomem)
        return NULL;
    return iomap;
}
static inline void io_mapping_fini(struct io_mapping *iomap)
{
    iounmap(iomap->iomem);
}
static inline struct io_mapping *io_mapping_create_wc(resource_size_t base, unsigned long size)
{
    struct io_mapping *iomap = kmalloc(sizeof(*iomap), GFP_KERNEL);
    if (!iomap)
        return NULL;
    if (!io_mapping_init_wc(iomap, base, size)) {
        kfree(iomap);
        return NULL;
    }
    return iomap;
}
static inline void io_mapping_free(struct io_mapping *iomap)
{
    io_mapping_fini(iomap);
    kfree(iomap);
}
static inline void __iomem *io_mapping_map_wc(struct io_mapping *mapping, unsigned long offset, unsigned long size)
{
    return (char __iomem *)mapping->iomem + offset;
}
static inline void io_mapping_unmap(void __iomem *vaddr) { }
#define io_mapping_map_atomic_wc(m, o)      io_mapping_map_wc(m, o, PAGE_SIZE)
#define io_mapping_unmap_atomic(a)          do { } while (0)
#define io_mapping_map_local_wc(m, o)       io_mapping_map_wc(m, o, PAGE_SIZE)
#define io_mapping_unmap_local(a)           do { } while (0)

#endif /* _LINUX_IO_MAPPING_H_ */
