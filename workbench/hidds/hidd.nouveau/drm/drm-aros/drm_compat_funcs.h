/*
    Copyright 2009, The AROS Development Team. All rights reserved.
    $Id$
*/

#ifndef _DRM_COMPAT_FUNCS_
#define _DRM_COMPAT_FUNCS_

#include <proto/exec.h>
#include <aros/debug.h>

#include "drm_compat_types.h"

#define writeq(val, addr)               (*(volatile UQUAD*)(addr) = (val))
#define readq(addr)                     (*(volatile UQUAD*)(addr)
#define writel(val, addr)               (*(volatile ULONG*)(addr) = (val))
#define readl(addr)                     (*(volatile ULONG*)(addr))
#define writew(val, addr)               (*(volatile UWORD*)(addr) = (val))
#define readw(addr)                     (*(volatile UWORD*)(addr))
#define writeb(val, addr)               (*(volatile UBYTE*)(addr) = (val))
#define readb(addr)                     (*(volatile UBYTE*)(addr))
#define kzalloc(size, flags)            AllocVec(size, MEMF_ANY | MEMF_CLEAR)
#define kcalloc(count, size, flags)     AllocVec(count * size, MEMF_ANY | MEMF_CLEAR);
#define kmalloc(size, flags)            AllocVec(size, MEMF_ANY)
#define vmalloc_user(size)              AllocVec(size, MEMF_ANY | MEMF_CLEAR)
#define vmalloc(size)                   AllocVec(size, MEMF_ANY)
#define kfree(objp)                     FreeVec(objp)
#define vfree(objp)                     FreeVec(objp)
#define capable(p)                      TRUE
#define roundup(x, y)                   ((((x) + ((y) - 1)) / (y)) * (y))
#define lower_32_bits(n)                ((u32)(n))
#define upper_32_bits(n)                ((u32)(((n) >> 16) >> 16))
#define mutex_lock(x)                   ObtainSemaphore(x.semaphore)
#define mutex_unlock(x)                 ReleaseSemaphore(x.semaphore)
#define mutex_init(x)                   InitSemaphore(x.semaphore);
#define likely(x)                       __builtin_expect((ULONG)(x),1)
#define unlikely(x)                     __builtin_expect((ULONG)(x),0)
#define mb()                            __asm __volatile("lock; addl $0,0(%%esp)" : : : "memory");
#define ffs(x)                          __builtin_ffs(x)
#define fls_long(x)                     ((sizeof(x) * 8) - __builtin_clzl(x))
#define max(a, b)                       ((a) > (b) ? (a) : (b))
#define min(a, b)                       ((a) < (b) ? (a) : (b))
#define ilog2(n)                        (fls_long(n) - 1)
#define rounddown_pow_of_two(n)         (1UL << ilog2(n))
#define is_power_of_2(x)                (x != 0 && ((x & (x - 1)) == 0))
#define access_ok(a, b, c)              TRUE
#define le16_to_cpu(x)                  AROS_LE2WORD(x)
#define le32_to_cpu(x)                  AROS_LE2LONG(x)
#define cpu_to_le16(x)                  AROS_WORD2LE(x)
#define mdelay(x)                       udelay(1000 * x)
#define msleep(x)                       udelay(1000 * x)
#define KHZ2PICOS(x)                    (1000000000UL/(x))
#define uninitialized_var(x)            x
#define get_user(x, p)                  ({u32 ret = 0; x = *(p); ret;})


#define MODULE_FIRMWARE(x)


void iowrite32(u32 val, void * addr);
unsigned int ioread32(void * addr);
void iowrite16(u16 val, void * addr);
unsigned int ioread16(void * addr);
void iowrite8(u8 val, void * addr);
unsigned int ioread8(void * addr);

void udelay(unsigned long usecs);
int abs(int j); /* Code in librom.a */

static inline ULONG copy_from_user(APTR to, APTR from, IPTR size)
{
    memcpy(to, from, size);
    return 0;
}

static inline ULONG copy_to_user(APTR to, APTR from, IPTR size)
{
    memcpy(to, from, size);
    return 0;
}

static inline VOID memcpy_toio(APTR dst, APTR src, ULONG size)
{
    /* TODO: optimize by using writel */
    UBYTE * srcp = (UBYTE*)src;
    ULONG i = 0;
    
    for (i = 0; i < size; i++)
        writeb(*(srcp + i), dst + i);
}

