/*
    Copyright © 1995-2002, The AROS Development Team. All rights reserved.
    $Id$

    System header file <alloca.h>
*/

#ifndef	_ALLOCA_H
#define	_ALLOCA_H	

#include <sys/types.h>

__BEGIN_DECLS

/* Discard any previous definition.  */
#undef alloca

/* Allocate a block of memory which will be automatically freed upon function exiting. */
extern void *alloca(size_t size);

/* GNU C provides a builtin alloca function. */
#ifdef __GNUC__
#    define alloca(size) __builtin_alloca(size)
#endif /* GCC.  */

__END_DECLS

#endif /* alloca.h */
