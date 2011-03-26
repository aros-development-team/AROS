#ifndef _AROS_TYPES_UCONTEXT_T_H
#define _AROS_TYPES_UCONTEXT_T_H

/*
    Copyright © 2010-2011, The AROS Development Team. All rights reserved.
    $Id: /aros/branches/ABI_V1/trunk-aroscsplit/AROS/compiler/arosnixc/include/aros/types/ucontext_t.h 35143 2010-10-23T21:19:57.420395Z verhaegs  $

    POSIX.1-2008 mcontext_t and ucontext_t type definition
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
    mcontext_t		 uc_mcontext;

    struct __ucontext	*uc_link;
    sigset_t		 uc_sigmask;
    stack_t		 uc_stack;
} ucontext_t;

#endif /* _AROS_TYPES_UCONTEXT_T_H */