static inline VOID memcpy_fromio(APTR dst, APTR src, ULONG size)
{
    /* TODO: optimize by using readl */
    UBYTE * dstp = (UBYTE*)dst;
    ULONG i = 0;
    
    for (i = 0; i < size; i++)
        *(dstp + i) = readb(src + i);
}

#define BUG_ON(condition)           do { if (unlikely(condition)) bug("BUG: %s:%d\n", __FILE__, __LINE__); } while(0)
#define WARN_ON(condition)          do { if (unlikely(condition)) bug("WARN: %s:%d\n", __FILE__, __LINE__); } while(0)
#define EXPORT_SYMBOL(x)
#define PTR_ERR(addr)               (SIPTR)addr
#define ERR_PTR(error)              (APTR)error
static inline IPTR IS_ERR(APTR ptr)
{
    return (IPTR)(ptr) >= (IPTR)-MAX_ERRNO;
}

/* Kernel debug */
#define KERN_ERR
#define KERN_DEBUG
#define KERN_WARNING
#define KERN_INFO
#define printk(fmt, ...)                bug(fmt, ##__VA_ARGS__)
#define IMPLEMENT(fmt, ...)             bug("------IMPLEMENT(%s): " fmt, __func__ , ##__VA_ARGS__)
#define TRACE(fmt, ...)                 D(bug("[TRACE](%s): " fmt, __func__ , ##__VA_ARGS__))
#define BUG(x)                          bug("BUG:(%s)\n", __func__)
#define WARN(condition, message, ...)   do { if (unlikely(condition)) bug("WARN: %s:%d" message "\n", __FILE__, __LINE__, ##__VA_ARGS__); } while(0)
#define dev_warn(dev, fmt, ...)         bug(fmt, ##__VA_ARGS__)

/* PCI handling */
void * ioremap(resource_size_t offset, unsigned long size);
#define pci_map_page(a, b, c, d, e)     (dma_addr_t)(b->address + c)
#define pci_dma_mapping_error(a, b)     FALSE
#define pci_unmap_page(a, b, c, d)      
#define ioremap_nocache                 ioremap
#define ioremap_wc                      ioremap
void iounmap(void * addr);
resource_size_t pci_resource_start(void * pdev, unsigned int barnum);
unsigned long pci_resource_len(void * pdev, unsigned int barnum);
#define PCI_DEVFN(dev, fun)             dev, fun
void * pci_get_bus_and_slot(unsigned int bus, unsigned int dev, unsigned int fun);
int pci_read_config_word(void *dev, int where, u16 *val);
int pci_read_config_dword(void *dev, int where, u32 *val);
int pci_write_config_dword(void *dev, int where, u32 val);



/* Bit operations */
void clear_bit(int nr, volatile void * addr);
void set_bit(int nr, volatile void *addr);
int test_bit(int nr, volatile void *addr);
#define __set_bit(nr, addr)         set_bit(nr, addr)
#define __clear_bit(nr, addr)       clear_bit(nr, addr)

/* Page handling */
void __free_page(struct page * p);
struct page * create_page_helper();                     /* Helper function - not from compat */
#define PageHighMem(p)              FALSE
#define put_page(p)                 __free_page(p)  /*FIXME: This might be wrong */
#define page_to_phys(p)             (dma_addr_t)p->address
#define kmap(p)                     p->address
#define kmap_atomic(p, type)        p->address
#define vmap(p, count, flags, prot) (p)[0]->address
#define kunmap_atomic(addr, type)
#define kunmap(addr)
#define vunmap(addr)
#define set_page_dirty(p)

/* Atomic handling */
static inline int atomic_add_return(int i, atomic_t *v)
{
    return __sync_add_and_fetch(&v->count, i);
}

static inline void atomic_add(int i, atomic_t *v)
{
    (void)__sync_add_and_fetch(&v->count, i);
}

static inline void atomic_inc(atomic_t *v)
{
    __sync_add_and_fetch(&v->count, 1);
}

static inline void atomic_set(atomic_t *v, int i)
{
    v->count = i;
}

static inline int atomic_read(atomic_t *v)
{
    return v->count;
}

static inline void atomic_sub(int i, atomic_t *v)
{
    (void)__sync_sub_and_fetch(&v->count, i);
}

static inline void atomic_dec(atomic_t *v)
{
    __sync_sub_and_fetch(&v->count, 1);
}

