/*
    Copyright 2009, The AROS Development Team. All rights reserved.
*/

#ifndef _DRM_COMPAT_FUNCS_
#define _DRM_COMPAT_FUNCS_

#include <proto/exec.h>
#include <aros/debug.h>
#include <string.h>

#include "drm_compat_types.h"

#define writeq(val, addr)               (*(volatile UQUAD*)(addr) = (val))
#define readq(addr)                     (*(volatile UQUAD*)(addr)
#define writel(val, addr)               (*(volatile ULONG*)(addr) = (val))
#define readl(addr)                     (*(volatile ULONG*)(addr))
#define writew(val, addr)               (*(volatile UWORD*)(addr) = (val))
#define readw(addr)                     (*(volatile UWORD*)(addr))
#define writeb(val, addr)               (*(volatile UBYTE*)(addr) = (val))
#define readb(addr)                     (*(volatile UBYTE*)(addr))
#define capable(p)                      TRUE
#define round_up(x, y)                  roundup(x, y)
#define lower_32_bits(n)                ((u32)(n))
#define upper_32_bits(n)                ((u32)(((n) >> 16) >> 16))
#define mutex_lock(x)                   ObtainSemaphore(&(x)->semaphore)
#define mutex_lock_nested(x, y)         mutex_lock(x)
#define mutex_unlock(x)                 ReleaseSemaphore(&(x)->semaphore)
#define mutex_trylock(x)                AttemptSemaphore(&(x)->semaphore)
#define mutex_init(x)                   InitSemaphore(&(x)->semaphore);
#define mutex_destroy(x)
#define likely(x)                       __builtin_expect((IPTR)(x),1)
#define unlikely(x)                     __builtin_expect((IPTR)(x),0)
#define mb()                            __asm __volatile("lock; addl $0,0(%%esp)" : : : "memory");
#define wmb()                           __asm __volatile("" : : : "memory");
#define fls_long(x)                     ((sizeof(x) * 8) - __builtin_clzl(x))
#define max(a, b)                       ((a) > (b) ? (a) : (b))
#define min(a, b)                       ((a) < (b) ? (a) : (b))
#define clamp(a, vmin, vmax)            ((min(vmax, max(vmin, a))))
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
#define uninitialized_var(x)            x = 0
#define get_user(x, p)                  ({u32 ret = 0; x = *(p); ret;})
#define put_user(x, p)                  ({u32 ret = 0; *(p) = x; ret;})
#define rounddown(x, y)                 (((x)/(y))*(y))
#define DIV_ROUND_UP(x, y)              (((x) + (y) - 1) / (y))
#define EREMOTEIO                       EIO
#define max_t(t, x, y)                  ({(t)(x) > (t)(y) ? (t)(x) : (t)(y);})
#define min_t(t, x, y)                  ({(t)(x) < (t)(y) ? (t)(x) : (t)(y);})

static inline void __mutex_init(struct mutex *m, const char *name, void *ingore)
{
    InitSemaphore(&m->semaphore);
    m->name = name;
}


static inline u16 get_unaligned_le16(const void *p)
{
    return AROS_LE2WORD(*(const u16 *)p);
}

static inline u32 get_unaligned_le32(const void *p)
{
    return AROS_LE2LONG(*(const u32 *)p);
}

APTR HIDDNouveauAlloc(ULONG size);
VOID HIDDNouveauFree(APTR memory);

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

static inline VOID memcpy_toio(APTR dst, CONST_APTR src, ULONG size)
{
    /* TODO: optimize by using writel */
    UBYTE * srcp = (UBYTE*)src;
    ULONG i = 0;
    
    for (i = 0; i < size; i++)
        writeb(*(srcp + i), dst + i);
}

static inline VOID memcpy_fromio(APTR dst, CONST_APTR src, ULONG size)
{
    /* TODO: optimize by using readl */
    UBYTE * dstp = (UBYTE*)dst;
    ULONG i = 0;
    
    for (i = 0; i < size; i++)
        *(dstp + i) = readb(src + i);
}

