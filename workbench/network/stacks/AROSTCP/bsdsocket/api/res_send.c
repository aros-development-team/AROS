/*
 * Copyright (c) 1985, 1989 Regents of the University of California.
 * All rights reserved.
 * Copyright (C) 2005 - 2007 The AROS Dev Team
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

 */

#if defined(LIBC_SCCS) && !defined(lint)
static char sccsid[] = "@(#)res_send.c	6.27 (Berkeley) 2/24/91";
#endif /* LIBC_SCCS and not lint */

/*
 * Send query to name server and wait for reply.
 */

#include <conf.h>

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/malloc.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/errno.h>

#include <arpa/nameser.h>
#include <api/resolv.h>
#include <kern/amiga_includes.h>
#include <api/apicalls.h>
#include <api/amiga_api.h>
#include <kern/amiga_subr.h>     
#include <kern/amiga_netdb.h>     
#include <stdio.h>

#ifndef AMITCP /* AmiTCP has this in the SocketBase */     
static int res_sock = -1;	/* socket used for communications */
#endif

/* constant */
static const struct sockaddr no_addr = { sizeof(struct sockaddr), AF_INET, { 0 } };

#ifndef FD_SET
#define	NFDBITS		32
#define	FD_SETSIZE	32
#define	FD_SET(n, p)	((p)->fds_bits[(n)/NFDBITS] |= (1 << ((n) % NFDBITS)))
#define	FD_CLR(n, p)	((p)->fds_bits[(n)/NFDBITS] &= ~(1 << ((n) % NFDBITS)))
#define	FD_ISSET(n, p)	((p)->fds_bits[(n)/NFDBITS] & (1 << ((n) % NFDBITS)))
#define FD_ZERO(p)	bzero((char *)(p), sizeof(*(p)))
#endif

extern const char * const __sys_errlist[];
#define Perror(string) Printf("%s: %s\n", string, __sys_errlist[readErrnoValue(libPtr)])

