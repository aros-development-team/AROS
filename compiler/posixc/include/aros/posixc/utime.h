#ifndef	_UTIME_H_
#define	_UTIME_H_

/*
    Copyright © 1995-2012, The AROS Development Team. All rights reserved.
    $Id$

    Desc: POSIX.1-2008 header file utime.h
*/

#include <aros/types/time_t.h>

struct utimbuf {
    time_t actime;		/* Access time */
    time_t modtime;		/* Modification time */
};

__BEGIN_DECLS

int utime(const char *, const struct utimbuf *);

__END_DECLS

#endif /* !_UTIME_H_ */
