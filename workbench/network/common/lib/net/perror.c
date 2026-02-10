/* $Id$
 *
 *      perror.c - print error message
 *
 *      Copyright © 1994 AmiTCP/IP Group, 
 *                       Network Solutions Development Inc.
 *                       All rights reserved.
 */

/****** net.lib/perror *******************************************************

    NAME
        perror - socket error messages

    SYNOPSIS
        extern int errno;

        #include <stdio.h>

        perror(banner)
        void perror(const char *)

    FUNCTION
        The perror() function finds the error message corresponding to the
        current value of the global variable errno and writes it, followed
        by a newline, to the stderr. If the argument string is non-NULL it
        is preappended to the message string and separated from it by a
        colon and space (`: '). If string is NULL only the error message
        string is printed.

    NOTES
        The perror() function requires the stdio functions to be linked.

    SEE ALSO
        strerror(), PrintNetFault(), <netinclude:sys/errno.h>

******************************************************************************
*/

#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <clib/netlib_protos.h>

void 
perror(const char *banner)
{
  const char *err = strerror(errno);

  if (banner != NULL) {
    fputs(banner, stderr);
    fputs(": ", stderr);
  }
  fputs(err, stderr);
  fputc('\n', stderr);
  fflush(stderr);
}
