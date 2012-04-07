/*
    Copyright © 1995-2012, The AROS Development Team. All rights reserved.
    $Id$

    POSIX.1-2008 function mkstemp().
*/
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <proto/dos.h>

#include "__upath.h"

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
    A file descriptor of opened temporary file or -1 on error.

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

******************************************************************************/
{
    char *c = template + strlen(template);
    char *c_start;
    BPTR  lock= BNULL;
    int ctr = 0;
    const char filename_letters[] = "01234567890abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZFILLTO64";
    const char *atemplate;

    while (c > template && *--c == 'X') {
        ctr++;
    }

    if (ctr < 6) {
        errno = EINVAL;
        return -1;      
    }
    
    c++;
    c_start = c;

    while (1) {
        while (*c) {
            *c = filename_letters[rand() & 0x3F];
            c++;
        }

        atemplate = __path_u2a(template);
        if(!atemplate)
            return -1;
        if (!(lock = Lock(atemplate, ACCESS_READ))) {
            int fd = open(template, O_WRITE|O_CREAT|O_EXCL);
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
    
    errno = EEXIST;
    return -1; 
 
} /* mkstemp() */
