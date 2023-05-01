#ifndef _AROS_TYPES_SIGEVENT_S_H
#define _AROS_TYPES_SIGEVENT_S_H

/*
    Copyright © 2010-2012, The AROS Development Team. All rights reserved.
    $Id$

    Desc: POSIX.1-2008 struct sigevent definition
*/

#define SIGEV_NONE	0	/* No notification */
#define SIGEV_SIGNAL	1	/* Generate a queued signal */
#define SIGEV_THREAD	2	/* Call a notification function */

union sigval
{
    int	    sigval_int;
    void *  sigval_ptr;
};

struct sigevent
{
    int		    sigev_notify;	/* notification type */
    int	            sigev_signo;	/* signal number */
    union sigval    sigev_value;	/* signal value */
    void	  (*__sigev_notify_function)(union sigval);	/* call a function */
    /* NOTIMPL pthread_attr_t	*segev_notify_attributes; */
    void *pad; /* Remove when segev_notify_attributes is implemented */
};

#endif /* _AROS_TYPES_SIGEVENT_S_H */
