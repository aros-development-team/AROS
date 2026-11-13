/*
    Copyright 2009-2026, The AROS Development Team. All rights reserved.
*/

#ifndef _LINUX_MMAN_H_
#define _LINUX_MMAN_H_

#include <linux/mm.h>
#include <linux/percpu_counter.h>
#include <linux/atomic.h>
#include <uapi/linux/mman.h>
#define VM_READ         0x1
#define VM_WRITE        0x2
#define VM_EXEC         0x4
#define VM_SHARED       0x8
#define VM_MAYREAD      0x10
#define VM_MAYWRITE     0x20
#define VM_MAYEXEC      0x40
#define VM_MAYSHARE     0x80
#define VM_IO           0x4000
#define VM_DONTEXPAND   0x40000
#define VM_DONTDUMP     0x4000000
#define VM_PFNMAP       0x400
#define VM_MIXEDMAP     0x10000000

#endif /* _LINUX_MMAN_H_ */
