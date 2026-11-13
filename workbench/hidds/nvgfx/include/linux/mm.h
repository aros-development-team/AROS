/*
    Copyright 2009-2026, The AROS Development Team. All rights reserved.
*/

#ifndef _LINUX_MM_H_
#define _LINUX_MM_H_

#include <string.h>
#include <linux/types.h>
#include <linux/gfp.h>
#include <linux/slab.h>
#include <linux/kernel.h>
#include <linux/list.h>
#include <linux/atomic.h>
#include <linux/overflow.h>

/*
 * Pages. A struct page pointer is the (page-aligned) virtual address of the
 * page itself; RAM is identity mapped on every target this runs on, so the
 * same value doubles as the physical address. Nothing dereferences a page,
 * which is why the type stays incomplete.
 */
#undef PAGE_SIZE
#undef PAGE_SHIFT
#undef PAGE_MASK
#define PAGE_SHIFT              12
#define PAGE_SIZE               (1UL << PAGE_SHIFT)
#define PAGE_MASK               (~(PAGE_SIZE - 1))
#define PAGE_ALIGN(addr)        (typeof(addr))ALIGN((IPTR)(addr), PAGE_SIZE)
#define PAGE_ALIGNED(addr)      IS_ALIGNED((IPTR)(addr), PAGE_SIZE)
#define PFN_UP(x)               (((x) + PAGE_SIZE - 1) >> PAGE_SHIFT)
#define PFN_DOWN(x)             ((x) >> PAGE_SHIFT)
#define PFN_PHYS(x)             ((phys_addr_t)(x) << PAGE_SHIFT)
#define PHYS_PFN(x)             ((unsigned long)((x) >> PAGE_SHIFT))
#define HPAGE_PMD_SIZE          (1UL << 21)
#define HPAGE_PMD_ORDER         9
#define HPAGE_PUD_SIZE          (1UL << 30)
#define PMD_SIZE                HPAGE_PMD_SIZE
#define PMD_SHIFT               21
#define PMD_MASK                (~(PMD_SIZE - 1))
#define PUD_SHIFT               30
#define MAX_ORDER               11
#define MAX_PAGE_ORDER          10
#define NR_PAGE_ORDERS          (MAX_PAGE_ORDER + 1)
#define totalram_pages()        (compat_totalram_pages())
unsigned long compat_totalram_pages(void);
#define si_mem_available()      (compat_totalram_pages() / 2)

static inline void *page_address(const struct page *page)     { return (void *)page; }
static inline struct page *virt_to_page(const void *addr)     { return (struct page *)((IPTR)addr & PAGE_MASK); }
static inline struct page *vmalloc_to_page(const void *addr)  { return virt_to_page(addr); }
static inline phys_addr_t virt_to_phys(const void *addr)      { return (phys_addr_t)(IPTR)addr; }
static inline void *phys_to_virt(phys_addr_t p)               { return (void *)(IPTR)p; }
static inline phys_addr_t page_to_phys(const struct page *p)  { return (phys_addr_t)(IPTR)p; }
static inline unsigned long page_to_pfn(const struct page *p) { return (IPTR)p >> PAGE_SHIFT; }
static inline struct page *pfn_to_page(unsigned long pfn)     { return (struct page *)(pfn << PAGE_SHIFT); }
static inline struct page *nth_page(const struct page *p, unsigned long n) { return (struct page *)p + n; }
static inline unsigned long page_size(const struct page *p)   { return PAGE_SIZE; }
static inline unsigned int page_shift(const struct page *p)   { return PAGE_SHIFT; }
static inline bool pfn_valid(unsigned long pfn)               { return true; }
#define virt_addr_valid(a)      (1)
#define is_vmalloc_addr(a)      (0)
#define page_to_virt(p)         page_address(p)
#define page_folio(p)           (p)
#define folio_page(f, n)        nth_page(f, n)
#define compound_head(p)        (p)
#define page_count(p)           (1)
#define PageHighMem(p)          (0)
#define PageReserved(p)         (0)
#define PageSlab(p)             (0)
#define PageAnon(p)             (0)
#define PageDirty(p)            (0)
#define PageTransHuge(p)        (0)
#define PageSwapCache(p)        (0)
#define PageMlocked(p)          (0)
#define PageCompound(p)         (0)
#define set_page_dirty(p)       do { } while (0)
#define set_page_dirty_lock(p)  do { } while (0)
#define mark_page_accessed(p)   do { } while (0)
#define SetPageReserved(p)      do { } while (0)
#define ClearPageReserved(p)    do { } while (0)
#define SetPageDirty(p)         do { } while (0)
#define ClearPageDirty(p)       do { } while (0)
#define lock_page(p)            do { } while (0)
#define unlock_page(p)          do { } while (0)
#define get_page(p)             do { } while (0)
#define put_page(p)             do { } while (0)
#define page_ref_count(p)       (1)
#define page_zone(p)            (0)
static inline void *kmap(struct page *p)                     { return page_address(p); }
static inline void *kmap_atomic(struct page *p)              { return page_address(p); }
static inline void *kmap_local_page(struct page *p)          { return page_address(p); }
static inline void *kmap_atomic_prot(struct page *p, pgprot_t prot) { return page_address(p); }
static inline void *kmap_local_page_prot(struct page *p, pgprot_t prot) { return page_address(p); }
static inline void kunmap(struct page *p)                    { }
static inline void kunmap_atomic(void *a)                    { }
static inline void kunmap_local(void *a)                     { }
#define flush_dcache_page(p)    do { } while (0)
#define flush_kernel_dcache_page(p) do { } while (0)
#define offset_in_page(p)       ((unsigned long)(p) & ~PAGE_MASK)

