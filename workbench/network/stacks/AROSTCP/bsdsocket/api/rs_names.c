/*
 * Copyright (C) 2005-2026, The AROS Dev Team
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

/*
 * getaddrinfo() and getnameinfo() of the resolver module
 * (api/resolver_ops.h): ONE implementation of each, for both front-ends -
 * bsdsocket.library's (vectors 136, 138) and miami.library's (44, 45),
 * whose vectors are stubs in the core (api/resolver_stubs.c) that call
 * these with the base's resolver lock held.
 *
 * The two front-ends had a body each, the same line for line (one from
 * api/amiga_roadshow.c, one from api/miami_api.c) but for three things,
 * and this one does what the first did in each:
 *  - miami.library's took no resolver lock: the lock of the base the
 *    look-up writes its hostent into is now taken for both;
 *  - miami.library's looked services up with getservbyname() and
 *    getservbyport() through the library base, which checks the calling
 *    task, copies the entry into the base's buffer and sets errno to 0 when
 *    there is none; this one reads the network database itself, as the
 *    other did;
 *  - a redundant ai_next = NULL on memory that was cleared.
 * The services are now read under the database's lock to the end (port,
 * or name copied), where both read the entry after giving the lock back.
 */

#include <conf.h>

#include <exec/lists.h>
#include <proto/exec.h>
#include <sys/param.h>
#include <sys/systm.h>
#include <sys/types.h>
#include <sys/malloc.h>
#include <sys/socket.h>
#include <sys/errno.h>
#include <netinet/in.h>
#include <net/if.h>
#include <net/netmodule.h>      /* netfamily_running() */
#include <kern/amiga_includes.h>
#include <api/amiga_api.h>
#include <kern/amiga_netdb.h>
#include <api/resolver_ops.h>
#include <net/netcore.h>        /* __inet_ntop(), __inet_pton(), findServentNode() */

#include <string.h>
#include <netdb.h>

#include <net/netcore_map.h>    /* always the last include */

/*
 * A port number as strtol(s, &end, 10) would read it - leading white
 * space, a sign, digits, and nothing after them: 1 and the number in *val
 * (65536 for any larger one); 0 when it is not one. (strtol() itself would
 * bring the C library's character table into the module.)
 */
static int
res_portnumber(const char *s, long *val)
{
    long v = 0;
    int neg = 0, digits = 0;

    while(*s == ' ' || (*s >= '\t' && *s <= '\r'))
        s++;
    if(*s == '+' || *s == '-')
        neg = *s++ == '-';
    for(; *s >= '0' && *s <= '9'; s++, digits++)
        if(v <= 65535)
            v = v * 10 + (*s - '0');
    if(digits == 0 || *s != '\0')
        return 0;
    *val = neg ? -v : v;
    return 1;
}

/* the port (network order) of a service by name in the database: 1, or 0 */
static int
res_servport(const char *name, const char *proto, int *port)
{
    struct ServentNode *entNode;
    int found = 0;

    LOCK_R_NDB(NDB);
    if((entNode = findServentNode(NDB, name, proto)) != NULL) {
        *port = entNode->sn_Ent.s_port;
        found = 1;
    }
    UNLOCK_NDB(NDB);
    return found;
}

/*
 * The name of a service by port (network order) into buf: 1 copied, 0 none
 * of that port, -1 it does not fit.
 */
static int
res_servname(int port, const char *proto, char *buf, LONG len)
{
    struct ServentNode *entNode;
    int found = 0;

    LOCK_R_NDB(NDB);
    for(entNode = (struct ServentNode *)NDB->ndb_Services.mlh_Head;
            entNode->sn_Node.mln_Succ;
            entNode = (struct ServentNode *)entNode->sn_Node.mln_Succ)
        if(entNode->sn_Ent.s_port == port &&
                (proto == NULL || strcmp(entNode->sn_Ent.s_proto, proto) == 0)) {
            if((LONG)strnlen(entNode->sn_Ent.s_name, len) >= len)
                found = -1;
            else {
                strcpy(buf, entNode->sn_Ent.s_name);
                found = 1;
            }
            break;
        }
    UNLOCK_NDB(NDB);
    return found;
}

static void
res_freelist(struct addrinfo *ai)
{
    struct addrinfo *next;

    for(; ai != NULL; ai = next) {
        next = ai->ai_next;
        if(ai->ai_canonname)
            FreeVec(ai->ai_canonname);
        FreeVec(ai);
    }
}

/*
 * The list is exec memory (AllocVec()): freeaddrinfo() is the core's and
 * needs nothing of this module, which may be unloaded before the list is
 * given back.
 */
