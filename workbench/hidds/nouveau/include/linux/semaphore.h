/*
    Copyright 2009-2026, The AROS Development Team. All rights reserved.
*/

#ifndef _LINUX_SEMAPHORE_H_
#define _LINUX_SEMAPHORE_H_

#include <linux/rwsem.h>
struct semaphore {
    struct SignalSemaphore semaphore;
};
static inline void sema_init(struct semaphore *s, int val) { InitSemaphore(&s->semaphore); }
static inline void down(struct semaphore *s)               { ObtainSemaphore(&s->semaphore); }
static inline void up(struct semaphore *s)                 { ReleaseSemaphore(&s->semaphore); }
static inline int  down_trylock(struct semaphore *s)       { return AttemptSemaphore(&s->semaphore) ? 0 : 1; }
static inline int  down_interruptible(struct semaphore *s) { down(s); return 0; }

#endif /* _LINUX_SEMAPHORE_H_ */