static inline int atomic_dec_and_test(atomic_t *v)
{
    return (__sync_sub_and_fetch(&v->count, 1) == 0);
}

static inline int atomic_cmpxchg(atomic_t *v, int old, int new)
{
    return __sync_val_compare_and_swap(&v->count, old, new);
}

/* Lock handling */
static inline void spin_lock_init(spinlock_t * lock)
{
    atomic_set(&lock->lock, 0);
}
static inline void spin_lock(spinlock_t * lock)
{
    while(atomic_cmpxchg(&lock->lock, 0, 1) != 0);
}
static inline void spin_unlock(spinlock_t * lock)
{
    atomic_set(&lock->lock, 0);
}
static inline void spin_lock_irqsave(spinlock_t * lock, unsigned long flags)
{
    (void)flags;
    
    Disable();
    
    while(atomic_cmpxchg(&lock->lock, 0, 1) != 0);
}
static inline void spin_unlock_irqrestore(spinlock_t * lock, unsigned long flags)
{
    atomic_set(&lock->lock, 0);

    Enable();
    
    (void)flags;
}
#define spin_lock_irq(x)                spin_lock_irqsave(x, 0)
#define spin_unlock_irq(x)              spin_unlock_irqrestore(x, 0)
/* TODO: This may work incorreclty if write_lock and read_lock are used for the same lock
   read_lock allows concurent readers as lock as there is no writer */
static inline void rwlock_init(rwlock_t * lock)
{
    atomic_set(&lock->lock, 0);
}
static inline void write_lock(rwlock_t * lock)
{
    while(atomic_cmpxchg(&lock->lock, 0, 1) != 0);
}
static inline void write_unlock(rwlock_t * lock)
{
    atomic_set(&lock->lock, 0);
}

/* Reference counted objects implementation */
static inline void kref_init(struct kref *kref)
{
    atomic_set(&kref->refcount, 1);
}

static inline void kref_get(struct kref *kref)
{
    atomic_inc(&kref->refcount);
}

static inline int kref_put(struct kref *kref, void (*release) (struct kref *kref))
{
    if (atomic_dec_and_test(&kref->refcount)) release(kref);
    return atomic_read(&kref->refcount);
}

/* IDR handling */
#define idr_pre_get(a, b)               idr_pre_get_internal(a)
int idr_pre_get_internal(struct idr *idp);
int idr_get_new_above(struct idr *idp, void *ptr, int starting_id, int *id);
void *idr_find(struct idr *idp, int id);
void idr_remove(struct idr *idp, int id);
void idr_init(struct idr *idp);

/* AGP handling */
struct agp_bridge_data *agp_backend_acquire(void * dev);
void agp_backend_release(struct agp_bridge_data * bridge);
struct agp_bridge_data * agp_find_bridge(void * dev);
int agp_copy_info(struct agp_bridge_data * bridge, struct agp_kern_info * info);
void agp_enable(struct agp_bridge_data * bridge, u32 mode);
struct agp_memory *agp_allocate_memory(struct agp_bridge_data * bridge, size_t num_pages , u32 type);
void agp_free_memory(struct agp_memory * mem);
int agp_bind_memory(struct agp_memory * mem, off_t offset);
int agp_unbind_memory(struct agp_memory * mem);
void agp_flush_chipset(struct agp_bridge_data * bridge);

/* io_mapping handling */
#define __copy_from_user_inatomic_nocache(to, from, size)   copy_from_user(to, from, size)
#define io_mapping_map_atomic_wc(mapping, offset)   (APTR)(mapping->address + offset)
#define io_mapping_unmap_atomic(address)
static inline struct io_mapping * io_mapping_create_wc(resource_size_t base, unsigned long size)
{
    struct io_mapping * mapping = AllocVec(sizeof(struct io_mapping), MEMF_PUBLIC | MEMF_CLEAR);
    mapping->address = (IPTR)ioremap(base, size);
    return mapping;
}
static inline void io_mapping_free(struct io_mapping *mapping)
{
    iounmap((APTR)mapping->address);
    FreeVec(mapping);
}

/* jiffies (lame) handling */
#define jiffies get_jiffies()
unsigned long get_jiffies();

/* other */
#define do_div(n,base) ({ \
int __res; \
__res = ((unsigned long) n) % (unsigned) base; \
n = ((unsigned long) n) / (unsigned) base; \
__res; })

#endif /* _DRM_COMPAT_FUNCS_ */
