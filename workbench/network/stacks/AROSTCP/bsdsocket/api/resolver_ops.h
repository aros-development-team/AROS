/*
 * Copyright (C) 2026 The AROS Dev Team
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef API_RESOLVER_OPS_H
#define API_RESOLVER_OPS_H

/*
 * THE RESOLVER AS A MODULE (module "resolver", resolver.atcp;
 * Stage 7, package R).
 *
 * The name look-ups - the DNS resolver and its cache, the hosts table's
 * half of gethostbyname() and gethostbyaddr(), getaddrinfo() and
 * getnameinfo() of both front-ends - are a module that is started, and its
 * library loaded, the first time an application asks for a name. The
 * vectors of bsdsocket.library and miami.library keep their places: they
 * are stubs in the core (api/resolver_stubs.c) that call through the table
 * below, which the module puts into the core's demand slot "resolver" when
 * it starts (net/netdemand.h).
 *
 * What stays in the core, so that no library is needed for it:
 *  - a numeric address, and the localhost names, asked of gethostbyname(),
 *    gethostbyname2() and gethostbyname_r(): answered by the stubs
 *    (resolver_literal(); the library asks the same function, through the
 *    core's table, for the names it is given by getaddrinfo());
 *  - freeaddrinfo() and gai_strerror() of both front-ends: a list from
 *    getaddrinfo() is exec memory (AllocVec()), freed by the core whether
 *    or not the library is still there; an error text needs no library;
 *  - each library base's resolver state and buffers (struct SocketBase),
 *    and res_cleanup_db(), which frees the state at a base's close: the
 *    state is the core's pool memory, freed by the core after an unload;
 *  - the resolver options of miami.library (a member of the base).
 *
 * Every function of the table is called by a stub only: inside the demand
 * slot (netdemand_enter() .. netdemand_leave()), with the base's resolver
 * lock held (ObtainResolver()), never with the socket calls fenced. They
 * may sleep (a name server is asked through the core's socket calls); a
 * stop of the module is refused while one runs (the slot's count).
 */

#include <exec/types.h>

struct SocketBase;
struct hostent;
struct addrinfo;
struct sockaddr;
struct state;

struct resolver_ops {
    unsigned short ro_size;     /* sizeof(struct resolver_ops) of the module */

    /* gethostbyname() (af AF_INET), gethostbyname2(); a name, not an address */
    struct hostent *(*ro_gethostbyname2)(const char *name, int af,
                                         struct SocketBase *libPtr);
    struct hostent *(*ro_gethostbyaddr)(UBYTE *addr, int len, int type,
                                        struct SocketBase *libPtr);
    /* gethostbyname_r(): arguments checked, not an address */
    struct hostent *(*ro_gethostbyname_r)(const char *name, struct hostent *result,
                                          char *buf, LONG buflen, LONG *h_errnop,
                                          struct SocketBase *libPtr);
    struct hostent *(*ro_gethostbyaddr_r)(const char *addr, LONG len, LONG type,
                                          struct hostent *result, char *buf,
                                          LONG buflen, LONG *h_errnop,
                                          struct SocketBase *libPtr);
    /* the one getaddrinfo() and getnameinfo() of both front-ends */
    LONG (*ro_getaddrinfo)(const char *hostname, const char *servname,
                           const struct addrinfo *hints, struct addrinfo **result,
                           struct SocketBase *libPtr);
    LONG (*ro_getnameinfo)(struct sockaddr *sa, LONG salen, char *hostname,
                           LONG hostlen, char *servicename, LONG servicelen,
                           LONG flags, struct SocketBase *libPtr);
};

#define RESOLVER_MODULE "resolver"

/*
 * THE CORE'S SIDE (api/resolver_stubs.c), also used by the library through
 * the core's table (net/netcore.def).
 *
 * resolver_literal(): whether `name', asked for addresses of family `af'
 * (AF_INET: gethostbyname(); AF_INET6 or another: gethostbyname2()), is
 * answered without a look-up - a numeric address of that family or a
 * localhost name (RFC 6761 6.3), or a family gethostbyname2() does not
 * know. 1: answered, the base's hostent (or NULL, with errno set) in *hpp.
 * 0: it is a name to look up.
 *
 * res_cleanup_db(): frees a base's resolver state (api/resolv.h).
 *
 * This header is safe in any file of the stack (it declares, it includes
 * nothing of the stack), so that net/netcore.h can include it for the
 * names above that are in the core's table.
 */
int resolver_literal(struct SocketBase *libPtr, const char *name, int af,
                     struct hostent **hpp);
void res_cleanup_db(struct state *state);

/* the name service order (config item USENS; core, api/amiga_hostname.c) */
extern LONG usens;

/* the message of the CHECK_TASK macros (core, api/amiga_api.c) */
extern const char wrongTaskErrorFmt[];

/*
 * allocDataBuffer() of api/allocdatabuffer.h is an inline function that
 * calls the core by name, which a module library cannot do: the same test,
 * with the call written where the core's table maps it.
 */
#define res_allocdatabuffer(DB, size) \
    ((DB)->db_Size < (size) ? doAllocDataBuffer((DB), (size)) : TRUE)

/*
 * THE LIBRARY'S SIDE (the files of conf/group-resolver.cfg): what the table
 * above is filled with, and the cache's start and stop.
 */
struct hostent *res_gethostbyname(const char *name, struct SocketBase *libPtr);
struct hostent *res_gethostbyname2(const char *name, int af, struct SocketBase *libPtr);
struct hostent *res_gethostbyaddr(UBYTE *addr, int len, int type, struct SocketBase *libPtr);
struct hostent *res_gethostbyname_r(const char *name, struct hostent *result,
                                    char *buf, LONG buflen, LONG *h_errnop,
                                    struct SocketBase *libPtr);
struct hostent *res_gethostbyaddr_r(const char *addr, LONG len, LONG type,
                                    struct hostent *result, char *buf, LONG buflen,
                                    LONG *h_errnop, struct SocketBase *libPtr);
LONG res_getaddrinfo(const char *hostname, const char *servname,
                     const struct addrinfo *hints, struct addrinfo **result,
                     struct SocketBase *libPtr);
LONG res_getnameinfo(struct sockaddr *sa, LONG salen, char *hostname, LONG hostlen,
                     char *servicename, LONG servicelen, LONG flags,
                     struct SocketBase *libPtr);

int res_strcasecmp(const char *a, const char *b);   /* api/gethtent.c: ASCII */

void dns_cache_start(void);     /* api/dns_cache.c: at the module's start */
void dns_cache_stop(void);      /* ... and stop: every entry freed */

#endif /* !API_RESOLVER_OPS_H */
