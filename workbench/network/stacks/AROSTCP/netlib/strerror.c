/* $Id$
 *
 *      strerror.c - network errno support for AmiTCP/IP
 *
 *      Copyright © 1994 AmiTCP/IP Group, 
 *                       Network Solutions Development Inc.
 *                       All rights reserved.
 *      Copyright © 2005 - 2011 Pavel Fedin
 */

#include <errno.h>
#include <proto/socket.h>
#include <bsdsocket/socketbasetags.h>

#include "conf.h"

/****** net.lib/strerror *****************************************************

    NAME
        strerror -- return the text for given error number

    SYNOPSIS
        string = strerror(error);

        char * strerror(int);

    FUNCTION
        This function returns pointer to the (English) string describing the
        error code given as argument. The error strings are defined for the
        error codes defined in <sys/errno.h>.

    NOTES
        The string pointed to by the return value should not be modified by
        the program, but may be overwritten by a subsequent call to this
        function.

    BUGS
        The strerror() prototype should be 
	const char *strerror(unsigned int); 
	However, the SAS C includes define it differently.

    SEE ALSO
        <netinclude:sys/errno.h>, perror(), PrintNetFault()
*****************************************************************************
*/

#ifdef notyet
const char *
strerror(unsigned int error)
#else
char *
strerror(int error)
#endif
{
  IPTR taglist[3];

  taglist[0] = SBTM_GETVAL(SBTC_ERRNOSTRPTR);
  taglist[1] = error;
  taglist[2] = TAG_END;

  SocketBaseTagList((struct TagItem *)taglist);
  return (char *)taglist[1];
}
