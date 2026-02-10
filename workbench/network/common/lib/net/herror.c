/* $Id$
 *
 *      herror.c - print host error message
 *
 *      Copyright © 1994 AmiTCP/IP Group, 
 *                       Network Solutions Development Inc.
 *                       All rights reserved.
 *      Copyright © 2005 Pavel Fedin
 */

/****** net.lib/herror *******************************************************

    NAME
        herror - print name resolver error message to stderr.

    SYNOPSIS
        #include <clib/netlib_protos.h>

        herror(banner)
        void herror(const char *)

    FUNCTION
        The herror() function finds the error message corresponding to the
        current value of host error using the SocketBaseTags() and writes
        it, followed by a newline, to the stderr. If the argument string
        is non-NULL it is used as a prefix to the message string and
        separated from it by a colon and space (`: '). If the argument is
        NULL only the error message string is printed.

    NOTES
        The herror() function requires the stdio functions to be linked.

    SEE ALSO
        <netinclude:netdb.h>, SocketBaseTagList(), perror()

******************************************************************************
*/

#include <stdio.h>
#include <string.h>
#include <proto/socket.h>
#include <bsdsocket/socketbasetags.h>

void 
herror(const char *banner)
{
  const char *err;

  /*
   * First fetch the h_errno value to (ULONG)err, and then convert it to 
   * error string pointer.
   */
  SocketBaseTags(SBTM_GETREF(SBTC_HERRNO), &err,
		 SBTM_GETREF(SBTC_HERRNOSTRPTR), &err,
		 TAG_END);

  if (banner != NULL) {
    fputs(banner, stderr);
    fputs(": ", stderr);
  }
  fputs(err, stderr);
  fputc('\n', stderr);
  fflush(stderr);
}
