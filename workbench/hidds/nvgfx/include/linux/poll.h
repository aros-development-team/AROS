/*
    Copyright 2009-2026, The AROS Development Team. All rights reserved.
*/

#ifndef _LINUX_POLL_H_
#define _LINUX_POLL_H_

#include <linux/fs.h>
#include <linux/wait.h>
#include <linux/string.h>
typedef struct poll_table_struct { int dummy; } poll_table;
#define poll_wait(f, w, p)      do { } while (0)
#define EPOLLIN                 0x1
#define EPOLLRDNORM             0x40
#define EPOLLOUT                0x4
#define EPOLLERR                0x8
#define EPOLLHUP                0x10

#endif /* _LINUX_POLL_H_ */
