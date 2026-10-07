/*
    Copyright 2009-2026, The AROS Development Team. All rights reserved.
*/

#ifndef _LINUX_FILE_H_
#define _LINUX_FILE_H_

#include <linux/fs.h>

#include <linux/cleanup.h>

struct fd {
    struct file *file;
};

#define fd_file(f)              ((f).file)
#define fd_empty(f)             (!(f).file)
static inline struct fd fdget(unsigned int fd) { struct fd f = { NULL }; return f; }
static inline void fdput(struct fd fd) { }
DEFINE_CLASS(fd, struct fd, fdput(_T), fdget(fd), int fd)

#endif /* _LINUX_FILE_H_ */
