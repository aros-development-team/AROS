/*
    Copyright 2009-2026, The AROS Development Team. All rights reserved.
*/

#ifndef _LINUX_KERNEL_H_
#define _LINUX_KERNEL_H_

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>

#include <proto/exec.h>

#include <linux/types.h>
#include <linux/compiler.h>
#include <linux/stddef.h>
#include <linux/container_of.h>
#include <linux/array_size.h>
#include <linux/stringify.h>
#include <linux/typecheck.h>
#include <linux/build_bug.h>
#include <linux/const.h>
#include <linux/kconfig.h>
#include <linux/limits.h>
#include <linux/errno.h>
#include <linux/err.h>
#include <linux/minmax.h>
#include <linux/math.h>
#include <linux/bitops.h>
#include <linux/log2.h>
#include <linux/kstrtox.h>
#include <linux/printk.h>
#include <linux/bug.h>
#include <linux/panic.h>
#include <linux/instruction_pointer.h>
#include <linux/hex.h>
#include <linux/wordpart.h>
#include <linux/lockdep.h>
#include <linux/sprintf.h>

#undef ALIGN
#define ALIGN(x, a)                 (((x) + ((typeof(x))(a) - 1)) & ~((typeof(x))(a) - 1))
#define ALIGN_DOWN(x, a)            ((x) & ~((typeof(x))(a) - 1))
#define __ALIGN_KERNEL(x, a)        ALIGN(x, a)
#define __ALIGN_KERNEL_MASK(x, m)   (((x) + (m)) & ~(m))
#define __ALIGN_MASK(x, m)          __ALIGN_KERNEL_MASK(x, m)
#define PTR_ALIGN(p, a)             ((typeof(p))ALIGN((IPTR)(p), (a)))
#define PTR_ALIGN_DOWN(p, a)        ((typeof(p))ALIGN_DOWN((IPTR)(p), (a)))
#define IS_ALIGNED(x, a)            (((x) & ((typeof(x))(a) - 1)) == 0)

#define upper_32_bits(n)            ((u32)(((n) >> 16) >> 16))
#define lower_32_bits(n)            ((u32)((n) & 0xffffffff))
#define upper_16_bits(n)            ((u16)((n) >> 16))
#define lower_16_bits(n)            ((u16)((n) & 0xffff))

#define might_sleep()               do { } while (0)
#define might_sleep_if(c)           do { } while (0)
#define might_fault()               do { } while (0)
#define cond_resched()              (0)
#define cant_sleep()                do { } while (0)
#define non_block_start()           do { } while (0)
#define non_block_end()             do { } while (0)
#define local_irq_disable()         Disable()
#define local_irq_enable()          Enable()
#define local_irq_save(f)           do { (void)(f); Disable(); } while (0)
#define local_irq_restore(f)        do { (void)(f); Enable(); } while (0)
#define in_interrupt()              (0)
#define in_atomic()                 (0)
#define in_irq()                    (0)
#define in_task()                   (1)
#define irqs_disabled()             (0)
#define preempt_disable()           Forbid()
#define preempt_enable()            Permit()
#define preempt_disable_notrace()   Forbid()
#define preempt_enable_notrace()    Permit()
#define preempt_enable_no_resched() Permit()
#define preemptible()               (1)
#define get_cpu()                   (0)
#define put_cpu()                   do { } while (0)
#define smp_processor_id()          (0)
#define raw_smp_processor_id()      (0)
#define num_online_cpus()           (1)
#define num_possible_cpus()         (1)
#define cpu_relax()                 __asm__ __volatile__("" : : : "memory")
#define cpu_relax_lowlatency()      cpu_relax()
#define touch_softlockup_watchdog() do { } while (0)
#define oops_in_progress            (0)
#define __maybe_unused_arg(x)       (void)(x)
#define REPEAT_BYTE(x)              ((~0ul / 0xff) * (x))
#define ROUND_UP(x, y)              ALIGN(x, y)
#define ROUND_DOWN(x, y)            ALIGN_DOWN(x, y)
#define IS_ERR_VALUE_PTR(p)         IS_ERR(p)
#define TAINT_WARN                  0
#define add_taint(a, b)             do { } while (0)
#define KBUILD_MODNAME              "nouveau"

#define __round_mask(x, y)          ((typeof(x))((y) - 1))
#define round_up(x, y)              ((((x) - 1) | __round_mask(x, y)) + 1)
#define round_down(x, y)            ((x) & ~__round_mask(x, y))

#define sched_annotate_sleep()      do { } while (0)
#define trace_printk(...)           do { } while (0)
#define dump_stack()                do { } while (0)
void print_hex_dump(const char *level, const char *prefix_str, int prefix_type,
    int rowsize, int groupsize, const void *buf, size_t len, bool ascii);
#define print_hex_dump_bytes(p, t, b, l)    print_hex_dump("", p, t, 16, 1, b, l, true)
#define print_hex_dump_debug(...)   do { } while (0)
#define hex_dump_to_buffer(...)     (0)
#define DUMP_PREFIX_NONE            0
#define DUMP_PREFIX_ADDRESS         1
#define DUMP_PREFIX_OFFSET          2

/* character helpers that ctype.h may lack in freestanding builds */
#include <ctype.h>
#include <linux/ctype.h>

/* string -> number helpers */
static inline unsigned long simple_strtoul(const char *cp, char **endp, unsigned int base)
{
    return strtoul(cp, endp, base);
}
static inline long simple_strtol(const char *cp, char **endp, unsigned int base)
{
    return strtol(cp, endp, base);
}
static inline unsigned long long simple_strtoull(const char *cp, char **endp, unsigned int base)
{
    return strtoull(cp, endp, base);
}
static inline long long simple_strtoll(const char *cp, char **endp, unsigned int base)
{
    return strtoll(cp, endp, base);
}

/* struct sysinfo / si_meminfo: only totalram is ever consulted */
struct sysinfo {
    unsigned long totalram;
    unsigned long freeram;
    unsigned long totalhigh;
    unsigned long freehigh;
    unsigned int mem_unit;
};
void si_meminfo(struct sysinfo *si);

extern int panic_timeout;
#define CAP_SYS_ADMIN 21
#define capable(c) (true)
#define request_module(fmt, ...) (0)
#define orderly_poweroff(f) do { } while (0)
#define I2C_MODULE_PREFIX "i2c:"

#include <linux/uaccess.h>

#endif /* _LINUX_KERNEL_H_ */
