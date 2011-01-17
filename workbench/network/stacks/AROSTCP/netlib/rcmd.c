/* $Id$
 *
 *      rcmd.c - rcmd() for AmiTCP/IP and usergroup.library
 *
 *      Copyright © 1994 AmiTCP/IP Group, 
 *                       Network Solutions Development Inc.
 *                       All rights reserved.
 */

/*
 * Copyright (c) 1983 Regents of the University of California.
 * All rights reserved.
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

#if __SASC
#include <proto/dos.h>
#include <proto/socket.h>
#include <proto/exec.h>
#elif __GNUC__
#include <inline/dos.h>
#include <inline/socket.h>
#include <inline/exec.h>
#else
#include <clib/dos_protos.h>
#include <clib/socket_protos.h>
#include <clib/exec_protos.h>
#endif

#include <sys/param.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#if 0
#include <signal.h>
#endif
#include <fcntl.h>
#include <netdb.h>
#include <pwd.h>
#include <errno.h>
#include <stdio.h>
#include <ctype.h>

#include <unistd.h>
#include <string.h>

#ifdef ioctl
#undef ioctl
#endif
#ifdef close
#undef close
#endif
#define ioctl IoctlSocket
#define close CloseSocket
#define getpid() ((pid_t)FindTask(0L))


/****** net.lib/rcmd *********************************************************

    NAME
        rcmd, rresvport - routines for returning a stream to a remote command

    SYNOPSIS
        #include <clib/socket_protos.h>

        int rcmd(char **ahost, int inport, const char *locuser, 
                 const char *remuser, const char *cmd, int *fd2p);

        int rresvport(int *port);

    FUNCTION
        The rcmd() function is used by the super-user to execute a command on
        a remote machine using an authentication scheme based on reserved port
        numbers.  The rresvport() function returns a descriptor to a socket
        with an address in the privileged port space.  Both functions are
        present in the same file and are used by the rsh command (among
        others).

        The rcmd() function looks up the host *ahost using gethostbyname(),
        returning -1 if the host does not exist.  Otherwise *ahost is set to
        the standard name of the host and a connection is established to a
        server residing at the well-known Internet port inport.

        If the connection succeeds, a socket in the Internet domain of type
        SOCK_STREAM is returned to the caller, and given to the remote command
        as stdin and stdout. If fd2p is non-zero, then an auxiliary channel to
        a control process will be set up, and a descriptor for it will be
        placed in *fd2p. The control process will return diagnostic output
        from the command (unit 2) on this channel, and will also accept bytes
        on this channel as being UNIX signal numbers, to be forwarded to the
        process group of the command.  If fd2p is 0, then the stderr (unit 2
        of the remote command) will be made the same as the stdout and no
        provision is made for sending arbitrary signals to the remote process,
        although you may be able to get its attention by using out-of-band
        data.

        The protocol is described in detail in netutil/rshd.

        The rresvport() function is used to obtain a socket with a privileged
        address bound to it.  This socket is suitable for use by rcmd() and
        several other functions.  Privileged Internet ports are those in the
        range 0 to 1023.  Only the super-user is allowed to bind an address of
        this sort to a socket.

    DIAGNOSTICS
        The rcmd() function returns a valid socket descriptor on success.  It
        returns -1 on error and prints a diagnostic message on the standard
        error.

        The rresvport() function returns a valid, bound socket descriptor on
        success.  It returns -1 on error with the global value errno set
        according to the reason for failure.  The error code EAGAIN is
        overloaded to mean `All network ports in use.'

    SEE ALSO
        netutil/rlogin,  netutil/rsh,  rexec(),  netutil/rexecd,
        netutil/rlogind, netutil/rshd

******************************************************************************
*/

#include <clib/netlib_protos.h>

