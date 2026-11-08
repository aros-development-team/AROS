/*
    Copyright 2009-2026, The AROS Development Team. All rights reserved.
*/

#ifndef _LINUX_AGP_BACKEND_H_
#define _LINUX_AGP_BACKEND_H_

#include <linux/types.h>
#include <linux/mm.h>
#include <linux/list.h>
#include <linux/pci.h>

struct agp_version {
    u16 major;
    u16 minor;
};
struct agp_kern_info {
    struct agp_version version;
    struct pci_dev *device;
    unsigned long mode;
    unsigned long aper_base;
    size_t aper_size;
    int max_memory;
    int current_memory;
    bool cant_use_aperture;
    unsigned long page_mask;
    const struct vm_operations_struct *vm_ops;
};
struct agp_bridge_data {
    IPTR agpbridgedevice;
    ULONG mode;
    IPTR aperturebase;
    ULONG aperturesize;
    struct pci_dev *dev;
};
struct agp_memory {
    struct agp_memory *next;
    struct agp_memory *prev;
    struct agp_bridge_data *bridge;
    struct page **pages;
    size_t page_count;
    int key;
    int num_scratch_pages;
    off_t pg_start;
    u32 type;
    u32 physical;
    bool is_bound;
    bool is_flushed;
};
#define AGP_NORMAL_MEMORY       0
#define AGP_USER_TYPES          (1 << 16)
#define AGP_USER_MEMORY         (AGP_USER_TYPES)
#define AGP_USER_CACHED_MEMORY  (AGP_USER_TYPES + 1)
#define AGP3_RESERVED_MASK      0x00ff00c4
#define AGP2_RESERVED_MASK      0x00fffcc8
#define AGPSTAT_MODE_3_0        (1 << 3)
#define AGPSTAT_RQ_DEPTH        (0xff000000)
#define AGPSTAT_SBA             (1 << 9)
#define AGPSTAT_AGP             (1 << 8)
#define AGPSTAT_FW              (1 << 4)
#define AGPSTAT2_1X             (1 << 0)
#define AGPSTAT2_2X             (1 << 1)
#define AGPSTAT2_4X             (1 << 2)
#define AGPSTAT3_8X             (1 << 1)
#define AGPSTAT3_4X             (1 << 0)
struct agp_bridge_data *agp_backend_acquire(struct pci_dev *dev);
void agp_backend_release(struct agp_bridge_data *bridge);
struct agp_bridge_data *agp_find_bridge(struct pci_dev *dev);
int agp_copy_info(struct agp_bridge_data *bridge, struct agp_kern_info *info);
void agp_enable(struct agp_bridge_data *bridge, u32 mode);
struct agp_memory *agp_allocate_memory(struct agp_bridge_data *bridge, size_t num_pages, u32 type);
void agp_free_memory(struct agp_memory *mem);
int agp_bind_memory(struct agp_memory *mem, off_t offset);
int agp_unbind_memory(struct agp_memory *mem);
void agp_flush_chipset(struct agp_bridge_data *bridge);

#endif /* _LINUX_AGP_BACKEND_H_ */
