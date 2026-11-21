/*
 * Copyright (C) 2026 The AROS Dev Team
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef API_RES_MDNS_H
#define API_RES_MDNS_H

/*
 * Names in .local by multicast DNS (RFC 6762 section 3), for the resolver
 * module: api/res_mdns.c.
 */

#include <dnssd/dnssd.h>      /* struct DNSSDAddress: the network's common includes */

struct SocketBase;

/* How many addresses one look-up gives at most. */
#define RES_MDNS_ADDRESSES  16

/* name is in .local: a label or more before "local", any case, the final
 * dot optional. */
int res_mdns_name(const char *name);

/* An IPv6 address that needs a zone (fe80::/10). */
int res_mdns_linklocal6(const struct DNSSDAddress *a);

/*
 * The addresses of name (res_mdns_name()) of family - AF_INET, AF_INET6 or
 * AF_UNSPEC - into out, room for RES_MDNS_ADDRESSES of them: how many,
 * each once (an IPv6 link-local address once per link, da_Interface its
 * zone, RFC 4007). The caller's room: on a 68000 the stack is small. With
 * none, 0 and
 * *herr: HOST_NOT_FOUND when no host on the links answered for the name
 * in time, or dnssd.library is not there; TRY_AGAIN when mdnsd does not
 * run, or the look-up was broken off (errno EINTR); NETDB_INTERNAL with
 * errno ENOMEM.
 */
int res_mdns_lookup(struct SocketBase *libPtr, const char *name, int family,
    struct DNSSDAddress *out, int *herr);

#endif /* API_RES_MDNS_H */