#define BUG_ON(condition)           do { if (unlikely(condition)) bug("BUG: %s:%d\n", __FILE__, __LINE__); } while(0)
#define WARN_ON(condition)          ({do { if (unlikely(condition)) bug("WARN: %s:%d\n", __FILE__, __LINE__); } while(0); condition;})
#define WARN_ON_ONCE(condition)     ({do { bug("WARN_ON_ONCE NOT IMPLEMENTED\n"); } while (0); condition;})
#define EXPORT_SYMBOL(x)
#define PTR_ERR(addr)               ((SIPTR)addr)
#define ERR_PTR(error)              ((APTR)(SIPTR)error)
static inline IPTR IS_ERR(APTR ptr)
{
    return (IPTR)(ptr) >= (IPTR)-MAX_ERRNO;
}
static inline bool IS_ERR_OR_NULL(APTR ptr)
{
    if (ptr == NULL) return TRUE;
    return IS_ERR(ptr);
}

#define IS_ENABLED(option)  option##_ENABLED

#define __rcu

/* Kernel debug */
#define CONFIG_NOUVEAU_DEBUG            3 /* NV_DGB_INFO */
#define CONFIG_NOUVEAU_DEBUG_DEFAULT    3
#define KERN_CRIT
#define KERN_ERR
#define KERN_DEBUG
#define KERN_WARNING
#define KERN_INFO
#define KERN_NOTICE
#define printk(fmt, ...)                bug(fmt, ##__VA_ARGS__)
#define IMPLEMENT(fmt, ...)             bug("------IMPLEMENT(%s): " fmt, __func__ , ##__VA_ARGS__)
#define TRACE(fmt, ...)                 D(bug("[TRACE](%s): " fmt, __func__ , ##__VA_ARGS__))
#define BUG(x)                          bug("BUG:(%s)\n", __func__)
#define WARN(condition, message, ...)   do { if (unlikely(condition)) bug("WARN: %s:%d" message "\n", __FILE__, __LINE__, ##__VA_ARGS__); } while(0)
#define dev_dbg(dev, fmt, ...)          bug(fmt, ##__VA_ARGS__)
#define dev_warn(dev, fmt, ...)         bug(fmt, ##__VA_ARGS__)
#define dev_err(dev, fmt, ...)          bug(fmt, ##__VA_ARGS__)
#define dev_info(dev, fmt, ...)         bug(fmt, ##__VA_ARGS__)
#define dev_notice(dev, fmt, ...)       bug(fmt, ##__VA_ARGS__)
#define dev_crit(dev, fmt, ...)         bug(fmt, ##__VA_ARGS__)
#define dev_WARN(dev, fmt, ...)         bug(fmt, ##__VA_ARGS__)
#define pr_err(fmt, ...)                bug(fmt, ##__VA_ARGS__)
#define pr_debug(fmt, ...)              bug(fmt, ##__VA_ARGS__)
#define NOT_IMPLEMENTED_STOP            { bug("NOT IMPLEMENTED STOP %s, %d\n", __func__, __LINE__); while(1); }
#define NOT_IMPLEMENTED_CONTINUE        { bug("NOT IMPLEMENTED %s, %d\n", __func__, __LINE__); }

/* Page handling */
void __free_page(struct page * p);
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
struct page *pfn_to_page(unsigned long pfn);
struct page *alloc_page(ULONG mask);

/* Atomic handling */
static inline int atomic_add_return(int i, atomic_t *v)
{
    return __sync_add_and_fetch(&v->count, i);
}

static inline int atomic_dec_return(atomic_t *v)
{
    return __sync_sub_and_fetch(&v->count, 1);
}

static inline int atomic_inc_return(atomic_t *v)
{
    return __sync_add_and_fetch(&v->count, 1);
}

