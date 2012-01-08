#ifndef _ERRNO_H_
#define _ERRNO_H_

/*
    Copyright © 1995-2012, The AROS Development Team. All rights reserved.
    $Id$

    Desc: ANSI-C header file errno.h
    Lang: english
*/

#include <sys/errno.h>

/* AROS specific functions to translate DOS error numbers to errno.
   ioerrno2errno() will always call the function for the selected C
   linklib, __arosc_ioerr2errno() is always the arosc.library version.
 */
int ioerr2errno(int ioerr);
int __arosc_ioerr2errno(int ioerr); 

#define EDEADLOCK	EDEADLK
#define ENOTSUP		EOPNOTSUPP
#define MAX_ERRNO	ELAST	/* Last errno */

#endif /* _ERRNO_H_ */
