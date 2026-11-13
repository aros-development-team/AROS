/*
 * Copyright (C) 2005-2026, The AROS Development Team.
 *
 * This file contains Roadshow-specific functions which are not required for MorphOS build
 */

#include <conf.h>

#ifdef __CONFIG_ROADSHOW__

#include <aros/libcall.h>
#include <exec/lists.h>
#include <proto/exec.h>
#include <libraries/bsdsocket.h>
#include <utility/tagitem.h>
#include <proto/utility.h>
#include <sys/types.h>
#include <sys/malloc.h>
#include <sys/mbuf.h>
#include <sys/socket.h>
#include <sys/synch.h>
#include <netinet/in.h>
#include <net/route.h>
#include <net/if.h>
#include <net/radix.h>
#include "../net/bpf.h"
#include <net/if_sana.h>
#include <net/netmodule.h>
#if INET6
#include <netinet6/nd6.h>	/* struct llinfo_nd6 (ND6 neighbour expiry) */
#endif
#include <sys/ioctl.h>
#include <protos/net/route_protos.h>
#include <syslog.h>
#include <api/amiga_api.h>
#include <api/apicalls.h>
#include <kern/amiga_netdb.h>

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include <netdb.h>

#include <api/gethtbynamadr.h>

#include <sys/errno.h>

/* BPF kernel functions (from net/bpf.c) */
extern int  bpf_allocate(void);
extern struct bpf_d *bpf_lookup(int handle);
extern void bpf_freed(int handle);
extern void bpf_rotate(struct bpf_d *d);
extern int  bpf_setif(struct bpf_d *d, const char *ifname);
extern int  bpf_setf(struct bpf_d *d, struct bpf_program *fp);

extern struct ifnet *ifnet;

extern struct hostent *__gethostbyname(const char *name, struct SocketBase *);
extern struct hostent *__gethostbyname2(const char *name, int af, struct SocketBase *);
extern struct hostent *__gethostbyname_r(const char *name,
    struct hostent *result, char *buf, LONG buflen, LONG *h_errnop,
    struct SocketBase *libPtr);
extern struct hostent *__gethostbyaddr_r(const char *addr, LONG len, LONG type,
    struct hostent *result, char *buf, LONG buflen, LONG *h_errnop,
    struct SocketBase *libPtr);

/* Internal implementation functions */
extern const char *__inet_ntop(int af, const void *src, char *dst,
                               socklen_t size, struct SocketBase *SocketBase);
extern int __inet_pton(int af, const char *src, void *dst,
                       struct SocketBase *SocketBase);

/*
 * In_LocalAddr() and In_CanForward() are questions about an IPv4 address:
 * IPv4 answers them (net/netmodule.h), and when it is not part of the
 * running stack the answer is no.
 */
static LONG
in_addr_ask(int what, ULONG address)
{
    struct sockaddr_in sin;

    bzero((caddr_t)&sin, sizeof(sin));
    sin.sin_len = sizeof(sin);
    sin.sin_family = AF_INET;
    sin.sin_addr.s_addr = address;
    return (LONG)(netfamily_addrquery(AF_INET, what, (struct sockaddr *)&sin, NULL) > 0);
}

extern struct ServentNode *findServentNode(struct NetDataBase *ndb,
                                           const char *name, const char *proto);


// setnetent/endnetent/getnetent defined in amiga_ndbent.c
// setprotoent defined in amiga_ndbent.c
// endprotoent defined in amiga_ndbent.c
// getprotoent defined in amiga_ndbent.c
// setservent/endservent/getservent defined in amiga_ndbent.c

// inet_aton defined in amiga_libcalls.c

/* ------------------------------------------------------------------ */
/* Slot 100: inet_ntop                                                */
/* ------------------------------------------------------------------ */
AROS_LH4(STRPTR, RS_inet_ntop,
         AROS_LHA(LONG, af, D0),
         AROS_LHA(void *, src, A0),
         AROS_LHA(char *, dst, A1),
         AROS_LHA(LONG, size, D1),
         struct SocketBase *, libPtr, 100, UL)
{
    AROS_LIBFUNC_INIT
    return (STRPTR)__inet_ntop((int)af, src, dst, (socklen_t)size, libPtr);
    AROS_LIBFUNC_EXIT
}

/* ------------------------------------------------------------------ */
/* Slot 101: inet_pton                                                */
/* ------------------------------------------------------------------ */
AROS_LH3(LONG, RS_inet_pton,
         AROS_LHA(LONG, af, D0),
         AROS_LHA(const char *, src, A0),
         AROS_LHA(void *, dst, A1),
         struct SocketBase *, libPtr, 101, UL)
{
    AROS_LIBFUNC_INIT
    return (LONG)__inet_pton((int)af, src, dst, libPtr);
    AROS_LIBFUNC_EXIT
}

