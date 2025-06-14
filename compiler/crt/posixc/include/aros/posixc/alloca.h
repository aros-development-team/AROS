/*
    Copyright (C) 1995-2025, The AROS Development Team. All rights reserved.

    UNIX header file <alloca.h>
    It is not part of POSIX or C99 but we provide it for legacy applications.
*/

#ifndef _POSIXC_ALLOCA_H
#define _POSIXC_ALLOCA_H

/*
 * This header is only available if explicitly requested.
 * It is not part of ISO C or POSIX, so we check for the appropriate feature macros.
 */
#if defined(_GNU_SOURCE) || defined(_BSD_SOURCE) || defined(_SVID_SOURCE)

#include <aros/system.h>
#include <aros/types/size_t.h>

__BEGIN_DECLS

/* Allocate a block of memory which will be automatically freed upon function exit. */
void *alloca(size_t size);

#ifdef __GNUC__
#define alloca(size) __builtin_alloca(size)
#else
#error "alloca() is not supported on this compiler without __builtin_alloca"
#endif

__END_DECLS

#endif /* Feature macro guard */

#endif /* _POSIXC_ALLOCA_H */
