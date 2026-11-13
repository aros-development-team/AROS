/*
    Copyright 2009-2026, The AROS Development Team. All rights reserved.
*/

#ifndef _LINUX_SRCU_H_
#define _LINUX_SRCU_H_

#include <linux/rcupdate.h>
#include <linux/mutex.h>
#include <linux/workqueue.h>
struct srcu_struct { int dummy; };
#define DEFINE_SRCU(name)               struct srcu_struct name
#define DEFINE_STATIC_SRCU(name)        static struct srcu_struct name
#define init_srcu_struct(s)             (0)
#define cleanup_srcu_struct(s)          do { } while (0)
#define srcu_read_lock(s)               (0)
#define srcu_read_unlock(s, i)          do { } while (0)
#define synchronize_srcu(s)             do { } while (0)
#define srcu_dereference(p, s)          (p)
#define srcu_read_lock_held(s)          (1)

#endif /* _LINUX_SRCU_H_ */
