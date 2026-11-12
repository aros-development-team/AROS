/*
 * Copyright (C) 2026 The AROS Dev Team
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef NET_NETCORE_H
#define NET_NETCORE_H

/*
 * The core's services as a module in a library file of its own sees them.
 *
 * The stack publishes one library, arostcp.library (MakeLibrary() and
 * AddLibrary(), as it publishes bsdsocket.library). A module opens it and
 * asks, through a library vector, for `struct netcore': a table with one
 * typed pointer for every function and every variable of the core a module
 * may use. The module keeps a copy of the table in its own data, so a call
 * into the core is one load and one indirect call.
 *
 * The table holds addresses only. The functions and the variables exist
 * once, in the stack; nothing of the core's state is copied.
 *
 * WORKING NAMES: "arostcp.library" (below) is the name of the design
 * (LIBRARY-MECHANISM.md). The names of the module files and the directory
 * they are installed in are in net/netmodule.h (NETMODULE_KIND_PROT,
 * NETMODULE_KIND_ATCP, NETMODULE_LIBDIR) and, for the build, in stack.cfg.
 */

#include <exec/types.h>
#include <exec/libraries.h>
#include <aros/libcall.h>

#define NETCORE_LIBNAME     "arostcp.library"
#define NETCORE_LIBVERSION  1       /* of the library: its vectors */
#define NETCORE_LIBREVISION 0
#define NETCORE_VERSION     10      /* of the table: raised with every line added to netcore.def */

/*
 * Every declaration named in netcore.def must be visible here, so that the
 * table's members get their types from the real prototypes: a wrong
 * argument in a module is the same compile error it is in the core.
 *
 * Only headers that are safe in any file of the stack: no header of a
 * protocol (the core names none), and nothing that takes a name away from
 * the file that includes this one (see `log' below).
 */
#include <conf.h>
#include <sys/param.h>
#include <sys/systm.h>
#include <sys/malloc.h>
#include <sys/mbuf.h>
#include <sys/socket.h>
#include <sys/socketvar.h>
#include <sys/protosw.h>
#include <sys/domain.h>
#include <sys/errno.h>
#include <sys/kernel.h>
#include <sys/synch.h>
#include <sys/time.h>
#include <sys/sysctl.h>
#include <stdarg.h>
#include <net/if.h>
#include <net/if_types.h>
#include <net/route.h>
#include <net/raw_cb.h>
#include <net/netisr.h>
#include <net/pfil.h>
#include <net/netmodule.h>
#include <net/if_sana.h>
#include <kern/amiga_time.h>
#include <kern/amiga_subr.h>
#include <kern/amiga_dhcp.h>
#include <kern/accesscontrol.h>
#include <protos/kern/amiga_main_protos.h>
#include <protos/kern/kern_malloc_protos.h>
#include <protos/kern/kern_random_protos.h>
#include <protos/kern/kern_subr_protos.h>
#include <protos/kern/uipc_domain_protos.h>
#include <protos/kern/uipc_socket2_protos.h>
#include <protos/kern/uipc_socket_protos.h>
#include <protos/net/if_loop_protos.h>
#include <protos/net/if_protos.h>
#include <protos/net/radix_protos.h>
#include <protos/net/raw_cb_protos.h>
#include <protos/net/raw_usrreq_protos.h>
#include <protos/net/route_protos.h>
#include <protos/net/rtsock_protos.h>
/*
 * Since version 6, for the optional parts that become modules (Stage 7:
 * the resolver, administration, the packet services): the socket calls a
 * module makes itself, the error values of the caller's library base, the
 * network database, logging's port, the configuration's reply buffers.
 */
#include <exec/semaphores.h>
#include <api/amiga_api.h>
#include <kern/amiga_config.h>
#include <kern/amiga_log.h>
#include <kern/amiga_netdb.h>
#include <kern/admin_ops.h>
#include <api/resolver_ops.h>
#include <net/packet_ops.h>
#include <kern/netconfig_ops.h>

/*
 * Names of the core in the table that no header declares in a way every
 * file of the stack can include: declared here, as the files that define
 * them have them. (api/apicalls.h has a type of its own that other files
 * define differently; api/allocdatabuffer.h an inline function that names
 * the core.)
 */
