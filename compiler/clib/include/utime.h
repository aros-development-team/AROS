#ifndef	_UTIME_H_
#define	_UTIME_H_

/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: ANSI-C header file utime.h
    Lang: english
*/

#include <time.h>

struct utimbuf {
	time_t actime;		/* Access time */
	time_t modtime;		/* Modification time */
};


int utime(const char *filename, struct utimbuf *buf);

#endif /* !_UTIME_H_ */
