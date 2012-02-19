#ifndef _SYS_TIMEB_H_
#define _SYS_TIMEB_H_
/*
    Copyright © 1995-2002, The AROS Development Team. All rights reserved.
    $Id$

    Header <sys/timeb.h>
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
