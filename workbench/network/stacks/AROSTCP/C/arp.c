/* $Id$
 *
 * arp.c --- arp utility for AmiTCP/IP
 *
 * Copyright © 1993 AmiTCP/IP Group, <amitcp-group@hut.fi>,
 *                  Helsinki University of Technology, Finland.
 *                  All rights reserved.
 * Copyright © 2005 Pavel Fedin
 *
 * Created      : Sun Apr 18 05:08:20 1993 ppessi
 * Last modified: Wed Feb 16 19:48:50 1994 ppessi
 *
 * $Log: arp.c,v $
 * Revision 1.2  2005/12/22 06:44:55  sonic_amiga
 * - Fixed address printout in "arp" tool.
 * - "netstat" fully works now.
 * - Improved hostname handling.
 * - Fixed FD_ACCEPT generation
 * - DHCP client now supports nameservers option
 *
 * Revision 1.1.1.1  2005/12/07 10:50:36  sonic_amiga
 * First full import into the CVS
 *
 * Revision 3.2  1994/05/02  19:39:03  jraja
 * Update for the new net.lib
 *
 * Revision 3.1  1994/02/21  20:32:19  ppessi
 * Changed the version tag.
 *
 * Revision 1.10  1993/11/07  00:19:45  ppessi
 * Reverted back to scanf()
 *
 * Revision 1.9  1993/10/29  01:26:40  ppessi
 * Implemented dumping of arp tables.
 * Cleaned up code, fixed close() bug.
 */

static const char version[] = "$VER: arp 3.2 (12.09.2005) "
  "Copyright © 2005 Pavel Fedin <sonic_amiga@rambler.ru>\n"
  "Copyright © 1993 AmiTCP/IP Group, <amitcp-group@hut.fi>\n"
  "Helsinki University of Technology, Finland.\n"
  "Copyright © 1984 The Regents of the University of California.\n"
  "All rights reserved.\n";


/*
 * Copyright © 1984 Regents of the University of California.
 * All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * Sun Microsystems, Inc.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by the University of
 *	California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

/****** netutil.doc/arp *****************************************************
*
*  NAME
*       Arp - address resolution display and control
*
*  SYNOPSIS
*       arp hostname
*       arp -a [netname | hostname]
*       arp -d hostname
*       arp -s hostname address [temp] [pub] 
*       arp -f filename
*
*  DESCRIPTION
*       Arp displays and modifies the Internet to hardware address
*       translation tables used by the Address Resolution Protocol. The
*       hardware address is a hexadecimal string with each octet separated
*       by a colon, for instance 0:12:ff:a. The length of the address must
*       be correct for the specified interface.
*
*  OPTIONS
*        none If no options are specified (first form above), arp displays
*             the current ARP entry for hostname.  The hostname must either
*             appear in the hostname database (SEE hosts), or be a DARPA
*             Internet address expressed in Internet standard "dot
*             notation". Hostname can also be resolved by nameserver.
*
*       -a    Display all current ARP entries by reading the address mapping
*             table of the specified (sub)network. `Hostname' is used to as
*             default network specifier.
*
*       -d    If an ARP entry exists for the host called hostname, delete
*             it. [This requires super-user privileges.]
*
*       -s    Create an ARP entry for the host called hostname with the
*             hardware station address address. The hardware station address
*             is given as hexadecimal bytes separated by colons. If an ARP
*             entry already exists for hostname, the existing entry is
*             updated with the new information. The entry is permanent
*             unless the word temp is given in the command. If the word pub
*             is specified, the entry is published, which means that this
*             system will act as an ARP server responding to requests for
*             hostname even though the host address is not its own.
*
*       -f    Read file filename and set multiple entries in the ARP tables.
*             Entries in the file should be of the form:
*
*                  hostname address [temp] [pub]
*
*             Argument meanings are the same as for the -s option.
*
*  AUTHOR
*       Arp was developed by the University of California, Berkeley, for the
*       BSD Unix system.
*
*  SEE ALSO
*       ifconfig, netif.protocols/arp, "net/if_arp.h"
*
*****************************************************************************
*
*/

/*
 * arp - display, set, and delete arp table entries
 */

#ifdef AMIGA
#include <proto/dos.h>
#include <proto/socket.h>
#if !defined(__AROS__)
#include <clib/netlib_protos.h>
#else
#include <proto/exec.h>
#endif
#endif /* AMIGA */