/* ------------------------------------------------------------------ */
/* Slot 102: In_LocalAddr                                             */
/* ------------------------------------------------------------------ */
AROS_LH1(LONG, In_LocalAddr,
         AROS_LHA(ULONG, address, D0),
         struct SocketBase *, libPtr, 102, UL)
{
    AROS_LIBFUNC_INIT
    return in_addr_ask(NFQ_LOCAL, address);
    AROS_LIBFUNC_EXIT
}

/* ------------------------------------------------------------------ */
/* Slot 103: In_CanForward                                            */
/* ------------------------------------------------------------------ */
AROS_LH1(LONG, In_CanForward,
         AROS_LHA(ULONG, address, D0),
         struct SocketBase *, libPtr, 103, UL)
{
    AROS_LIBFUNC_INIT
    return in_addr_ask(NFQ_FORWARDABLE, address);
    AROS_LIBFUNC_EXIT
}

/* ------------------------------------------------------------------ */
/* Slots 104-114: mbuf kernel memory API                              */
/* ------------------------------------------------------------------ */

AROS_LH3(struct mbuf *, mbuf_copym,
         AROS_LHA(struct mbuf *, buffer, A0),
         AROS_LHA(LONG, offset, D0),
         AROS_LHA(LONG, length, D1),
         struct SocketBase *, libPtr, 104, UL)
{
    AROS_LIBFUNC_INIT
    return m_copym(buffer, (int)offset, (int)length, M_DONTWAIT);
    AROS_LIBFUNC_EXIT
}

AROS_LH4(LONG, mbuf_copydata,
         AROS_LHA(struct mbuf *, buffer, A0),
         AROS_LHA(LONG, offset, D0),
         AROS_LHA(LONG, length, D1),
         AROS_LHA(void *, data, A1),
         struct SocketBase *, libPtr, 105, UL)
{
    AROS_LIBFUNC_INIT
    m_copydata(buffer, (int)offset, (int)length, (caddr_t)data);
    return 0;
    AROS_LIBFUNC_EXIT
}

AROS_LH4(LONG, mbuf_copyback,
         AROS_LHA(struct mbuf *, buffer, A0),
         AROS_LHA(LONG, offset, D0),
         AROS_LHA(LONG, length, D1),
         AROS_LHA(void *, data, A1),
         struct SocketBase *, libPtr, 106, UL)
{
    AROS_LIBFUNC_INIT
    m_copyback(buffer, (int)offset, (int)length, (caddr_t)data);
    return 0;
    AROS_LIBFUNC_EXIT
}

AROS_LH1(struct mbuf *, mbuf_free,
         AROS_LHA(struct mbuf *, buffer, A0),
         struct SocketBase *, libPtr, 107, UL)
{
    AROS_LIBFUNC_INIT
    return m_free(buffer);
    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, mbuf_freem,
         AROS_LHA(struct mbuf *, chain, A0),
         struct SocketBase *, libPtr, 108, UL)
{
    AROS_LIBFUNC_INIT
    m_freem(chain);
    AROS_LIBFUNC_EXIT
}

AROS_LH0(struct mbuf *, mbuf_get,
         struct SocketBase *, libPtr, 109, UL)
{
    AROS_LIBFUNC_INIT
    return m_get(M_DONTWAIT, MT_DATA);
    AROS_LIBFUNC_EXIT
}

AROS_LH0(struct mbuf *, mbuf_gethdr,
         struct SocketBase *, libPtr, 110, UL)
{
    AROS_LIBFUNC_INIT
    return m_gethdr(M_DONTWAIT, MT_DATA);
    AROS_LIBFUNC_EXIT
}

AROS_LH2(struct mbuf *, mbuf_prepend,
         AROS_LHA(struct mbuf *, buffer, A0),
         AROS_LHA(LONG, length, D0),
         struct SocketBase *, libPtr, 111, UL)
{
    AROS_LIBFUNC_INIT
    return m_prepend(buffer, (int)length, M_DONTWAIT);
    AROS_LIBFUNC_EXIT
}

AROS_LH2(LONG, mbuf_adj,
         AROS_LHA(struct mbuf *, buffer, A0),
         AROS_LHA(LONG, length, D0),
         struct SocketBase *, libPtr, 112, UL)
{
    AROS_LIBFUNC_INIT
    m_adj(buffer, (int)length);
    return 0;
    AROS_LIBFUNC_EXIT
}

