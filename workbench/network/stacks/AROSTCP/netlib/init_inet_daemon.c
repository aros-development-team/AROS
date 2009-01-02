/* $Id$
 *
 *      init_inet_daemon.c - obtain socket accepted by the inetd
 *
 *      Copyright © 1994 AmiTCP/IP Group, 
 *                       Network Solutions Development Inc.
 *                       All rights reserved.
 */

/****** net.lib/init_inet_daemon ****************************************

    NAME
        init_inet_daemon - obtain socket accepted by the inetd

    SYNOPSIS
        int init_inet_daemon(void);

    FUNCTION
        Obtain the server socket accepted by the inetd, the Internet
        super-server.

    RETURN VALUES
        socket descriptor if successful, -1 with specific error code
        on errno otherwise.

    ERRORS
        ENXIO     - The process was not started by the inetd.

    NOTES
        If the process was started by the inetd, but the ObtainSocket()
        call fails, then this function exit()s with some specific exit
        code, so that inetd can clean up the unobtained socket.

        Use the net.lib function set_socket_stdio() to redirect stdio,
        stdout and stderr to the returned socket, if necessary.

    SEE ALSO
        serveraccept(), set_socket_stdio(), bsdsocket/ObtainSocket(),
        netutil/inetd
*****************************************************************************
*
*/

#include <exec/types.h>
#include <dos/dosextens.h>

#include <proto/socket.h>
#include <proto/exec.h>

#include <stdlib.h>
#include <errno.h>
#include <inetd.h>

int
init_inet_daemon(void)
{
  struct Process *me = (struct Process *)FindTask(NULL);
  struct DaemonMessage *dm = (struct DaemonMessage *)me->pr_ExitData;
  int sock;

  if (dm == NULL) {
    /*
     * No DaemonMessage, return error code
     */	
    errno = ENXIO; /* "Device not configured" */
    return -1;
  }
  
  /*
   * Obtain the server socket
   */
  sock = ObtainSocket(dm->dm_Id, dm->dm_Family, dm->dm_Type, 0);
  if (sock < 0) {
    /*
     * If ObtainSocket fails we need to exit with this specific exit code
     * so that the inetd knows to clean things up
     */
    exit(DERR_OBTAIN);
  }

  return sock;
}
