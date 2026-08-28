/*
    Copyright 2009-2026, The AROS Development Team. All rights reserved.
*/

#ifndef _LINUX_SPINLOCK_H_
#define _LINUX_SPINLOCK_H_

#include <proto/exec.h>
#include <linux/spinlock_types.h>
#include <linux/atomic.h>
#include <linux/kernel.h>
#include <linux/typecheck.h>
#include <linux/preempt.h>
#include <linux/lockdep.h>
#include <linux/bottom_half.h>
#include <linux/irqflags.h>
#include <linux/mmdebug.h>
#include <linux/thread_info.h>
#include <linux/stringify.h>
#include <linux/compiler.h>
#include <linux/list.h>

static inline void spin_lock_init(spinlock_t *lock)                 { lock->owner_nest = 0; }
static inline void spin_lock(spinlock_t *lock)                      { Forbid(); lock->owner_nest++; }
static inline void spin_unlock(spinlock_t *lock)                    { lock->owner_nest--; Permit(); }
static inline int  spin_trylock(spinlock_t *lock)                   { spin_lock(lock); return 1; }
static inline int  spin_is_locked(spinlock_t *lock)                 { return lock->owner_nest != 0; }
static inline void assert_spin_locked(spinlock_t *lock)             { (void)lock; }
#define spin_lock_bh(l)                     spin_lock(l)
#define spin_unlock_bh(l)                   spin_unlock(l)
#define spin_trylock_bh(l)                  spin_trylock(l)
#define spin_lock_nested(l, s)              spin_lock(l)
#define spin_lock_nest_lock(l, n)           spin_lock(l)
#define spin_lock_irq(l)                    do { Disable(); spin_lock(l); } while (0)
#define spin_unlock_irq(l)                  do { spin_unlock(l); Enable(); } while (0)
#define spin_lock_irqsave(l, f)             do { (void)(f); Disable(); spin_lock(l); } while (0)
#define spin_lock_irqsave_nested(l, f, s)   spin_lock_irqsave(l, f)
#define spin_unlock_irqrestore(l, f)        do { spin_unlock(l); Enable(); (void)(f); } while (0)
#define spin_trylock_irq(l)                 ({ spin_lock_irq(l); 1; })
#define spin_trylock_irqsave(l, f)          ({ spin_lock_irqsave(l, f); 1; })
#define spin_lock_irqsave_check(l, f)       spin_lock_irqsave(l, f)
#define spin_lock_release(l)                do { } while (0)
#define spin_lock_acquire(l)                do { } while (0)
#define lockdep_assert_held_spin(l)         do { } while (0)
#define spinlock_check(l)                   (l)
#define local_bh_disable()                  do { } while (0)
#define local_bh_enable()                   do { } while (0)

static inline void raw_spin_lock_init(raw_spinlock_t *lock)         { lock->owner_nest = 0; }
static inline void raw_spin_lock(raw_spinlock_t *lock)              { Forbid(); lock->owner_nest++; }
static inline void raw_spin_unlock(raw_spinlock_t *lock)            { lock->owner_nest--; Permit(); }
static inline int  raw_spin_trylock(raw_spinlock_t *lock)           { raw_spin_lock(lock); return 1; }
static inline int  raw_spin_is_locked(raw_spinlock_t *lock)         { return lock->owner_nest != 0; }
#define raw_spin_lock_irq(l)                do { Disable(); raw_spin_lock(l); } while (0)
#define raw_spin_unlock_irq(l)              do { raw_spin_unlock(l); Enable(); } while (0)
#define raw_spin_lock_irqsave(l, f)         do { (void)(f); Disable(); raw_spin_lock(l); } while (0)
#define raw_spin_unlock_irqrestore(l, f)    do { raw_spin_unlock(l); Enable(); (void)(f); } while (0)
#define raw_spin_trylock_irqsave(l, f)      ({ raw_spin_lock_irqsave(l, f); 1; })

static inline void rwlock_init(rwlock_t *l)                         { l->dummy = 0; }
static inline void read_lock(rwlock_t *l)                           { Forbid(); }
static inline void read_unlock(rwlock_t *l)                         { Permit(); }
static inline void write_lock(rwlock_t *l)                          { Forbid(); }
static inline void write_unlock(rwlock_t *l)                        { Permit(); }
#define read_lock_bh(l)                     read_lock(l)
#define read_unlock_bh(l)                   read_unlock(l)
#define write_lock_bh(l)                    write_lock(l)
#define write_unlock_bh(l)                  write_unlock(l)
#define read_lock_irq(l)                    do { Disable(); read_lock(l); } while (0)
#define read_unlock_irq(l)                  do { read_unlock(l); Enable(); } while (0)
#define write_lock_irq(l)                   do { Disable(); write_lock(l); } while (0)
#define write_unlock_irq(l)                 do { write_unlock(l); Enable(); } while (0)
#define read_lock_irqsave(l, f)             do { (void)(f); Disable(); read_lock(l); } while (0)
#define read_unlock_irqrestore(l, f)        do { read_unlock(l); Enable(); (void)(f); } while (0)
#define write_lock_irqsave(l, f)            do { (void)(f); Disable(); write_lock(l); } while (0)
#define write_unlock_irqrestore(l, f)       do { write_unlock(l); Enable(); (void)(f); } while (0)

#include <linux/cleanup.h>
DEFINE_LOCK_GUARD_1(raw_spinlock, raw_spinlock_t, raw_spin_lock(_T->lock), raw_spin_unlock(_T->lock))
DEFINE_LOCK_GUARD_1(raw_spinlock_irq, raw_spinlock_t, raw_spin_lock_irq(_T->lock), raw_spin_unlock_irq(_T->lock))
DEFINE_LOCK_GUARD_1(raw_spinlock_irqsave, raw_spinlock_t, raw_spin_lock_irqsave(_T->lock, _T->flags), raw_spin_unlock_irqrestore(_T->lock, _T->flags), unsigned long flags)
DEFINE_LOCK_GUARD_1(spinlock, spinlock_t, spin_lock(_T->lock), spin_unlock(_T->lock))
DEFINE_LOCK_GUARD_1(spinlock_irq, spinlock_t, spin_lock_irq(_T->lock), spin_unlock_irq(_T->lock))
DEFINE_LOCK_GUARD_1(spinlock_irqsave, spinlock_t, spin_lock_irqsave(_T->lock, _T->flags), spin_unlock_irqrestore(_T->lock, _T->flags), unsigned long flags)
DEFINE_LOCK_GUARD_1(read_lock, rwlock_t, read_lock(_T->lock), read_unlock(_T->lock))
DEFINE_LOCK_GUARD_1(write_lock, rwlock_t, write_lock(_T->lock), write_unlock(_T->lock))

#endif /* _LINUX_SPINLOCK_H_ */
