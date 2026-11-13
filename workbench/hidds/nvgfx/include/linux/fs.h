/*
    Copyright 2009-2026, The AROS Development Team. All rights reserved.
*/

#ifndef _LINUX_FS_H_
#define _LINUX_FS_H_

#include <linux/types.h>
#include <linux/list.h>
#include <linux/wait.h>
#include <linux/mutex.h>
#include <linux/rwsem.h>
#include <linux/mm.h>
#include <linux/rbtree.h>
#include <linux/list.h>
#include <linux/errno.h>

/*
 * There is no VFS: a struct file is what the ioctl entry points get handed
 * and it only carries the drm_file pointer.
 */
struct address_space { void *host; gfp_t gfp_mask; };
struct inode { void *i_private; unsigned long i_ino; struct address_space *i_mapping; struct address_space i_data; };
struct poll_table_struct;
struct vm_area_struct;
struct kiocb;
struct iov_iter;
struct dentry { const char *d_name; };
struct file {
    void *private_data;
    struct inode *f_inode;
    unsigned int f_flags;
    fmode_t f_mode;
    loff_t f_pos;
    struct address_space *f_mapping;
    struct dentry *f_path_dentry;
    struct { struct dentry *dentry; } f_path;
};
struct file_operations {
    struct module *owner;
    int (*open)(struct inode *, struct file *);
    int (*release)(struct inode *, struct file *);
    long (*unlocked_ioctl)(struct file *, unsigned int, unsigned long);
    long (*compat_ioctl)(struct file *, unsigned int, unsigned long);
    int (*mmap)(struct file *, struct vm_area_struct *);
    __poll_t (*poll)(struct file *, struct poll_table_struct *);
    ssize_t (*read)(struct file *, char __user *, size_t, loff_t *);
    ssize_t (*write)(struct file *, const char __user *, size_t, loff_t *);
    loff_t (*llseek)(struct file *, loff_t, int);
    int (*fasync)(int, struct file *, int);
    int (*flush)(struct file *, void *);
    ssize_t (*show_fdinfo)(struct file *, void *);
    unsigned long (*get_unmapped_area)(struct file *, unsigned long, unsigned long, unsigned long, unsigned long);
    int (*fop_flags);
};
#define FOP_UNSIGNED_OFFSET     0
#define O_NONBLOCK              0x800
#define O_CLOEXEC               0x80000
#define O_RDWR                  2
#define O_RDONLY                0
#define O_WRONLY                1
#define FMODE_READ              0x1
#define FMODE_WRITE             0x2
#define FMODE_UNSIGNED_OFFSET   0x2000
#define S_IRUGO                 0444
#define S_IWUSR                 0200
#define S_IRUSR                 0400
#define S_IFREG                 0100000
#define SEEK_SET                0
#define SEEK_CUR                1
#define SEEK_END                2
static inline loff_t noop_llseek(struct file *f, loff_t o, int w) { return 0; }
static inline loff_t no_llseek(struct file *f, loff_t o, int w) { return -ESPIPE; }
#define nonseekable_open(i, f)  (0)
#define fput(f)                 do { } while (0)
#define get_file(f)             (f)
#define fget(fd)                ((struct file *)NULL)
#define file_inode(f)           ((f)->f_inode)
#define iminor(i)               (0)
#define imajor(i)               (0)
#define simple_read_from_buffer(a, b, c, d, e) (0)
#define simple_write_to_buffer(a, b, c, d, e) (0)
#define memcpy_fromiovec(a, b, c) (0)
#define fixed_size_llseek(f, o, w, s) (0)
#define generic_file_llseek(f, o, w) (0)
#define anon_inode_getfile(n, f, p, fl) ERR_PTR(-ENOSYS)
#define get_unused_fd_flags(f)  (-ENOSYS)
#define put_unused_fd(fd)       do { } while (0)
#define fd_install(fd, f)       do { } while (0)
#define mapping_set_gfp_mask(m, g) do { } while (0)
#define mapping_gfp_mask(m)     GFP_KERNEL
#define mapping_set_unevictable(m) do { } while (0)
#define i_size_read(i)          (0)
#define i_size_write(i, s)      do { } while (0)
#define stream_open(i, f)       (0)
struct seq_file { void *private; char *buf; size_t size; size_t count; };
#define seq_printf(m, fmt, ...) do { } while (0)
#define seq_puts(m, s)          do { } while (0)
#define seq_putc(m, c)          do { } while (0)
#define seq_write(m, s, n)      (0)
#define seq_vprintf(m, f, a)    do { } while (0)
#define seq_hex_dump(...)       do { } while (0)
#define single_open(f, s, d)    (-ENOSYS)
#define single_release(i, f)    (0)
#define seq_read                NULL
#define seq_lseek               NULL
#define DEFINE_SHOW_ATTRIBUTE(n)
#define DEFINE_SIMPLE_ATTRIBUTE(n, g, s, f)
#define DEFINE_DEBUGFS_ATTRIBUTE(n, g, s, f)

#endif /* _LINUX_FS_H_ */