LONG
res_getaddrinfo(const char *hostname, const char *servname,
                const struct addrinfo *hints, struct addrinfo **result,
                struct SocketBase *libPtr)
{
    struct addrinfo sentinel;
    struct addrinfo *cur;
    int family = PF_UNSPEC;
    int socktype = 0;
    int protocol = 0;
    int flags = 0;
    int port = 0;
    int error;

    *result = NULL;
    memset(&sentinel, 0, sizeof(sentinel));
    cur = &sentinel;

    if (hostname == NULL && servname == NULL)
        return EAI_NONAME;

    if (hints) {
        if (hints->ai_addrlen || hints->ai_canonname ||
            hints->ai_addr || hints->ai_next)
            return EAI_BADFLAGS;
        family = hints->ai_family;
        socktype = hints->ai_socktype;
        protocol = hints->ai_protocol;
        flags = hints->ai_flags;
        switch (family) {
        case PF_UNSPEC: case PF_INET: case PF_INET6: break;
        default: return EAI_FAMILY;
        }
        switch (socktype) {
        case 0: case SOCK_STREAM: case SOCK_DGRAM: case SOCK_RAW: break;
        default: return EAI_SOCKTYPE;
        }
    }

    /*
     * Only the families that are part of the running stack are offered:
     * an application must not be handed an address it cannot make a
     * socket for. Asked for one that does not run, the call fails
     * (EAI_FAMILY); asked for any, it gives those of the running ones.
     * A numeric address of a family that does not run is an error too
     * (EAI_ADDRFAMILY), not a name to look up. On a stack with both
     * families nothing here changes anything.
     */
    int run4 = netfamily_running(AF_INET);
    int run6 = netfamily_running(AF_INET6);

    if ((family == PF_INET && !run4) || (family == PF_INET6 && !run6) ||
        (!run4 && !run6))
        return EAI_FAMILY;
    if (hostname != NULL && (!run4 || !run6)) {
        struct in6_addr lit;    /* room for an address of either family */

        if (__inet_pton(run4 ? AF_INET6 : AF_INET, hostname, &lit, libPtr) == 1)
            return EAI_ADDRFAMILY;
    }

    if (servname != NULL && servname[0] != '\0') {
        long pval;
        const char *proto_name = NULL;
        if (socktype == SOCK_STREAM) proto_name = "tcp";
        else if (socktype == SOCK_DGRAM) proto_name = "udp";
        if (res_portnumber(servname, &pval) && pval >= 0 && pval <= 65535) {
            port = htons((unsigned short)pval);
        } else {
            if (flags & AI_NUMERICSERV)
                return EAI_NONAME;
            /* by the socket type's protocol; for any type, TCP's then UDP's */
            if (!res_servport(servname, proto_name, &port) &&
                (proto_name != NULL ||
                 (!res_servport(servname, "tcp", &port) &&
                  !res_servport(servname, "udp", &port))))
                return EAI_SERVICE;
        }
    }

    int stypes[3];
    int nstypes = 0;
    if (socktype != 0) { stypes[0] = socktype; nstypes = 1; }
    else { stypes[0] = SOCK_STREAM; stypes[1] = SOCK_DGRAM; nstypes = 2; }

    int families[2];
    int nfamilies = 0;
    if (family == PF_INET || family == PF_INET6) {
        families[0] = family; nfamilies = 1;
    } else {
        /* any family: those the stack runs, IPv6 first */
        if (run6) families[nfamilies++] = PF_INET6;
        if (run4) families[nfamilies++] = PF_INET;
    }

