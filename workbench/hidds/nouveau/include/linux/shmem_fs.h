/*
    Copyright 2009-2026, The AROS Development Team. All rights reserved.
*/

#ifndef _LINUX_SHMEM_FS_H_
#define _LINUX_SHMEM_FS_H_

#include <linux/fs.h>
#include <linux/file.h>
#include <linux/swap.h>
#include <linux/mempolicy.h>
#include <linux/pagemap.h>
#include <linux/percpu_counter.h>
#include <linux/xattr.h>
#include <linux/fs_parser.h>
#include <linux/userfaultfd_k.h>
static inline struct file *shmem_file_setup(const char *name, loff_t size, unsigned long flags) { return ERR_PTR(-ENOSYS); }
static inline struct page *shmem_read_mapping_page(struct address_space *m, pgoff_t index) { return ERR_PTR(-ENOSYS); }
static inline struct page *shmem_read_mapping_page_gfp(struct address_space *m, pgoff_t index, gfp_t g) { return ERR_PTR(-ENOSYS); }
static inline void shmem_truncate_range(struct inode *i, loff_t s, loff_t e) { }

#endif /* _LINUX_SHMEM_FS_H_ */