#include <sys/param.h>
#include <sys/socket.h>
#include <sys/sockio.h>

#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/in.h>
#include <net/if.h>

#include <errno.h>
#include <stdlib.h>
#include <stdio.h>
#if defined(__AROS__)
#include <string.h>
#include <unistd.h>
#endif

struct Library * SocketBase;

#define max(a,b) ((a) > (b) ? (a) : (b))

extern int errno;
#warning "TODO: NicJA - h_errno should be extern .."
int h_errno;

static int file(char *name);
static int set(int argc, char **argv);
static void get(char *host);
static void delete(char *host);
static int arpreq_print(struct arpreq *ar, int bynumber);
static void sana_print(const u_char *cp, int len);
static int sana_aton(const char *a, u_char *n, u_char *lenp);
static void usage(void);
static void dump(char *name);
char * strsep(register char **stringp, register const char *delim);

char usage_str[] = 
  "usage: arp hostname\n"
  "       arp -a [netname | hostname]\n"
  "       arp -d hostname\n"
  "       arp -s hostname address [temp] [pub]\n"
  "       arp -f filename\n";

#define SOCKET_VERSION 3
const TEXT socket_name[] = "bsdsocket.library";

main(argc, argv)
	int argc;
	char **argv;
{
  int ch;

   if (!(SocketBase = OpenLibrary(socket_name, SOCKET_VERSION)))
   {
      return RETURN_FAIL;   
   }

  while ((ch = getopt(argc, argv, "adsf")) != EOF)
    switch((char)ch) {
    case 'a': {
      char *net;
      char myname[MAXHOSTNAMELEN];

      if (argc > 3)
	usage();
      if (argc == 3) {
	net = argv[2];
      } else {
	net = myname;
	gethostname(myname, sizeof(myname));
      }
      dump(net);
      exit(0);
    }
    case 'd':
      if (argc != 3)
	usage();
      delete(argv[2]);
      exit(0);
    case 's':
      if (argc < 4 || argc > 7)
	usage();
      exit(set(argc-2, &argv[2]) ? 1 : 0);
    case 'f':
      if (argc != 3)
	usage();
      exit (file(argv[2]) ? 1 : 0);
    case '?':
    default:
      usage();
    }
  if (argc != 2)
    usage();
  get(argv[1]);
  exit(0);
}

/*
 * Process a file to set standard arp entries
 */
static int
file(char *name)
{
  FILE *fp;
  int i, retval;
  char line[100], arg[5][50], *args[5];

  if ((fp = fopen(name, "r")) == NULL) {
    fprintf(stderr, "arp: cannot open %s\n", name);
    exit(1);
  }

  args[0] = &arg[0][0];
  args[1] = &arg[1][0];
  args[2] = &arg[2][0];
  args[3] = &arg[3][0];
  args[4] = &arg[4][0];

  retval = 0;
  while(fgets(line, 100, fp) != NULL) {
    i = sscanf(line, "%s %s %s %s %s", arg[0], arg[1], arg[2], arg[3], arg[4]);
    if (i < 2) {
      fprintf(stderr, "arp: bad line: %s\n", line);
      retval = 1;
      continue;
    }
    if (set(i, args))
      retval = 1;
  }
  fclose(fp);
  return (retval);
}

/*
 * Set an individual arp entry 
 */
static int
set(int argc, char **argv)
{
  struct arpreq ar;
  struct hostent *hp;
  struct sockaddr_in *sin;
  u_char *ea;
  int s;
  char *host = argv[0], *eaddr = argv[1];

  argc -= 2;
  argv += 2;
  bzero((caddr_t)&ar, sizeof ar);
  sin = (struct sockaddr_in *)&ar.arp_pa;
  sin->sin_len = sizeof(*sin);
  sin->sin_family = AF_INET;
  sin->sin_addr.s_addr = inet_addr(host);
  if (sin->sin_addr.s_addr == -1) {
    if (!(hp = gethostbyname(host))) {
      fprintf(stderr, "arp: %s: ", host);
#warning "TODO: NicJA - Implement herror()"
#if !defined(__AROS__)
      herror((char *)NULL);
#endif
      return (1);
    }
    bcopy((char *)hp->h_addr, (char *)&sin->sin_addr,
	  sizeof sin->sin_addr);
  }
  ea = (u_char *)ar.arp_ha.sa_data;
  if (sana_aton(eaddr, ea, &ar.arp_ha.sa_len))
    return (1);
  ar.arp_ha.sa_len += 2;
  ar.arp_flags = ATF_PERM;
  while (argc-- > 0) {
    if (strncmp(argv[0], "temp", 4) == 0)
      ar.arp_flags &= ~ATF_PERM;
    else if (strncmp(argv[0], "pub", 3) == 0)
      ar.arp_flags |= ATF_PUBL;
    else if (strncmp(argv[0], "trail", 5) == 0)
      ar.arp_flags |= ATF_USETRAILERS;
    argv++;
  }
	
  s = socket(AF_INET, SOCK_DGRAM, 0);
  if (s < 0) {
    perror("arp: socket");
    exit(1);
  }
  if (IoctlSocket(s, SIOCSARP, (caddr_t)&ar) < 0) {
    perror(host);
    exit(1);
  }
  CloseSocket(s);

  return (0);
}

