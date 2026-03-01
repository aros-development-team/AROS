/* $Id$
 *
 * resolve.c --- resolve the given IP address or hostname
 *
 * Author: ppessi <Pekka.Pessi@hut.fi>
 *	   Pavel Fedin <sonic_amiga@rambler.ru>
 *
 * Copyright © 1994 AmiTCP/IP Group, <AmiTCP-group@hut.fi>
 *                  Helsinki University of Technology, Finland.
 8 Copyright © 2005 Pavel Fedin
 *
 * Created      : Tue Jan 11 22:33:06 1994 ppessi
 * Last modified: Mon Oct 17 12:25:13 2005 sonic
 */

static const char version[] __attribute__((used)) = "$VER: resolve 5.0 (28.02.2026)\n"
"Copyright © 1994 AmiTCP/IP Group, <amitcp-group@hut.fi>\n"
"Helsinki University of Technology, Finland.\n"
"Copyright © 2005 Pavel Fedin <sonic_amiga@rambler.ru";

#define USE_INLINE_STDARG

#include <aros/config.h>

#include <proto/socket.h>
#include <proto/miami.h>
#include <proto/dos.h>

#include <proto/exec.h>

#include <dos/dos.h>

#include <netdb.h>

#include <string.h>
#include <stdlib.h>

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#ifndef MAXLINELENGTH 
#define MAXLINELENGTH 1024
#endif

int main(void)
{
  int retval = 128;
  struct DosLibrary *DOSBase;

#ifdef __SASC
  struct ExecBase *SysBase;
  SysBase = *(struct ExecBase**)4;
#endif

  DOSBase = (struct DosLibrary *)OpenLibrary("dos.library", 37L);

  if (DOSBase) {
    const char *template = "HOST/A,IPV4/S,IPV6/S";
    struct {
      STRPTR a_ipaddr;
      IPTR   a_ipv4;
      IPTR   a_ipv6;
    } args[1] = { { 0 } };
    struct RDArgs *rdargs = NULL;

    if (rdargs = ReadArgs((UBYTE *)template, (IPTR *)args, NULL)) {
      struct in_addr  addr4;
      struct in6_addr addr6;
      int is_v4_addr = 0;
      int is_v6_addr = 0;
      int want_v4 = args->a_ipv4;
      int want_v6 = args->a_ipv6;

      /* If neither flag given, query both */
      if (!want_v4 && !want_v6) {
        want_v4 = 1;
        want_v6 = 1;
      }

      /* Determine if input is a numeric address */
      if (inet_pton(AF_INET, args->a_ipaddr, &addr4) == 1)
        is_v4_addr = 1;
      else if (inet_pton(AF_INET6, args->a_ipaddr, &addr6) == 1)
        is_v6_addr = 1;

      if (is_v4_addr || is_v6_addr) {
        /* Reverse lookup: numeric address -> hostname */
        struct hostent *hp = NULL;

        if (is_v4_addr)
          hp = gethostbyaddr((caddr_t)&addr4, sizeof(addr4), AF_INET);
        else
          hp = gethostbyaddr((caddr_t)&addr6, sizeof(addr6), AF_INET6);

        if (hp) {
          char **alias = hp->h_aliases;
          Printf("HOST %s %s", args->a_ipaddr, hp->h_name);
          while (alias && *alias)
            Printf(" %s", *alias++);
          Printf("\n");
          retval = RETURN_OK;
        } else {
          Printf("resolve: unknown host %s\n", args->a_ipaddr);
          retval = 1;
        }
      } else {
        /* Forward lookup: hostname -> addresses */
        int found = 0;

        Printf("ADDR %s", args->a_ipaddr);

        /* IPv4 lookup */
        if (want_v4) {
          struct hostent *hp = gethostbyname(args->a_ipaddr);
          if (hp && hp->h_addrtype == AF_INET) {
            char **ip = hp->h_addr_list;
            while (ip && *ip) {
              struct in_addr *inaddr = (struct in_addr *)*ip++;
              Printf(" %s", Inet_NtoA(inaddr->s_addr));
              found = 1;
            }
          }
        }

        /* IPv6 lookup */
        if (want_v6) {
          struct hostent *hp6 = gethostbyname2(args->a_ipaddr, AF_INET6);
          if (hp6 && hp6->h_addrtype == AF_INET6) {
            char **ip = hp6->h_addr_list;
            char buf[INET6_ADDRSTRLEN];
            while (ip && *ip) {
              if (inet_ntop(AF_INET6, *ip, buf, sizeof(buf)))
                Printf(" %s", buf);
              ip++;
              found = 1;
            }
          }
        }

        Printf("\n");

        if (found) {
          retval = RETURN_OK;
        } else {
          Printf("resolve: unknown host %s\n", args->a_ipaddr);
          retval = 1;
        }
      }
      FreeArgs(rdargs);
    }

    else {
        Printf("usage: resolve host [IPV4] [IPV6]\n");
        retval = 1;
    }
    CloseLibrary((struct Library *)DOSBase);
  }

  return retval;
}
