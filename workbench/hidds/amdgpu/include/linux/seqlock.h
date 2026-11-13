/*
    Copyright 2009-2026, The AROS Development Team. All rights reserved.
*/

#ifndef _LINUX_SEQLOCK_H_
#define _LINUX_SEQLOCK_H_

#include <linux/spinlock.h>
#include <linux/mutex.h>
#include <linux/compiler.h>
#include <linux/kcsan-checks.h>
#include <linux/lockdep.h>
#include <linux/preempt.h>
#include <linux/spinlock.h>
#include <linux/ww_mutex.h>
#include <linux/processor.h>

typedef struct seqcount {
    unsigned sequence;
} seqcount_t;
typedef struct { seqcount_t seqcount; } seqcount_spinlock_t;
typedef struct { seqcount_t seqcount; } seqcount_mutex_t;
typedef struct { seqcount_t seqcount; } seqcount_ww_mutex_t;
typedef struct { seqcount_t seqcount; spinlock_t lock; } seqlock_t;

#define SEQCNT_ZERO(name)               { 0 }
#define SEQCNT_SPINLOCK_ZERO(name, l)   { { 0 } }
#define SEQCNT_MUTEX_ZERO(name, l)      { { 0 } }
#define SEQCNT_WW_MUTEX_ZERO(name, l)   { { 0 } }
#define __SEQLOCK_UNLOCKED(name)        { { 0 }, __SPIN_LOCK_UNLOCKED(name) }
#define DEFINE_SEQLOCK(x)               seqlock_t x = __SEQLOCK_UNLOCKED(x)

static inline void __seqcount_init(seqcount_t *s)   { s->sequence = 0; }
#define seqcount_init(s)                __seqcount_init(s)
#define seqcount_spinlock_init(s, l)    __seqcount_init(&(s)->seqcount)
#define seqcount_mutex_init(s, l)       __seqcount_init(&(s)->seqcount)
#define seqcount_ww_mutex_init(s, l)    __seqcount_init(&(s)->seqcount)
#define seqlock_init(sl)                do { __seqcount_init(&(sl)->seqcount); spin_lock_init(&(sl)->lock); } while (0)

static inline unsigned __read_seqcount_begin(const seqcount_t *s)
{
    unsigned ret;
    for (;;) {
        ret = READ_ONCE(s->sequence);
        if (!(ret & 1))
            break;
        Forbid(); Permit();
    }
    __sync_synchronize();
    return ret;
}
static inline unsigned __read_seqcount_retry(const seqcount_t *s, unsigned start)
{
    __sync_synchronize();
    return READ_ONCE(s->sequence) != start;
}
static inline void __write_seqcount_begin(seqcount_t *s) { s->sequence++; __sync_synchronize(); }
static inline void __write_seqcount_end(seqcount_t *s)   { __sync_synchronize(); s->sequence++; }
#define __seqprop_ptr(s) _Generic(*(s),                                 \
    seqcount_t:             (seqcount_t *)(s),                          \
    seqcount_spinlock_t:    &((seqcount_spinlock_t *)(s))->seqcount,    \
    seqcount_mutex_t:       &((seqcount_mutex_t *)(s))->seqcount,       \
    seqcount_ww_mutex_t:    &((seqcount_ww_mutex_t *)(s))->seqcount)
#define read_seqcount_begin(s)          __read_seqcount_begin(__seqprop_ptr(s))
#define raw_read_seqcount_begin(s)      __read_seqcount_begin(__seqprop_ptr(s))
#define raw_read_seqcount(s)            READ_ONCE(__seqprop_ptr(s)->sequence)
#define read_seqcount_retry(s, start)   __read_seqcount_retry(__seqprop_ptr(s), start)
#define write_seqcount_begin(s)         __write_seqcount_begin(__seqprop_ptr(s))
#define write_seqcount_end(s)           __write_seqcount_end(__seqprop_ptr(s))
#define write_seqcount_invalidate(s)    do { __seqprop_ptr(s)->sequence += 2; } while (0)
#define raw_write_seqcount_begin(s)     write_seqcount_begin(s)
#define raw_write_seqcount_end(s)       write_seqcount_end(s)
#define seqcount_lockdep_reader_access(s) do { } while (0)
static inline unsigned read_seqbegin(const seqlock_t *sl)               { return __read_seqcount_begin(&sl->seqcount); }
static inline unsigned read_seqretry(const seqlock_t *sl, unsigned s)   { return __read_seqcount_retry(&sl->seqcount, s); }
static inline void write_seqlock(seqlock_t *sl)   { spin_lock(&sl->lock); __write_seqcount_begin(&sl->seqcount); }
static inline void write_sequnlock(seqlock_t *sl) { __write_seqcount_end(&sl->seqcount); spin_unlock(&sl->lock); }
#define write_seqlock_irqsave(sl, f)    do { (void)(f); write_seqlock(sl); } while (0)
#define write_sequnlock_irqrestore(sl, f) do { write_sequnlock(sl); (void)(f); } while (0)
#define write_seqlock_irq(sl)           write_seqlock(sl)
#define write_sequnlock_irq(sl)         write_sequnlock(sl)

#endif /* _LINUX_SEQLOCK_H_ */
