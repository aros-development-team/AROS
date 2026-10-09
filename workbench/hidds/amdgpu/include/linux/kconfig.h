/*
    Copyright 2026, The AROS Development Team. All rights reserved.
*/

#ifndef _LINUX_KCONFIG_H_
#define _LINUX_KCONFIG_H_

#define __ARG_PLACEHOLDER_1 0,
#define __take_second_arg(__ignored, val, ...) val
#define __and(x, y)     ___and(x, y)
#define ___and(x, y)    ____and(__ARG_PLACEHOLDER_##x, y)
#define ____and(arg1_or_junk, y) __take_second_arg(arg1_or_junk y, 0)
#define __or(x, y)      ___or(x, y)
#define ___or(x, y)     ____or(__ARG_PLACEHOLDER_##x, y)
#define ____or(arg1_or_junk, y) __take_second_arg(arg1_or_junk 1, y)
#define __is_defined(x) ___is_defined(x)
#define ___is_defined(val) ____is_defined(__ARG_PLACEHOLDER_##val)
#define ____is_defined(arg1_or_junk) __take_second_arg(arg1_or_junk 1, 0)
#define IS_BUILTIN(option) __is_defined(option)
#define IS_MODULE(option) __is_defined(option##_MODULE)
#define IS_REACHABLE(option) __or(IS_BUILTIN(option), __and(IS_MODULE(option), __is_defined(MODULE)))
#define IS_ENABLED(option) __or(IS_BUILTIN(option), IS_MODULE(option))

#endif /* _LINUX_KCONFIG_H_ */
