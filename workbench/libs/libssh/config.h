/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    config.h - libssh build configuration for AROS (hand-written; replaces
    the cmake-generated config.h, see config.h.cmake in the libssh archive).

    Crypto backend: mbedTLS, built against the same mbedtls_aros_config.h
    as the AROS mbedTLS linklibs.  Networking: bsdsocket.library through
    the inline bindings (sockets unconditionally; getaddrinfo and friends
    need __CONFIG_ROADSHOW__, defined on the compile line).
*/

#ifndef _LIBSSH_AROS_CONFIG_H
#define _LIBSSH_AROS_CONFIG_H

/* AROS's BSD-style <net/if.h> and <netinet/tcp.h> are not self-contained
 * (they want sockaddr, timeval and the BYTE_ORDER macros declared first);
 * every libssh source includes config.h before any system header, so
 * satisfy those prerequisites here. */
#include <sys/types.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <endian.h>

/* Not declared by AROS <net/if.h>; provided by libssh_net_aros.c (a
 * miami.library binding, 0 without it). */
unsigned int if_nametoindex(const char *ifname);

/* A bsdsocket.library base belongs to the task that opened it, so a program
 * that serves sessions from several tasks needs one base per task. Every
 * socket call libssh makes resolves its base through this lookup;
 * libssh_net_aros.c supplies a default returning the program's global base,
 * which a multi-task program overrides (sshd). proto/bsdsocket.h honours
 * SocketBase being a macro. */
struct Library;
struct Library *__libssh_SocketBase(void);
#define SocketBase (__libssh_SocketBase())

#define PACKAGE "libssh"
#define VERSION "0.12.0"

/* Global configuration directory and file paths (the client config is also
 * set on the compile line) */
#define GLOBAL_CONF_DIR "ENVARC:SSH"
#ifndef GLOBAL_CLIENT_CONFIG
#define GLOBAL_CLIENT_CONFIG "ENVARC:SSH/ssh_config"
#endif
#ifndef GLOBAL_BIND_CONFIG
#define GLOBAL_BIND_CONFIG "ENVARC:SSH/sshd_config"
#endif

/************************** HEADER FILES *************************/

#define HAVE_ARPA_INET_H 1
#define HAVE_SYS_TIME_H 1
#define HAVE_UNISTD_H 1
#define HAVE_STDINT_H 1
#define HAVE_TERMIOS_H 1            /* posixc tcgetattr/tcsetattr */

/*************************** CRYPTO ******************************/

/* mbedTLS backend */
#define HAVE_LIBMBEDCRYPTO 1
#define HAVE_ECC 1
#define HAVE_MBEDTLS_CURVE25519 1   /* MBEDTLS_ECP_DP_CURVE25519_ENABLED */

/*************************** FUNCTIONS ***************************/

#define HAVE_SNPRINTF 1
#define HAVE_VSNPRINTF 1
#define HAVE_ISBLANK 1
#define HAVE_STRNCPY 1
#define HAVE_STRNDUP 1
#define HAVE_STRTOULL 1
#define HAVE_GETADDRINFO 1          /* bsdsocket RS_getaddrinfo binding */
#define HAVE_SELECT 1               /* bsdsocket WaitSelect binding     */
#define HAVE_CLOCK_GETTIME 1        /* posixc                           */
#define HAVE_NTOHLL 1               /* netinet/in.h BEQuad()            */
#define HAVE_HTONLL 1               /* netinet/in.h MakeBEQuad()        */

/**************************** OPTIONS ****************************/

#define HAVE_FALLTHROUGH_ATTRIBUTE 1
#define HAVE_UNUSED_ATTRIBUTE 1
#define HAVE_CONSTRUCTOR_ATTRIBUTE 1
#define HAVE_DESTRUCTOR_ATTRIBUTE 1
#define HAVE_GCC_VOLATILE_MEMORY_PROTECTION 1
#define HAVE_COMPILER__FUNC__ 1
#define HAVE_COMPILER__FUNCTION__ 1

#define WITH_SFTP 1
#define WITH_SERVER 1
#define WITH_GEX 1

/*************************** ENDIAN *****************************/

#if defined(__BIG_ENDIAN__) || \
    (defined(__BYTE_ORDER__) && defined(__ORDER_BIG_ENDIAN__) && \
     __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__)
#define WORDS_BIGENDIAN 1
#endif

#endif /* _LIBSSH_AROS_CONFIG_H */
