/* includes/config.h - Hand-crafted for AROS (replaces autoconf output) */
#ifndef ISC_DHCP_CONFIG_H
#define ISC_DHCP_CONFIG_H 1

/* ISC DHCP 4.4.3 */
#define PACKAGE         "dhcp"
#define PACKAGE_NAME    "DHCP"
#define PACKAGE_STRING  "DHCP 4.4.3"
#define PACKAGE_VERSION "4.4.3"
#define VERSION         "4.4.3"

/* Enable DHCPv6 support */
#define DHCPv6 1

/* Use standard BSD socket API (no raw sockets on AROS) */
#define USE_SOCKETS 1

/* Use built-in ifconfig (AROS has no external ifconfig for DHCP) */
#define BUILTIN_IFCONFIG 1

/* AROS socket is not a POSIX file descriptor */
#define SOCKET_IS_NOT_A_FILE 1

/* AROS has sockaddr sa_len field */
#define HAVE_SA_LEN 1

/* No fork() on AROS */
#define FORK_MISSING 1

/* No fsync() semantics needed */
#define FSYNC_MISSING 1

/* No getuid() on AROS */
#define GET_USER_ID_MISSING 1

/* No setservent/setprotoent on AROS */
#define SET_SERVENT_MISSING  1
#define SET_PROTOENT_MISSING 1

/* Available standard headers */
#define HAVE_SYS_TYPES_H    1
#define HAVE_SYS_SOCKET_H   1
#define HAVE_SYS_STAT_H     1
#define HAVE_STDLIB_H       1
#define HAVE_STRING_H       1
#define HAVE_STRINGS_H      1
#define HAVE_INTTYPES_H     1
#define HAVE_STDINT_H       1
#define HAVE_UNISTD_H       1
#define STDC_HEADERS        1

/* net/if_dl.h is present on AROS */
#define HAVE_NET_IF_DL_H    1

/* inet_ntop/inet_pton provided by Miami library */
#define HAVE_INET_NTOP      1
#define HAVE_INET_PTON      1

/* No strlcat on AROS (ISC will use its own) */
/* #undef HAVE_STRLCAT */

/* DHCP byte order - AROS on x86 is little-endian */
#define DHCP_BYTE_ORDER LITTLE_ENDIAN

/* Size of struct iaddr * */
#if __WORDSIZE == 64
#define SIZEOF_STRUCT_IADDR_P 8
#else
#define SIZEOF_STRUCT_IADDR_P 4
#endif

/* Features to enable */
#ifndef __AROS__
#define ENABLE_EXECUTE      1
#endif
#define DELAYED_ACK         1

/* Features to disable on AROS */
/* #undef FAILOVER_PROTOCOL */
/* #undef TRACING */
/* #undef DHCP4o6 */
/* #undef HAVE_LPF */
/* #undef HAVE_BPF */
/* #undef HAVE_DLPI */

/* noreturn attribute */
#define ISC_DHCP_NORETURN __attribute__((noreturn))

/* Flexible array member */
#define FLEXIBLE_ARRAY_MEMBER /**/

/* AROS paths */
#ifndef AROSTCP_DB
#define AROSTCP_DB "SYS:System/Network/AROSTCP/db/"
#endif
#ifndef AROSTCP_T
/* Use POSIX-style absolute path: /T/ maps to the T: assign via AROS posixc */
#define AROSTCP_T "/T/"
#endif
#define _PATH_DHCLIENT_PID   AROSTCP_T  "dhclient.pid"
#define _PATH_DHCLIENT_DB    AROSTCP_T  "dhclient.leases"
#define _PATH_DHCLIENT6_PID  AROSTCP_T  "dhclient6.pid"
#define _PATH_DHCLIENT6_DB   AROSTCP_T  "dhclient6.leases"
#define _PATH_DHCLIENT_CONF  AROSTCP_DB "dhclient.conf"
#define _PATH_DHCPD_PID      AROSTCP_T  "dhcpd.pid"
#define _PATH_DHCPD_DB       AROSTCP_DB "dhcpd.leases"
#define _PATH_DHCPD_CONF     AROSTCP_DB "dhcpd.conf"
#define _PATH_DHCRELAY_PID   AROSTCP_T  "dhcrelay.pid"

/* GNU source extensions */
#define _GNU_SOURCE 1

/* AROS-specific OS includes and adaptations */
#ifdef __AROS__
#include <netdb.h>
#include <dos/dosextens.h>
#include <exec/ports.h>
#include <libraries/miami.h>
#include <proto/bsdsocket.h>
#include <proto/miami.h>
#include <utility/hooks.h>
#include <utility/tagitem.h>
#include <net/if.h>
#include <net/if_dl.h>
#include <net/route.h>
#include <sys/sockio.h>
#define _SYS_IOCTL_H_
#include <sys/time.h>
#include <stdarg.h>
/* Override Miami library macros: Miami's inet_pton/inet_ntop do not support
 * AF_INET6.  We provide our own implementations in common/inet_pton.c using
 * aros_inet_pton/aros_inet_ntop names to avoid conflicting with the Miami
 * prototype declarations in clib/miami_protos.h. */