struct SocketBase;
struct CSource;
struct DataBuffer;
extern struct Task *AROSTCP_Task;                           /* kern/amiga_main.c */
LONG    __CloseSocket(LONG, struct SocketBase *);           /* api/amiga_generic.c */
LONG    __connect(LONG, const struct sockaddr *, LONG, struct SocketBase *);
LONG    __recv(LONG, char *, LONG, LONG, struct SocketBase *);
LONG    __send(LONG, const char *, LONG, LONG, struct SocketBase *);
LONG    __socket(LONG, LONG, LONG, struct SocketBase *);
LONG    __WaitSelect(ULONG, fd_set *, fd_set *, fd_set *, struct timeval *, ULONG *,
                     struct SocketBase *);
LONG    __gethostname(STRPTR, LONG, struct SocketBase *);   /* api/amiga_hostname.c */
LONG    __inet_aton(CONST_STRPTR cp, struct in_addr *addr); /* api/amiga_libcalls.c */
BOOL    doAllocDataBuffer(struct DataBuffer *DB, int size); /* api/allocdatabuffer.c */
const char *__inet_ntop(int af, const void *src, char *dst, socklen_t size,
                        struct SocketBase *SocketBase);     /* api/inet_ntop.c */
int     __inet_pton(int af, const char *src, void *dst,
                    struct SocketBase *SocketBase);         /* api/inet_pton.c */
extern LONG usens;                                          /* api/amiga_hostname.c */
int     sethostname(const char *name, size_t namelen);      /* api/amiga_hostname.c */
LONG    CS_Alloc(struct CSource *reply, size_t size);       /* kern/amiga_cstr.c */
extern STRPTR KW_Protocols;                                 /* kern/config_var.c */

/*
 * A kernel-private header pulled in above (kern/amiga_includes.h) takes
 * the log() macro of sys/systm.h away, for the sake of the mathematical
 * function; put it back for the file that included this one.
 */
#if defined(__AROS__) && !defined(log)
#define log(A, B, C...) __log(A, B , ## C)
#endif

struct netcore {
    UWORD   nc_size;        /* sizeof(struct netcore) of whoever filled it */
    UWORD   nc_version;     /* NETCORE_VERSION of whoever filled it */
#define NC(name)    __typeof__(name) *nc_##name;
#include <net/netcore.def>
#undef NC
};

/*
 * arostcp.library, vector 5:
 *      const struct netcore *NetCoreServices(ULONG version)   (D0)
 * NULL when the core's table is older than `version'. The caller's
 * `NetCoreBase' is the library base.
 */
#define NetCoreServices(version) \
    AROS_LC1(const struct netcore *, NetCoreServices, \
             AROS_LCA(ULONG, (version), D0), \
             struct Library *, NetCoreBase, 5, NetCore)

/*
 * Every module library has the same two vectors (net/netmodule.conf,
 * net/netmodule_lib.c):
 *      5   LONG NetModuleAttach(struct netmodule **desc)   (A0)
 *      6   void NetModuleDetach(void)
 * called by the stack, in the stack's task, with the socket calls fenced.
 *
 * NetModuleAttach() answers NMA_OK, or NMA_RESULT(code): why not, with the
 * version of the table the module was built for, so that the stack's
 * message can name both versions.
 */
#define NMA_OK          0
#define NMA_NOCORE      1   /* arostcp.library is not there, or its vectors are too old */
#define NMA_OLDCORE     2   /* the core's table is older than the module needs */
#define NMA_ATTACHED    3   /* already attached */

#define NMA_RESULT(code)    ((LONG)(code) | ((LONG)NETCORE_VERSION << 8))
#define NMA_CODE(res)       ((res) & 0xff)
#define NMA_WANTS(res)      ((LONG)(res) >> 8)

#define NetModuleAttach(base, desc) \
    AROS_LC1(LONG, NetModuleAttach, \
             AROS_LCA(struct netmodule **, (desc), A0), \
             struct Library *, (base), 5, NetModule)
#define NetModuleDetach(base) \
    AROS_LC0(void, NetModuleDetach, \
             struct Library *, (base), 6, NetModule)

#ifdef NETMODULE
extern struct netcore __netcore;    /* the module's copy (net/netmodule_lib.c) */
#endif

#endif /* !NET_NETCORE_H */