static inline void atomic_add(int i, atomic_t *v)
{
    (void)__sync_add_and_fetch(&v->count, i);
}

static inline void atomic_inc(atomic_t *v)
{
    (void)__sync_add_and_fetch(&v->count, 1);
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
    (void)__sync_sub_and_fetch(&v->count, 1);
}

static inline int atomic_dec_and_test(atomic_t *v)
{
    return (__sync_sub_and_fetch(&v->count, 1) == 0);
}

static inline int atomic_sub_and_test(int i, atomic_t *v)
{
    return (__sync_sub_and_fetch(&v->count, i) == 0);
}

static inline int atomic_cmpxchg(atomic_t *v, int old, int new)
{
    return __sync_val_compare_and_swap(&v->count, old, new);
}

static inline int atomic_inc_not_zero(atomic_t *v)
{
    int val = atomic_read(v);
    if (val != 0)
        atomic_inc(v);
    
    return val != 0;
}

/* Lock handling */

/* A code protected by spin lock is quaranteed to be atomic. This means that
 * preemtion on this CPU needs to be disabled for the time of executing.
 * Additionally, if the _irq variant of spin lock functions is used,
 * it is also guaraneteed that interrupts are disabled on the executing CPU.
 * The _bh variant disables the "bottom half" processing which is currently not
 * implemented in compat wrappers.
 */

static inline void spin_lock_init(spinlock_t * lock)
{
    /* No-Op */
}
static inline void spin_lock(spinlock_t * lock)
{
    Forbid();
}
static inline void spin_unlock(spinlock_t * lock)
{
    Permit();
}

#define spin_lock_bh(x)                 spin_lock(x)
#define spin_unlock_bh(x)               spin_unlock(x)

#define spin_lock_irqsave(lock, flags)      \
do                      \
{                       \
    (void)flags;        \
    Disable();          \
    spin_lock(lock);    \
}while(0)

#define spin_unlock_irqrestore(lock, flags)      \
do                      \
{                       \
    spin_unlock(lock);  \
    Enable();           \
    (void)flags;        \
}while(0)

#define spin_lock_irq(x)                spin_lock_irqsave(x, 0)
#define spin_unlock_irq(x)              spin_unlock_irqrestore(x, 0)

/* TODO: This may work incorrectly if write_lock and read_lock are used for the same lock as
 * read_lock allows concurent readers as long as there is no writer
 */
static inline void rwlock_init(rwlock_t * lock)
{
    /* No-Op */
}
static inline void write_lock(rwlock_t * lock)
{
    Forbid();
}
static inline void write_unlock(rwlock_t * lock)
{
    Permit();
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
    if (atomic_dec_and_test(&kref->refcount)) 
    {
        release(kref);
        return 1;
    }
    else
        return 0;
}

static inline int kref_sub(struct kref *kref, unsigned int count, void (*release) (struct kref *kref))
{
    if (atomic_sub_and_test(count, &kref->refcount)) 
    {
        release(kref);
        return 1;
    }
    else
        return 0; 
}

static inline unsigned int kref_read(struct kref *kref)
{
    return kref->refcount.count;
}

void refcount_set(refcount_t *r, int n);
bool refcount_dec_and_test(refcount_t *r);
void refcount_inc(refcount_t *r);


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
void * ioremap(resource_size_t offset, unsigned long size);
void iounmap(void * addr);
#define ioremap_nocache ioremap
#define ioremap_wc      ioremap
#define __copy_from_user_inatomic_nocache(to, from, size)   copy_from_user(to, from, size)
#define io_mapping_map_atomic_wc(mapping, offset)   (APTR)(mapping->address + (offset))
#define io_mapping_unmap_atomic(address)
static inline struct io_mapping * io_mapping_create_wc(resource_size_t base, unsigned long size)
{
    struct io_mapping * mapping = HIDDNouveauAlloc(sizeof(struct io_mapping));
    mapping->address = (IPTR)ioremap(base, size);
    return mapping;
}
static inline void io_mapping_free(struct io_mapping *mapping)
{
    iounmap((APTR)mapping->address);
    HIDDNouveauFree(mapping);
}

