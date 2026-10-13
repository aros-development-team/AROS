/*
    Copyright 2026, The AROS Development Team. All rights reserved.
*/
#ifndef _LINUX_KMEMLEAK_H_
#define _LINUX_KMEMLEAK_H_

#define kmemleak_alloc(p, s, m, g)  do { } while (0)
#define kmemleak_free(p)            do { } while (0)
#define kmemleak_update_trace(p)    do { } while (0)
#define kmemleak_ignore(p)          do { } while (0)
#define kmemleak_not_leak(p)        do { } while (0)

#endif
