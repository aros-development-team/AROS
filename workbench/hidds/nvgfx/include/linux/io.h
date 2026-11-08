/*
    Copyright 2009-2026, The AROS Development Team. All rights reserved.
*/

#ifndef _LINUX_IO_H_
#define _LINUX_IO_H_

#include <linux/types.h>
#include <linux/kernel.h>
#include <linux/atomic.h>
#include <linux/init.h>
#include <linux/bug.h>
#include <linux/err.h>
#include <linux/ioport.h>

/*
 * MMIO needs explicit ordering on riscv: a write into a DMA buffer must
 * be visible to the device before the doorbell that makes it fetch, and
 * a read must complete before dependent code runs. x86 orders such
 * accesses in hardware.
 */
#if defined(__riscv)
#define __io_bw()               __asm__ __volatile__("fence w,o" : : : "memory")
#define __io_ar()               __asm__ __volatile__("fence i,r" : : : "memory")
#else
#define __io_bw()               do { } while (0)
#define __io_ar()               do { } while (0)
#endif

static inline u8  __raw_readb(const volatile void __iomem *a) { return *(const volatile u8 *)a; }
static inline u16 __raw_readw(const volatile void __iomem *a) { return *(const volatile u16 *)a; }
static inline u32 __raw_readl(const volatile void __iomem *a) { return *(const volatile u32 *)a; }
static inline u64 __raw_readq(const volatile void __iomem *a) { return *(const volatile u64 *)a; }
static inline void __raw_writeb(u8 v, volatile void __iomem *a)  { *(volatile u8 *)a = v; }
static inline void __raw_writew(u16 v, volatile void __iomem *a) { *(volatile u16 *)a = v; }
static inline void __raw_writel(u32 v, volatile void __iomem *a) { *(volatile u32 *)a = v; }
static inline void __raw_writeq(u64 v, volatile void __iomem *a) { *(volatile u64 *)a = v; }
static inline u8  readb(const volatile void __iomem *a) { u8 v = __raw_readb(a); __io_ar(); return v; }
static inline u16 readw(const volatile void __iomem *a) { u16 v = __raw_readw(a); __io_ar(); return v; }
static inline u32 readl(const volatile void __iomem *a) { u32 v = __raw_readl(a); __io_ar(); return v; }
static inline u64 readq(const volatile void __iomem *a) { u64 v = __raw_readq(a); __io_ar(); return v; }
static inline void writeb(u8 v, volatile void __iomem *a)  { __io_bw(); __raw_writeb(v, a); }
static inline void writew(u16 v, volatile void __iomem *a) { __io_bw(); __raw_writew(v, a); }
static inline void writel(u32 v, volatile void __iomem *a) { __io_bw(); __raw_writel(v, a); }
static inline void writeq(u64 v, volatile void __iomem *a) { __io_bw(); __raw_writeq(v, a); }
#define readb_relaxed(a)        __raw_readb(a)
#define readw_relaxed(a)        __raw_readw(a)
#define readl_relaxed(a)        __raw_readl(a)
#define readq_relaxed(a)        __raw_readq(a)
#define writeb_relaxed(v, a)    __raw_writeb(v, a)
#define writew_relaxed(v, a)    __raw_writew(v, a)
#define writel_relaxed(v, a)    __raw_writel(v, a)
#define writeq_relaxed(v, a)    __raw_writeq(v, a)
#define ioread8(a)              readb(a)
#define ioread16(a)             readw(a)
#define ioread32(a)             readl(a)
#define ioread64(a)             readq(a)
#define iowrite8(v, a)          writeb(v, a)
#define iowrite16(v, a)         writew(v, a)
#define iowrite32(v, a)         writel(v, a)
#define iowrite64(v, a)         writeq(v, a)
#define ioread16be(a)           __builtin_bswap16(readw(a))
#define ioread32be(a)           __builtin_bswap32(readl(a))
#define iowrite16be(v, a)       writew(__builtin_bswap16(v), a)
#define iowrite32be(v, a)       writel(__builtin_bswap32(v), a)
#define readq_relaxed_lo_hi(a)  readq(a)
#define lo_hi_readq(a)          readq(a)
#define lo_hi_writeq(v, a)      writeq(v, a)
#define hi_lo_readq(a)          readq(a)
#define hi_lo_writeq(v, a)      writeq(v, a)
#define mmiowb()                do { } while (0)

void memcpy_fromio(void *dst, const volatile void __iomem *src, size_t count);
void memcpy_toio(volatile void __iomem *dst, const void *src, size_t count);
void memset_io(volatile void __iomem *dst, int c, size_t count);
#define ioread8_rep(a, b, c)    memcpy_fromio(b, a, c)
#define iowrite8_rep(a, b, c)   memcpy_toio(a, b, c)
static inline void ioread32_rep(const void __iomem *addr, void *buf, unsigned long count)
{
    u32 *b = buf;
    while (count--)
        *b++ = readl(addr);
}
static inline void iowrite32_rep(void __iomem *addr, const void *buf, unsigned long count)
{
    const u32 *b = buf;
    while (count--)
        writel(*b++, addr);
}

/*
 * ioremap goes through the PCI driver so that a bus address lands on the
 * CPU window that reaches it; the length is remembered for iounmap.
 */
void __iomem *ioremap(resource_size_t offset, unsigned long size);
void iounmap(volatile void __iomem *addr);
#define ioremap_nocache(o, s)   ioremap(o, s)
#define ioremap_wc(o, s)        ioremap(o, s)
#define ioremap_uc(o, s)        ioremap(o, s)
#define ioremap_cache(o, s)     ioremap(o, s)
#define ioremap_np(o, s)        ioremap(o, s)
#define devm_ioremap(d, o, s)   ioremap(o, s)
#define devm_ioremap_wc(d, o, s) ioremap(o, s)
#define devm_iounmap(d, a)      iounmap(a)
#define memremap(o, s, f)       ioremap(o, s)
#define memunmap(a)             iounmap(a)
#define MEMREMAP_WB             1
#define MEMREMAP_WT             2
#define MEMREMAP_WC             4
#define arch_io_reserve_memtype_wc(b, s)  (0)
#define arch_io_free_memtype_wc(b, s)     do { } while (0)
#define arch_phys_wc_add(b, s)  (0)
#define arch_phys_wc_del(h)     do { } while (0)
#define phys_to_dma(dev, p)     ((dma_addr_t)(p))
#define dma_to_phys(dev, d)     ((phys_addr_t)(d))
#define outb(v, p)              do { } while (0)
#define outw(v, p)              do { } while (0)
#define outl(v, p)              do { } while (0)
#define inb(p)                  (0xff)
#define inw(p)                  (0xffff)
#define inl(p)                  (0xffffffff)
#define virt_to_bus(a)          ((unsigned long)virt_to_phys(a))
#define bus_to_virt(a)          phys_to_virt(a)
#define IO_SPACE_LIMIT          0xffff
#define request_mem_region(s, n, name)  ((void *)1)
#define release_mem_region(s, n)        do { } while (0)
#define request_region(s, n, name)      ((void *)1)
#define release_region(s, n)            do { } while (0)
struct resource {
    resource_size_t start;
    resource_size_t end;
    const char *name;
    unsigned long flags;
};
static inline resource_size_t resource_size(const struct resource *res) { return res->end - res->start + 1; }
#define DEFINE_RES_MEM(s, l)    { .start = (s), .end = (s) + (l) - 1, .flags = IORESOURCE_MEM }

#endif /* _LINUX_IO_H_ */
