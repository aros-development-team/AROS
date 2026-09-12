/*
    Copyright 2009-2026, The AROS Development Team. All rights reserved.
*/

#ifndef _LINUX_RWSEM_H_
#define _LINUX_RWSEM_H_

#include <proto/exec.h>
#include <exec/semaphores.h>
#include <linux/types.h>
#include <linux/linkage.h>
#include <linux/list.h>
#include <linux/spinlock.h>
#include <linux/atomic.h>
#include <linux/err.h>
#include <linux/cleanup.h>

struct rw_semaphore {
    struct SignalSemaphore semaphore;
};
#define __RWSEM_INITIALIZER(name)   { }
#define DECLARE_RWSEM(name)         struct rw_semaphore name
static inline void init_rwsem(struct rw_semaphore *s)       { InitSemaphore(&s->semaphore); }
static inline void down_read(struct rw_semaphore *s)        { ObtainSemaphoreShared(&s->semaphore); }
static inline void up_read(struct rw_semaphore *s)          { ReleaseSemaphore(&s->semaphore); }
static inline void down_write(struct rw_semaphore *s)       { ObtainSemaphore(&s->semaphore); }
static inline void up_write(struct rw_semaphore *s)         { ReleaseSemaphore(&s->semaphore); }
static inline int  down_read_trylock(struct rw_semaphore *s) { return AttemptSemaphoreShared(&s->semaphore) ? 1 : 0; }
static inline int  down_write_trylock(struct rw_semaphore *s) { return AttemptSemaphore(&s->semaphore) ? 1 : 0; }
static inline int  down_read_interruptible(struct rw_semaphore *s) { down_read(s); return 0; }
static inline int  down_read_killable(struct rw_semaphore *s) { down_read(s); return 0; }
static inline int  down_write_killable(struct rw_semaphore *s) { down_write(s); return 0; }
static inline void downgrade_write(struct rw_semaphore *s)  { }
static inline int  rwsem_is_locked(struct rw_semaphore *s)  { return s->semaphore.ss_NestCount != 0 || s->semaphore.ss_QueueCount >= 0; }
#define down_read_nested(s, c)      down_read(s)
#define down_write_nested(s, c)     down_write(s)
#define down_read_non_owner(s)      down_read(s)
#define up_read_non_owner(s)        up_read(s)
#define lockdep_assert_held_write(s) do { } while (0)
#define lockdep_assert_held_read(s)  do { } while (0)

#include <linux/cleanup.h>
DEFINE_GUARD(rwsem_read, struct rw_semaphore *, down_read(_T), up_read(_T))
DEFINE_GUARD(rwsem_write, struct rw_semaphore *, down_write(_T), up_write(_T))

#endif /* _LINUX_RWSEM_H_ */