/*
 * Display an individual arp entry
 */
static void
get(char *host)
{
  struct arpreq ar;
  struct hostent *hp;
  struct sockaddr_in *sin;
  int s;

  bzero((caddr_t)&ar, sizeof ar);
  sin = (struct sockaddr_in *)&ar.arp_pa;
  sin->sin_len = sizeof(*sin);
  sin->sin_family = AF_INET;
  sin->sin_addr.s_addr = inet_addr(host);
  if (sin->sin_addr.s_addr == -1) {
    if (!(hp = gethostbyname(host))) {
      fprintf(stderr, "arp: %s: ", host);
#warning "TODO: NicJA - Implement herror()"
#if !defined(__AROS__)
      herror((char *)NULL);
#endif
      exit(1);
    }
    bcopy((char *)hp->h_addr, (char *)&sin->sin_addr, sizeof sin->sin_addr);
  }
  s = socket(AF_INET, SOCK_DGRAM, 0);
  if (s < 0) {
    perror("arp: socket");
    exit(1);
  }
  if (IoctlSocket(s, SIOCGARP, (caddr_t)&ar) < 0) {
    if (errno == ENXIO)
      printf("%s (%s) -- no entry\n", host, inet_ntoa(sin->sin_addr));
    else
      perror("SIOCGARP");
    exit(1);
  }
  CloseSocket(s);

  (void)arpreq_print(&ar, 0);
}

/*
 * Delete an arp entry 
 */
static void
delete(char *host)
{
  struct arpreq ar;
  struct hostent *hp;
  struct sockaddr_in *sin;
  int s;

  bzero((caddr_t)&ar, sizeof ar);
  sin = (struct sockaddr_in *)&ar.arp_pa;
  sin->sin_len = sizeof(*sin);
  sin->sin_family = AF_INET;
  sin->sin_addr.s_addr = inet_addr(host);
  if (sin->sin_addr.s_addr == -1) {
    if (!(hp = gethostbyname(host))) {
      fprintf(stderr, "arp: %s: ", host);
#warning "TODO: NicJA - Implement herror()"
#if !defined(__AROS__)
      herror((char *)NULL);
#endif
      exit(1);
    }
    bcopy((char *)hp->h_addr, (char *)&sin->sin_addr,
	  sizeof sin->sin_addr);
  }
  s = socket(AF_INET, SOCK_DGRAM, 0);
  if (s < 0) {
    perror("arp: socket");
    exit(1);
  }
  if (IoctlSocket(s, SIOCDARP, (caddr_t)&ar) < 0) {
    if (errno == ENXIO)
      printf("%s (%s) -- no entry\n",
	     host, inet_ntoa(sin->sin_addr));
    else
      perror("SIOCDARP");
    exit(1);
  }
  CloseSocket(s);
  printf("%s (%s) deleted\n", host, inet_ntoa(sin->sin_addr));
}

/*
 * Dump the entire arp table
 */
