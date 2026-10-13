/*
    Copyright 2009-2026, The AROS Development Team. All rights reserved.
*/

#ifndef _LINUX_DYNAMIC_DEBUG_H_
#define _LINUX_DYNAMIC_DEBUG_H_

#define DYNAMIC_DEBUG_BRANCH(d)     (0)
#define DEFINE_DYNAMIC_DEBUG_METADATA(n, f)
#define _dynamic_func_call_cls(cls, fmt, func, ...) do { } while (0)
#define _dynamic_func_call_no_desc(id, func, ...) do { } while (0)
#define _dynamic_func_call(id, func, ...) do { } while (0)
#define DECLARE_DYNDBG_CLASSMAP(...)
#define DYNDBG_CLASSMAP_DEFINE(...)
#define DYNDBG_CLASSMAP_PARAM(...)
#define DYNDBG_CLASSMAP_PARAM_REF(...)
#define DYNDBG_CLASSMAP_USE(...)
#define DYNDBG_CLASSMAP_PARAM_REF_ARGS(...)
#define DD_CLASS_TYPE_DISJOINT_BITS 0
#define __dynamic_pr_debug(d, fmt, ...) do { } while (0)

#endif /* _LINUX_DYNAMIC_DEBUG_H_ */
