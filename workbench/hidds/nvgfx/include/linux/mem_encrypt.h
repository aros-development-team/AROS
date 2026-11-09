/*
    Copyright 2009-2026, The AROS Development Team. All rights reserved.
*/

#ifndef _LINUX_MEM_ENCRYPT_H_
#define _LINUX_MEM_ENCRYPT_H_

static inline bool mem_encrypt_active(void) { return false; }
#define force_dma_unencrypted(d) (0)

#include <linux/cc_platform.h>
#endif /* _LINUX_MEM_ENCRYPT_H_ */
