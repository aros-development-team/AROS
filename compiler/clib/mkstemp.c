/*
    Copyright © 1995-2003, The AROS Development Team. All rights reserved.
    $Id$

    POSIX function mkstemp().
*/

#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <proto/dos.h>

/*****************************************************************************

    NAME */
#include <stdlib.h>

	int mkstemp(

/*  SYNOPSIS */
	char *template)

/*  FUNCTION

    INPUTS
        A template that must end with 'XXXXXX'
        
    RESULT

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

******************************************************************************/
{
    char *c = template + strlen(template);
    char *c_start;
    BPTR  lock;
    int ctr = 0;
    static char filename_letters[] = "01234567890abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZFILLTO64";
    
    while (c > template && *--c == 'X') {
      ctr++;
    }

    if (ctr < 6) {
      return EINVAL;
    }
    
    c++;
    c_start = c;

    while (1) {
        while (*c) {
            *c = filename_letters[rand() & 0x3F];
            c++;
        }
        if (!(lock = Lock(template, ACCESS_READ))) {
            int fd = open(template, O_CREAT|O_EXCL);
            if (fd > 0) 
                return fd;
        }
	UnLock(lock);
        c = c_start;
        /*
         * Try around 1000 filenames and then give up.
         */
        if (++ctr > 1000)
            break;
    }

    return EEXIST; 
 
} /* mkstemp() */
