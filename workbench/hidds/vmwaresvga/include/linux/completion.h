/*
    Copyright 2009-2026, The AROS Development Team. All rights reserved.
*/

#ifndef _LINUX_COMPLETION_H_
#define _LINUX_COMPLETION_H_

#include <linux/wait.h>
#include <linux/swait.h>

struct completion {
    unsigned int done;
    struct wait_queue_head wait;
};

#define COMPLETION_INITIALIZER(work)            { 0, __WAIT_QUEUE_HEAD_INITIALIZER((work).wait) }
#define COMPLETION_INITIALIZER_ONSTACK(work)    COMPLETION_INITIALIZER(work)
#define DECLARE_COMPLETION(work)                struct completion work = COMPLETION_INITIALIZER(work)
#define DECLARE_COMPLETION_ONSTACK(work)        DECLARE_COMPLETION(work)

static inline void init_completion(struct completion *x)
{
    x->done = 0;
    init_waitqueue_head(&x->wait);
}
static inline void reinit_completion(struct completion *x)
{
    x->done = 0;
}
void complete(struct completion *x);
void complete_all(struct completion *x);
void wait_for_completion(struct completion *x);
unsigned long wait_for_completion_timeout(struct completion *x, unsigned long timeout);
int  wait_for_completion_interruptible(struct completion *x);
long wait_for_completion_interruptible_timeout(struct completion *x, unsigned long timeout);
bool try_wait_for_completion(struct completion *x);
bool completion_done(struct completion *x);
#define wait_for_completion_killable(x)                 wait_for_completion_interruptible(x)
#define wait_for_completion_killable_timeout(x, t)      wait_for_completion_interruptible_timeout(x, t)
#define wait_for_completion_io(x)                       wait_for_completion(x)
#define wait_for_completion_io_timeout(x, t)            wait_for_completion_timeout(x, t)

#endif /* _LINUX_COMPLETION_H_ */
