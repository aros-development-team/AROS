#ifndef _POSIXC_UCONTEXT_H_
#define _POSIXC_UCONTEXT_H_

/*
    Copyright © 1995-2012, The AROS Development Team. All rights reserved.
    $Id$

    Desc: POSIX.1-2004 header file ucontext.h
          This file is deprecated and not present in POSIX.1-2008
*/

#include <aros/system.h>

#include <aros/types/ucontext_t.h>

__BEGIN_DECLS

/* NOTIMPL int getcontext(ucontext_t *ucp); */
/* NOTIMPL int setcontext(const ucontext_t *ucp); */
/* NOTIMPL void makecontext(ucontext_t *ucp, void (*function)(), int argc, ...); */
/* NOTIMPL int swapcontext(ucontext_t *oucp, const ucontext_t *nucp); */

__END_DECLS

#endif /* _POSIXC_UCONTEXT_H_ */
