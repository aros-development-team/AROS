/*
    Copyright 2009-2010, The AROS Development Team. All rights reserved.
    $Id$
*/

/*-
 * This file incorporates work covered by the following copyright and
 * permission notice:
 *
 *  Copyright 1999 Precision Insight, Inc., Cedar Park, Texas.
 *  Copyright 2000 VA Linux Systems, Inc., Sunnyvale, California.
 *  All rights reserved.
 *
 *  Permission is hereby granted, free of charge, to any person obtaining a
 *  copy of this software and associated documentation files (the "Software"),
 *  to deal in the Software without restriction, including without limitation
 *  the rights to use, copy, modify, merge, publish, distribute, sublicense,
 *  and/or sell copies of the Software, and to permit persons to whom the
 *  Software is furnished to do so, subject to the following conditions:
 *
 *  The above copyright notice and this permission notice (including the next
 *  paragraph) shall be included in all copies or substantial portions of the
 *  Software.
 *
 *  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 *  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 *  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 *  VA LINUX SYSTEMS AND/OR ITS SUPPLIERS BE LIABLE FOR ANY CLAIM, DAMAGES OR
 *  OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 *  ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 *  OTHER DEALINGS IN THE SOFTWARE.
 *
 */

#if !defined(DRMP_H)
#define DRMP_H

#define DEBUG 0
#include <aros/debug.h>

#include <oop/oop.h>

#include "drm.h"
#include "drm_aros_config.h"
#include "drm_compat_funcs.h"
#include "drm_redefines.h"

#include <errno.h>
#include <stdio.h>
#include <string.h>

#include "drm_mm.h"

#define DRM_ERROR(fmt, ...) bug("[" DRM_NAME "(ERROR):%s] " fmt, __func__ , ##__VA_ARGS__)
#define DRM_DEBUG(fmt, ...) D(bug("[" DRM_NAME "(DEBUG):%s] " fmt, __func__ , ##__VA_ARGS__))
#define DRM_DEBUG_KMS(fmt, ...) D(bug("[" DRM_NAME "(DEBUG):%s] " fmt, __func__ , ##__VA_ARGS__))
#define DRM_IMPL(fmt, ...)  bug("------IMPLEMENT(%s): " fmt, __func__ , ##__VA_ARGS__)
#define DRM_INFO(fmt, ...)  bug("[" DRM_NAME "(INFO)] " fmt, ##__VA_ARGS__)
#define DRM_DEBUG_DRIVER    DRM_DEBUG

#define DRM_UT_CORE     0x01
#define DRM_UT_DRIVER   0x02
#define DRM_UT_KMS      0x04

extern unsigned int drm_debug;

/*
 * DRM_READMEMORYBARRIER() prevents reordering of reads.
 * DRM_WRITEMEMORYBARRIER() prevents reordering of writes.
 * DRM_MEMORYBARRIER() prevents reordering of reads and writes.
 */

#if defined(__i386__)
#define DRM_READMEMORYBARRIER()     __asm __volatile("lock; addl $0,0(%%esp)" : : : "memory");
#define DRM_WRITEMEMORYBARRIER()    __asm __volatile("" : : : "memory");
#define DRM_MEMORYBARRIER()         __asm __volatile("lock; addl $0,0(%%esp)" : : : "memory");
#elif defined(__x86_64__)
#define DRM_READMEMORYBARRIER()     __asm __volatile("lfence":::"memory");
#define DRM_WRITEMEMORYBARRIER()    __asm __volatile("sfence":::"memory");
#define DRM_MEMORYBARRIER()         __asm __volatile("mfence":::"memory");
#else
#error IMPLEMENT momory bariers for non-x86
#endif


#define DRM_COPY_FROM_USER(to, from, size)   copy_from_user(to, from, size)
#define DRM_COPY_TO_USER(to , from, size)    copy_to_user(to, from, size)
#define DRM_ARRAY_SIZE(x)   ARRAY_SIZE(x)
#define DRM_UDELAY(d)       udelay(d)
#define DRM_CURRENTPID      1
#define DRM_IRQ_ARGS        void *arg
#define DRM_SUSER(p)        capable(CAP_SYS_ADMIN)
typedef int                 irqreturn_t;
#define IRQ_NONE            1
#define IRQ_HANDLED         0

struct drm_file;
struct drm_device;
struct file;
struct drm_gem_object;

typedef int drm_ioctl_t(struct drm_device *dev, void *data,
            struct drm_file *file_priv);

#define DRM_AUTH            0x1
#define DRM_MASTER          0x2
#define DRM_ROOT_ONLY       0x4
#define DRM_CONTROL_ALLOW   0x8
#define DRM_UNLOCKED        0x10

