/*
    Copyright 2009-2026, The AROS Development Team. All rights reserved.
*/

#ifndef _LINUX_DEBUGFS_H_
#define _LINUX_DEBUGFS_H_

#include <linux/fs.h>
#include <linux/seq_file.h>
#include <linux/types.h>
#include <linux/compiler.h>
struct dentry;
struct debugfs_reg32 { char *name; unsigned long offset; };
struct debugfs_regset32 { const struct debugfs_reg32 *regs; int nregs; void __iomem *base; struct device *dev; };
#define debugfs_create_file(n, m, p, d, f)      ((struct dentry *)NULL)
#define debugfs_create_dir(n, p)                ((struct dentry *)NULL)
#define debugfs_create_u32(n, m, p, v)          do { } while (0)
#define debugfs_create_u64(n, m, p, v)          do { } while (0)
#define debugfs_create_bool(n, m, p, v)         do { } while (0)
#define debugfs_create_atomic_t(n, m, p, v)     do { } while (0)
#define debugfs_create_regset32(n, m, p, r)     do { } while (0)
#define debugfs_create_x32(n, m, p, v)          do { } while (0)
#define debugfs_create_size_t(n, m, p, v)       do { } while (0)
#define debugfs_create_ulong(n, m, p, v)        do { } while (0)
#define debugfs_create_symlink(n, p, t)         ((struct dentry *)NULL)
#define debugfs_remove(d)                       do { } while (0)
#define debugfs_remove_recursive(d)             do { } while (0)
#define debugfs_lookup(n, p)                    ((struct dentry *)NULL)
#define debugfs_initialized()                   (0)
#define DEFINE_DEBUGFS_ATTRIBUTE(n, g, s, f)

#endif /* _LINUX_DEBUGFS_H_ */
