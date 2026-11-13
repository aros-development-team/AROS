/*
    Copyright (C) 1995-2026, The AROS Development Team. All rights reserved.

    UNIX header file <alloca.h>
    It is not part of POSIX or C99 but we provide it for legacy applications.
*/

#ifndef _POSIXC_ALLOCA_H
#define _POSIXC_ALLOCA_H

/*
 * This header is only available if explicitly requested. It is not part of
 * ISO C or POSIX, so it is exposed for the GNU/BSD/SVID feature sets, or when
 * a translation unit opts in explicitly via the private __AROS_ENABLE_ALLOCA__
 * flag. The private flag lets internal headers (e.g. amiga.lib's slow-stack
 * varargs helpers) pull in alloca() without enabling a whole feature set such
 * as _GNU_SOURCE, which would otherwise leak the GNU stdio extensions
 * (dprintf()/getline()/...) into the including translation unit.
 */
#if !defined(__AROS_ENABLE_ALLOCA__) && \
    (defined(_GNU_SOURCE) || defined(_BSD_SOURCE) || defined(_SVID_SOURCE))
#  define __AROS_ENABLE_ALLOCA__
#endif

#if defined(__AROS_ENABLE_ALLOCA__)

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
