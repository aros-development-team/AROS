/*
 * Copyright (C) 2026 The AROS Dev Team
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

/*
 * The name look-up vectors of bsdsocket.library and miami.library, kept in
 * the core while the resolver is a module loaded on first use
 * (api/resolver_ops.h). Each keeps the place, the name and the arguments
 * of the vector it always was; what it does:
 *
 *  - it answers itself what needs no resolver: a numeric address or a
 *    localhost name asked of gethostbyname(), gethostbyname2() or
 *    gethostbyname_r() - so that a program that connects to an address,
 *    and asks for its hostent first as many do, does not load a library -
 *    and freeaddrinfo(), gai_strerror() and the resolver options;
 *  - for anything else it takes the base's resolver lock, as the vector
 *    always did, gets the resolver's table through the demand slot (which
 *    has the module started, and its library loaded, when it is not
 *    running), and calls through it;
 *  - when the resolver cannot be had (its library is not installed, its
 *    test switch ENV:AROSTCP/Without_resolver is set, the stack is
 *    ending) it answers what the call answers when the name service fails
 *    for a reason outside the application: h_errno NO_RECOVERY, or
 *    EAI_FAIL, with errno saying why (ENOENT: not installed). The log says
 *    so once, when the library is first looked for.
 *
 * Nothing of this is called for a packet or a socket call. The demand slot
 * is entered without the socket calls fenced (net/netdemand.h): none of
 * these vectors takes the fence. The base's resolver lock may be held
 * while the module is loaded - the stack's task, which loads it, never
 * takes a base's resolver lock.
 */

#include <conf.h>

#include <aros/libcall.h>

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/socket.h>
#include <sys/malloc.h>
#include <sys/errno.h>
#include <netinet/in.h>
#include <netdb.h>

#include <arpa/nameser.h>
#include <api/resolv.h>
#include <kern/amiga_includes.h>
#include <api/amiga_api.h>
#include <api/amiga_libcallentry.h>
#include <api/allocdatabuffer.h>
#include <api/apicalls.h>
#include <api/dns_cache.h>
#include <api/miami_api.h>
#include <net/netdemand.h>
#include <api/resolver_ops.h>

/* api/inet_pton.c (declared for the modules in net/netcore.h) */
extern int __inet_pton(int af, const char *src, void *dst, struct SocketBase *SocketBase);

/*
 * The demand slot: the resolver's table while the module runs. Found by
 * the module's name when it puts its table in (netdemand_set()).
 */
static struct netdemand resolver_demand = { RESOLVER_MODULE };
NETDEMAND_SLOT(resolver_demand);

/* longword align (as api/gethostnamadr.c does for the hostent it builds) */
#define ALIGN(p) (((long)(p) + (sizeof(long) - 1)) &~ (sizeof(long) - 1))

#define HOSTENT ((struct hostent *)libPtr->hostents.db_Addr)

/*
 * The resolver's table, or NULL - then the call is answered as when the
 * name service fails (h_errno NO_RECOVERY, errno why), and what is left to
 * answer is the stub's.
 */
static const struct resolver_ops *
resolver_enter(struct SocketBase *libPtr)
{
    const void *ops;
    int error;

    if((error = netdemand_enter_error(&resolver_demand, &ops)) != 0) {
        writeErrnoValue(libPtr, error);
        h_errno = NO_RECOVERY;
        return (NULL);
    }
    return ((const struct resolver_ops *)ops);
}

#define resolver_leave()    netdemand_leave(&resolver_demand)

/* ---- what is answered without a look-up ---------------------------------- */

/* a dotted quad of digits: what gethostbyname() always took for an address */
static int
isipaddr(const char *name)
{
    const char *c;
    int i = 0;

    for(c = name; *c; c++)
        if(*c < '0' || *c > '9') {
            if(*c == '.')
                i++;
            else
                return 0;
        }
    return (i == 3);
}

/*
 * The localhost names (RFC 6761 section 6.3): "localhost" and the names
 * within ".localhost", in any case, with or without the final dot. They
 * are the loopback addresses, never looked up in the host table or the
 * DNS.
 */
static int
islocalhost(const char *name)
{
    size_t n = strnlen(name, MAXDNAME);

    if(n > 0 && name[n - 1] == '.')
        n--;
    if(n < 9 || strncasecmp(name + n - 9, "localhost", 9) != 0)
        return 0;
    return n == 9 || name[n - 10] == '.';
}

static const u_char loopback4[4] = { 127, 0, 0, 1 };
static const u_char loopback6[16] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1 };

