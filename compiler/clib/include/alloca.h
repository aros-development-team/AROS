/*
    Copyright © 1995-2002, The AROS Development Team. All rights reserved.
    $Id$

    System header file <alloca.h>
*/

#ifndef	_ALLOCA_H
#define	_ALLOCA_H

#include <sys/cdefs.h>
#include <stddef.h>

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
             /*                                                                         \
                 private function to get the upper or lower bound                       \
                 (depending on the architecture) of the stack                           \
             */                                                                         \
             __BEGIN_DECLS                                                              \
             extern void *__alloca_get_stack_limit();                                   \
             __END_DECLS                                                                \
                                                                                        \
             ((void *)(AROS_GET_SP - AROS_ALIGN(size)) <= __alloca_get_stack_limit()) ? \
             NULL :                                                                     \
             __builtin_alloca(size);                                                    \
         })
#    else
#        define alloca(size)                                                            \
         ({                                                                             \
             /*                                                                         \
                 private function to get the upper or lower bound                       \
                 (depending on the architecture) of the stack                           \
             */                                                                         \
             __BEGIN_DECLS                                                              \
             extern void *__alloca_get_stack_limit();                                   \
             __END_DECLS                                                                \
                                                                                        \
             ((void *)(AROS_GET_SP + AROS_ALIGN(size)) >= __alloca_get_stack_limit()) ? \
             NULL :                                                                     \
             __builtin_alloca(size);                                                    \
         })
#    endif /* AROS_STACK_GROWS_DOWNWARDS */
#endif /* GCC.  */


#endif /* alloca.h */
