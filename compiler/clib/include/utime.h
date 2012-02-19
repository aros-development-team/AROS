#ifndef	_UTIME_H_
#define	_UTIME_H_

/*
    Copyright © 1995-2011, The AROS Development Team. All rights reserved.
    $Id$

    Desc: ANSI-C header file utime.h
    Lang: english
*/

#include <aros/types/time_t.h>

struct utimbuf {
	time_t actime;		/* Access time */
	time_t modtime;		/* Modification time */
};

__BEGIN_DECLS
int utime(const char *filename, const struct utimbuf *buf);
__END_DECLS

#endif /* !_UTIME_H_ */