/*
 * Page-granular allocations: page aligned, physically contiguous, zeroed.
 * Order-N blocks are 2^N consecutive pages.
 */
struct page *alloc_pages(gfp_t gfp, unsigned int order);
void __free_pages(struct page *page, unsigned int order);
unsigned long __get_free_pages(gfp_t gfp, unsigned int order);
void free_pages(unsigned long addr, unsigned int order);
#define alloc_page(gfp)             alloc_pages(gfp, 0)
#define __free_page(p)              __free_pages(p, 0)
#define __get_free_page(gfp)        __get_free_pages(gfp, 0)
#define get_zeroed_page(gfp)        __get_free_pages(gfp, 0)
#define free_page(addr)             free_pages(addr, 0)
#define alloc_pages_node(n, g, o)   alloc_pages(g, o)
#define __alloc_pages_node(n, g, o) alloc_pages(g, o)
#define split_page(p, o)            do { } while (0)
#define clear_page(a)               memset(a, 0, PAGE_SIZE)
#define clear_highpage(p)           memset(page_address(p), 0, PAGE_SIZE)
#define copy_highpage(d, s)         memcpy(page_address(d), page_address(s), PAGE_SIZE)

/*
 * vmalloc memory: page aligned and contiguous, so vmalloc_to_page() is a
 * plain address mask.
 */
void *vmalloc(unsigned long size);
void *vzalloc(unsigned long size);
void  vfree(const void *addr);
#define vmalloc_user(s)             vzalloc(s)
#define vmalloc_32(s)               vmalloc(s)
#define __vmalloc(s, g)             vmalloc(s)
#define vmalloc_array(n, s)         vmalloc((n) * (s))
#define vcalloc(n, s)               vzalloc((n) * (s))
#define __vmalloc_array(n, s, g)    vmalloc((n) * (s))
#define __vcalloc(n, s, g)          vzalloc((n) * (s))
#define is_vmalloc_or_module_addr(a) (0)

/* map/unmap a page array; only contiguous arrays can be mapped here */
void *vmap(struct page **pages, unsigned int count, unsigned long flags, pgprot_t prot);
void  vunmap(const void *addr);

/* page protection: purely informational on this platform */
#define PAGE_KERNEL             ((pgprot_t){ 0 })
#define PAGE_KERNEL_IO          ((pgprot_t){ 0 })
#define pgprot_writecombine(p)  (p)
#define pgprot_noncached(p)     (p)
#define pgprot_decrypted(p)     (p)
#define pgprot_val(p)           ((p).pgprot)
#define __pgprot(v)             ((pgprot_t){ (v) })
#define vm_get_page_prot(f)     PAGE_KERNEL

typedef unsigned int vm_fault_t;
#define VM_FAULT_OOM        0x000001
#define VM_FAULT_SIGBUS     0x000002
#define VM_FAULT_NOPAGE     0x000100
#define VM_FAULT_RETRY      0x000400
#define FAULT_FLAG_ALLOW_RETRY  0x04
#define FAULT_FLAG_RETRY_NOWAIT 0x08
struct vm_operations_struct;
struct vm_area_struct { unsigned long vm_start, vm_end, vm_pgoff, vm_flags; void *vm_private_data; pgprot_t vm_page_prot; const struct vm_operations_struct *vm_ops; struct file *vm_file; };
struct vm_operations_struct { void (*open)(struct vm_area_struct *area); void (*close)(struct vm_area_struct *area); vm_fault_t (*fault)(struct vm_fault *vmf); int (*access)(struct vm_area_struct *vma, unsigned long addr, void *buf, int len, int write); };
#define VM_NORESERVE 0x00200000
static inline void vm_flags_set(struct vm_area_struct *vma, unsigned long flags) { vma->vm_flags |= flags; }
static inline void vm_flags_clear(struct vm_area_struct *vma, unsigned long flags) { vma->vm_flags &= ~flags; }
static inline unsigned long vma_pages(struct vm_area_struct *vma) { return (vma->vm_end - vma->vm_start) >> PAGE_SHIFT; }
/* what the GSP is told the largest user address is; the x86 figures */
#if BITS_PER_LONG == 64
#define TASK_SIZE ((1UL << 47) - PAGE_SIZE)
#else
#define TASK_SIZE (0xC0000000UL)
#endif
#define NUMA_NO_NODE (-1)
static inline bool want_init_on_free(void) { return false; }
static inline bool current_is_kswapd(void) { return false; }
#define kmap_local_page_try_from_panic(p) page_address(p)
static inline gfp_t mapping_gfp_constraint(struct address_space *m, gfp_t gfp) { return gfp; }
#define mapping_clear_unevictable(m) do { } while (0)
struct vm_fault { struct vm_area_struct *vma; unsigned long address, pgoff; unsigned int flags; };
static inline void unmap_mapping_range(struct address_space *mapping, loff_t const holebegin, loff_t const holelen, int even_cows) { }
struct mm_struct;
struct page **compat_page_array(void *base, unsigned int npages);

#endif /* _LINUX_MM_H_ */
