/*
    Copyright 2026, The AROS Development Team. All rights reserved.
*/

#ifndef _LINUX_MINMAX_H_
#define _LINUX_MINMAX_H_

#include <linux/types.h>
#include <linux/build_bug.h>
#include <linux/compiler.h>
#include <linux/const.h>

#define __cmp_op_min <
#define __cmp_op_max >
#define __cmp(op, x, y)     ((x) __cmp_op_##op (y) ? (x) : (y))
#define __cmp_once(op, x, y) ({ typeof(x) __x = (x); typeof(y) __y = (y); __cmp(op, __x, __y); })
#define __cmp_once_t(op, type, x, y) ({ type __x = (x); type __y = (y); __cmp(op, __x, __y); })

#define min(x, y)               __cmp_once(min, x, y)
#define max(x, y)               __cmp_once(max, x, y)
#define umin(x, y)              __cmp_once_t(min, u64, x, y)
#define umax(x, y)              __cmp_once_t(max, u64, x, y)
#define min_t(type, x, y)       __cmp_once_t(min, type, x, y)
#define max_t(type, x, y)       __cmp_once_t(max, type, x, y)
#define MIN(a, b)               (((a) < (b)) ? (a) : (b))
#define MAX(a, b)               (((a) > (b)) ? (a) : (b))
#define MIN_T(type, a, b)       MIN((type)(a), (type)(b))
#define MAX_T(type, a, b)       MAX((type)(a), (type)(b))
#define min3(x, y, z)           min((typeof(x))min(x, y), z)
#define max3(x, y, z)           max((typeof(x))max(x, y), z)
#define min_not_zero(x, y) ({ typeof(x) __x = (x); typeof(y) __y = (y); __x == 0 ? __y : ((__y == 0) ? __x : min(__x, __y)); })
#define clamp(val, lo, hi)      min(max(val, lo), hi)
#define clamp_t(type, val, lo, hi) min_t(type, max_t(type, val, lo), hi)
#define clamp_val(val, lo, hi)  clamp_t(typeof(val), val, lo, hi)
#define swap(a, b)              do { typeof(a) __tmp = (a); (a) = (b); (b) = __tmp; } while (0)
#define __minmax_array(op, array, len) ({                               \
    typeof(&(array)[0]) __array = (array);                              \
    typeof(len) __len = (len);                                          \
    typeof(__array[0]) __element = __array[--__len];                    \
    while (__len--)                                                     \
        __element = op(__element, __array[__len]);                      \
    __element; })
#define min_array(array, len)   __minmax_array(min, array, len)
#define max_array(array, len)   __minmax_array(max, array, len)
#define in_range(val, start, len) ((val) >= (start) && (val) < (start) + (len))

#endif /* _LINUX_MINMAX_H_ */
