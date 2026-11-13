 /* aros.h

   System dependencies for AROS */

/*
 * Copyright (c) 1996 The Internet Software Consortium.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of The Internet Software Consortium nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE INTERNET SOFTWARE CONSORTIUM AND
 * CONTRIBUTORS ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING,
 * BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL
 * THE INTERNET SOFTWARE CONSORTIUM OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
 * OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * This software was written for the Internet Software Consortium by Ted Lemon
 * under a contract with Vixie Laboratories.
 */

#define PROTO_USERGROUP_H

#include <netdb.h>
#include <dos/dosextens.h>
#include <exec/ports.h>
#include <libraries/miami.h>
#include <proto/bsdsocket.h>
#include <utility/hooks.h>
#include <utility/tagitem.h>
#include <errno.h>
#include <limits.h>
#include <setjmp.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <syslog.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#include <net/if.h>
#include <net/if_dl.h>
#include <net/route.h>
#include <proto/miami.h>

/* sys/ioctl.h is a stupid #warning here, sys/sockio must be used instead */
#include <sys/sockio.h>
#define	_SYS_IOCTL_H_

/* Varargs stuff... */
#include <stdarg.h>
#define VA_DOTDOTDOT ...
#define va_dcl
#define VA_start(list, last) va_start (list, last)

#if __WORDSIZE == 64
#define PTRSIZE_64BIT
#endif

#define SOCKLEN_T LONG
#ifndef INADDR_LOOPBACK
#define INADDR_LOOPBACK 0x7f000001
#endif

#ifndef AROSTCP_DB
/* this is the default path, overridable with ENV:AROTCP/Config */
#define	AROSTCP_DB "SYS:System/Network/AROSTCP/db/"
#endif

#ifndef AROSTCP_T
#define AROSTCP_T "T:"
#endif

#define _PATH_DHCPD_PID		AROSTCP_T  "dhcpd.pid"
#define _PATH_DHCPD_DB		AROSTCP_DB "dhcpd.leases"
#define _PATH_DHCPD_CONF	AROSTCP_DB "dhcpd.conf"
#define _PATH_DHCLIENT_PID	AROSTCP_T  "dhclient.pid"
#define _PATH_DHCLIENT_DB	AROSTCP_DB "dhclient.leases"
#define _PATH_DHCLIENT_CONF	AROSTCP_DB "dhclient.conf"
#define _PATH_RESOLV_CONF	AROSTCP_DB "resolv.conf"
#define _PATH_DHCRELAY_PID	AROSTCP_T  "dhcrelay.pid"

#define EOL	'\n'
#define VOIDPTR void *

/* Time stuff... */
#include <sys/time.h>
#define TIME time_t

//??AGR time and gettimeofday does not give the same result on AROS
//?? #define GET_TIME(x)	time ((x))
#define GET_TIME(x) do {	\
    struct timeval xt1;		\
    gettimeofday(&xt1, NULL);	\
    *(x) = xt1.tv_sec;		\
} while (0)

#define HAVE_SA_LEN
/* #define HAVE_CHKABORT ?? would be cool */
#define GET_USER_ID_MISSING
#define SET_SERVENT_MISSING
#define SET_PROTOENT_MISSING
#define FORK_MISSING
#define FSYNC_MISSING
#define SOCKET_IS_NOT_A_FILE
#define BUILTIN_IFCONFIG
#undef F_SETFD

#if defined (USE_DEFAULT_NETWORK)
#  define USE_SOCKETS
#endif

#ifdef NEED_PRAND_CONF

const char *cmds[] = {
	"arp -an",
//	"/usr/bin/netstat -an 2>&1",
	"info",
//	"dig com. soa +ti=1 +retry=0 2>&1",
//	"/usr/bin/netstat -an 2>&1",
//	"dig . soa +ti=1 +retry=0 2>&1",
	NULL
};

const char *dirs[] = {
	"T:",
	"SYS:T",
	"",
	":",
	NULL
};

const char *files[] = {
	"Net:Logs/Syslog",
	NULL
};
#endif /* NEED_PRAND_CONF */
