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

static const char version[] = "$VER: resolve 4.0 (17.10.2005)\n"
"Copyright © 1994 AmiTCP/IP Group, <amitcp-group@hut.fi>\n"
"Helsinki University of Technology, Finland.\n"
"Copyright © 2005 Pavel Fedin <sonic_amiga@rambler.ru";

#define USE_INLINE_STDARG

#include <aros/config.h>

#include <proto/socket.h>
#include <proto/dos.h>

#include <proto/exec.h>

#include <dos/dos.h>

#include <netdb.h>

#include <string.h>
#include <stdlib.h>

#include <sys/socket.h>
#include <netinet/in.h>

#define SOCKETVERSION 2	/* minimum version to use */
#define SOCKETNAME "bsdsocket.library"

#ifndef MAXLINELENGTH 
#define MAXLINELENGTH 1024
#endif

int main(void)
{
  int retval = 128;
  struct DosLibrary *DOSBase;
  struct Library *SocketBase;

#if ! (AROS_FLAVOUR & AROS_FLAVOUR_EMULATION)
  struct ExecBase *SysBase;
  SysBase = *(struct ExecBase**)4;
#endif

  DOSBase = (struct DosLibrary *)OpenLibrary("dos.library", 37L);
  SocketBase = OpenLibrary(SOCKETNAME, SOCKETVERSION);
  
  if (DOSBase && SocketBase) {
    const char *template = "HOST/A";
    struct {
      STRPTR a_ipaddr;
    } args[1] = { { 0 } };
    struct RDArgs *rdargs = NULL;

    if (rdargs = ReadArgs((UBYTE *)template, (IPTR *)args, NULL)) {
      long addr = inet_addr(args->a_ipaddr);
      struct hostent *hp;
      if (addr == INADDR_NONE)
	hp = gethostbyname(args->a_ipaddr);
      else
        hp = gethostbyaddr((caddr_t)&addr, sizeof(addr), AF_INET);
      if (hp) {
	if (addr == -1) {
	  char **ip = hp->h_addr_list;
	  struct in_addr *inaddr;
	  Printf("ADDR %s", args->a_ipaddr);
	  while (ip && *ip) {
	    inaddr = (struct in_addr *)*ip++;
	    Printf(" %s", Inet_NtoA(inaddr->s_addr));
	  }
	} else {
	  char **alias = hp->h_aliases;
	  Printf("HOST %s %s", args->a_ipaddr, hp->h_name);
	  while (alias && *alias) {
	    Printf(" %s", *alias++);
	  }
	}
	Printf("\n");
        retval = RETURN_OK;
      } else {
        Printf("resolve: unknown host %s\n", args->a_ipaddr);
	retval = 1;
      }
      FreeArgs(rdargs);
    }

    else {
        Printf("usage: resolve host\n");
        retval = 1;
    }
  }

  if (SocketBase) {
    CloseLibrary(SocketBase);
  } else {
    if (DOSBase)
      Printf("resolve: cannot open bsdsocket.library version 2\n");
  }

  if (DOSBase)
    CloseLibrary((struct Library *)DOSBase);

  return retval;
}
