/*
    Copyright 2009-2026, The AROS Development Team. All rights reserved.
*/

#ifndef _LINUX_UTIL_MACROS_H_
#define _LINUX_UTIL_MACROS_H_

#define __find_closest(x, a, as, op) ({ typeof(as) __fc_i, __fc_as = (as) - 1; typeof(x) __fc_x = (x); typeof(*a) const *__fc_a = (a); for (__fc_i = 0; __fc_i < __fc_as; __fc_i++) { if (__fc_x op DIV_ROUND_CLOSEST(__fc_a[__fc_i] + __fc_a[__fc_i + 1], 2)) break; } (__fc_i); })
#define find_closest(x, a, as) __find_closest(x, a, as, <=)
#define find_closest_descending(x, a, as) __find_closest(x, a, as, >=)
#define is_insidevar(ptr, var) ((uintptr_t)(ptr) >= (uintptr_t)(var) && (uintptr_t)(ptr) < (uintptr_t)(var) + sizeof(*(var)))
#define PTR_IF(cond, ptr)      ((cond) ? (ptr) : NULL)
#define for_each_if(condition) if (!(condition)) {} else

#endif /* _LINUX_UTIL_MACROS_H_ */
