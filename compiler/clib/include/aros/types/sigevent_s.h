#ifndef _AROS_TYPES_SIGEVENT_S_H
#define _AROS_TYPES_SIGEVENT_S_H

/*
    Copyright © 2010-2011, The AROS Development Team. All rights reserved.
    $Id: /aros/branches/ABI_V1/trunk-aroscsplit/AROS/compiler/arosnixc/include/aros/types/sigevent_s.h 36762 2011-01-11T21:10:26.430438Z verhaegs  $

    POSIX.1-2008 struct sigevent definition
*/

#define SIGEV_NONE	0	/* No notification */
#define SIGEV_SIGNAL	1	/* Generate a queued signal */
#define SIGEV_THREAD	2	/* Call a notification function */

/* Value passed to sigevent() SIGEV_THREAD functions */
union sigval
{
    int	    sigval_int;
    void *  sigval_ptr;
};

struct sigevent
{
    int		    sigev_notify;	/* notification type */
    union {
	int	    __sigev_signo;	/* signal number */
	struct {
	    void    (*__sigev_notify_function)(union sigval);
	} __sigev_notify_call;		/* call a function */
    } __sigev_u;
    union sigval    sigev_value;	/* signal value */
};

/* Send a signal to the process */
#define sigev_signo		__sigev_u.__sigev_signo

#endif /* _AROS_TYPES_SIGEVENT_S_H */
