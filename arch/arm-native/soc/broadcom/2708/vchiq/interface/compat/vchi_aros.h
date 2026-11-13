/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    The Linux kernel primitives the Broadcom VCHIQ core expects, on exec.

    Everything here runs in task context: the doorbell interrupt only
    signals the vchiq IRQ task, which polls the remote events. So locks
    are SignalSemaphores and the counting semaphores sleep on a signal.
*/

#ifndef __VCHI_AROS_H__
#define __VCHI_AROS_H__

#include <exec/types.h>
#include <exec/lists.h>
#include <exec/semaphores.h>
#include <exec/tasks.h>

#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <sys/types.h>

#include <aros/debug.h>

/*
 * Barriers
 */
#define dsb(opt)        __asm__ __volatile__("dsb " #opt : : : "memory")
#define wmb()           dsb(st)
#define rmb()           dsb(ld)
#define mb()            dsb(sy)
#define smp_mb()        dsb(sy)
#define smp_rmb()       rmb()
#define smp_wmb()       wmb()

/*
 * Atomic API
 */
typedef volatile int atomic_t;

#define ATOMIC_INIT(v)          (v)
#define atomic_set(p, v)        __atomic_store_n((p), (v), __ATOMIC_SEQ_CST)
#define atomic_read(p)          __atomic_load_n((p), __ATOMIC_SEQ_CST)
#define atomic_inc(p)           __atomic_add_fetch((p), 1, __ATOMIC_SEQ_CST)
#define atomic_dec(p)           __atomic_sub_fetch((p), 1, __ATOMIC_SEQ_CST)
#define atomic_add(v, p)        __atomic_add_fetch((p), (v), __ATOMIC_SEQ_CST)
#define atomic_sub(v, p)        __atomic_sub_fetch((p), (v), __ATOMIC_SEQ_CST)
#define atomic_add_return(v, p) __atomic_add_fetch((p), (v), __ATOMIC_SEQ_CST)
#define atomic_sub_return(v, p) __atomic_sub_fetch((p), (v), __ATOMIC_SEQ_CST)
#define atomic_inc_return(p)    atomic_add_return(1, p)
#define atomic_dec_return(p)    atomic_sub_return(1, p)
#define atomic_dec_and_test(p)  (atomic_dec(p) == 0)
#define atomic_xchg(p, v)       __atomic_exchange_n((p), (v), __ATOMIC_SEQ_CST)

static inline int atomic_cmpxchg(atomic_t *v, int oldv, int newv)
{
    int expected = oldv;

    __atomic_compare_exchange_n(v, &expected, newv, 0,
                                __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST);
    return expected;
}

/*
 * Locks. All of them may sleep, which is fine with no IRQ-context users.
 */
/* exec has a spinlock_t of its own */
#define spinlock_t              vchiq_spinlock_t
typedef struct SignalSemaphore spinlock_t;

#define DEFINE_SPINLOCK(name)   spinlock_t name
#define spin_lock_init(l)       InitSemaphore(l)
#define spin_lock(l)            ObtainSemaphore(l)
#define spin_unlock(l)          ReleaseSemaphore(l)
#define spin_lock_bh(l)         spin_lock(l)
#define spin_unlock_bh(l)       spin_unlock(l)

struct mutex {
    struct SignalSemaphore mtx;
};

#define lmutex_init(l)                  InitSemaphore(&(l)->mtx)
#define lmutex_lock(l)                  ObtainSemaphore(&(l)->mtx)
#define lmutex_lock_interruptible(l)    (ObtainSemaphore(&(l)->mtx), 0)
#define lmutex_unlock(l)                ReleaseSemaphore(&(l)->mtx)
#define lmutex_destroy(l)               do { } while (0)

/*
 * Counting semaphore
 */
struct semaphore {
    struct SignalSemaphore lock;
    int value;
    struct MinList waiters;
};