int
res_send(struct SocketBase *	libPtr,
	 const char *		buf,
	 int			buflen,
	 char *			answer,
	 int 			anslen)
{
	register int n;
	int try, v_circuit, resplen, nscount;
	int gotsomewhere = 0, connected = 0;
	int connreset = 0;
	u_short id, len;
	char *cp;
	fd_set dsmask;
	struct timeval timeout;
	struct in_addr *ns;
	struct sockaddr_in host;
	HEADER *hp = (HEADER *) buf;
	HEADER *anhp = (HEADER *) answer;
	u_char terrno = ETIMEDOUT;
#define JUNK_SIZE 512
	char junk[JUNK_SIZE]; /* buffer for trash data */

#if defined(__AROS__)
D(bug("[AROSTCP](res_send.c) res_send()\n"));
D(bug("[AROSTCP](res_send.c) res_send: using socket %d\n", res_sock));
#endif

#ifdef RES_DEBUG
		printf("res_send()\n");
		__p_query(buf, libPtr);
#endif /* RES_DEBUG */

	v_circuit = (_res.options & RES_USEVC) || buflen > PACKETSZ;
	id = hp->id;
	/*
	 * Send request, RETRY times, or until successful
	 */
	for (try = 0; try < _res.retry; try++) {
#if defined(__AROS__)
D(bug("[AROSTCP](res_send.c) res_send: Attempt %d\n", try));
#endif
	  nscount = 0;
	  DRES(Printf("Retry #%ld\n",try);)
	  for (ns = _res.nsaddr_list; ns->s_addr; ns++) {
	    nscount++;
#if defined(__AROS__)
D(bug("[AROSTCP](res_send.c) res_send: Querying server #%ld address = %s\n", nscount,
			      __inet_ntoa(ns->s_addr, libPtr)));
#endif

#ifdef RES_DEBUG
			Printf("Querying server #%ld address = %s\n", nscount,
			      __Inet_NtoA(ns->s_addr, libPtr));
#endif /* RES_DEBUG */
	    host.sin_len = sizeof(host);
	    host.sin_family = AF_INET;
	    host.sin_port = htons(NAMESERVER_PORT);
	    host.sin_addr.s_addr = ns->s_addr;
	    aligned_bzero_const(&host.sin_zero, sizeof(host.sin_zero));
	usevc:
		if (v_circuit) {
#if defined(__AROS__)
D(bug("[AROSTCP](res_send.c) res_send: Using v_circuit\n"));
#endif
			int truncated = 0;

			/*
			 * Use virtual circuit;
			 * at most one attempt per server.
			 */
			try = _res.retry;
			if (res_sock < 0) {
				res_sock = __socket(AF_INET, SOCK_STREAM, 0, libPtr);
				if (res_sock < 0) {
#if defined(__AROS__)
D(bug("[AROSTCP](res_send.c) res_send: Failed to create socket!!\n"));
					terrno = readErrnoValue(libPtr);
#endif
#ifdef RES_DEBUG
					    Perror("socket (vc)");
#endif /* RES_DEBUG */
					continue;
				}
#if defined(__AROS__)
D(bug("[AROSTCP](res_send.c) res_send: created socket %d\n", res_sock));
#endif
				if (__connect(res_sock,
					    (struct sockaddr *)&host,
					    sizeof(struct sockaddr), libPtr) < 0) {
#if defined(__AROS__)
D(bug("[AROSTCP](res_send.c) res_send: Failed to connect\n"));
#endif
				        terrno = readErrnoValue(libPtr);
#ifdef RES_DEBUG
					    Perror("connect (vc)");
#endif /* RES_DEBUG */
					(void) __CloseSocket(res_sock, libPtr);
					res_sock = -1;
					continue;
				}
			}
			/*
			 * Send length & message
			 */
			len = htons((u_short)buflen);
			if ((__send(res_sock, (char *)&len, sizeof(len), 0, libPtr)
			     != sizeof(len)) ||
			   ((__send(res_sock, (char *)buf, buflen, 0, libPtr)
			     != buflen))) {
#if defined(__AROS__)
D(bug("[AROSTCP](res_send.c) res_send: Failed sending query\n"));
#endif
				terrno = readErrnoValue(libPtr);
#ifdef RES_DEBUG
					Perror("write(vc)");
#endif /* RES_DEBUG */
				(void) __CloseSocket(res_sock, libPtr);
				res_sock = -1;
				continue;
			}
			/*
			 * Receive length & response
			 */
			cp = answer;
			len = sizeof(short);
			while (len != 0 &&
			    (n = __recv(res_sock,
				      (char *)cp, (int)len, 0, libPtr)) > 0) {
				cp += n;
				len -= n;
			}
			if (n <= 0) {
				terrno = readErrnoValue(libPtr);
#if defined(__AROS__)
D(bug("[AROSTCP](res_send.c) res_send: Failed receiving response\n"));
#endif
#ifdef RES_DEBUG
					Perror("read (vc)");
#endif /* RES_DEBUG */
				(void) __CloseSocket(res_sock, libPtr);
				res_sock = -1;
				/*
				 * A long running process might get its TCP
				 * connection reset if the remote server was
				 * restarted.  Requery the server instead of
				 * trying a new one.  When there is only one
				 * server, this means that a query might work
				 * instead of failing.  We only allow one reset
				 * per query to prevent looping.
				 */
				if (terrno == ECONNRESET && !connreset) {
					connreset = 1;
					ns--;
				}
				continue;
			}
			cp = answer;
			if ((resplen = ntohs(*(u_short *)cp)) > anslen) {
#if defined(__AROS__)
D(bug("[AROSTCP](res_send.c) res_send: Truncated response\n"));
#endif
#ifdef RES_DEBUG
				       Printf("response truncated\n");
#endif /* RES_DEBUG */
				len = anslen;
				truncated = 1;
			} else
				len = resplen;
			while (len != 0 &&
			   (n = __recv(res_sock,
				     (char *)cp, (int)len, 0, libPtr)) > 0) {
				cp += n;
				len -= n;
			}
			if (n <= 0) {
#if defined(__AROS__)
D(bug("[AROSTCP](res_send.c) res_send: Error receiving response\n"));
#endif
				terrno = readErrnoValue(libPtr);
#ifdef RES_DEBUG
					Perror("read (vc)");
#endif /* RES_DEBUG */
				(void) __CloseSocket(res_sock, libPtr);
				res_sock = -1;
				continue;
			}
			if (truncated) {
				/*
				 * Flush rest of answer
				 * so connection stays in synch.
				 */
				anhp->tc = 1;
				len = resplen - anslen;
				while (len != 0) {
					n = (len > JUNK_SIZE ? JUNK_SIZE : len);
					if ((n = __recv(res_sock,
						      junk, n, 0, libPtr)) > 0)
						len -= n;
					else
						break;
				}
			}
		} else {
#if defined(__AROS__)
D(bug("[AROSTCP](res_send.c) res_send: Using datagrams\n"));
#endif
			/*
			 * Use datagrams.
			 */
			if (res_sock < 0) {
				res_sock = __socket(AF_INET, SOCK_DGRAM, 0, libPtr);
				if (res_sock < 0) {
#if defined(__AROS__)
D(bug("[AROSTCP](res_send.c) res_send: Failed to create socket\n"));
#endif
					terrno = readErrnoValue(libPtr);
#ifdef RES_DEBUG
					    Perror("socket (dg)");
#endif /* RES_DEBUG */
					continue;
				}
			}
			/*
			 * I'm tired of answering this question, so:
			 * On a 4.3BSD+ machine (client and server,
			 * actually), sending to a nameserver datagram
			 * port with no nameserver will cause an
			 * ICMP port unreachable message to be returned.
			 * If our datagram socket is "connected" to the
			 * server, we get an ECONNREFUSED error on the next
			 * socket operation, and select returns if the
			 * error message is received.  We can thus detect
			 * the absence of a nameserver without timing out.
			 * If we have sent queries to at least two servers,
			 * however, we don't want to remain connected,
			 * as we wish to receive answers from the first
			 * server to respond.
			 */
/* TODO*: see comment here .. */
			/* This piece of code still behaves slightly wrong in
			   case of ECONNREFUSED error. On next retry socket will
			   be in disconnected state and instead of getting
			   ECONNREFUSED again we'll timeout in WaitSelect() and
			   get ETIMEDOUT. However, this is not critical and is
			   queued for future - Pavel Fedin*/
			if (try == 0 && nscount == 1) {
				/*
				 * Don't use connect if we might
				 * still receive a response
				 * from another server.
				 */
				if (connected == 0) {
				  if (__connect(res_sock,
					      (struct sockaddr *)&host,
					      sizeof(struct sockaddr),
					      libPtr) < 0) {
#if defined(__AROS__)
D(bug("[AROSTCP](res_send.c) res_send: Error connecting\n"));
#endif
#ifdef RES_DEBUG
							Perror("connect (dg)");
#endif /* RES_DEBUG */
						continue;
					}
					connected = 1;
				}
				if (__send(res_sock,
					 buf, buflen, 0, libPtr) != buflen) {
#if defined(__AROS__)
D(bug("[AROSTCP](res_send.c) res_send: Error sending\n"));
#endif
#ifdef RES_DEBUG
						Perror("send (dg)");
#endif /* RES_DEBUG */
					continue;
				}
			} else {
				/*
				 * Disconnect if we want to listen
				 * for responses from more than one server.
				 */
				if (connected) {
					(void) __connect(res_sock, &no_addr,
					    sizeof(no_addr), libPtr);
					connected = 0;
				}
				if (__sendto(res_sock, buf, buflen, 0,
				    (struct sockaddr *)&host,
				    sizeof(struct sockaddr), libPtr) != buflen) {
#if defined(__AROS__)
D(bug("[AROSTCP](res_send.c) res_send: [__sendto] Error\n"));
#endif
#ifdef RES_DEBUG
						Perror("sendto (dg)");
#endif /* RES_DEBUG */
					continue;
				}
			}

			/*
			 * Wait for reply
			 */
			timeout.tv_sec = (_res.retrans << try);
			if (try > 0)
				timeout.tv_sec /= nscount;
			if (timeout.tv_sec <= 0)
				timeout.tv_sec = 1;
			timeout.tv_usec = 0;
wait:
			FD_ZERO(&dsmask);
			FD_SET(res_sock, &dsmask);
			n = __WaitSelect(res_sock+1, &dsmask, NULL,
				NULL, &timeout, NULL, libPtr);
			if (n < 0) {
#if defined(__AROS__)
D(bug("[AROSTCP](res_send.c) res_send: [__WaitSelect] Error\n"));
#endif
#ifdef RES_DEBUG
					Perror("select");
#endif /* RES_DEBUG */

            terrno = readErrnoValue(libPtr);
				if (terrno == EINTR) {
#if defined(__AROS__)
D(bug("[AROSTCP](res_send.c) res_send: closing socket\n"));
#endif
					__CloseSocket(res_sock, libPtr);
					res_sock = -1;
					return (-1);
				}
				continue;
			}
			if (n == 0) {
				/*
				 * timeout
				 */
#if defined(__AROS__)
D(bug("[AROSTCP](res_send.c) res_send: Timeout!\n"));
#endif
#ifdef RES_DEBUG
					Printf("timeout\n");
#endif /* RES_DEBUG */
#if 1 || BSD >= 43
				gotsomewhere = 1;
#endif
				continue;
			}
			if ((resplen = __recv(res_sock,
					    answer, anslen, 0, libPtr)) <= 0) {
#if defined(__AROS__)
D(bug("[AROSTCP](res_send.c) res_send: Error receiving\n"));
#endif
#ifdef RES_DEBUG
					Perror("recv (dg)");
#endif /* RES_DEBUG */
				continue;
			}
			gotsomewhere = 1;
			if (id != anhp->id) {
				/*
				 * response from old query, ignore it
				 */
#ifdef RES_DEBUG
					Printf("old answer:\n");
					__p_query(answer, libPtr);
#endif /* RES_DEBUG */
				goto wait;
			}
			if (!(_res.options & RES_IGNTC) && anhp->tc) {
				/*
				 * get rest of answer;
				 * use TCP with same server.
				 */
#if defined(__AROS__)
D(bug("[AROSTCP](res_send.c) res_send: Response is truncated\n"));
#endif
#ifdef RES_DEBUG
					Printf("truncated answer\n");
#endif /* RES_DEBUG */
				(void)__CloseSocket(res_sock, libPtr);
				res_sock = -1;
				v_circuit = 1;
				goto usevc;
			}
		}

#if defined(__AROS__)
D(bug("[AROSTCP](res_send.c) res_send: Received answer\n"));
#endif

#ifdef RES_DEBUG
			Printf("got answer:\n");
			__p_query(answer, libPtr);
#endif /* RES_DEBUG */
		/*
		 * If using virtual circuits, we assume that the first server
		 * is preferred * over the rest (i.e. it is on the local
		 * machine) and only keep that one open.
		 * If we have temporarily opened a virtual circuit,
		 * or if we haven't been asked to keep a socket open,
		 * close the socket.
		 */
		if ((v_circuit &&
		    ((_res.options & RES_USEVC) == 0 || ns->s_addr != 0)) ||
		    (_res.options & RES_STAYOPEN) == 0) {
#if defined(__AROS__)
D(bug("[AROSTCP](res_send.c) res_send: Closing socket\n"));
#endif
			(void) __CloseSocket(res_sock, libPtr);
			res_sock = -1;
		}
		return (resplen);
	   }
	}
	if (res_sock >= 0) {
#if defined(__AROS__)
D(bug("[AROSTCP](res_send.c) res_send: Closing open socket\n"));
#endif
		(void) __CloseSocket(res_sock, libPtr);
		res_sock = -1;
	}

	if (v_circuit == 0)
	{
	  if (gotsomewhere == 0)
	  {
#if defined(__AROS__)
D(bug("[AROSTCP](res_send.c) res_send: No NAMESERVERs Found!\n"));
#endif
	    writeErrnoValue(libPtr, ECONNREFUSED); /* no nameservers found */
	  }
	  else
	  {
#if defined(__AROS__)
D(bug("[AROSTCP](res_send.c) res_send: No Response!\n"));
#endif
	    writeErrnoValue(libPtr, ETIMEDOUT);	   /* no answer obtained */
	  }
	}
	else writeErrnoValue(libPtr, terrno);

#if defined(__AROS__)
D(bug("[AROSTCP](res_send.c) res_send: Finished\n"));
#endif

	DRES(Perror("res_send()");)
	return (-1);
}

/*
 * This routine is for closing the socket if a virtual circuit is used and
 * the program wants to close it.  This provides support for endhostent()
 * which expects to close the socket.
 *
 * This routine is not expected to be user visible.
 */
void
_res_close(struct SocketBase * libPtr)
{
#if defined(__AROS__)
D(bug("[AROSTCP](res_send.c) _res_close()\n"));
#endif
	if (res_sock != -1) {
#if defined(__AROS__)
D(bug("[AROSTCP](res_send.c) _res_close: Closing socket\n"));
#endif
		(void) __CloseSocket(res_sock, libPtr);
		res_sock = -1;
	}
}
