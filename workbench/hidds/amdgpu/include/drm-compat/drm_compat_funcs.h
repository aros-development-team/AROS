/*
    Copyright 2009-2026, The AROS Development Team. All rights reserved.
*/

#ifndef _DRM_COMPAT_FUNCS_
#define _DRM_COMPAT_FUNCS_

#include <proto/exec.h>
#include <aros/debug.h>

#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/mm.h>
#include <linux/io.h>
#include <linux/delay.h>
#include <linux/jiffies.h>
#include <linux/mutex.h>
#include <linux/spinlock.h>
#include <linux/wait.h>
#include <linux/workqueue.h>
#include <linux/interrupt.h>
#include <linux/firmware.h>
#include <linux/pci.h>
#include <linux/dma-mapping.h>

#include "drm_compat_mem.h"

#define IMPLEMENT(fmt, ...)         bug("------IMPLEMENT(%s): " fmt, __func__ , ##__VA_ARGS__)
#define NOT_IMPLEMENTED_STOP        { bug("NOT IMPLEMENTED STOP %s, %d\n", __func__, __LINE__); for (;;) { Wait(0); } }
#define NOT_IMPLEMENTED_CONTINUE    { bug("NOT IMPLEMENTED %s, %d\n", __func__, __LINE__); }

#endif /* _DRM_COMPAT_FUNCS_ */
