#ifndef _STDC_SIGNAL_H_
#define _STDC_SIGNAL_H_

/*
    Copyright © 1995-2012, The AROS Development Team. All rights reserved.
    $Id$

    C99 header file <signal.h>
*/

#include <aros/system.h>

#include <aros/types/__sighandler_t.h>

#define SIG_DFL	    ((__sighandler_t *)0)   /* default signal handling */
#define SIG_IGN	    ((__sighandler_t *)1)   /* ignore this signal */
#define SIG_ERR	    ((__sighandler_t *)-1)  /* return from signal() on error */

typedef AROS_SIG_ATOMIC_T   sig_atomic_t;

/* Signal values */
#define SIGABRT		6	/* abort() */
#define SIGFPE		8	/* floating point exception */
#define SIGILL		4	/* illegal instr. */
#define SIGINT		2	/* interrupt */
#define SIGSEGV		11	/* segmentation violation */
#define SIGTERM		15	/* software termination */

/* Function Prototypes */
__BEGIN_DECLS

__sighandler_t *signal(int, __sighandler_t *);
int raise(int);

__END_DECLS

#endif /* _STDC_SIGNAL_H_ */
