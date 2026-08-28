/*
    Copyright 2009-2026, The AROS Development Team. All rights reserved.
*/

#ifndef _LINUX_FAULT_INJECT_H_
#define _LINUX_FAULT_INJECT_H_

struct fault_attr { int dummy; };
#define DECLARE_FAULT_ATTR(n) struct fault_attr n
#define should_fail(a, s) (0)
#define fault_create_debugfs_attr(n, p, a) NULL

#endif /* _LINUX_FAULT_INJECT_H_ */