    for (int fi = 0; fi < nfamilies; fi++) {
        int af = families[fi];
        struct sockaddr_storage addrs[16];
        char *canonname = NULL;
        int naddrs = 0;

        if (hostname == NULL) {
            if (af == PF_INET) {
                struct sockaddr_in *sin = (struct sockaddr_in *)&addrs[0];
                memset(sin, 0, sizeof(*sin));
                sin->sin_len = sizeof(*sin);
                sin->sin_family = AF_INET;
                sin->sin_addr.s_addr = (flags & AI_PASSIVE) ? htonl(INADDR_ANY) : htonl(INADDR_LOOPBACK);
                naddrs = 1;
            } else {
                struct sockaddr_in6 *sin6 = (struct sockaddr_in6 *)&addrs[0];
                memset(sin6, 0, sizeof(*sin6));
                sin6->sin6_len = sizeof(*sin6);
                sin6->sin6_family = AF_INET6;
                if (!(flags & AI_PASSIVE))
                    sin6->sin6_addr.s6_addr[15] = 1;
                naddrs = 1;
            }
        } else if (flags & AI_NUMERICHOST) {
            if (af == PF_INET) {
                struct sockaddr_in *sin = (struct sockaddr_in *)&addrs[0];
                memset(sin, 0, sizeof(*sin));
                sin->sin_len = sizeof(*sin);
                sin->sin_family = AF_INET;
                if (__inet_pton(AF_INET, hostname, &sin->sin_addr, libPtr) == 1)
                    naddrs = 1;
            } else {
                struct sockaddr_in6 *sin6 = (struct sockaddr_in6 *)&addrs[0];
                memset(sin6, 0, sizeof(*sin6));
                sin6->sin6_len = sizeof(*sin6);
                sin6->sin6_family = AF_INET6;
                if (__inet_pton(AF_INET6, hostname, &sin6->sin6_addr, libPtr) == 1)
                    naddrs = 1;
            }
        } else {
            int numeric_ok = 0;
            if (af == PF_INET) {
                struct sockaddr_in *sin = (struct sockaddr_in *)&addrs[0];
                memset(sin, 0, sizeof(*sin));
                sin->sin_len = sizeof(*sin);
                sin->sin_family = AF_INET;
                if (__inet_pton(AF_INET, hostname, &sin->sin_addr, libPtr) == 1) {
                    naddrs = 1; numeric_ok = 1;
                }
            } else {
                struct sockaddr_in6 *sin6 = (struct sockaddr_in6 *)&addrs[0];
                memset(sin6, 0, sizeof(*sin6));
                sin6->sin6_len = sizeof(*sin6);
                sin6->sin6_family = AF_INET6;
                if (__inet_pton(AF_INET6, hostname, &sin6->sin6_addr, libPtr) == 1) {
                    naddrs = 1; numeric_ok = 1;
                }
            }
            if (!numeric_ok) {
                struct hostent *hp;
                if (af == PF_INET)
                    hp = res_gethostbyname(hostname, libPtr);
                else
                    hp = res_gethostbyname2(hostname, AF_INET6, libPtr);
                if (hp != NULL && hp->h_addr_list != NULL) {
                    if (flags & AI_CANONNAME) canonname = hp->h_name;
                    for (int i = 0; hp->h_addr_list[i] && naddrs < 16; i++) {
                        if (af == PF_INET) {
                            struct sockaddr_in *sin = (struct sockaddr_in *)&addrs[naddrs];
                            memset(sin, 0, sizeof(*sin));
                            sin->sin_len = sizeof(*sin);
                            sin->sin_family = AF_INET;
                            memcpy(&sin->sin_addr, hp->h_addr_list[i], hp->h_length);
                        } else {
                            struct sockaddr_in6 *sin6 = (struct sockaddr_in6 *)&addrs[naddrs];
                            memset(sin6, 0, sizeof(*sin6));
                            sin6->sin6_len = sizeof(*sin6);
                            sin6->sin6_family = AF_INET6;
                            memcpy(&sin6->sin6_addr, hp->h_addr_list[i], hp->h_length);
                        }
                        naddrs++;
                    }
                }
            }
        }

        if (naddrs == 0) continue;

        for (int ai = 0; ai < naddrs; ai++) {
            struct sockaddr *sa = (struct sockaddr *)&addrs[ai];
            int addrlen = (af == PF_INET)
                          ? sizeof(struct sockaddr_in) : sizeof(struct sockaddr_in6);
            if (af == PF_INET)
                ((struct sockaddr_in *)sa)->sin_port = port;
            else
                ((struct sockaddr_in6 *)sa)->sin6_port = port;

            for (int si = 0; si < nstypes; si++) {
                struct addrinfo *ai_new;
                int proto;
                if (stypes[si] == SOCK_STREAM) proto = IPPROTO_TCP;
                else if (stypes[si] == SOCK_DGRAM) proto = IPPROTO_UDP;
                else proto = protocol;

                ai_new = (struct addrinfo *)AllocVec(
                             sizeof(struct addrinfo) + addrlen,
                             MEMF_PUBLIC | MEMF_CLEAR);
                if (ai_new == NULL) { error = EAI_MEMORY; goto fail; }

                ai_new->ai_flags = flags;
                ai_new->ai_family = af;
                ai_new->ai_socktype = stypes[si];
                ai_new->ai_protocol = proto;
                ai_new->ai_addrlen = addrlen;
                ai_new->ai_addr = (struct sockaddr *)(ai_new + 1);
                memcpy(ai_new->ai_addr, sa, addrlen);

                if (sentinel.ai_next == NULL && canonname != NULL) {
                    int clen = strnlen(canonname, MAXHOSTNAMELEN) + 1;
                    ai_new->ai_canonname = (char *)AllocVec(clen, MEMF_PUBLIC);
                    if (ai_new->ai_canonname)
                        memcpy(ai_new->ai_canonname, canonname, clen);
                } else if (sentinel.ai_next == NULL && (flags & AI_CANONNAME) && hostname != NULL) {
                    int clen = strnlen(hostname, MAXHOSTNAMELEN) + 1;
                    ai_new->ai_canonname = (char *)AllocVec(clen, MEMF_PUBLIC);
                    if (ai_new->ai_canonname)
                        memcpy(ai_new->ai_canonname, hostname, clen);
                }

                cur->ai_next = ai_new;
                cur = ai_new;
            }
        }
    }

