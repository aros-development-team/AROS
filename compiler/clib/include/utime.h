#ifndef	_UTIME_H_
#define	_UTIME_H_

/*
    Copyright 1997-2001 AROS - The Amiga Research OS
    $Id$

    Desc: ANSI-C header file utime.h
    Lang: english
*/

#include <time.h>

struct utimbuf {
	time_t actime;		/* Access time */
	time_t modtime;		/* Modification time */
};


int utime (const char *, const struct utimbuf *);

#endif /* !_UTIME_H_ */