void _sema_init(struct semaphore *s, int value);
void _sema_destroy(struct semaphore *s);
void down(struct semaphore *s);
int down_interruptible(struct semaphore *s);
int down_trylock(struct semaphore *s);
void up(struct semaphore *s);

/*
 * Completions, rwlocks and timers only appear in the suspend state the
 * AROS port leaves out, apart from the keepalive event.
 */
struct completion {
    struct semaphore sem;
};

#define init_completion(c)      _sema_init(&(c)->sem, 0)
#define complete(c)             up(&(c)->sem)
#define wait_for_completion_interruptible(c)    down_interruptible(&(c)->sem)

typedef struct SignalSemaphore rwlock_t;

#define rwlock_init(l)          InitSemaphore(l)
#define read_lock_bh(l)         ObtainSemaphoreShared(l)
#define read_unlock_bh(l)       ReleaseSemaphore(l)
#define write_lock_bh(l)        ObtainSemaphore(l)
#define write_unlock_bh(l)      ReleaseSemaphore(l)

struct timer_list {
    int unused;
};

#ifndef PAGE_SIZE
#define PAGE_SIZE               4096
#endif
#define PAGE_ALIGN(x)           (((x) + PAGE_SIZE - 1) & ~(PAGE_SIZE - 1))

#define min(a, b)               (((a) < (b)) ? (a) : (b))

/*
 * Threads
 */
typedef struct Task *VCHIQ_THREAD_T;

VCHIQ_THREAD_T vchiq_thread_create(int (*threadfn)(void *data),
                                   void *data, const char namefmt[], ...);
void set_user_nice(VCHIQ_THREAD_T p, int nice);
void wake_up_process(VCHIQ_THREAD_T p);

#define current                 FindTask(NULL)
#define flush_signals(t)        do { } while (0)
#define fatal_signal_pending(t) 0

/*
 * Logging and assertions
 */
#define printf(fmt, ...)        bug(fmt, ##__VA_ARGS__)
#define printk(fmt, ...)        bug(fmt, ##__VA_ARGS__)
#define panic(fmt, ...)         do { bug(fmt "\n", ##__VA_ARGS__); for (;;) ; } while (0)

#define KERN_ERR                ""
#define KERN_WARNING            ""
#define KERN_INFO               ""
#define KERN_CONT               ""

#define might_sleep()

#define WARN_ON(condition)                                      \
({                                                              \
        int __ret_warn_on = !!(condition);                      \
        if (unlikely(__ret_warn_on))                            \
                bug("WARN_ON: %s (%s:%d)\n", #condition,        \
                    __FILE__, __LINE__);                        \
        unlikely(__ret_warn_on);                                \
})

#define WARN(condition, msg)                                    \
({                                                              \
        int __ret_warn_on = !!(condition);                      \
        if (unlikely(__ret_warn_on))                            \
                bug("%s", (msg));                               \
        unlikely(__ret_warn_on);                                \
})

#define WARN_ON_ONCE(condition) WARN_ON(condition)

#define BUG_ON(cond)                                            \
        do {                                                    \
                if (cond)                                       \
                        panic("BUG_ON: " #cond);                \
        } while (0)

#define BUG()   panic("BUG: %s:%d", __FILE__, __LINE__)

/*
 * Memory
 */
#define GFP_KERNEL              0
#define GFP_ATOMIC              0

void *kzalloc(size_t size, int flags);
void kfree(const void *p);
#define kmalloc(size, flags)    kzalloc((size), (flags))

/*
 * Misc
 */
#define __init
#define __exit
#define __user
#ifndef __unused
#define __unused                __attribute__((unused))
#endif
#define EXPORT_SYMBOL(x)

#define likely(x)               __builtin_expect(!!(x), 1)
#define unlikely(x)             __builtin_expect(!!(x), 0)

void udelay(unsigned int usec);

#define BCM2835_MBOX_CHAN_VCHIQ 3

#endif /* __VCHI_AROS_H__ */
