/*
    Copyright © 1995-2012, The AROS Development Team. All rights reserved.
    $Id$

    UNIX header file <alloca.h>
    It is not part of POSIX or C99 but we provide for legacy applications.
*/

#ifndef	_POSIXC_ALLOCA_H
#define	_POSIXC_ALLOCA_H

#include <aros/system.h>

#include <aros/types/size_t.h>

__BEGIN_DECLS

/* Allocate a block of memory which will be automatically freed upon function exiting. */
void *alloca(size_t size);

#ifdef __GNUC__
#define alloca(size) __builtin_alloca(size)
#else
#error Unsupported compiler for alloca()
#endif /* GCC.  */

__END_DECLS


#endif /* _POSIXC_ALLOCA_H */
