/*
    Copyright 2009-2026, The AROS Development Team. All rights reserved.
*/

#ifndef _LINUX_VMALLOC_H_
#define _LINUX_VMALLOC_H_

#include <linux/types.h>
#include <linux/gfp.h>
#include <linux/mm.h>
#include <linux/list.h>
#include <linux/rbtree.h>

#define VM_MAP                  0x00000004
#define VM_IOREMAP              0x00000001
#define VM_USERMAP              0x00000008
#define VM_NO_GUARD             0x00000040

#endif /* _LINUX_VMALLOC_H_ */
