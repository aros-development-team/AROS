/*
    Copyright 2026, The AROS Development Team. All rights reserved.
*/

#ifndef _LINUX_CC_PLATFORM_H_
#define _LINUX_CC_PLATFORM_H_

#include <linux/types.h>
enum cc_attr { CC_ATTR_MEM_ENCRYPT, CC_ATTR_HOST_MEM_ENCRYPT, CC_ATTR_GUEST_MEM_ENCRYPT, CC_ATTR_GUEST_STATE_ENCRYPT };
static inline bool cc_platform_has(enum cc_attr attr) { return false; }

#endif /* _LINUX_CC_PLATFORM_H_ */
