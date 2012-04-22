/*
    Copyright © 1995-2012, The AROS Development Team. All rights reserved.
    $Id$

    UNIX header file <alloca.h>
    It is not part of POSIX or C99 but we provide for legacy applications.
*/

#ifndef	_ALLOCA_H
#define	_ALLOCA_H

#include <aros/system.h>

#include <aros/types/size_t.h>

__BEGIN_DECLS

/* Allocate a block of memory which will be automatically freed upon function exiting. */
void *alloca(size_t size);

#ifdef __GNUC__
/* Private function to get the upper or lower bound (depending on the architecture)
   of the stack.  */
/* FIXME: Can't it be made more general ? */
void *__alloca_get_stack_limit(void);

/* GNU C provides a builtin alloca function. */
#    if AROS_STACK_GROWS_DOWNWARDS
#        define alloca(size)                                                            \
         ({                                                                             \
             ((void *)(AROS_GET_SP - AROS_ALIGN(size)) <= __alloca_get_stack_limit()) ? \
             NULL :                                                                     \
             __builtin_alloca(size);                                                    \
         })
#    else
#        define alloca(size)                                                            \
         ({                                                                             \
             ((void *)(AROS_GET_SP + AROS_ALIGN(size)) >= __alloca_get_stack_limit()) ? \
             NULL :                                                                     \
             __builtin_alloca(size);                                                    \
         })
#    endif /* AROS_STACK_GROWS_DOWNWARDS */
#endif /* GCC.  */

__END_DECLS


#endif /* alloca.h */