/*
 * hostent_literal - the base's hostent for one address of family af
 * (4 or 16 bytes at addr) named name: hostent | address | h_addr_list[2]
 * (whose NULL is also the empty h_aliases) | name.
 */
static struct hostent *
hostent_literal(struct SocketBase *libPtr, int af, const void *addr, const char *name)
{
    int alen = af == AF_INET6 ? sizeof(struct in6_addr) : sizeof(struct in_addr);
    int nlen = strnlen(name, MAXDNAME) + 1;
    char *base, **addrlist;

    if(allocDataBuffer(&libPtr->hostents, sizeof(struct hostent) + ALIGN(alen) +
                       2 * sizeof(char *) + nlen) == FALSE) {
        writeErrnoValue(libPtr, ENOMEM);
        h_errno = 0;
        return NULL;
    }
    HOSTENT->h_addrtype = af;
    HOSTENT->h_length = alen;
    base = (char *)(HOSTENT + 1);
    bcopy(addr, base, alen);
    addrlist = (char **)(base + ALIGN(alen));
    addrlist[0] = base;
    addrlist[1] = NULL;
    HOSTENT->h_addr_list = addrlist;
    HOSTENT->h_aliases = addrlist + 1;
    HOSTENT->h_name = (char *)(addrlist + 2);
    memcpy(HOSTENT->h_name, name, nlen);
    HOSTENT->h_name[nlen - 1] = '\0';
    return HOSTENT;
}

/*
 * Answered here as the resolver always answered them, before it looked
 * anything up (api/resolver_ops.h). AF_INET: gethostbyname(); any other
 * family: gethostbyname2().
 */
int
resolver_literal(struct SocketBase *libPtr, const char *name, int af, struct hostent **hpp)
{
    struct in6_addr in6;
    struct in_addr in4;

    *hpp = NULL;
    if(af == AF_INET) {
        if(isipaddr(name)) {
            if(!__inet_aton(name, &in4)) {
                writeErrnoValue(libPtr, EINVAL);
                h_errno = 0;
            } else
                *hpp = hostent_literal(libPtr, AF_INET, &in4, name);
            return 1;
        }
        if(islocalhost(name)) {
            *hpp = hostent_literal(libPtr, AF_INET, loopback4, name);
            return 1;
        }
        return 0;
    }
    if(af != AF_INET6) {
        writeErrnoValue(libPtr, EAFNOSUPPORT);
        return 1;
    }
    if(islocalhost(name)) {
        *hpp = hostent_literal(libPtr, AF_INET6, loopback6, name);
        return 1;
    }
    /* an IPv6 literal (it has a ':'): parsed, never looked up */
    if(strchr(name, ':') != NULL) {
        if(__inet_pton(AF_INET6, name, &in6, libPtr) == 1)
            *hpp = hostent_literal(libPtr, AF_INET6, &in6, name);
        else
            writeErrnoValue(libPtr, EINVAL);
        return 1;
    }
    return 0;
}

/*
 * hostent_literal_r -- the hostent for one IPv4 address named name, in the
 * caller's buffer: address | h_addr_list[2] | h_aliases[1] | name.
 * Returns 0, or ERANGE if buf is too small.
 */
static int
hostent_literal_r(const struct in_addr *addr, const char *name,
                  struct hostent *result, char *buf, LONG buflen)
{
    int nlen = strnlen(name, MAXDNAME) + 1;
    char *bp = buf;
    char **addrlist;

    if((int)ALIGN(sizeof(struct in_addr)) + 3 * (int)sizeof(char *) + nlen > buflen)
        return ERANGE;
    memcpy(bp, addr, sizeof(struct in_addr));
    bp += ALIGN(sizeof(struct in_addr));
    addrlist = (char **)bp;
    addrlist[0] = buf;
    addrlist[1] = NULL;
    result->h_addr_list = addrlist;
    bp += 2 * sizeof(char *);
    result->h_aliases = (char **)bp;    /* empty */
    *(char **)bp = NULL;
    bp += sizeof(char *);
    result->h_name = bp;
    memcpy(bp, name, nlen);
    bp[nlen - 1] = '\0';
    result->h_addrtype = AF_INET;
    result->h_length = sizeof(struct in_addr);
    return 0;
}

/* ---- gethostbyname(), gethostbyname2(), gethostbyaddr() ------------------ */