int
rcmd(char **ahost,
     int rport,
     const char *locuser, 
     const char *remuser, 
     const char *cmd,
     int *fd2p)			/* Socket for stderr  */
{
  int s, timo = 1;
  pid_t pid;
  struct sockaddr_in sin, from;
  char c;
  int lport = IPPORT_RESERVED - 1;
  struct hostent *hp;
  fd_set reads;

  pid = getpid();
  hp = gethostbyname(*ahost);
  if (hp == 0) {
    herror(*ahost);
    errno = EADDRNOTAVAIL;
    return (-1);
  }
  *ahost = hp->h_name;

  for (;;) {
    s = rresvport(&lport);
    if (s < 0) {
      errno == EAGAIN ? 
	fprintf(stderr, "socket: All ports in use\n")
	  : perror("rcmd: socket");
      return (-1);
    }
    ioctl(s, FIOSETOWN, (caddr_t)&pid);
    sin.sin_len = sizeof(sin);
    sin.sin_family = hp->h_addrtype;
    bcopy(hp->h_addr_list[0], (caddr_t)&sin.sin_addr, hp->h_length);
    sin.sin_port = rport;
    if (connect(s, (struct sockaddr *)&sin, sizeof(sin)) >= 0)
      break;
    (void) close(s);
    if (errno == EADDRINUSE) {
      lport--;
      continue;
    }
    if (errno == ECONNREFUSED && timo <= 16) {
      sleep(timo);
      timo *= 2;
      continue;
    }
    if (hp->h_addr_list[1] != NULL) {
      int oerrno = errno;

      fprintf(stderr, "connect to address %s: ", inet_ntoa(sin.sin_addr));
      errno = oerrno;
      perror("");
      hp->h_addr_list++;
      bcopy(hp->h_addr_list[0], (caddr_t)&sin.sin_addr, hp->h_length);
      fprintf(stderr, "Trying %s...\n", inet_ntoa(sin.sin_addr));
      continue;
    }
    perror(hp->h_name);
    return (-1);
  }

  lport--;
  if (fd2p == 0) {
    send(s, "", 1, 0);
    lport = 0;
  } else {
    char num[8];
    long s2 = rresvport(&lport), s3;
    long len = sizeof (from);

    if (s2 < 0)
      goto bad;
    listen(s2, 1);
    (void) sprintf(num, "%ld", lport);
    if (send(s, num, strlen(num)+1, 0) != strlen(num)+1) {
      perror("send: setting up stderr");
      (void) close(s2);
      goto bad;
    }
    FD_ZERO(&reads);
    FD_SET(s, &reads);
    FD_SET(s2, &reads);
    errno = 0;
    if (select(((s > s2) ? s : s2) + 1,
	       &reads, 0, 0, 0) < 1 || !FD_ISSET(s2, &reads)) {
      errno != 0 ?
          perror("select: setting up stderr")
	  :
	  fprintf(stderr, "select: protocol failure in circuit setup.\n");
      (void) close(s2);
      goto bad;
    }
    s3 = accept(s2, (struct sockaddr *)&from, &len);
    (void) close(s2);
    if (s3 < 0) {
      perror("accept");
      lport = 0;
      goto bad;
    }
    *fd2p = s3;
    from.sin_port = ntohs((u_short)from.sin_port);
    if (from.sin_family != AF_INET ||
	from.sin_port >= IPPORT_RESERVED ||
	from.sin_port < IPPORT_RESERVED / 2) {
      fprintf(stderr, "socket: protocol failure in circuit setup.\n");
      goto bad2;
    }
  }
  (void) send(s, locuser, strlen(locuser) + 1, 0);
  (void) send(s, remuser, strlen(remuser) + 1, 0);
  (void) send(s, cmd, strlen(cmd) + 1, 0);
  if (recv(s, &c, 1, 0) != 1) {
    perror(*ahost);
    goto bad2;
  }
  if (c != 0) {
    while (recv(s, &c, 1, 0) == 1) {
      (void) fputc(c, stderr);
      if (c == '\n')
	break;
    }
    goto bad2;
  }

  return (s);
 bad2:
  if (lport)
    (void) close(*fd2p);
 bad:
  (void) close(s);
  return (-1);
}

int rresvport(int *alport)
{
  struct sockaddr_in sin;
  int s;

  sin.sin_len = sizeof(sin);
  sin.sin_family = AF_INET;
  sin.sin_addr.s_addr = INADDR_ANY;
  s = socket(AF_INET, SOCK_STREAM, 0);
  if (s < 0)
    return (-1);
  for (;;) {
    sin.sin_port = htons((u_short)*alport);
    if (bind(s, (struct sockaddr *)&sin, sizeof (sin)) >= 0)
      return (s);
    if (errno != EADDRINUSE) {
      (void) close(s);
      return (-1);
    }
    (*alport)--;
    if (*alport == IPPORT_RESERVED/2) {
      (void) close(s);
      errno = EAGAIN;		/* close */
      return (-1);
    }
  }
}
