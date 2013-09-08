#ifndef _POSIXC_SYS_SELECT_H_
#define _POSIXC_SYS_SELECT_H_

/*
    Copyright © 2010-2012, The AROS Development Team. All rights reserved.
    $Id$

    Desc: POSIX.1-2008 header file sys/select.h
*/

#include <aros/cpu.h>

#include <aros/types/timeval_s.h> /* get struct timeval */

#include <aros/types/time_t.h>
#include <aros/types/suseconds_t.h>
#include <aros/types/sigset_t.h>
#include <aros/types/timespec_s.h>

#define NBBY            8

#ifndef FD_SETSIZE
#define FD_SETSIZE      64
#endif

typedef signed AROS_32BIT_TYPE
                        fd_mask;
#define NFDBITS         (sizeof(fd_mask) * NBBY)

#ifndef howmany
#define howmany(x, y)   (((x) + ((y) - 1)) / (y))
#endif

typedef struct fd_set {
    fd_mask fds_bits[howmany(FD_SETSIZE, NFDBITS)];
} fd_set;

#define FD_SET(n, p)    ((p)->fds_bits[(n)/NFDBITS] |=  (1 << ((n) % NFDBITS)))
#define FD_CLR(n, p)    ((p)->fds_bits[(n)/NFDBITS] &= ~(1 << ((n) % NFDBITS)))
#define FD_ISSET(n, p)  ((p)->fds_bits[(n)/NFDBITS] &   (1 << ((n) % NFDBITS)))
#define	FD_COPY(f, t)   memcpy(t, f, sizeof(*(f)))
#define	FD_ZERO(p)      memset(p, 0, sizeof(*(p)))

__BEGIN_DECLS

/* NOTIMPL int  pselect(int, fd_set *restrict, fd_set *restrict, fd_set *restrict,
   const struct timespec *restrict, const sigset_t *restrict); */
/* NOTIMPL int  select(int, fd_set *restrict, fd_set *restrict, fd_set *restrict,
   struct timeval *restrict); */

__END_DECLS

#endif /* _POSIXC_SYS_SELECT_H_ */