static inline void memset_io(volatile void *__addr, int value, size_t size)
{
    memset((void *)__addr, value, size);
}

/* iommu */
struct iommu_domain;
#define IOMMU_READ 1
#define IOMMU_WRITE 2
size_t iommu_unmap(struct iommu_domain *domain, unsigned long iova, size_t size);
int iommu_map(struct iommu_domain *domain, unsigned long iova, phys_addr_t paddr, size_t size, int prot);


/* I2C handling */
int i2c_transfer(struct i2c_adapter *adap, struct i2c_msg *msgs, int num);
int i2c_del_adapter(struct i2c_adapter *);


/* jiffies (lame) handling */
#define jiffies get_jiffies()
unsigned long get_jiffies();

typedef struct {
    struct MinNode wt_Node;
    struct Task *   wt_Task;
} waitqueue_task_t;

/* Wait queue (lame) handling */
#define init_waitqueue_head(x)  NEWLIST(x)
#define wake_up_all(x)                          \
    {                                           \
        waitqueue_task_t *wt,*next;             \
        ForeachNodeSafe(x, wt, next) {          \
            REMOVE(wt);                         \
            Signal(wt->wt_Task, SIGF_SINGLE);   \
        }                                       \
    }

#define wait_event(wq, condition)       \
    {                                   \
        waitqueue_task_t wt;            \
        wt.wt_Task = FindTask(NULL);    \
        for(;;) {                       \
            if (condition)              \
                break;                  \
            ADDTAIL(&wq, &wt);          \
            Wait(SIGF_SINGLE);          \
        }                               \
    }

#define wait_event_interruptible(wq, condition)                   \
    ({                                                            \
        int __ret = 0;                                            \
        {                                                         \
            waitqueue_task_t wt;                                  \
            wt.wt_Task = FindTask(NULL);                          \
            for (;;)                                              \
            {                                                     \
                if (condition)                                    \
                    break;                                        \
                ADDTAIL(&wq, &wt);                                \
                Wait(SIGF_SINGLE);                                \
            }                                                     \
        }                                                         \
        __ret;                                                    \
    })

/* firmaware handling */
#define MODULE_FIRMWARE(x)
int request_firmware(const struct firmware **fw, const char *name, struct device *device);
int firmware_request_nowarn(const struct firmware **fw, const char *name, struct device *device);
void release_firmware(const struct firmware *fw);

/* scatterlist */
struct scatterlist *sg_next(struct scatterlist *s);
dma_addr_t sg_dma_address(struct scatterlist *s);
IPTR sg_dma_len(struct scatterlist *s);

/* dma handling */
#define DMA_BIDIRECTIONAL   0
#define DMA_TO_DEVICE       1
#define DMA_FROM_DEVICE     2
#define DMA_ATTR_NON_CONSISTENT (1UL << 3)
#define DMA_ATTR_WEAK_ORDERING (1UL << 1)
#define DMA_ATTR_WRITE_COMBINE (1UL << 2)
dma_addr_t dma_map_page(struct device *dev, struct page *page, unsigned long offset, size_t size, ULONG dir);
static inline int dma_mapping_error(struct device *dev, dma_addr_t dma_addr) { return 0; }
void dma_unmap_page(struct device *dev, dma_addr_t dma_handle, size_t size, ULONG dir);
void dma_free_attrs(struct device *dev, size_t size, void *cpuaddr, dma_addr_t dma_handle, unsigned long attrs);
void *dma_alloc_attrs(struct device *dev, size_t size, dma_addr_t *dma_handle, ULONG flags, unsigned long attrs);
void *dma_alloc_coherent(struct device *dev, size_t size, dma_addr_t *dma_handle, ULONG flags);
void dma_free_coherent(struct device *dev, size_t size, void *cpuaddr, dma_addr_t dma_handle);
void dma_sync_single_for_device(struct device *dev, dma_addr_t dma_addr, size_t size, ULONG dir);
void dma_sync_single_for_cpu(struct device *dev, dma_addr_t dma_addr, size_t size, ULONG dir);

