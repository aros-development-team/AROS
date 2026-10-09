/*
    Copyright 2009-2026, The AROS Development Team. All rights reserved.
*/

#ifndef _LINUX_LOCKDEP_H_
#define _LINUX_LOCKDEP_H_

#include <linux/kernel.h>
struct lock_class_key { int dummy; };
struct lockdep_map { const char *name; };
#define STATIC_LOCKDEP_MAP_INIT(n, k)   { 0 }
#define lockdep_assert_held(l)          do { (void)(l); } while (0)
#define lock_map_acquire(l)             do { } while (0)
#define lock_map_release(l)             do { } while (0)
#define lockdep_is_held(l)              (1)
#define SINGLE_DEPTH_NESTING            1
#define lock_is_held(l)                 (1)
#define lockdep_assert(c)               do { } while (0)
#define lockdep_assert_once(c)          do { } while (0)
#define lockdep_assert_none_held_once() do { } while (0)
#define lockdep_assert_not_held(l)      do { } while (0)
#define lockdep_assert_held_once(l)     do { } while (0)
#define lockdep_init_map(a, b, c, d)    do { } while (0)
#define lockdep_set_class(a, b)         do { } while (0)
#define lockdep_set_class_and_name(a, b, c) do { } while (0)
#define lockdep_register_key(k)         do { } while (0)
#define lockdep_unregister_key(k)       do { } while (0)
#define lock_acquire(...)               do { } while (0)
#define lock_release(...)               do { } while (0)
#define lock_acquire_shared_recursive(l, s, t, n, i) do { (void)(l); } while (0)
#define lock_acquire_shared(l, s, t, n, i) do { (void)(l); } while (0)
#define lock_acquire_exclusive(l, s, t, n, i) do { (void)(l); } while (0)
#define might_lock(l)                   do { } while (0)
#define might_lock_read(l)              do { } while (0)
#define might_lock_nested(l, s)         do { } while (0)
#define lockdep_assert_irqs_disabled()  do { } while (0)
#define lockdep_assert_preemption_disabled() do { } while (0)
#define lockdep_assert_preemption_enabled()  do { } while (0)
#define lockdep_assert_in_softirq()     do { } while (0)
#define lockdep_assert_held_write(s)    do { } while (0)
#define lockdep_assert_held_read(s)     do { } while (0)
#define lockdep_pin_lock(l)             (0)
#define lockdep_unpin_lock(l, c)        do { } while (0)
#define lockdep_depth(t)                (0)
#define lockdep_recursing(t)            (0)
#define debug_locks                     (0)
#define lockdep_off()                   do { } while (0)
#define lockdep_on()                    do { } while (0)
#define LD_WAIT_INV                     0
#define LD_WAIT_FREE                    0
#define LD_WAIT_SPIN                    0
#define LD_WAIT_CONFIG                  0
#define LD_WAIT_SLEEP                   0

#endif /* _LINUX_LOCKDEP_H_ */