struct drm_ioctl_desc 
{
    unsigned int cmd;
    int flags;
    drm_ioctl_t *func;
    unsigned int cmd_drv;
};

/*
 * Creates a driver or general drm_ioctl_desc array entry for the given
 * ioctl, for use by drm_ioctl().
 */
#define DRM_IOCTL_NR(n)     ((n) & 0xff)
#define DRM_IOCTL_DEF_DRV(ioctl, _func, _flags)			\
	[DRM_IOCTL_NR(DRM_##ioctl)] = {.cmd = DRM_##ioctl, .func = _func, .flags = _flags, .cmd_drv = DRM_IOCTL_##ioctl}


/* Memory management */
struct drm_local_map
{
    resource_size_t offset;         /* Requested physical address (0 for SAREA)*/
    unsigned long size;             /* Requested physical size (bytes) */
    enum drm_map_type type;         /* Type of memory to map */
    enum drm_map_flags flags;       /* Flags */
    void *handle;                   /* Memory address */
    int mtrr;                       /* MTRR slot used */
};

typedef struct drm_local_map drm_local_map_t;

struct drm_map_list
{
    struct list_head head;                  /* list head */
/* FIXME: Commented out fields */    
//    struct drm_hash_item hash;
    struct drm_local_map *map;              /* mapping */
    uint64_t user_token;
    struct drm_master *master;
    struct drm_mm_node *file_offset_node;   /* fake offset */
};

struct drm_sg_mem {
    unsigned long handle;
    void *virtual;
    int pages;
#if !defined(__AROS__)    
    struct page **pagelist;
#else
    void * buffer;
#endif    
    dma_addr_t *busaddr;
};

#define DRM_AGP_KERN struct agp_kern_info
#define DRM_AGP_MEM struct agp_memory 
struct drm_agp_mem {
    DRM_AGP_MEM *memory;
    unsigned long bound;
    int pages;
    struct list_head head;
};

struct drm_agp_head {
    DRM_AGP_KERN agp_info;          /* AGP device information */
    struct list_head memory;
    unsigned long mode;             /* AGP mode */
    struct agp_bridge_data *bridge;
    int enabled;                    /* whether the AGP bus as been enabled */
    int acquired;                   /* whether the AGP device has been acquired */
    unsigned long base;
    int agp_mtrr;
    int cant_use_aperture;
    unsigned long page_mask;
};

struct drm_pciid 
{
    UWORD VendorID;
    UWORD ProductID;
};

#include "drm_crtc.h"

/* Contains a collection of functions common to each drm driver */

#define DRIVER_USE_AGP     0x1
#define DRIVER_REQUIRE_AGP 0x2
#define DRIVER_GEM         0x1000
#define DRIVER_MODESET     0x2000

/* Size of ringbuffer for vblank timestamps. Just double-buffer
 * in initial implementation.
 */
#define DRM_VBLANKTIME_RBSIZE 2

/* Flags and return codes for get_vblank_timestamp() driver function. */
#define DRM_CALLED_FROM_VBLIRQ 1
#define DRM_VBLANKTIME_SCANOUTPOS_METHOD (1 << 0)
#define DRM_VBLANKTIME_INVBL             (1 << 1)

/* get_scanout_position() return flags */
#define DRM_SCANOUTPOS_VALID        (1 << 0)
#define DRM_SCANOUTPOS_INVBL        (1 << 1)
#define DRM_SCANOUTPOS_ACCURATE     (1 << 2)

struct drm_driver
{
    /* PCI */
    UWORD               VendorID;
    UWORD               ProductID;
    UWORD               SubSystemVendorID;
    UWORD               SubSystemProductID;
    struct drm_pciid    *PciIDs;
    OOP_Object          *pciDevice;
    BOOL                IsAGP;
    BOOL                IsPCIE;
    
    /* DRM device */
    struct drm_device   *dev;
    
    int         (*load)(struct drm_device *, unsigned long);
    int         (*firstopen)(struct drm_device *);
    int         (*open) (struct drm_device *, struct drm_file *);
    void        (*preclose)(struct drm_device *dev, struct drm_file *);
    void        (*postclose) (struct drm_device *, struct drm_file *);
    void        (*lastclose)(struct drm_device *);
    int         (*unload)(struct drm_device *);
    
    /* IRQ */
    irqreturn_t (*irq_handler)(DRM_IRQ_ARGS);
    void        (*irq_preinstall)(struct drm_device *);
    int         (*irq_postinstall)(struct drm_device *);
    void        (*irq_uninstall)(struct drm_device *);

    /* GEM */
    int         (*gem_init_object) (struct drm_gem_object *obj);
    void        (*gem_free_object) (struct drm_gem_object *obj);
    void        (*gem_free_object_unlocked) (struct drm_gem_object *obj);
    int         (*gem_open_object) (struct drm_gem_object *, struct drm_file *);
    void        (*gem_close_object) (struct drm_gem_object *, struct drm_file *);

    int                     version_patchlevel;
    unsigned int            driver_features;
    struct drm_ioctl_desc   *ioctls;
};

struct drm_device
{
    struct list_head maplist;       /* Linked list of regions */

    int irq_enabled;                /* True if irq handler is enabled */
    int pci_vendor;                 /* PCI vendor id */
    int pci_device;                 /* PCI device id */
    
    struct drm_agp_head *agp;       /* AGP data */
    
    struct drm_driver *driver;      /* Driver functions */
    void *dev_private;              /* Device private data */
    struct mutex  struct_mutex;
    
    struct address_space *dev_mapping;
    struct drm_mode_config mode_config;
    
    struct pci_dev * pdev;
    
    /* GEM information */
    spinlock_t object_name_lock;
    struct idr object_name_idr;
    uint32_t invalidate_domains;    /* domains pending invalidation */
    uint32_t flush_domains;         /* domains pending flush */

    /* AROS specific fields */
    struct Interrupt        IntHandler;
};

static __inline__ int drm_core_check_feature(struct drm_device *dev,
                         int feature)
{
    return ((dev->driver->driver_features & feature) ? 1 : 0);
}

typedef struct drm_dma_handle {
	dma_addr_t busaddr;
	void *vaddr;
	size_t size;
} drm_dma_handle_t;

struct drm_file
{
    struct idr object_idr;
    spinlock_t table_lock;
    void *driver_priv;
    struct list_head fbs;
};

struct drm_gem_object {
    /** Reference count of this object */
    struct kref refcount;

    /** Handle count of this object. Each handle also holds a reference */
    atomic_t handle_count; /* number of handles on this object */

    /** Related drm device */
    struct drm_device *dev;

    /** File representing the shmem storage */
    struct file *filp;

    /* Mapping info for this object */
    struct drm_map_list map_list;

    /**
     * Size of the object, in bytes.  Immutable over the object's
     * lifetime.
     */
    size_t size;

    /**
     * Global name for this object, starts at 1. 0 means unnamed.
     * Access is covered by the object_name_lock in the related drm_device
     */
    int name;

    /**
     * Memory domains. These monitor which caches contain read/write data
     * related to the object. When transitioning from one set of domains
     * to another, the driver is called to ensure that caches are suitably
     * flushed and invalidated
     */
    uint32_t read_domains;
    uint32_t write_domain;

    /**
     * While validating an exec operation, the
     * new read/write domain values are computed here.
     * They will be transferred to the above values
     * at the point that any cache flushing occurs
     */
    uint32_t pending_read_domains;
    uint32_t pending_write_domain;

    void *driver_private;
};

static inline int drm_core_has_AGP(struct drm_device *dev)
{
	return drm_core_check_feature(dev, DRIVER_USE_AGP);
}

static __inline__ int drm_device_is_agp(struct drm_device *dev)
{
    return dev->driver->IsAGP;
}

static __inline__ int drm_device_is_pcie(struct drm_device *dev)
{
    return dev->driver->IsPCIE;
}

/* drm_bufs.c */
int drm_order(unsigned long size);

/* drm_drv.c */
void drm_exit(struct drm_driver *driver);
int drm_init(struct drm_driver *driver);

/* drm_irq.c */
int drm_irq_install(struct drm_device *dev);
int drm_irq_uninstall(struct drm_device *dev);

int drm_vblank_init(struct drm_device *dev, int num_crtcs);
void drm_vblank_cleanup(struct drm_device *dev);
void drm_vblank_pre_modeset(struct drm_device *dev, int crtc);
void drm_vblank_post_modeset(struct drm_device *dev, int crtc);
int drm_vblank_get(struct drm_device *dev, int crtc);
void drm_vblank_put(struct drm_device *dev, int crtc);
void drm_handle_vblank(struct drm_device *dev, int crtc);
u32 drm_vblank_count(struct drm_device *dev, int crtc);

/* drm_memory.c */
void drm_core_ioremap(struct drm_local_map *map, struct drm_device *dev);
void drm_core_ioremap_wc(struct drm_local_map *map, struct drm_device *dev);
void drm_core_ioremapfree(struct drm_local_map *map, struct drm_device *dev);
int drm_unbind_agp(DRM_AGP_MEM * handle);
void drm_free_agp(DRM_AGP_MEM * handle, int pages);

/* drm_agpsupport.c */
struct drm_agp_head *drm_agp_init(struct drm_device *dev);
int drm_agp_acquire(struct drm_device *dev);
int drm_agp_release(struct drm_device *dev);
int drm_agp_enable(struct drm_device *dev, struct drm_agp_mode mode);
int drm_agp_info(struct drm_device *dev, struct drm_agp_info *info);
int drm_agp_free_memory(DRM_AGP_MEM * handle);
int drm_agp_unbind_memory(DRM_AGP_MEM * handle);
void drm_agp_chipset_flush(struct drm_device *dev);
DRM_AGP_MEM * drm_agp_bind_pages(struct drm_device *dev, struct page **pages,
		   unsigned long num_pages, uint32_t gtt_offset, u32 type);

/* drm_cache.c */
void drm_clflush_pages(struct page *pages[], unsigned long num_pages);

/* GEM */
int drm_gem_init(struct drm_device *dev);
void drm_gem_object_release(struct drm_gem_object *obj);
void drm_gem_object_free(struct kref *kref);
void drm_gem_object_free_unlocked(struct kref *kref);
void drm_gem_object_handle_free(struct drm_gem_object *obj);
struct drm_gem_object *drm_gem_object_alloc(struct drm_device *dev,
                        size_t size);
int drm_gem_handle_create(struct drm_file *file_priv,
              struct drm_gem_object *obj,
              u32 *handlep);

struct drm_gem_object *drm_gem_object_lookup(struct drm_device *dev,
                         struct drm_file *filp,
                         u32 handle);

int drm_gem_handle_delete(struct drm_file *filp, u32 handle);
                         
static inline void
drm_gem_object_reference(struct drm_gem_object *obj)
{
    kref_get(&obj->refcount);
}

static inline void
drm_gem_object_unreference(struct drm_gem_object *obj)
{
    if (obj == NULL)
        return;

    kref_put(&obj->refcount, drm_gem_object_free);
}

static inline void
drm_gem_object_unreference_unlocked(struct drm_gem_object *obj)
{
    if (obj != NULL) {
        struct drm_device *dev = obj->dev;
        mutex_lock(&dev->struct_mutex);
        kref_put(&obj->refcount, drm_gem_object_free);
        mutex_unlock(&dev->struct_mutex);
    }
}

int drm_gem_handle_create(struct drm_file *file_priv,
              struct drm_gem_object *obj,
              u32 *handlep);

static inline void
drm_gem_object_handle_reference(struct drm_gem_object *obj)
{
    drm_gem_object_reference(obj);
    atomic_inc(&obj->handle_count);
}

static inline void
drm_gem_object_handle_unreference(struct drm_gem_object *obj)
{
    if (obj == NULL)
        return;

    if (atomic_read(&obj->handle_count) == 0)
        return;
    /*
    * Must bump handle count first as this may be the last
    * ref, in which case the object would disappear before we
    * checked for a name
    */
    if (atomic_dec_and_test(&obj->handle_count))
        drm_gem_object_handle_free(obj);
    drm_gem_object_unreference(obj);
}

static inline void
drm_gem_object_handle_unreference_unlocked(struct drm_gem_object *obj)
{
    if (obj == NULL)
        return;

    if (atomic_read(&obj->handle_count) == 0)
        return;

    /*
    * Must bump handle count first as this may be the last
    * ref, in which case the object would disappear before we
    * checked for a name
    */

    if (atomic_dec_and_test(&obj->handle_count))
        drm_gem_object_handle_free(obj);
    drm_gem_object_unreference_unlocked(obj);
}

/* GEM IOCTL */
int drm_gem_open_ioctl(struct drm_device *dev, void *data,
               struct drm_file *file_priv);
int drm_gem_close_ioctl(struct drm_device *dev, void *data,
            struct drm_file *file_priv);
int drm_gem_flink_ioctl(struct drm_device *dev, void *data,
            struct drm_file *file_priv);

/* MTRR */
#define DRM_MTRR_WC     0
static inline int drm_mtrr_add(unsigned long offset, unsigned long size,
                   unsigned int flags)
{
    return -ENODEV;
}
static inline int drm_mtrr_del(int handle, unsigned long offset,
                   unsigned long size, unsigned int flags)
{
    return -ENODEV;
}

#define MTRR_TYPE_WRCOMB     1

static inline int mtrr_add(unsigned long base, unsigned long size,
                    unsigned int type, char increment)
{
    return -ENODEV;
}

static inline int drm_pci_device_is_agp(struct drm_device *dev)
{
    return (int)dev->driver->IsAGP;
}

#endif
