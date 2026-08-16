/*
    Copyright 2009-2026, The AROS Development Team. All rights reserved.
*/

#ifndef _LINUX_SYNC_FILE_H_
#define _LINUX_SYNC_FILE_H_

#include <linux/dma-fence.h>
#include <linux/types.h>
#include <linux/ktime.h>
#include <linux/list.h>
#include <linux/spinlock.h>
#include <linux/dma-fence-array.h>
struct sync_file { struct file *file; struct dma_fence *fence; };
static inline struct sync_file *sync_file_create(struct dma_fence *f) { return NULL; }
static inline struct dma_fence *sync_file_get_fence(int fd) { return NULL; }

#endif /* _LINUX_SYNC_FILE_H_ */