#ifdef inet_pton
#undef inet_pton
#endif
#ifdef inet_ntop
#undef inet_ntop
#endif
/* Declare our implementations using their actual (non-conflicting) names */
int aros_inet_pton(int af, const char *src, void *dst);
const char *aros_inet_ntop(int af, const void *src, char *dst, LONG size);
/* Redirect all uses of inet_pton / inet_ntop to our implementations */
#define inet_pton aros_inet_pton
#define inet_ntop aros_inet_ntop

#define EOL          '\n'
#define VOIDPTR      void *
#define TIME         time_t
#define SOCKLEN_T    LONG
#define VA_DOTDOTDOT ...
#define va_dcl
#define VA_start(list, last) va_start(list, last)

#define GET_TIME(x) do {         \
    struct timeval _xt;          \
    gettimeofday(&_xt, NULL);    \
    *(x) = _xt.tv_sec;          \
} while (0)

#ifndef INADDR_LOOPBACK
#define INADDR_LOOPBACK 0x7f000001
#endif
#undef F_SETFD

/* AROS's exec/types.h defines TEXT as typedef unsigned char TEXT;
 * This conflicts with the TEXT enum value in dhctoken.h.
 * Undefine it here so dhctoken.h can use TEXT as an identifier. */
#ifdef TEXT
#undef TEXT
#endif

#if __WORDSIZE == 64
#define PTRSIZE_64BIT
#endif

/* IF_NAMESIZE: AROS uses IFNAMSIZ */
#ifndef IF_NAMESIZE
# ifdef IFNAMSIZ
#  define IF_NAMESIZE IFNAMSIZ
# else
#  define IF_NAMESIZE 16
# endif
#endif

/* IPv6 address classification macros (from RFC 2553) */
#ifndef IN6_IS_ADDR_UNSPECIFIED
# define IN6_IS_ADDR_UNSPECIFIED(a) \
    (((a)->s6_addr[0] == 0) && ((a)->s6_addr[1] == 0) && \
     ((a)->s6_addr[2] == 0) && ((a)->s6_addr[3] == 0) && \
     ((a)->s6_addr[4] == 0) && ((a)->s6_addr[5] == 0) && \
     ((a)->s6_addr[6] == 0) && ((a)->s6_addr[7] == 0) && \
     ((a)->s6_addr[8] == 0) && ((a)->s6_addr[9] == 0) && \
     ((a)->s6_addr[10] == 0) && ((a)->s6_addr[11] == 0) && \
     ((a)->s6_addr[12] == 0) && ((a)->s6_addr[13] == 0) && \
     ((a)->s6_addr[14] == 0) && ((a)->s6_addr[15] == 0))
#endif
#ifndef IN6_IS_ADDR_LOOPBACK
# define IN6_IS_ADDR_LOOPBACK(a) \
    (((a)->s6_addr[0] == 0) && ((a)->s6_addr[1] == 0) && \
     ((a)->s6_addr[2] == 0) && ((a)->s6_addr[3] == 0) && \
     ((a)->s6_addr[4] == 0) && ((a)->s6_addr[5] == 0) && \
     ((a)->s6_addr[6] == 0) && ((a)->s6_addr[7] == 0) && \
     ((a)->s6_addr[8] == 0) && ((a)->s6_addr[9] == 0) && \
     ((a)->s6_addr[10] == 0) && ((a)->s6_addr[11] == 0) && \
     ((a)->s6_addr[12] == 0) && ((a)->s6_addr[13] == 0) && \
     ((a)->s6_addr[14] == 0) && ((a)->s6_addr[15] == 1))
#endif
#ifndef IN6_IS_ADDR_MULTICAST
# define IN6_IS_ADDR_MULTICAST(a) ((a)->s6_addr[0] == 0xff)
#endif
#ifndef IN6_IS_ADDR_LINKLOCAL
# define IN6_IS_ADDR_LINKLOCAL(a) \
    (((a)->s6_addr[0] == 0xfe) && (((a)->s6_addr[1] & 0xc0) == 0x80))
#endif
#ifndef IN6_IS_ADDR_SITELOCAL
# define IN6_IS_ADDR_SITELOCAL(a) \
    (((a)->s6_addr[0] == 0xfe) && (((a)->s6_addr[1] & 0xc0) == 0xc0))
#endif

/* struct in6_pktinfo (RFC 3542) - AROS netinet/in.h defines IPV6_PKTINFO
 * but doesn't declare the struct */
#ifndef HAVE_IN6_PKTINFO
# include <netinet/in.h>
struct in6_pktinfo {
	struct in6_addr ipi6_addr;
	unsigned int    ipi6_ifindex;
};
# define HAVE_IN6_PKTINFO 1
#endif

#endif /* __AROS__ */

#endif /* ISC_DHCP_CONFIG_H */