AROS_LH2(LONG, mbuf_cat,
         AROS_LHA(struct mbuf *, first_chain, A0),
         AROS_LHA(struct mbuf *, second_chain, A1),
         struct SocketBase *, libPtr, 113, UL)
{
    AROS_LIBFUNC_INIT
    m_cat(first_chain, second_chain);
    return 0;
    AROS_LIBFUNC_EXIT
}

AROS_LH2(struct mbuf *, mbuf_pullup,
         AROS_LHA(struct mbuf *, buffer, A0),
         AROS_LHA(LONG, length, D0),
         struct SocketBase *, libPtr, 114, UL)
{
    AROS_LIBFUNC_INIT
    return m_pullup(buffer, (int)length);
    AROS_LIBFUNC_EXIT
}

/* ------------------------------------------------------------------ */
/* Slot 115: ProcessIsServer                                          */
/* ------------------------------------------------------------------ */
AROS_LH1(LONG, ProcessIsServer,
         AROS_LHA(struct Process *, process, A0),
         struct SocketBase *, libPtr, 115, UL)
{
    AROS_LIBFUNC_INIT
    /* Not implemented -- always returns FALSE */
    return 0;
    AROS_LIBFUNC_EXIT
}

/* ------------------------------------------------------------------ */
/* Slot 116: ObtainServerSocket                                       */
/* ------------------------------------------------------------------ */
AROS_LH0(LONG, ObtainServerSocket,
         struct SocketBase *, libPtr, 116, UL)
{
    AROS_LIBFUNC_INIT
    writeErrnoValue(libPtr, ENOSYS);
    return -1;
    AROS_LIBFUNC_EXIT
}


/* Slots 125-127: Reserved (Null entries in function table) */

/* ------------------------------------------------------------------ */
/* Slots 128-134: IPF (IP Filter) API                                 */
/* Intentionally undocumented per Roadshow spec.                      */
/* ------------------------------------------------------------------ */

AROS_LH3(LONG, ipf_open,
         AROS_LHA(LONG, channel, D0),
         AROS_LHA(STRPTR, name, A0),
         AROS_LHA(LONG, flags, D1),
         struct SocketBase *, libPtr, 128, UL)
{
    AROS_LIBFUNC_INIT
    writeErrnoValue(libPtr, ENOSYS);
    return -1;
    AROS_LIBFUNC_EXIT
}

AROS_LH1(LONG, ipf_close,
         AROS_LHA(LONG, handle, D0),
         struct SocketBase *, libPtr, 129, UL)
{
    AROS_LIBFUNC_INIT
    writeErrnoValue(libPtr, ENOSYS);
    return -1;
    AROS_LIBFUNC_EXIT
}

AROS_LH3(LONG, ipf_read,
         AROS_LHA(LONG, handle, D0),
         AROS_LHA(void *, buffer, A0),
         AROS_LHA(LONG, len, D1),
         struct SocketBase *, libPtr, 130, UL)
{
    AROS_LIBFUNC_INIT
    writeErrnoValue(libPtr, ENOSYS);
    return -1;
    AROS_LIBFUNC_EXIT
}

AROS_LH3(LONG, ipf_write,
         AROS_LHA(LONG, handle, D0),
         AROS_LHA(const void *, buffer, A0),
         AROS_LHA(LONG, len, D1),
         struct SocketBase *, libPtr, 131, UL)
{
    AROS_LIBFUNC_INIT
    writeErrnoValue(libPtr, ENOSYS);
    return -1;
    AROS_LIBFUNC_EXIT
}

AROS_LH3(LONG, ipf_ioctl,
         AROS_LHA(LONG, handle, D0),
         AROS_LHA(ULONG, request, D1),
         AROS_LHA(char *, argp, A0),
         struct SocketBase *, libPtr, 132, UL)
{
    AROS_LIBFUNC_INIT
    writeErrnoValue(libPtr, ENOSYS);
    return -1;
    AROS_LIBFUNC_EXIT
}

AROS_LH2(LONG, ipf_set_notify_mask,
         AROS_LHA(LONG, handle, D0),
         AROS_LHA(ULONG, mask, D1),
         struct SocketBase *, libPtr, 133, UL)
{
    AROS_LIBFUNC_INIT
    writeErrnoValue(libPtr, ENOSYS);
    return -1;
    AROS_LIBFUNC_EXIT
}

AROS_LH2(LONG, ipf_set_interrupt_mask,
         AROS_LHA(LONG, handle, D0),
         AROS_LHA(ULONG, mask, D1),
         struct SocketBase *, libPtr, 134, UL)
{
    AROS_LIBFUNC_INIT
    writeErrnoValue(libPtr, ENOSYS);
    return -1;
    AROS_LIBFUNC_EXIT
}


#endif