/* With the base's resolver lock held. */
static struct hostent *
resolver_byname(const char *name, int af, struct SocketBase *libPtr)
{
    const struct resolver_ops *ro;
    struct hostent *hp;

    CHECK_TASK2();
    if(resolver_literal(libPtr, name, af, &hp))
        return (hp);
    if((ro = resolver_enter(libPtr)) == NULL)
        return (NULL);
    hp = ro->ro_gethostbyname2(name, af, libPtr);
    resolver_leave();
    return (hp);
}

/*
 * Also the core's own: gethostname() asks for the name of the host's
 * address when none is set (api/amiga_hostname.c).
 */
struct hostent *
__gethostbyaddr(UBYTE *addr, int len, int type, struct SocketBase *libPtr)
{
    const struct resolver_ops *ro;
    struct hostent *hp;

    /* a family it never looks up loads nothing (the answer it always gave) */
    if(type != AF_INET && type != AF_INET6)
        return (NULL);
    if((ro = resolver_enter(libPtr)) == NULL)
        return (NULL);
    hp = ro->ro_gethostbyaddr(addr, len, type, libPtr);
    resolver_leave();
    return (hp);
}

AROS_LH1(struct hostent *, gethostbyname,
         AROS_LHA(char *, name, A0),
         struct SocketBase *, libPtr, 35, UL)
{
    AROS_LIBFUNC_INIT
    struct hostent *r;

    ObtainResolver(libPtr);
    r = resolver_byname(name, AF_INET, libPtr);
    writeHErrnoValue(libPtr);
    ReleaseResolver(libPtr);
    return r;
    AROS_LIBFUNC_EXIT
}

AROS_LH3(struct hostent *, gethostbyaddr,
         AROS_LHA(UBYTE *, addr, A0),
         AROS_LHA(int, len, D0),
         AROS_LHA(int, type, D1),
         struct SocketBase *, libPtr, 36, UL)
{
    AROS_LIBFUNC_INIT
    struct hostent *r;

    ObtainResolver(libPtr);
    r = __gethostbyaddr(addr, len, type, libPtr);
    writeHErrnoValue(libPtr);
    ReleaseResolver(libPtr);
    return r;
    AROS_LIBFUNC_EXIT
}

#ifdef __CONFIG_ROADSHOW__

/* ---- gethostbyname_r(), gethostbyaddr_r() -------------------------------- */

AROS_LH5(struct hostent *, RS_gethostbyname_r,
         AROS_LHA(const char *, name, A0),
         AROS_LHA(struct hostent *, result, A1),
         AROS_LHA(char *, buf, A2),
         AROS_LHA(LONG, buflen, D0),
         AROS_LHA(LONG *, h_errnop, A3),
         struct SocketBase *, libPtr, 123, UL)
{
    AROS_LIBFUNC_INIT
    const struct resolver_ops *ro;
    struct hostent *r = NULL;
    struct in_addr inaddr;
    LONG herr;

    ObtainResolver(libPtr);
    if(!name || !result || !buf || buflen <= 0) {
        writeErrnoValue(libPtr, EINVAL);
        herr = NETDB_INTERNAL;
    } else if(isipaddr(name) || islocalhost(name)) {
        /* a dotted-quad address, or a localhost name: no look-up */
        if(isipaddr(name) && !__inet_aton(name, &inaddr)) {
            writeErrnoValue(libPtr, EINVAL);
            herr = HOST_NOT_FOUND;
        } else {
            if(!isipaddr(name))
                memcpy(&inaddr, loopback4, sizeof(inaddr));
            if(hostent_literal_r(&inaddr, name, result, buf, buflen) != 0) {
                writeErrnoValue(libPtr, ERANGE);
                herr = NETDB_INTERNAL;
            } else {
                r = result;
                herr = 0;
            }
        }
    } else if((ro = resolver_enter(libPtr)) == NULL)
        herr = NO_RECOVERY;
    else {
        /* it sets *h_errnop itself */
        r = ro->ro_gethostbyname_r(name, result, buf, buflen, h_errnop, libPtr);
        resolver_leave();
        ReleaseResolver(libPtr);
        return r;
    }
    if(h_errnop)
        *h_errnop = herr;
    ReleaseResolver(libPtr);
    return r;
    AROS_LIBFUNC_EXIT
}

AROS_LH7(struct hostent *, RS_gethostbyaddr_r,
         AROS_LHA(const char *, addr, A0),
         AROS_LHA(LONG, len, D0),
         AROS_LHA(LONG, type, D1),
         AROS_LHA(struct hostent *, result, A1),
         AROS_LHA(char *, buf, A2),
         AROS_LHA(LONG, buflen, D2),
         AROS_LHA(LONG *, h_errnop, A3),
         struct SocketBase *, libPtr, 124, UL)
{
    AROS_LIBFUNC_INIT
    const struct resolver_ops *ro;
    struct hostent *r = NULL;

