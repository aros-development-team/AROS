/*
    Copyright © 1995-2002, The AROS Development Team. All rights reserved.
    $Id$

    System header file <alloca.h>
*/

#ifndef	_ALLOCA_H
#define	_ALLOCA_H

#include <proto/exec.h>
#include <sys/types.h>


/* Discard any previous definition.  */
#undef alloca

__BEGIN_DECLS

/* Allocate a block of memory which will be automatically freed upon function exiting. */
extern void *alloca(size_t size);

__END_DECLS

/* GNU C provides a builtin alloca function. */
#ifdef __GNUC__
#    if AROS_STACK_GROWS_DOWNWARDS
#        define alloca(size)                                                            \
         ({                                                                             \
             ((void *)(AROS_GET_SP - AROS_ALIGN(size)) <= FindTask(NULL)->tc_SPLower) ? \
             NULL :                                                                     \
             __builtin_alloca(size);                                                    \
         })
#    else
#        define alloca(size)                                                            \
         ({                                                                             \
             ((void *)(AROS_GET_SP + AROS_ALIGN(size)) >= FindTask(NULL)->tc_SPUpper) ? \
             NULL :                                                                     \
             __builtin_alloca(size);                                                    \
         })
#    endif /* AROS_STACK_GROWS_DOWNWARDS */
#endif /* GCC.  */


#endif /* alloca.h */
