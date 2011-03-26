#ifndef _UCONTEXT_H_
#define _UCONTEXT_H_

/*
    Copyright © 1995-2011, The AROS Development Team. All rights reserved.
    $Id$

    Define the portable version of the execution context.
*/

#include <aros/cpu.h>
#include <sys/cdefs.h>
#include <signal.h>

#include <aros/types/ucontext_t.h>

__BEGIN_DECLS

int  getcontext(ucontext_t *ucp);
int  setcontext(const ucontext_t *ucp);
void makecontext(ucontext_t *ucp, void (*function)(), int argc, ...);
int  swapcontext(ucontext_t *oucp, const ucontext_t *nucp);

__END_DECLS

#endif /* _UCONTEXT_H_ */
