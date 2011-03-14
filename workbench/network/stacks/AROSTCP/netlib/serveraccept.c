/* $Id$
 *
 *      serveraccept - accept a server connection on named port
 *
 *      Copyright © 1994 AmiTCP/IP Group, 
 *                       Network Solutions Development Inc.
 *                       All rights reserved.
 *      Copyright © 2005 Pavel Fedin
 */

/****** net.lib/serveraccept ***********************************************
 
    NAME
        serveraccept - Accept a server connection on named port
 
    SYNOPSIS
        socket = serveraccept(name, peer);
 
        long serveraccept(char *, struct sockaddr_in *);
 
    DESCRIPTION
        The serveraccept() library call binds a socket to the named Internet
        TCP port. Then it listens the socket and accepts the connection to
        the port. The peer's socket address is returned in sockaddr pointed
        by sockaddr argument.
 
        The port name is resolved by getservbyname() call. A numeric value
        for port name is also accepted.
  
        This module is meant for daemon developing.
 
    INPUTS
        name   - port name or numeric string.
        peer   - pointer to struct sockaddr_in
 
    RESULT
        socket - positive socket id for success or -1 for failure.
 
        peer   - sockaddr_in structure containing peer's internet address.
                 Note that on error, the structure containing peer address
                 is not necessarily updated.
 
    SEE ALSO
        bsdsocket/accept, bsdsocket/getservbyname
 
*****************************************************************************
*
*/

#ifdef AMIGA
#include <clib/netlib_protos.h>
#include <proto/socket.h>
#include <proto/dos.h>
#endif /* AMIGA */

#include <errno.h>
#include <netdb.h>

#include <sys/param.h>
#include <sys/socket.h>
#include <netinet/in.h>

#include <signal.h>

#include <dos/dos.h>
#include <dos/var.h>

#include <stdlib.h>
#include <string.h>

/*
 * serveraccept:
 *      Accept a server socket from the named port
 */
long
serveraccept(char *pname, struct sockaddr_in *ha)
{
  struct sockaddr_in sin; 
  socklen_t ha_len = sizeof(*ha);
  int s, sa;
  LONG port;
  struct servent *sp;
  long on = 1;

  /* Create address corresponding our service */
  bzero((caddr_t)&sin, sizeof(sin));
  sin.sin_len = sizeof(struct sockaddr_in);
  sin.sin_family = AF_INET;

  /* A port must be in the range 1 - 65535 */
  if (StrToLong(pname, &port) > 0 && port < 65536 )
    sin.sin_port = port;
  else if (sp = getservbyname(pname, "tcp"))
    sin.sin_port = sp->s_port;
  else {
    return -1;
  }

  sin.sin_addr.s_addr = INADDR_ANY;

  if ((s = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
    PrintNetFault(Errno(), "socket");
    return -1;
  }

  /* Reuse this port */
  if (setsockopt(s, SOL_SOCKET, SO_REUSEADDR, (char *)&on, sizeof(on)) < 0) {
    PrintNetFault(Errno(), "setsockopt");
    sa = -1; goto Return;
  }

  /* Bind it to socket */
  if (bind(s, (struct sockaddr *)&sin, sizeof(sin)) < 0 ) {
    PrintNetFault(Errno(), "bind");
    sa = -1; goto Return;
  }

  if (listen(s, 1) < 0) {
    PrintNetFault(Errno(), "listen");
    sa = -1; goto Return;
  }

  if ((sa = accept(s, (struct sockaddr *)ha, &ha_len)) < 0){
    PrintNetFault(Errno(), "accept");
  }

 Return:
  CloseSocket(s);
  return sa;
}