    ObtainResolver(libPtr);
    if((ro = resolver_enter(libPtr)) == NULL) {
        if(h_errnop)
            *h_errnop = NO_RECOVERY;
    } else {
        r = ro->ro_gethostbyaddr_r(addr, len, type, result, buf, buflen, h_errnop, libPtr);
        resolver_leave();
    }
    ReleaseResolver(libPtr);
    return r;
    AROS_LIBFUNC_EXIT
}

#endif /* __CONFIG_ROADSHOW__ */

/* ---- getaddrinfo(), getnameinfo(): one of each for both front-ends ------- */

static LONG
resolver_getaddrinfo(const char *hostname, const char *servname,
                     const struct addrinfo *hints, struct addrinfo **result,
                     struct SocketBase *libPtr)
{
    const struct resolver_ops *ro;
    LONG r;

    ObtainResolver(libPtr);
    if((ro = resolver_enter(libPtr)) == NULL) {
        if(result != NULL)
            *result = NULL;
        r = EAI_FAIL;
    } else {
        r = ro->ro_getaddrinfo(hostname, servname, hints, result, libPtr);
        resolver_leave();
    }
    ReleaseResolver(libPtr);
    return r;
}

static LONG
resolver_getnameinfo(struct sockaddr *sa, LONG salen, char *hostname, LONG hostlen,
                     char *servicename, LONG servicelen, LONG flags,
                     struct SocketBase *libPtr)
{
    const struct resolver_ops *ro;
    LONG r;

    ObtainResolver(libPtr);
    if((ro = resolver_enter(libPtr)) == NULL)
        r = EAI_FAIL;
    else {
        r = ro->ro_getnameinfo(sa, salen, hostname, hostlen, servicename, servicelen,
                               flags, libPtr);
        resolver_leave();
    }
    ReleaseResolver(libPtr);
    return r;
}

/* A list from getaddrinfo(): exec memory, so the library need not be there. */
static void
resolver_freeaddrinfo(struct addrinfo *ai)
{
    struct addrinfo *next;

    for(; ai != NULL; ai = next) {
        next = ai->ai_next;
        if(ai->ai_canonname)
            FreeVec(ai->ai_canonname);
        FreeVec(ai);
    }
}

static const char *const resolver_ai_errors[] = {
    "No error",
    "Address family for host not supported",
    "Temporary failure in name resolution",
    "Invalid flags value",
    "Non-recoverable failure in name resolution",
    "Address family not supported",
    "Memory allocation failure",
    "No address associated with host",
    "Host nor service provided, or not known",
    "Service not supported for socket type",
    "Socket type not supported",
    "System error",
    "Unknown error"
};

static const char *
resolver_gai_strerror(LONG ecode)
{
    if(ecode < 0 || ecode > 11)
        return "Unknown error";
    return resolver_ai_errors[ecode];
}

#ifdef __CONFIG_ROADSHOW__

AROS_LH1(void, RS_freeaddrinfo,
         AROS_LHA(struct addrinfo *, ai, A0),
         struct SocketBase *, libPtr, 135, UL)
{
    AROS_LIBFUNC_INIT
    resolver_freeaddrinfo(ai);
    AROS_LIBFUNC_EXIT
}

AROS_LH4(LONG, RS_getaddrinfo,
         AROS_LHA(const char *, hostname, A0),
         AROS_LHA(const char *, servname, A1),
         AROS_LHA(const struct addrinfo *, hints, A2),
         AROS_LHA(struct addrinfo **, result, A3),
         struct SocketBase *, libPtr, 136, UL)
{
    AROS_LIBFUNC_INIT
    return resolver_getaddrinfo(hostname, servname, hints, result, libPtr);
    AROS_LIBFUNC_EXIT
}

AROS_LH1(STRPTR, RS_gai_strerror,
         AROS_LHA(LONG, ecode, D0),
         struct SocketBase *, libPtr, 137, UL)
{
    AROS_LIBFUNC_INIT
    return (STRPTR)resolver_gai_strerror(ecode);
    AROS_LIBFUNC_EXIT
}

