/*
    Copyright 2009-2026, The AROS Development Team. All rights reserved.
*/

#ifndef _LINUX_SWAP_H_
#define _LINUX_SWAP_H_

#include <linux/mm.h>
#include <linux/spinlock.h>
#include <linux/linkage.h>
#include <linux/mmzone.h>
#include <linux/list.h>
#include <linux/memcontrol.h>
#include <linux/sched.h>
#include <linux/node.h>
#include <linux/fs.h>
#include <linux/pagemap.h>
#include <linux/atomic.h>
#include <linux/page-flags.h>
#include <uapi/linux/mempolicy.h>
#define check_move_unevictable_pages(pv) do { } while (0)
#define check_move_unevictable_folios(fb) do { } while (0)

#endif /* _LINUX_SWAP_H_ */
