/*
    Copyright 2009-2026, The AROS Development Team. All rights reserved.
*/

#ifndef _LINUX_SLAB_H_
#define _LINUX_SLAB_H_

#include <string.h>
#include <stdarg.h>
#include <linux/workqueue.h>
#include <linux/cleanup.h>
#include <linux/types.h>
#include <linux/gfp.h>
#include <linux/compiler.h>
#include <linux/overflow.h>
#include <linux/types.h>
#include <linux/workqueue.h>
#include <linux/percpu-refcount.h>
#include <linux/cleanup.h>
#include <linux/hash.h>

/*
 * The whole driver allocates out of one memory pool. Every block is
 * zeroed on allocation, so the __GFP_ZERO variants cost nothing extra.
 */
void  *kmalloc(size_t size, gfp_t flags);
void  *krealloc(const void *p, size_t new_size, gfp_t flags);
void   kfree(const void *p);
size_t ksize(const void *p);
void  *kmemdup(const void *src, size_t len, gfp_t gfp);
char  *kstrdup(const char *s, gfp_t gfp);
char  *kstrndup(const char *s, size_t len, gfp_t gfp);
char  *kasprintf(gfp_t gfp, const char *fmt, ...);
char  *kvasprintf(gfp_t gfp, const char *fmt, va_list ap);

static inline void *kzalloc(size_t size, gfp_t flags)
{
    return kmalloc(size, flags | __GFP_ZERO);
}
static inline void *kmalloc_array(size_t n, size_t size, gfp_t flags)
{
    size_t bytes;
    if (check_mul_overflow(n, size, &bytes))
        return NULL;
    return kmalloc(bytes, flags);
}
static inline void *kcalloc(size_t n, size_t size, gfp_t flags)
{
    return kmalloc_array(n, size, flags | __GFP_ZERO);
}
static inline void *krealloc_array(void *p, size_t n, size_t size, gfp_t flags)
{
    size_t bytes;
    if (check_mul_overflow(n, size, &bytes))
        return NULL;
    return krealloc(p, bytes, flags);
}
static inline void *kmemdup_array(const void *src, size_t count, size_t size, gfp_t gfp)
{
    return kmemdup(src, count * size, gfp);
}
static inline size_t kmalloc_size_roundup(size_t size)
{
    return size;
}
static inline void kfree_sensitive(const void *p)
{
    kfree(p);
}
static inline void kfree_const(const void *p)
{
    kfree(p);
}
static inline const char *kstrdup_const(const char *s, gfp_t gfp)
{
    return kstrdup(s, gfp);
}
#define kzalloc_node(s, f, n)       kzalloc(s, f)
#define kmalloc_node(s, f, n)       kmalloc(s, f)
#define kvmalloc(size, flags)       kmalloc(size, flags)
#define kvzalloc(size, flags)       kzalloc(size, flags)
#define kvcalloc(n, size, flags)    kcalloc(n, size, flags)
#define kvmalloc_array(n, size, flags) kmalloc_array(n, size, flags)
#define kvrealloc(p, size, flags)   krealloc(p, size, flags)
#define kvfree(p)                   kfree(p)
#define kvfree_sensitive(p, s)      kfree(p)
#define ZERO_SIZE_PTR               ((void *)16)
#define ZERO_OR_NULL_PTR(x)         ((unsigned long)(x) <= (unsigned long)ZERO_SIZE_PTR)
#define ARCH_KMALLOC_MINALIGN       8
#define ARCH_DMA_MINALIGN           64
#define KMALLOC_MAX_SIZE            (1UL << 25)
#define KMALLOC_MAX_CACHE_SIZE      (1UL << 22)
#define SLAB_HWCACHE_ALIGN          0
#define SLAB_RECLAIM_ACCOUNT        0
#define SLAB_TYPESAFE_BY_RCU        0

/* kmem_cache: thin veneer, allocations just go to the pool */
struct kmem_cache {
    const char *name;
    size_t size;
    void (*ctor)(void *);
};
struct kmem_cache *kmem_cache_create(const char *name, unsigned int size, unsigned int align, unsigned int flags, void (*ctor)(void *));
void  kmem_cache_destroy(struct kmem_cache *s);
void *kmem_cache_alloc(struct kmem_cache *s, gfp_t flags);
void *kmem_cache_zalloc(struct kmem_cache *s, gfp_t flags);
void  kmem_cache_free(struct kmem_cache *s, void *p);
#define KMEM_CACHE(__struct, __flags) kmem_cache_create(#__struct, sizeof(struct __struct), __alignof__(struct __struct), (__flags), NULL)

#endif /* _LINUX_SLAB_H_ */
