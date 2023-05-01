#ifndef _AROS_TYPES_UCONTEXT_T_H
#define _AROS_TYPES_UCONTEXT_T_H

/*
    Copyright © 2010-2012, The AROS Development Team. All rights reserved.
    $Id$

    Desc: POSIX.1-2004 mcontext_t and ucontext_t type definition
          Not present anymore in POSIX.1-2008
*/

#include <aros/types/sigset_t.h>
#include <aros/types/stack_t.h>

/* TODO: Determine the size of mcontext_t, probably depending on CPU */
typedef struct
{
    void *mc;
} mcontext_t;

typedef struct __ucontext
{
    struct __ucontext	*uc_link;
    sigset_t		 uc_sigmask;
    stack_t		 uc_stack;
    mcontext_t		 uc_mcontext;
} ucontext_t;

#endif /* _AROS_TYPES_UCONTEXT_T_H */