/* dma fence handling */
struct dma_resv
{
    ULONG dummy;
};
struct dma_fence;
struct dma_fence_ops
{
    const char * (*get_driver_name)(struct dma_fence *fence);
    const char * (*get_timeline_name)(struct dma_fence *fence);
    bool (*enable_signaling)(struct dma_fence *fence);
    bool (*signaled)(struct dma_fence *fence);
    void (*release)(struct dma_fence *fence);
    signed long (*wait)(struct dma_fence *fence, bool intr, signed long timeout);
};
struct dma_fence
{
    struct kref refcount;
    ULONG flags;
    ULONG seqno;
    struct dma_fence_ops *ops;
    IPTR context;
    spinlock_t *lock;
};

#define DMA_FENCE_FLAG_SIGNALED_BIT 0
#define DMA_FENCE_FLAG_USER_BITS 3 /* last */

struct dma_fence *dma_fence_get(struct dma_fence *fence);
struct dma_fence *dma_resv_get_excl(struct dma_resv *resv);
void dma_resv_add_excl_fence(struct dma_resv *resv, struct dma_fence *fence);
void dma_resv_add_shared_fence(struct dma_resv *resv, struct dma_fence *fence);
void dma_fence_free(struct dma_fence *fence);
signed long dma_fence_wait(struct dma_fence *fence, bool intr);
bool dma_fence_is_signaled(struct dma_fence *fence);
void dma_fence_init(struct dma_fence *fence, const struct dma_fence_ops *ops, spinlock_t *lock, u64 context, u64 seqno);
void dma_fence_put(struct dma_fence *fence);
int dma_fence_signal_locked(struct dma_fence *fence);
void dma_fence_enable_sw_signaling(struct dma_fence *fence);
void dma_resv_assert_held(struct dma_resv *resv);
int dma_resv_trylock(struct dma_resv *resv);
struct ww_acquire_ctx;
int dma_resv_lock(struct dma_resv *resv, struct ww_acquire_ctx *ctx);
int dma_resv_lock_interruptible(struct dma_resv *resv, struct ww_acquire_ctx *ctx);
void dma_resv_unlock(struct dma_resv *resv);
struct ww_acquire_ctx *dma_resv_locking_ctx(struct dma_resv *resv);
void dma_resv_init(struct dma_resv *resv);
void dma_resv_fini(struct dma_resv *resv);
int dma_resv_reserve_shared(struct dma_resv *resv, unsigned int num);
struct dma_resv_list;
struct dma_resv_list *dma_resv_get_list(struct dma_resv *resv);
bool dma_resv_held(struct dma_resv *resv);
int dma_resv_copy_fences(struct dma_resv *dst, struct dma_resv *src);

/* other */
#define do_div(n,base) ({ \
int __res; \
__res = ((unsigned long) n) % (unsigned) base; \
n = ((unsigned long) n) / (unsigned) base; \
__res; })

int snprintf(char * restrict s, size_t n, const char * restrict format, ...);
int sprintf(char * restrict s, const char * restrict format, ...);
int order_base_2(unsigned long n);
unsigned long clk_get_rate(struct clk *);
u64 div_u64(u64 a, u32 b);
s64 div64_s64(s64 a, s64 b);
u32 get_unaligned_le32(const void *p);
u16 get_unaligned_le16(const void *p);
void drm_clflush_pages(struct page *pages[], unsigned long num_pages);

#endif /* _DRM_COMPAT_FUNCS_ */
