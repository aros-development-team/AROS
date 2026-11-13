/*
    Copyright 2026, The AROS Development Team. All rights reserved.
*/

#ifndef _LINUX_BUILD_BUG_H_
#define _LINUX_BUILD_BUG_H_

#define BUILD_BUG_ON_ZERO(e)        ((int)(sizeof(struct { int:(-!!(e)); })))
/*
 * A condition the optimiser can prove false costs nothing; one it cannot
 * prove leaves an undefined reference behind, which is where the build then
 * fails - the same contract as the kernel's own version.
 */
#define __compiletime_assert(condition, msg, prefix, suffix)            \
    do {                                                                \
        extern void prefix ## suffix(void) __attribute__((__error__(msg))); \
        if (!(condition))                                               \
            prefix ## suffix();                                         \
    } while (0)
#define _compiletime_assert(condition, msg, prefix, suffix) __compiletime_assert(condition, msg, prefix, suffix)
#define compiletime_assert(condition, msg) _compiletime_assert(condition, msg, __compiletime_assert_, __COUNTER__)
#define BUILD_BUG_ON_MSG(cond, msg) compiletime_assert(!(cond), msg)
#define BUILD_BUG_ON(condition)     BUILD_BUG_ON_MSG(condition, "BUILD_BUG_ON failed: " #condition)
#define BUILD_BUG()                 BUILD_BUG_ON_MSG(1, "BUILD_BUG failed")
#define BUILD_BUG_ON_INVALID(e)     ((void)(sizeof((long)(e))))
#define BUILD_BUG_ON_NOT_POWER_OF_2(n) BUILD_BUG_ON((n) == 0 || (((n) & ((n) - 1)) != 0))
#define static_assert(expr, ...)    _Static_assert(expr, ##__VA_ARGS__ "static assertion failed")

#endif /* _LINUX_BUILD_BUG_H_ */
