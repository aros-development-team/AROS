/*
    Copyright 2009-2026, The AROS Development Team. All rights reserved.
*/

#ifndef _LINUX_SPINLOCK_TYPES_H_
#define _LINUX_SPINLOCK_TYPES_H_

#include <proto/exec.h>
#include <exec/semaphores.h>
#include <linux/types.h>

struct raw_spinlock {
    volatile ULONG locked;
    struct Task * volatile owner;
    volatile ULONG owner_nest;
};
struct spinlock {
    volatile ULONG locked;
    struct Task * volatile owner;
    volatile ULONG owner_nest;
};
/* the exec headers own the spinlock_t name */
#define spinlock_t          struct spinlock
typedef struct raw_spinlock raw_spinlock_t;
typedef struct raw_spinlock rwlock_t;

#define __SPIN_LOCK_UNLOCKED(x)     { 0 }
#define __RAW_SPIN_LOCK_UNLOCKED(x) { 0 }
#define __RW_LOCK_UNLOCKED(x)       { 0 }
#define DEFINE_SPINLOCK(x)          spinlock_t x = __SPIN_LOCK_UNLOCKED(x)
#define DEFINE_RAW_SPINLOCK(x)      raw_spinlock_t x = __RAW_SPIN_LOCK_UNLOCKED(x)
#define DEFINE_RWLOCK(x)            rwlock_t x = __RW_LOCK_UNLOCKED(x)

#endif /* _LINUX_SPINLOCK_TYPES_H_ */
