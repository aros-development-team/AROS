/*
 * Copyright (C) 2026 The AROS Development Team. All rights reserved.
 * BSD 3-Clause License (see ip.c for full text).
 */

#define INET6 1

#include <dos/dos.h>
#include <proto/exec.h>
#include <proto/socket.h>
#include <proto/miami.h>

#include <sys/param.h>
#include <sys/socket.h>
#include <sys/sockio.h>

#include <net/if.h>
#include <net/if_dl.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "ip.h"

const char *
format_sockaddr(const struct sockaddr *sa, char *buf, size_t len)
{
	if (sa == NULL) {
		snprintf(buf, len, "(null)");
		return buf;
	}

	switch (sa->sa_family) {
	case AF_INET: {
		struct sockaddr_in *sin =
		    (struct sockaddr_in *)sa;
		if (sin->sin_addr.s_addr == INADDR_ANY) {
			snprintf(buf, len, "0.0.0.0");
		} else {
			inet_ntop(AF_INET, &sin->sin_addr, buf, len);
		}
		break;
	}
#ifdef INET6
	case AF_INET6: {
		struct sockaddr_in6 *sin6 =
		    (struct sockaddr_in6 *)sa;
		inet_ntop(AF_INET6, &sin6->sin6_addr, buf, len);
		break;
	}
#endif
	case AF_LINK: {
		const struct sockaddr_dl *sdl =
		    (const struct sockaddr_dl *)sa;
		if (sdl->sdl_alen > 0) {
			format_lladdr((const unsigned char *)LLADDR(sdl),
			    sdl->sdl_alen, buf, len);
		} else if (sdl->sdl_nlen > 0) {
			size_t n = sdl->sdl_nlen;
			if (n >= len)
				n = len - 1;
			memcpy(buf, sdl->sdl_data, n);
			buf[n] = '\0';
		} else {
			snprintf(buf, len, "link#%d", sdl->sdl_index);
		}
		break;
	}
	default:
		snprintf(buf, len, "family=%d", sa->sa_family);
		break;
	}
	return buf;
}

const char *
format_lladdr(const unsigned char *addr, int len, char *buf, size_t buflen)
{
	int i;
	char *p = buf;
	size_t remain = buflen;

	for (i = 0; i < len && remain > 3; i++) {
		int n;
		if (i > 0) {
			*p++ = ':';
			remain--;
		}
		n = snprintf(p, remain, "%02x", addr[i]);
		p += n;
		remain -= n;
	}
	*p = '\0';
	return buf;
}

void
format_flags(unsigned int flags, char *buf, size_t buflen)
{
	char *p = buf;
	size_t remain = buflen;
	int first = 1;

#define FLAGADD(f, name) \
	if ((flags & (f)) && remain > 1) { \
		int n = snprintf(p, remain, "%s%s", first ? "" : ",", name); \
		p += n; remain -= n; first = 0; \
	}

	FLAGADD(IFF_UP,        "UP")
	FLAGADD(IFF_BROADCAST, "BROADCAST")
	FLAGADD(IFF_DEBUG,     "DEBUG")
	FLAGADD(IFF_LOOPBACK,  "LOOPBACK")
	FLAGADD(IFF_POINTOPOINT, "POINTOPOINT")
	FLAGADD(IFF_DRV_RUNNING, "RUNNING")
	FLAGADD(IFF_NOARP,     "NOARP")
	FLAGADD(IFF_PROMISC,   "PROMISC")
	FLAGADD(IFF_ALLMULTI,   "ALLMULTI")
	FLAGADD(IFF_SIMPLEX,   "SIMPLEX")
	FLAGADD(IFF_MULTICAST, "MULTICAST")
#undef FLAGADD

	*p = '\0';
}

int
mask2prefix(const struct sockaddr *sa)
{
	const unsigned char *addr;
	int len;
	int prefix = 0;
	int i;

	if (sa == NULL)
		return -1;

	switch (sa->sa_family) {
	case AF_INET: {
		const struct sockaddr_in *sin =
		    (const struct sockaddr_in *)sa;
		addr = (const unsigned char *)&sin->sin_addr;
		len = 4;
		break;
	}
#ifdef INET6
	case AF_INET6: {
		const struct sockaddr_in6 *sin6 =
		    (const struct sockaddr_in6 *)sa;
		addr = (const unsigned char *)&sin6->sin6_addr;
		len = 16;
		break;
	}
#endif
	case 0:
		/*
		 * Some routing messages use sa_family == 0 for netmasks.
		 * Treat the raw bytes after sa_family as the mask.
		 */
		addr = (const unsigned char *)sa + 2;
		if (sa->sa_len <= 2)
			return 0;
		len = sa->sa_len - 2;
		break;
	default:
		return -1;
	}

	for (i = 0; i < len; i++) {
		if (addr[i] == 0xff) {
			prefix += 8;
		} else {
			unsigned char v = addr[i];
			while (v & 0x80) {
				prefix++;
				v <<= 1;
			}
			break;
		}
	}
	return prefix;
}

int
ip_socket(int family)
{
	int s;

	if (family == AF_UNSPEC || family == AF_LINK)
		family = AF_INET;

	s = socket(family, SOCK_DGRAM, 0);
	if (s < 0) {
		perror("ip: socket");
	}
	return s;
}
