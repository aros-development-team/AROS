#ifndef _SYS_TIMEB_H_
#define _SYS_TIMEB_H_
/*
    Copyright © 1995-2012, The AROS Development Team. All rights reserved.
    $Id$

    Desc: POSIX.1-2001 header file <sys/timeb.h>
          This is deprecated and not present anymore in POSIX.1-2008
*/

#include <aros/system.h>

#include <aros/types/time_t.h>

struct timeb
{
    time_t	    time;
    unsigned short  millitm;
    short	    timezone;
    short	    dstflag;
};

__BEGIN_DECLS

int ftime(struct timeb *tp);

__END_DECLS

#endif /* _SYS_TIMEB_H_ */