    if (sentinel.ai_next == NULL)
        return EAI_NONAME;

    *result = sentinel.ai_next;
    return 0;

fail:
    res_freelist(sentinel.ai_next);
    return error;
}

LONG
res_getnameinfo(struct sockaddr *sa, LONG salen, char *hostname, LONG hostlen,
                char *servicename, LONG servicelen, LONG flags,
                struct SocketBase *libPtr)
{
    if (sa == NULL)
        return EAI_FAIL;

    if (hostname != NULL && hostlen > 0) {
        if (flags & NI_NUMERICHOST) {
            const char *p = NULL;
            if (sa->sa_family == AF_INET) {
                struct sockaddr_in *sin = (struct sockaddr_in *)sa;
                p = __inet_ntop(AF_INET, &sin->sin_addr,
                                hostname, hostlen, libPtr);
            } else if (sa->sa_family == AF_INET6) {
                struct sockaddr_in6 *sin6 = (struct sockaddr_in6 *)sa;
                p = __inet_ntop(AF_INET6, &sin6->sin6_addr,
                                hostname, hostlen, libPtr);
            } else {
                return EAI_FAMILY;
            }
            if (p == NULL)
                return EAI_FAIL;
        } else {
            struct hostent *hp = NULL;

            if (sa->sa_family == AF_INET) {
                struct sockaddr_in *sin = (struct sockaddr_in *)sa;
                hp = res_gethostbyaddr((UBYTE *)&sin->sin_addr,
                                       sizeof(sin->sin_addr),
                                       AF_INET, libPtr);
            } else if (sa->sa_family == AF_INET6) {
                struct sockaddr_in6 *sin6 = (struct sockaddr_in6 *)sa;
                hp = res_gethostbyaddr((UBYTE *)&sin6->sin6_addr,
                                       sizeof(sin6->sin6_addr),
                                       AF_INET6, libPtr);
            } else {
                return EAI_FAMILY;
            }

            if (hp != NULL && hp->h_name != NULL) {
                char *name = hp->h_name;
                if (flags & NI_NOFQDN) {
                    char *dot = strchr(name, '.');
                    if (dot) {
                        int dlen = dot - name;
                        if (dlen >= hostlen)
                            return EAI_FAIL;
                        memcpy(hostname, name, dlen);
                        hostname[dlen] = '\0';
                        goto host_done;
                    }
                }
                if ((int)strnlen(name, hostlen) >= hostlen)
                    return EAI_FAIL;
                strcpy(hostname, name);
            } else {
                const char *numaddr = NULL;
                if (flags & NI_NAMEREQD)
                    return EAI_NONAME;
                if (sa->sa_family == AF_INET) {
                    struct sockaddr_in *sin = (struct sockaddr_in *)sa;
                    numaddr = __inet_ntop(AF_INET, &sin->sin_addr,
                                          hostname, hostlen, libPtr);
                } else {
                    struct sockaddr_in6 *sin6 = (struct sockaddr_in6 *)sa;
                    numaddr = __inet_ntop(AF_INET6, &sin6->sin6_addr,
                                          hostname, hostlen, libPtr);
                }
                if (numaddr == NULL)
                    return EAI_FAIL;
            }
        }
    }
host_done:

    if (servicename != NULL && servicelen > 0) {
        struct CSource cs;
        int port;
        if (sa->sa_family == AF_INET)
            port = ((struct sockaddr_in *)sa)->sin_port;
        else if (sa->sa_family == AF_INET6)
            port = ((struct sockaddr_in6 *)sa)->sin6_port;
        else
            return EAI_FAMILY;

        if (!(flags & NI_NUMERICSERV)) {
            switch (res_servname(port, (flags & NI_DGRAM) ? "udp" : "tcp",
                                 servicename, servicelen)) {
            case 1:
                return 0;
            case -1:
                return EAI_FAIL;
            }
        }
        /* the number, cut to the buffer as snprintf() would */
        cs.CS_Buffer = servicename;
        cs.CS_Length = servicelen;
        cs.CS_CurChr = 0;
        (void)csprintf(&cs, "%lu", (unsigned long)ntohs(port));
    }

    return 0;
}
