 /* morphos.h

   System dependencies for MorphOS */

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

#include <clib/netlib_protos.h>
#include <netdb.h>
#include <dos/dosextens.h>
#include <exec/ports.h>
#include <libraries/miami.h>
#include <utility/hooks.h>
#include <utility/tagitem.h>
#include <syslog.h>
#include <sys/types.h>
#include <string.h>
#include <paths.h>
#include <errno.h>
#include <malloc.h>
#include <unistd.h>
#include <setjmp.h>
#include <limits.h>

#include <sys/filio.h>
#include <sys/wait.h>
#include <signal.h>

extern int h_errno;

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

#define SOCKLEN_T LONG
#define INADDR_LOOPBACK 0x7f000001

#define _PATH_DHCPD_PID	"MOSNet:T/dhcpd.pid"
#define _PATH_DHCPD_DB "MOSNet:db/dhcpd.leases"
#define _PATH_DHCPD_CONF "MOSNet:db/dhcpd.conf"
#define _PATH_DHCLIENT_PID "MOSNet:T/dhclient.pid"
#define _PATH_DHCLIENT_DB "MOSNet:db/dhclient.leases"
#define _PATH_DHCLIENT_CONF "MOSNet:db/dhclient.conf"
#define _PATH_RESOLV_CONF "MOSNet:db/resolv.conf"
#define _PATH_DHCRELAY_PID "MOSNet:T/dhcrelay.pid"

#define EOL	'\n'
#define VOIDPTR void *

/* Time stuff... */
#include <sys/time.h>
#define TIME time_t
#define GET_TIME(x)	time ((x))

#define HAVE_SA_LEN
#define HAVE_CHKABORT
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
	"MOSNet:Logs/Syslog",
	NULL
};
#endif /* NEED_PRAND_CONF */
