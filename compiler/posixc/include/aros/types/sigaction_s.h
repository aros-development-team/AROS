#ifndef _AROS_TYPES_SIGACTION_S_H
#define _AROS_TYPES_SIGACTION_S_H

/*
    Copyright © 2010-2012, The AROS Development Team. All rights reserved.
    $Id: /aros/branches/ABI_V1/trunk-aroscsplit/AROS/compiler/arosnixc/include/aros/types/sigaction_s.h 35142 2010-10-23T20:40:12.589298Z verhaegs  $

    Desc: POSIX.1-2008 struct sigaction definition
*/

#include <aros/types/sigset_t.h>
#include <aros/types/siginfo_t.h>

/* sa_flags field */
#define SA_NOCLDSTOP	0x0001
#define SA_ONSTACK	0x0002
#define	SA_RESETHAND	0x0004
#define SA_RESTART	0x0008
#define SA_SIGINFO	0x0010
#define SA_NOCLDWAIT	0x0020
#define SA_NODEFER	0x0040

/*
    sigaction() provides an advanced interface for setting signal handling
    options.
*/
struct sigaction
{
    union {
	void		(*__sa_handler)(int);
	void		(*__sa_sigaction)(int, siginfo_t *, void *);
    } __sigaction_u;			/* signal handler */
    sigset_t		sa_mask;	/* signal mask to apply */
    int			sa_flags;	/* see above */
};

#define sa_handler	__sigaction_u.__sa_handler
/* if SA_SIGINFO is set, use sa_sigaction rather than sa_handler */
#define sa_sigaction	__sigaction_u.__sa_sigaction

#endif /* _AROS_TYPES_SIGACTION_S_H */
