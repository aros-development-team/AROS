/*
    Copyright 2009-2026, The AROS Development Team. All rights reserved.
*/

#ifndef _LINUX_SPINLOCK_TYPES_H_
#define _LINUX_SPINLOCK_TYPES_H_

#include <proto/exec.h>
#include <exec/semaphores.h>
/* The real lock: kernel.resource's spinlock (KrnSpinLock & co.). Included
   here, before this file's spinlock_t macro, so the kernel protos still see
   AROS's own spinlock_t. KernelBase is set up in Nouveau_Init(). */
#include <aros/types/spinlock_s.h>
#include <proto/kernel.h>
#include <linux/types.h>

/*
 * Spinlocks are Forbid()/Disable() sections: the code inside never sleeps
 * and every target here is single-processor from the driver's point of view.
 */
/*
 * owner_nest is what the old Forbid()-only emulation counted. On SMP,
 * Forbid() stops only this CPU, so the DRM lists these locks guard were
 * raced from other CPUs (fences, push buffers, the workqueue - seen as
 * 'Nouveau WorkQueue' crashes on garbage and a flood of "push of a buffer
 * not in the push list"). krn carries a kernel spinlock: lock word, then
 * the owner, laid out as <aros/types/spinlock_s.h> expects.
 */
struct nouveau_krnspin {
    volatile unsigned int lock;
    unsigned int pad;
    void *s_Owner;
};
struct raw_spinlock {
    volatile ULONG owner_nest;
    ULONG pad;
    struct nouveau_krnspin krn;
};
struct spinlock {
    volatile ULONG owner_nest;
    ULONG pad;
    struct nouveau_krnspin krn;
};
/* the exec headers own the spinlock_t name */
#define spinlock_t          struct spinlock
typedef struct raw_spinlock raw_spinlock_t;
typedef struct { ULONG dummy; ULONG pad; struct nouveau_krnspin krn; } rwlock_t;

#define __SPIN_LOCK_UNLOCKED(x)     { 0 }
#define __RAW_SPIN_LOCK_UNLOCKED(x) { 0 }
#define __RW_LOCK_UNLOCKED(x)       { 0 }
#define DEFINE_SPINLOCK(x)          spinlock_t x = __SPIN_LOCK_UNLOCKED(x)
#define DEFINE_RAW_SPINLOCK(x)      raw_spinlock_t x = __RAW_SPIN_LOCK_UNLOCKED(x)
#define DEFINE_RWLOCK(x)            rwlock_t x = __RW_LOCK_UNLOCKED(x)

#endif /* _LINUX_SPINLOCK_TYPES_H_ */