static void 
dump(char *host)
{
  struct arptabreq atr; 
  struct arpreq *ar;
  struct sockaddr_in *sin;
  int s, bynumber;
  long n;
  
  bzero((caddr_t)&atr, sizeof atr);

  /* Identify the used interface according the host/net name */
  sin = (struct sockaddr_in *)&atr.atr_arpreq.arp_pa;
  sin->sin_len = sizeof(*sin);
  sin->sin_family = AF_INET;
  sin->sin_addr.s_addr = inet_addr(host);
  if (sin->sin_addr.s_addr == -1) {
    struct hostent *hp;
    struct netent *np = NULL;
    if (!(hp = gethostbyname(host)) &&
	!(np = getnetbyname(host))) {
      fprintf(stderr, "arp: %s: ", host);
#warning "TODO: NicJA - Implement herror()"
#if !defined(__AROS__)
      herror((char *)NULL);
#endif
      exit(1);
    }
    if (hp) 
      bcopy((char *)hp->h_addr, (char *)&sin->sin_addr, sizeof sin->sin_addr);
    else
      bcopy((char *)&np->n_net, (char *)&sin->sin_addr, sizeof np->n_net);
  }

  s = socket(AF_INET, SOCK_DGRAM, 0);
  if (s < 0) {
    perror("arp: socket");
    exit(1);
  }

  if (IoctlSocket(s, SIOCGARPT, (caddr_t)&atr) < 0) {
    perror("SIOCGARPT");
    exit(1);
  }

  n = atr.atr_size = atr.atr_inuse;
  if (n) {
    ar = atr.atr_table = malloc(sizeof(*ar) * n);
    if (!ar) {
      perror("arp: malloc");
      exit(1);
    }
    if (IoctlSocket(s, SIOCGARPT, (caddr_t)&atr) < 0) {
      perror("SIOCGARPT");
      exit(1);
    }
    /* Entries may be deleted */
    n = max(n, atr.atr_inuse);
  }
  CloseSocket(s);

  bynumber = 0;
  while (n-- > 0) {
    bynumber = arpreq_print(ar++, bynumber);
  }
}

static int
arpreq_print(struct arpreq *ar, int bynumber)
{
  struct sockaddr_in *sin;
  struct hostent *hp;
  char *host;

  sin = (struct sockaddr_in *)&ar->arp_pa;

  if (bynumber == 0)
    hp = gethostbyaddr((caddr_t)&sin->sin_addr, 
		       sizeof(struct in_addr), AF_INET);
  else
    hp = 0;

  if (hp)
    host = hp->h_name;
  else {
    host = "?";
    if (h_errno == TRY_AGAIN)
      bynumber = 1;
  }

  Printf("%s (%s) at ", host, inet_ntoa(sin->sin_addr));
  if (ar->arp_flags & ATF_COM)
    sana_print(ar->arp_ha.sa_data, ar->arp_ha.sa_len - 2);
  else
    Printf("(incomplete)");
  if (ar->arp_flags & ATF_PERM)
    Printf(" permanent");
  if (ar->arp_flags & ATF_PUBL)
    Printf(" published");
  if (ar->arp_flags & ATF_USETRAILERS)
    Printf(" trailers");
  Printf("\n");

  return bynumber;
}

static void
sana_print(const u_char *cp, int len)
{
  char format[] = 
    "%lx:%lx:%lx:%lx:%lx:%lx:%lx:%lx:%lx:%lx:%lx:%lx:%lx:%lx:%lx:%lx";
  int i; long o[16];

  if (len) {
    format[4*len - 1] = '\0';
    for (i = 0; i < len; i++)
      o[i] = cp[i];
//    vprintf(format, (va_list)o);
    VPrintf(format, o);
  }
}

static int
sana_aton(const char *str, u_char *n, u_char *lenp)
{
  int i, chars, o[16];
  u_char c;
  const char *a = str;

  for (i = 0; i < 16; a++) {
    o[i] = chars = 0;
    while ((c = *a - '0') <= 9 
	  || (c = c - 'A' + '0' + 10) >= 9 && c < 16  /*ABCDEF*/
	  || (c = c - 'a' + 'A') >= 9 && c < 16) /*abcdef*/ {
      a++; chars++;
      o[i] = (o[i] << 4) + c;
      if (o[i] >= 0x100)
	return 1;
    }
    if (!chars)
      return 1;
    i++;
    if (*a == '\0')
      break;
    else if (*a == ':')
      continue;
    return 1;
  }

  if (i >= 16) {
    fprintf(stderr, "arp: invalid SANA-II address '%s'\n", str);
    return (1);
  }

  *lenp = i;
  while (i-- > 0)
    n[i] = o[i];
  return 0;
}

static void
usage(void)
{
  printf(usage_str);

  exit(1);
}
