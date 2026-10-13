/*
    Copyright 2009-2026, The AROS Development Team. All rights reserved.
*/

#ifndef _LINUX_MUTEX_H_
#define _LINUX_MUTEX_H_

#include <proto/exec.h>
#include <exec/semaphores.h>
#include <aros/symbolsets.h>
#include <linux/types.h>
#include <linux/spinlock_types.h>
#include <linux/spinlock.h>
#include <linux/lockdep.h>
#include <linux/atomic.h>
#include <linux/list.h>
#include <linux/kernel.h>

struct mutex {
    struct SignalSemaphore semaphore;
    struct Task *owner;
    ULONG depth;
};

#define __MUTEX_INITIALIZER(name)   { .owner = NULL, .depth = 0 }
#define DEFINE_MUTEX(name)                                              \
    struct mutex name = __MUTEX_INITIALIZER(name);                      \
    static int __init_mutex_##name(void) { mutex_init(&name); return 1; } \
    ADD2INIT(__init_mutex_##name, 0)

static inline void mutex_init(struct mutex *m)
{
    InitSemaphore(&m->semaphore);
    m->owner = NULL;
    m->depth = 0;
}
#define __mutex_init(m, name, key)  mutex_init(m)
static inline void mutex_destroy(struct mutex *m)   { (void)m; }
/*
 * Linux code may take a mutex that was only ever zeroed (nouveau does so
 * for drm->audio.lock when the audio component is absent); a zeroed
 * SignalSemaphore is not usable, so bring such a mutex up on first use.
 */
static inline void mutex_ensure(struct mutex *m)
{
    if (m->semaphore.ss_Link.ln_Type != NT_SIGNALSEM) {
        Disable();
        if (m->semaphore.ss_Link.ln_Type != NT_SIGNALSEM)
            mutex_init(m);
        Enable();
    }
}
static inline void mutex_lock(struct mutex *m)
{
    mutex_ensure(m);
    ObtainSemaphore(&m->semaphore);
    m->owner = FindTask(NULL);
    m->depth++;
}
static inline void mutex_unlock(struct mutex *m)
{
    if (--m->depth == 0)
        m->owner = NULL;
    ReleaseSemaphore(&m->semaphore);
}
static inline int mutex_trylock(struct mutex *m)
{
    mutex_ensure(m);
    if (!AttemptSemaphore(&m->semaphore))
        return 0;
    m->owner = FindTask(NULL);
    m->depth++;
    return 1;
}
static inline int mutex_lock_interruptible(struct mutex *m) { mutex_lock(m); return 0; }
static inline int mutex_lock_killable(struct mutex *m)      { mutex_lock(m); return 0; }
static inline bool mutex_is_locked(struct mutex *m)          { return m->owner != NULL; }
#define mutex_lock_nested(m, s)                 mutex_lock(m)
#define mutex_lock_nest_lock(m, n)              mutex_lock(m)
#define mutex_lock_interruptible_nested(m, s)   mutex_lock_interruptible(m)
#define mutex_lock_killable_nested(m, s)        mutex_lock_killable(m)
#define mutex_lock_io(m)                        mutex_lock(m)
#define mutex_trylock_recursive(m)              mutex_trylock(m)
#define mutex_release(l, i)                     do { } while (0)
#define mutex_acquire(l, s, t, i)               do { } while (0)
#define mutex_acquire_nest(l, s, t, n, i)       do { } while (0)
#define lockdep_assert_held_mutex(m)            do { } while (0)

static inline int atomic_dec_and_mutex_lock(atomic_t *cnt, struct mutex *lock)
{
    mutex_lock(lock);
    if (atomic_dec_and_test(cnt))
        return 1;
    mutex_unlock(lock);
    return 0;
}
#include <linux/cleanup.h>
DEFINE_GUARD(mutex, struct mutex *, mutex_lock(_T), mutex_unlock(_T))
DEFINE_GUARD_COND(mutex, _try, mutex_trylock(_T))
DEFINE_GUARD_COND(mutex, _intr, mutex_lock_interruptible(_T) == 0)

#endif /* _LINUX_MUTEX_H_ */
