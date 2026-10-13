/* $Id$
 *
 *      stubs.c - common stubs for bsdsocket.library
 *
 *      Copyright (C) 1994 AmiTCP/IP Group,
 *                       Network Solutions Development Inc.
 *                       All rights reserved.
 */

#include <exec/types.h>
#include <sys/types.h>
#include <proto/socket.h>

#include <sys/param.h>
#include <sys/ioctl.h>
#include <sys/socket.h>

#include <netinet/in.h>

/* Standard IPv6 address constants (POSIX / RFC 2553) */
const struct in6_addr in6addr_any      = IN6ADDR_ANY_INIT;
const struct in6_addr in6addr_loopback = IN6ADDR_LOOPBACK_INIT;

#undef select

int 
select(int nfds, fd_set *readfds, fd_set *writefds, fd_set *exeptfds,
	 struct timeval *timeout)
{
  /* call WaitSelect with NULL signal mask pointer */
  return WaitSelect(nfds, readfds, writefds, exeptfds, timeout, NULL);
}

#undef inet_ntoa

char * 
inet_ntoa(struct in_addr addr) 
{
  return Inet_NtoA(addr.s_addr);
}

#undef inet_makeaddr

struct in_addr 
inet_makeaddr(in_addr_t net, in_addr_t host)
{
  struct in_addr addr;
  addr.s_addr = Inet_MakeAddr(net, host);
  return addr;
}

#undef inet_lnaof

in_addr_t 
inet_lnaof(struct in_addr addr) 
{
  return Inet_LnaOf(addr.s_addr);
}

#undef inet_netof

in_addr_t 
inet_netof(struct in_addr addr)
{
  return Inet_NetOf(addr.s_addr);
}
