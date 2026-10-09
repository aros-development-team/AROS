#include <linux/errno.h>
#include <linux/mutex.h>
#include <linux/rwsem.h>
#include <linux/srcu.h>
/*
    Copyright 2009-2026, The AROS Development Team. All rights reserved.
*/

#ifndef _LINUX_NOTIFIER_H_
#define _LINUX_NOTIFIER_H_

struct notifier_block;
typedef int (*notifier_fn_t)(struct notifier_block *nb, unsigned long action, void *data);
struct notifier_block { notifier_fn_t notifier_call; struct notifier_block *next; int priority; };
struct blocking_notifier_head { struct notifier_block *head; };
struct atomic_notifier_head { struct notifier_block *head; };
#define BLOCKING_NOTIFIER_HEAD(name) struct blocking_notifier_head name = { NULL }
#define BLOCKING_INIT_NOTIFIER_HEAD(name) do { (name)->head = NULL; } while (0)
#define ATOMIC_INIT_NOTIFIER_HEAD(name) do { (name)->head = NULL; } while (0)
#define NOTIFY_DONE             0x0000
#define NOTIFY_OK               0x0001
#define NOTIFY_BAD              0x8002
#define NOTIFY_STOP             0x8000
static inline int blocking_notifier_chain_register(struct blocking_notifier_head *h, struct notifier_block *n) { return 0; }
static inline int blocking_notifier_chain_unregister(struct blocking_notifier_head *h, struct notifier_block *n) { return 0; }
static inline int blocking_notifier_call_chain(struct blocking_notifier_head *h, unsigned long v, void *p) { return NOTIFY_DONE; }
static inline int atomic_notifier_chain_register(struct atomic_notifier_head *h, struct notifier_block *n) { return 0; }
static inline int atomic_notifier_chain_unregister(struct atomic_notifier_head *h, struct notifier_block *n) { return 0; }
static inline int atomic_notifier_call_chain(struct atomic_notifier_head *h, unsigned long v, void *p) { return NOTIFY_DONE; }

#endif /* _LINUX_NOTIFIER_H_ */