AROS_LH7(LONG, RS_getnameinfo,
         AROS_LHA(struct sockaddr *, sa, A0),
         AROS_LHA(LONG, salen, D0),
         AROS_LHA(char *, hostname, A1),
         AROS_LHA(LONG, hostlen, D1),
         AROS_LHA(char *, servicename, A2),
         AROS_LHA(LONG, servicelen, D2),
         AROS_LHA(LONG, flags, D3),
         struct SocketBase *, libPtr, 138, UL)
{
    AROS_LIBFUNC_INIT
    return resolver_getnameinfo(sa, salen, hostname, hostlen, servicename, servicelen,
                                flags, libPtr);
    AROS_LIBFUNC_EXIT
}

#endif /* __CONFIG_ROADSHOW__ */

/* ---- miami.library ------------------------------------------------------- */

AROS_LH2(struct hostent *, gethostbyname2,
         AROS_LHA(const char *, name, A0),
         AROS_LHA(LONG, family, D0),
         struct MiamiBase *, MiamiBase, 41, Miami)
{
    AROS_LIBFUNC_INIT
    struct SocketBase *libPtr = MiamiBase->_SocketBase;
    struct hostent *r;

    ObtainResolver(libPtr);
    r = resolver_byname(name, (int)family, libPtr);
    ReleaseResolver(libPtr);
    return r;
    AROS_LIBFUNC_EXIT
}

AROS_LH1(char *, gai_strerror,
         AROS_LHA(LONG, error, D0),
         struct MiamiBase *, MiamiBase, 42, Miami)
{
    AROS_LIBFUNC_INIT
    return (char *)resolver_gai_strerror(error);
    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, freeaddrinfo,
         AROS_LHA(struct addrinfo *, addrinfo, A0),
         struct MiamiBase *, MiamiBase, 43, Miami)
{
    AROS_LIBFUNC_INIT
    resolver_freeaddrinfo(addrinfo);
    AROS_LIBFUNC_EXIT
}

AROS_LH4(LONG, getaddrinfo,
         AROS_LHA(char *, hostname, A0),
         AROS_LHA(char *, servname, A1),
         AROS_LHA(struct addrinfo *, hints, A2),
         AROS_LHA(struct addrinfo **, result, A3),
         struct MiamiBase *, MiamiBase, 44, Miami)
{
    AROS_LIBFUNC_INIT
    return resolver_getaddrinfo(hostname, servname, hints, result, MiamiBase->_SocketBase);
    AROS_LIBFUNC_EXIT
}

AROS_LH7(LONG, getnameinfo,
         AROS_LHA(struct sockaddr *, sa, A0),
         AROS_LHA(LONG, addrlen, D0),
         AROS_LHA(char *, hostname, A1),
         AROS_LHA(LONG, hostlen, D1),
         AROS_LHA(char *, servicename, A2),
         AROS_LHA(LONG, servicelen, D2),
         AROS_LHA(LONG, flags, D3),
         struct MiamiBase *, MiamiBase, 45, Miami)
{
    AROS_LIBFUNC_INIT
    return resolver_getnameinfo(sa, addrlen, hostname, hostlen, servicename, servicelen,
                                flags, MiamiBase->_SocketBase);
    AROS_LIBFUNC_EXIT
}

/* The resolver options are a member of the base: no resolver needed for them. */
AROS_LH0(LONG, MiamiGetResOptions,
         struct MiamiBase *, MiamiBase, 51, Miami)
{
    AROS_LIBFUNC_INIT
    return MiamiBase->_SocketBase->res_state.options;
    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, MiamiSetResOptions,
         AROS_LHA(LONG, options, D0),
         struct MiamiBase *, MiamiBase, 52, Miami)
{
    AROS_LIBFUNC_INIT
    MiamiBase->_SocketBase->res_state.options = options;
    AROS_LIBFUNC_EXIT
}

/* ---- what the rest of the core calls ------------------------------------- */

/*
 * At a base's close (api/amiga_api.c): its resolver state, which the
 * resolver allocated from the core's pool. Here and not in the library, so
 * that it is freed also after the library was unloaded; the library calls
 * it too, through the core's table, when it reads the configuration again.
 */
void
res_cleanup_db(struct state *state)
{
    if(state->dnsrch) {
        char **domain;

        for(domain = state->dnsrch; *domain; domain++)
            bsd_free(*domain, NULL);
        bsd_free(state->dnsrch, NULL);
        state->dnsrch = NULL;
    }
    if(state->nsaddr_list) {
        bsd_free(state->nsaddr_list, NULL);
        state->nsaddr_list = NULL;
    }
    state->options = 0;
}

/*
 * At start (api/amiga_api.c): nothing. The cache is the module's; it is
 * set up when the module starts (dns_cache_start()).
 */
void
dns_cache_init(void)
{
}
