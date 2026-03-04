/* Copyright (C) 2005 by Pavel Fedin.
 * Copyright (C) 2005-2026 The AROS Dev Team
 */

//#include <clib/debug_protos.h>
#include <dos/dos.h>
//#include <emul/emulregs.h>
#include <exec/libraries.h>
#include <exec/lists.h>
#include <exec/resident.h>
#include <exec/semaphores.h>
#include <sys/types.h>
#include <libraries/miami.h>
#include <proto/exec.h>
#include <proto/utility.h>

#define _GRP_H_
#define _PWD_H_
#define USE_INLINE_STDARG
//#include <proto/usergroup.h>

#include <conf.h>
#include <sys/malloc.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <kern/amiga_log.h>
#include <net/route.h>
#include <net/if.h>
#include <net/if_protos.h>
#include <net/pfil.h>
#include <errno.h>
#include <netdb.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <syslog.h>
#include <version.h>

#include "amiga_api.h"
#include "gethtbynamadr.h"
#include "miami_api.h"

#include <proto/bsdsocket.h>

extern const char *__inet_ntop(int af, const void *src, char *dst, socklen_t size, struct SocketBase *SocketBase);
extern int __inet_pton(int af, const char *src, void *dst, struct SocketBase *SocketBase);

extern struct hostent * __gethostbyaddr(UBYTE *, int, int, struct SocketBase *);

#undef SocketBase
#define SocketBase MiamiBase->_SocketBase
#define UserGroupBase MiamiBase->_UserGroupBase

typedef VOID (* f_void)(APTR args, ...);

extern f_void Miami_UserFuncTable[];
extern struct ifnet *ifnet;

void __MiamiLIB_Cleanup(struct MiamiBase *MiamiBase)
{
	void *freestart;
	IPTR size;

#if defined(__AROS__)
D(bug("[AROSTCP.MIAMI] miami_api.c: __MiamiLIB_Cleanup()\n"));
#endif

	if (SocketBase)
		CloseLibrary(&SocketBase->libNode);

	freestart = (void *)((IPTR)MiamiBase - (IPTR)MiamiBase->Lib.lib_NegSize);
	size = MiamiBase->Lib.lib_NegSize + MiamiBase->Lib.lib_PosSize;
	FreeMem(freestart, size);
}

AROS_LH1 (struct Library *, Open,
	AROS_LHA(ULONG, version, D0),
        struct Library *, MasterBase, 1, Miami)
{
	AROS_LIBFUNC_INIT

	struct MiamiBase *MiamiBase;
	WORD *i;

#if defined(__AROS__)
D(bug("[AROSTCP.MIAMI] miami_api.c: MiamiLIB_Open()\n"));
#endif
  
	D(kprintf("MiamiLIB_Open: 0x%p <%s> OpenCount %ld\n",
		     MasterBase,
		     MasterBase->lib_Node.ln_Name,
		     MasterBase->lib_OpenCnt));

	MiamiBase = (struct MiamiBase *)MakeLibrary(Miami_UserFuncTable,
#if !defined(__AROS__)
				             (UWORD *)&Miami_initTable,
#else
					     NULL,
#endif
					     NULL,
					     sizeof(struct MiamiBase),
					     BNULL);
#if defined(__AROS__)
	((struct Library *)MiamiBase)->lib_Node.ln_Type = NT_LIBRARY;
	((struct Library *)MiamiBase)->lib_Node.ln_Name = (APTR)MIAMILIBNAME;
	((struct Library *)MiamiBase)->lib_Flags = (LIBF_SUMUSED|LIBF_CHANGED);
	((struct Library *)MiamiBase)->lib_Version = MIAMI_VERSION;
	((struct Library *)MiamiBase)->lib_Revision = MIAMI_REVISION;
	((struct Library *)MiamiBase)->lib_IdString = (APTR)RELEASESTRING MIAMI_VSTRING;

D(bug("[AROSTCP](miami_api.c) MiamiLIB_Open: Created MIAMI user library base: 0x%p\n", MiamiBase));
#endif
	D(kprintf("Created user miami.library base: 0x%p\n", MiamiBase);)
	if (MiamiBase) {
		for (i = (WORD *)((struct Library *)MiamiBase + 1); i < (WORD *)(MiamiBase + 1); i++)
			*i = 0;
		MiamiBase->Lib.lib_OpenCnt = 1;
		SocketBase = (APTR)OpenLibrary("bsdsocket.library", VERSION);
		if (SocketBase) {
			D(__log(LOG_DEBUG,"miami.library opened: SocketBase = 0x%p, MiamiBase = 0x%p", (IPTR)SocketBase, (IPTR)MiamiBase);)
			MasterBase->lib_OpenCnt++;
			return((struct Library *)MiamiBase);
		}
		D(else kprintf("Unable to open bsdsocket.library\n");)
		__MiamiLIB_Cleanup(MiamiBase);
	}
	return(NULL);

	AROS_LIBFUNC_EXIT
}

AROS_LH0(ULONG *, Close, struct MiamiBase *, MiamiBase, 2, Miami)
{
	AROS_LIBFUNC_INIT

#if defined(__AROS__)
D(bug("[AROSTCP.MIAMI] miami_api.c: MiamiLIB_Close()\n"));
#endif

	D(kprintf("MiamiLIB_Close: 0x%p <%s> OpenCount %ld\n",
		      MiamiBase,
		      MasterMiamiBase->lib_Node.ln_Name,
		      MasterMiamiBase->lib_OpenCnt));

	if (MiamiBase->_UserGroupBase)
		CloseLibrary(MiamiBase->_UserGroupBase);

	__MiamiLIB_Cleanup(MiamiBase);
	MasterMiamiBase->lib_OpenCnt--;
	D(kprintf("MiamiLIB_Close: done\n"));

	return(0);
	AROS_LIBFUNC_EXIT
}


AROS_LH7(int, MiamiSysCtl,
         AROS_LHA(LONG *, name, A0),
         AROS_LHA(ULONG, namelen, D0),
         AROS_LHA(void *, oldp, A1),
         AROS_LHA(LONG *, oldlenp, A2),
         AROS_LHA(void *, newp, A3),
         AROS_LHA(LONG, newlen, D1),
         AROS_LHA(int, len, D2),
         struct MiamiBase *, MiamiBase, 5, Miami
)
{
	AROS_LIBFUNC_INIT
#if defined(__AROS__)
D(bug("[AROSTCP.MIAMI] miami_api.c: MiamiSysCtl()\n"));
#endif
	
	__log(LOG_CRIT,"MiamiSysCtl() is not implemented");
	return ENOSYS;
	AROS_LIBFUNC_EXIT
}

AROS_LH1(void, MiamiDisallowDNS,
         AROS_LHA(LONG, value, D0),
         struct MiamiBase *, MiamiBase, 11, Miami
)
{
	AROS_LIBFUNC_INIT

#if defined(__AROS__)
D(bug("[AROSTCP.MIAMI] miami_api.c: MiamiDisallowDNS()\n"));
#endif

	DSYSCALLS(__log(LOG_DEBUG,"MiamiDisallowDNS(%ld) called",value);)
	if (value)
		SocketBase->res_state.options |= AROSTCP_RES_DISABLED;
	else
		SocketBase->res_state.options &= ~AROSTCP_RES_DISABLED;

	AROS_LIBFUNC_EXIT
}

AROS_LH0(void *, MiamiGetPid,
         struct MiamiBase *, MiamiBase, 13, Miami
)
{
	AROS_LIBFUNC_INIT

#if defined(__AROS__)
D(bug("[AROSTCP.MIAMI] miami_api.c: MiamiGetPid()\n"));
#endif
	
	return FindTask(NULL);

	AROS_LIBFUNC_EXIT
}

AROS_LH3(APTR, MiamiPFAddHook,
         AROS_LHA(struct Hook *, hook, A0),
         AROS_LHA(UBYTE *, interface, A1),
         AROS_LHA(struct TagItem *, taglist, A2),
         struct MiamiBase *, MiamiBase, 16, Miami
)
{
	AROS_LIBFUNC_INIT

	struct packet_filter_hook *pf = NULL;
	struct ifnet *ifp;

#if defined(__AROS__)
D(bug("[AROSTCP.MIAMI] miami_api.c: MiamiPFAddHook()\n"));
#endif

#ifdef ENABLE_PACKET_FILTER
	DPF(__log(LOG_DEBUG,"MiamiPFAddHook(0x%p, %s) called", hook, interface);)

	ifp = ifunit(interface);
	DPF(__log(LOG_DEBUG,"ifp = 0x%pn");)
	if (ifp) {
		pf = bsd_malloc(sizeof(struct packet_filter_hook), NULL, NULL);
		DPF(syslog(LOG_DEBUG,"Handle = 0x%p", pf);)
		if (pf) {
			pf->pfil_if = ifp;
			pf->pfil_hook = hook;
			pf->pfil_hooktype = GetTagData(MIAMITAG_HOOKTYPE, MIAMICPU_M68KREG, taglist);
			ObtainSemaphore(&pfil_list_lock);
			AddTail((struct List *)&pfil_list, (struct Node *)pf);
			ReleaseSemaphore(&pfil_list_lock);
			DPF(__log(LOG_DEBUG,"Added packet filter hook:");)
			DPF(__log(LOG_DEBUG,"Function: 0x%p", hook->h_Entry);)
			DPF(__log(LOG_DEBUG,"CPU type: %lu", pf->pfil_hooktype);)
		}
	}
#else
	__log(LOG_CRIT,"Miami packet filter disabled", NULL);
#endif
	return pf;

	AROS_LIBFUNC_EXIT
}

AROS_LH1(void, MiamiPFRemoveHook,
         AROS_LHA(APTR, handle, A0),
         struct MiamiBase *, MiamiBase, 17, Miami
)
{
	AROS_LIBFUNC_INIT

#if defined(__AROS__)
D(bug("[AROSTCP.MIAMI] miami_api.c: MiamiPFRemoveHook()\n"));
#endif
	
	DPF(log("MiamiPFRemoveHook(0x%p) called", handle);)
	if (handle) {
		ObtainSemaphore(&pfil_list_lock);
		Remove(handle);
		ReleaseSemaphore(&pfil_list_lock);
		bsd_free(handle, NULL);
	}

	AROS_LIBFUNC_EXIT
}

AROS_LH1(int, MiamiGetHardwareLen,
         AROS_LHA(char *, name, A0),
         struct MiamiBase *, MiamiBase, 18, Miami
)
{
	AROS_LIBFUNC_INIT

#if defined(__AROS__)
D(bug("[AROSTCP.MIAMI] miami_api.c: MiamiGetHardwareLen()\n"));
#endif

	__log(LOG_CRIT,"MiamiGetHardwareLen() is not implemented");
	return 0;

	AROS_LIBFUNC_EXIT
}

AROS_LH1(struct Library *, MiamiOpenSSL,
         AROS_LHA(struct TagItem *, taglist, A0),
         struct MiamiBase *, MiamiBase, 25, Miami
)
{
	AROS_LIBFUNC_INIT

#if defined(__AROS__)
D(bug("[AROSTCP.MIAMI] miami_api.c: MiamiOpenSSL()\n"));
#endif
	return NULL;

	AROS_LIBFUNC_EXIT
}

AROS_LH0(void, MiamiCloseSSL,
         struct MiamiBase *, MiamiBase, 26, Miami
)
{
	AROS_LIBFUNC_INIT

#if defined(__AROS__)
D(bug("[AROSTCP.MIAMI] miami_api.c: MiamiCloseSSL()\n"));
#endif

	AROS_LIBFUNC_EXIT
}

AROS_LH2(int, MiamiSetSocksConn,
         AROS_LHA(struct sockaddr *, sa, A0),
         AROS_LHA(int, len, D0),
         struct MiamiBase *, MiamiBase, 33, Miami
)
{
	AROS_LIBFUNC_INIT

#if defined(__AROS__)
D(bug("[AROSTCP.MIAMI] miami_api.c: MiamiSetSocksConn()\n"));
#endif
	__log(LOG_CRIT,"MiamiSetSocksConn() is not implemented");
	return FALSE;

	AROS_LIBFUNC_EXIT
}

AROS_LH1(int, MiamiIsOnline,
         AROS_LHA(char *, name, A0),
         struct MiamiBase *, MiamiBase, 35, Miami
)
{
	AROS_LIBFUNC_INIT

	struct ifnet *ifp;
	long online = FALSE;

#if defined(__AROS__)
D(bug("[AROSTCP.MIAMI] miami_api.c: MiamiIsOnline()\n"));
#endif
	
	DSYSCALLS(__log(LOG_DEBUG,"MiamiIsOnline(%s) called", (ULONG)(name ? name : "<NULL>"));)
	if (name && strcmp(name,"mi0")) {
		ifp = ifunit(name);
		if (ifp)
			online = (ifp->if_flags & IFF_UP) ? TRUE : FALSE;
	} else {
		for (ifp = ifnet; ifp; ifp = ifp->if_next) {
			DSYSCALLS(__log(LOG_DEBUG,"Checking interface %s%u",ifp->if_name, ifp->if_unit);)
			if ((ifp->if_flags & (IFF_UP | IFF_LOOPBACK)) == IFF_UP) {
				online = TRUE;
				break;
			}
		}
	}
	DSYSCALLS(__log(LOG_DEBUG,"MiamiIsOnline() result: %ld", online);)
	return online;

	AROS_LIBFUNC_EXIT
}

AROS_LH2(void, MiamiOnOffline,
         AROS_LHA(char *, name, A0),
         AROS_LHA(int, online, D0),
         struct MiamiBase *, MiamiBase, 36, Miami
)
{
	AROS_LIBFUNC_INIT

	struct ifnet *ifp;

#if defined(__AROS__)
D(bug("[AROSTCP.MIAMI] miami_api.c: MiamiOnOffline()\n"));
#endif
	
	ifp = ifunit(name);
	if (ifp)
		ifupdown(ifp, online);

	AROS_LIBFUNC_EXIT
}

AROS_LH4(STRPTR, inet_ntop,
         AROS_LHA(LONG, family, D0),
         AROS_LHA(char *, addrptr, A0),
         AROS_LHA(char *, strptr, A1),
         AROS_LHA(LONG, len, D1),
         struct MiamiBase *, MiamiBase, 38, Miami
)
{
	AROS_LIBFUNC_INIT

#if defined(__AROS__)
D(bug("[AROSTCP.MIAMI] miami_api.c: inet_ntop()\n"));
#endif

	DSYSCALLS(__log(LOG_DEBUG,"inet_ntop(%ld) called",family);)
	return (STRPTR)__inet_ntop((int)family, addrptr, strptr, (socklen_t)len, SocketBase);

	AROS_LIBFUNC_EXIT
}

AROS_LH2(int, Miami_inet_aton,
         AROS_LHA(char *, strptr, A0),
         AROS_LHA(void *, addrptr, A2),
         struct MiamiBase *, MiamiBase, 39, Miami
)
{
	AROS_LIBFUNC_INIT

#if defined(__AROS__)
D(bug("[AROSTCP.MIAMI] miami_api.c: Miami_inet_aton()\n"));
#endif

	DSYSCALLS(syslog(LOG_DEBUG,"inet_aton(%s) called", (ULONG)strptr);)
	return __inet_aton(strptr, addrptr);

	AROS_LIBFUNC_EXIT
}

AROS_LH3(int, inet_pton,
         AROS_LHA(LONG, family, D0),
         AROS_LHA(char *, strptr, A0),
         AROS_LHA(void *, addrptr, A1),
         struct MiamiBase *, MiamiBase, 40, Miami
)
{
	AROS_LIBFUNC_INIT

#if defined(__AROS__)
D(bug("[AROSTCP.MIAMI] miami_api.c: inet_pton()\n"));
#endif

	DSYSCALLS(__log(LOG_DEBUG,"inet_pton(%ld, %s) called", family, (ULONG)strptr);)
	return __inet_pton((int)family, strptr, addrptr, SocketBase);

	AROS_LIBFUNC_EXIT
}

AROS_LH2(struct hostent *, gethostbyname2,
         AROS_LHA(const char *, name, A0),
         AROS_LHA(LONG, family, D0),
         struct MiamiBase *, MiamiBase, 41, Miami
)
{
	AROS_LIBFUNC_INIT

#if defined(__AROS__)
D(bug("[AROSTCP.MIAMI] miami_api.c: gethostbyname2('%s', af=%ld)\n", name, family));
#endif

	DSYSCALLS(__log(LOG_DEBUG,"gethostbyname2(%s, %ld) called", (ULONG)name, family);)
	return __gethostbyname2(name, (int)family, SocketBase);

	AROS_LIBFUNC_EXIT
}

char *ai_errors[] = {
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

AROS_LH1(char *, gai_strerror,
         AROS_LHA(LONG, error, D0),
         struct MiamiBase *, MiamiBase, 42, Miami
)
{
	AROS_LIBFUNC_INIT

#if defined(__AROS__)
D(bug("[AROSTCP.MIAMI] miami_api.c: gai_strerror()\n"));
#endif

	DSYSCALLS(__log(LOG_DEBUG,"gai_strerror(%ld) called", error);)
	if (error < 0 || error > 11)
		return "Unknown error";
	return ai_errors[error];

	AROS_LIBFUNC_EXIT
}

AROS_LH1(void, freeaddrinfo,
         AROS_LHA(struct addrinfo *, addrinfo, A0),
         struct MiamiBase *, MiamiBase, 43, Miami
)
{
	AROS_LIBFUNC_INIT

#if defined(__AROS__)
D(bug("[AROSTCP.MIAMI] miami_api.c: freeaddrinfo()\n"));
#endif

	struct addrinfo *ai, *next;

	for (ai = addrinfo; ai != NULL; ai = next) {
		next = ai->ai_next;
		if (ai->ai_canonname)
			FreeVec(ai->ai_canonname);
		FreeVec(ai);
	}

	AROS_LIBFUNC_EXIT
}

AROS_LH4(LONG, getaddrinfo,
         AROS_LHA(char *, hostname, A0),
         AROS_LHA(char *, servname, A1),
         AROS_LHA(struct addrinfo *, hints, A2),
         AROS_LHA(struct addrinfo **, result, A3),
         struct MiamiBase *, MiamiBase, 44, Miami
)
{
	AROS_LIBFUNC_INIT

#if defined(__AROS__)
D(bug("[AROSTCP.MIAMI] miami_api.c: getaddrinfo('%s', '%s')\n",
      hostname ? hostname : "(null)", servname ? servname : "(null)"));
#endif

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

	/* Process hints */
	if (hints) {
		if (hints->ai_addrlen || hints->ai_canonname ||
		    hints->ai_addr || hints->ai_next)
			return EAI_BADFLAGS;
		family = hints->ai_family;
		socktype = hints->ai_socktype;
		protocol = hints->ai_protocol;
		flags = hints->ai_flags;

		switch (family) {
		case PF_UNSPEC:
		case PF_INET:
		case PF_INET6:
			break;
		default:
			return EAI_FAMILY;
		}
		switch (socktype) {
		case 0:
		case SOCK_STREAM:
		case SOCK_DGRAM:
		case SOCK_RAW:
			break;
		default:
			return EAI_SOCKTYPE;
		}
	}

	/* Resolve service name to port number */
	if (servname != NULL && servname[0] != '\0') {
		char *ep;
		long pval;
		const char *proto_name = NULL;

		if (socktype == SOCK_STREAM)
			proto_name = "tcp";
		else if (socktype == SOCK_DGRAM)
			proto_name = "udp";

		/* Try numeric port first */
		pval = strtol(servname, &ep, 10);
		if (ep != servname && *ep == '\0' && pval >= 0 && pval <= 65535) {
			port = htons((unsigned short)pval);
		} else {
			struct servent *sp;

			if (flags & AI_NUMERICSERV)
				return EAI_NONAME;

			sp = getservbyname(servname, proto_name);
			if (sp == NULL) {
				/* try the other protocol if socktype was unspecified */
				if (proto_name == NULL) {
					sp = getservbyname(servname, "tcp");
					if (sp == NULL)
						sp = getservbyname(servname, "udp");
				}
				if (sp == NULL)
					return EAI_SERVICE;
			}
			port = sp->s_port; /* already in network byte order */
		}
	}

	/* Determine which socket types to iterate */
	int stypes[3];
	int nstypes = 0;

	if (socktype != 0) {
		stypes[0] = socktype;
		nstypes = 1;
	} else {
		stypes[0] = SOCK_STREAM;
		stypes[1] = SOCK_DGRAM;
		nstypes = 2;
	}

	/* Determine which address families to iterate */
	int families[2];
	int nfamilies = 0;

	if (family == PF_INET || family == PF_INET6) {
		families[0] = family;
		nfamilies = 1;
	} else {
		/* PF_UNSPEC: try both */
		families[0] = PF_INET6;
		families[1] = PF_INET;
		nfamilies = 2;
	}

	for (int fi = 0; fi < nfamilies; fi++) {
		int af = families[fi];

		/* Resolve or construct addresses */
		struct sockaddr_storage addrs[16];
		char *canonname = NULL;
		int naddrs = 0;

		if (hostname == NULL) {
			/* NULL hostname: wildcard or loopback */
			if (af == PF_INET) {
				struct sockaddr_in *sin = (struct sockaddr_in *)&addrs[0];
				memset(sin, 0, sizeof(*sin));
				sin->sin_len = sizeof(*sin);
				sin->sin_family = AF_INET;
				sin->sin_addr.s_addr = (flags & AI_PASSIVE)
					? htonl(INADDR_ANY) : htonl(INADDR_LOOPBACK);
				naddrs = 1;
			} else {
				struct sockaddr_in6 *sin6 = (struct sockaddr_in6 *)&addrs[0];
				memset(sin6, 0, sizeof(*sin6));
				sin6->sin6_len = sizeof(*sin6);
				sin6->sin6_family = AF_INET6;
				if (flags & AI_PASSIVE)
					memset(&sin6->sin6_addr, 0, sizeof(sin6->sin6_addr));
				else {
					memset(&sin6->sin6_addr, 0, sizeof(sin6->sin6_addr));
					sin6->sin6_addr.s6_addr[15] = 1; /* ::1 */
				}
				naddrs = 1;
			}
		} else if (flags & AI_NUMERICHOST) {
			/* Numeric-only parse */
			if (af == PF_INET) {
				struct sockaddr_in *sin = (struct sockaddr_in *)&addrs[0];
				memset(sin, 0, sizeof(*sin));
				sin->sin_len = sizeof(*sin);
				sin->sin_family = AF_INET;
				if (__inet_pton(AF_INET, hostname, &sin->sin_addr, SocketBase) == 1)
					naddrs = 1;
			} else {
				struct sockaddr_in6 *sin6 = (struct sockaddr_in6 *)&addrs[0];
				memset(sin6, 0, sizeof(*sin6));
				sin6->sin6_len = sizeof(*sin6);
				sin6->sin6_family = AF_INET6;
				if (__inet_pton(AF_INET6, hostname, &sin6->sin6_addr, SocketBase) == 1)
					naddrs = 1;
			}
		} else {
			/* Try numeric parse first, then DNS */
			int numeric_ok = 0;

			if (af == PF_INET) {
				struct sockaddr_in *sin = (struct sockaddr_in *)&addrs[0];
				memset(sin, 0, sizeof(*sin));
				sin->sin_len = sizeof(*sin);
				sin->sin_family = AF_INET;
				if (__inet_pton(AF_INET, hostname, &sin->sin_addr, SocketBase) == 1) {
					naddrs = 1;
					numeric_ok = 1;
				}
			} else {
				struct sockaddr_in6 *sin6 = (struct sockaddr_in6 *)&addrs[0];
				memset(sin6, 0, sizeof(*sin6));
				sin6->sin6_len = sizeof(*sin6);
				sin6->sin6_family = AF_INET6;
				if (__inet_pton(AF_INET6, hostname, &sin6->sin6_addr, SocketBase) == 1) {
					naddrs = 1;
					numeric_ok = 1;
				}
			}

			if (!numeric_ok) {
				/* DNS lookup */
				struct hostent *hp;

				if (af == PF_INET)
					hp = __gethostbyname(hostname, SocketBase);
				else
					hp = __gethostbyname2(hostname, AF_INET6, SocketBase);

				if (hp != NULL && hp->h_addr_list != NULL) {
					if (flags & AI_CANONNAME)
						canonname = hp->h_name;
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

		if (naddrs == 0)
			continue;

		/* Build result nodes for each address x socktype combination */
		for (int ai = 0; ai < naddrs; ai++) {
			struct sockaddr *sa = (struct sockaddr *)&addrs[ai];
			int addrlen = (af == PF_INET)
				? sizeof(struct sockaddr_in) : sizeof(struct sockaddr_in6);

			/* Set port in the address */
			if (af == PF_INET)
				((struct sockaddr_in *)sa)->sin_port = port;
			else
				((struct sockaddr_in6 *)sa)->sin6_port = port;

			for (int si = 0; si < nstypes; si++) {
				struct addrinfo *ai_new;
				int proto;

				if (stypes[si] == SOCK_STREAM)
					proto = IPPROTO_TCP;
				else if (stypes[si] == SOCK_DGRAM)
					proto = IPPROTO_UDP;
				else
					proto = protocol;

				/* Allocate addrinfo + sockaddr in single block */
				ai_new = (struct addrinfo *)AllocVec(
					sizeof(struct addrinfo) + addrlen,
					MEMF_PUBLIC | MEMF_CLEAR);
				if (ai_new == NULL) {
					error = EAI_MEMORY;
					goto fail;
				}

				ai_new->ai_flags = flags;
				ai_new->ai_family = af;
				ai_new->ai_socktype = stypes[si];
				ai_new->ai_protocol = proto;
				ai_new->ai_addrlen = addrlen;
				ai_new->ai_addr = (struct sockaddr *)(ai_new + 1);
				memcpy(ai_new->ai_addr, sa, addrlen);
				ai_new->ai_next = NULL;

				/* Set canonical name on first result only */
				if (sentinel.ai_next == NULL && canonname != NULL) {
					int clen = strlen(canonname) + 1;
					ai_new->ai_canonname = (char *)AllocVec(clen, MEMF_PUBLIC);
					if (ai_new->ai_canonname)
						memcpy(ai_new->ai_canonname, canonname, clen);
				} else if (sentinel.ai_next == NULL && (flags & AI_CANONNAME) && hostname != NULL) {
					int clen = strlen(hostname) + 1;
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
	/* Free any partial results */
	{
		struct addrinfo *ai_f, *next;
		for (ai_f = sentinel.ai_next; ai_f != NULL; ai_f = next) {
			next = ai_f->ai_next;
			if (ai_f->ai_canonname)
				FreeVec(ai_f->ai_canonname);
			FreeVec(ai_f);
		}
	}
	return error;

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
         struct MiamiBase *, MiamiBase, 45, Miami
)
{
	AROS_LIBFUNC_INIT

#if defined(__AROS__)
D(bug("[AROSTCP.MIAMI] miami_api.c: getnameinfo(af=%d)\n", sa ? sa->sa_family : -1));
#endif

	if (sa == NULL)
		return EAI_FAIL;

	/* Resolve hostname */
	if (hostname != NULL && hostlen > 0) {
		if (flags & NI_NUMERICHOST) {
			/* Numeric only */
			const char *p = NULL;
			if (sa->sa_family == AF_INET) {
				struct sockaddr_in *sin = (struct sockaddr_in *)sa;
				p = __inet_ntop(AF_INET, &sin->sin_addr,
						hostname, hostlen, SocketBase);
			} else if (sa->sa_family == AF_INET6) {
				struct sockaddr_in6 *sin6 = (struct sockaddr_in6 *)sa;
				p = __inet_ntop(AF_INET6, &sin6->sin6_addr,
						hostname, hostlen, SocketBase);
			} else {
				return EAI_FAMILY;
			}
			if (p == NULL)
				return EAI_FAIL;
		} else {
			/* Try reverse DNS */
			struct hostent *hp = NULL;
			const char *numaddr = NULL;

			if (sa->sa_family == AF_INET) {
				struct sockaddr_in *sin = (struct sockaddr_in *)sa;
				hp = __gethostbyaddr((UBYTE *)&sin->sin_addr,
						     sizeof(sin->sin_addr),
						     AF_INET, SocketBase);
			} else if (sa->sa_family == AF_INET6) {
				struct sockaddr_in6 *sin6 = (struct sockaddr_in6 *)sa;
				hp = __gethostbyaddr((UBYTE *)&sin6->sin6_addr,
						     sizeof(sin6->sin6_addr),
						     AF_INET6, SocketBase);
			} else {
				return EAI_FAMILY;
			}

			if (hp != NULL && hp->h_name != NULL) {
				char *name = hp->h_name;
				if (flags & NI_NOFQDN) {
					/* Truncate at first dot */
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
				if ((int)strlen(name) >= hostlen)
					return EAI_FAIL;
				strcpy(hostname, name);
			} else {
				if (flags & NI_NAMEREQD)
					return EAI_NONAME;
				/* Fall back to numeric */
				if (sa->sa_family == AF_INET) {
					struct sockaddr_in *sin = (struct sockaddr_in *)sa;
					numaddr = __inet_ntop(AF_INET, &sin->sin_addr,
							      hostname, hostlen, SocketBase);
				} else {
					struct sockaddr_in6 *sin6 = (struct sockaddr_in6 *)sa;
					numaddr = __inet_ntop(AF_INET6, &sin6->sin6_addr,
							      hostname, hostlen, SocketBase);
				}
				if (numaddr == NULL)
					return EAI_FAIL;
			}
		}
	}
host_done:

	/* Resolve service name */
	if (servicename != NULL && servicelen > 0) {
		int port;

		if (sa->sa_family == AF_INET)
			port = ((struct sockaddr_in *)sa)->sin_port;
		else if (sa->sa_family == AF_INET6)
			port = ((struct sockaddr_in6 *)sa)->sin6_port;
		else
			return EAI_FAMILY;

		if (!(flags & NI_NUMERICSERV)) {
			const char *proto = (flags & NI_DGRAM) ? "udp" : "tcp";
			struct servent *sp = getservbyport(port, proto);

			if (sp != NULL) {
				if ((int)strlen(sp->s_name) >= servicelen)
					return EAI_FAIL;
				strcpy(servicename, sp->s_name);
				goto serv_done;
			}
		}
		/* Numeric fallback */
		snprintf(servicename, servicelen, "%d", ntohs(port));
	}
serv_done:

	return 0;

	AROS_LIBFUNC_EXIT
}

AROS_LH0(LONG, MiamiSupportsIPV6,
         struct MiamiBase *, MiamiBase, 50, Miami
)
{
	AROS_LIBFUNC_INIT

#if defined(__AROS__)
D(bug("[AROSTCP.MIAMI] miami_api.c: MiamiSupportsIPV6()\n"));
#endif

	DSYSCALLS(__log(LOG_DEBUG,"MiamiSupportsIPV6() called");)
#if INET6
	return TRUE;
#else
	return FALSE;
#endif

	AROS_LIBFUNC_EXIT
}

AROS_LH0(LONG, MiamiGetResOptions,
         struct MiamiBase *, MiamiBase, 51, Miami
)
{
	AROS_LIBFUNC_INIT

#if defined(__AROS__)
D(bug("[AROSTCP.MIAMI] miami_api.c: MiamiResGetOptions()\n"));
#endif

	DSYSCALLS(__log(LOG_DEBUG,"MiamiResGetOptions() called, result: 0x%p", SocketBase.res_state.options);)
	return SocketBase->res_state.options;

	AROS_LIBFUNC_EXIT
}

AROS_LH1(void, MiamiSetResOptions,
         AROS_LHA(LONG, options, D0),
         struct MiamiBase *, MiamiBase, 52, Miami
)
{
	AROS_LIBFUNC_INIT

#if defined(__AROS__)
D(bug("[AROSTCP.MIAMI] miami_api.c: MiamiResSetOptions()\n"));
#endif

	DSYSCALLS(__log(LOG_DEBUG,"MiamiResSetOptions(0x%p) called", options);)
	SocketBase->res_state.options = options;

	AROS_LIBFUNC_EXIT
}

AROS_LH1(LONG, sockatmark,
         AROS_LHA(LONG, sockfd, D0),
         struct MiamiBase *, MiamiBase, 53, Miami
)
{
	AROS_LIBFUNC_INIT

#if defined(__AROS__)
D(bug("[AROSTCP.MIAMI] miami_api.c: sockatmark()\n"));
#endif

	__log(LOG_CRIT,"sockatmark() is not implemented");
	return 0;

	AROS_LIBFUNC_EXIT
}

AROS_LH3(void, MiamiSupportedCPUs,
         AROS_LHA(ULONG *, apis, A0),
         AROS_LHA(ULONG *, callbacks, A1),
         AROS_LHA(ULONG *, kernel, A2),
         struct MiamiBase *, MiamiBase, 54, Miami
)
{
	AROS_LIBFUNC_INIT

#if defined(__AROS__)
D(bug("[AROSTCP.MIAMI] miami_api.c: MiamiSupportedCPUs()\n"));
#endif

	DSYSCALLS(__log(LOG_DEBUG,"MiamiSupportedCPUs() called");)
	*apis = MIAMICPU_M68KREG;
	*callbacks = MIAMICPU_M68KREG;
	*kernel = MIAMICPU_PPCV4;

	AROS_LIBFUNC_EXIT
}

AROS_LH1(LONG, MiamiGetFdCallback,
         AROS_LHA(fdCallback_t *, cbptr, A0),
         struct MiamiBase *, MiamiBase, 55, Miami
)
{
	AROS_LIBFUNC_INIT

#if defined(__AROS__)
D(bug("[AROSTCP.MIAMI] miami_api.c: MiamiGetFdCallback()\n"));
#endif

	*cbptr = SocketBase->fdCallback;
	DSYSCALLS(__log(LOG_DEBUG,"MiamiGetFdCallback() called, *cbptr = 0x%p", (ULONG)*cbptr);)
	return *cbptr ? MIAMICPU_M68KREG : 0;

	AROS_LIBFUNC_EXIT
}

AROS_LH2(LONG, MiamiSetFdCallback,
         AROS_LHA(fdCallback_t, cbptr, A0),
         AROS_LHA(LONG, cputype, D0),
         struct MiamiBase *, MiamiBase, 56, Miami
)
{
	AROS_LIBFUNC_INIT

#if defined(__AROS__)
D(bug("[AROSTCP.MIAMI] miami_api.c: MiamiSetFdCallback()\n"));
#endif

	DSYSCALLS(__log(LOG_DEBUG,"MiamiSetFdCallback(0x%p, %ld) called", (ULONG)cbptr, cputype);)
	if (cputype == MIAMICPU_M68KREG) {
		SocketBase->fdCallback = cbptr;
		return TRUE;
	} else
		return FALSE;

	AROS_LIBFUNC_EXIT
}

AROS_LH0(void, SetSysLogPort,
         struct MiamiBase *, MiamiBase, 6, Miami
)
{
	AROS_LIBFUNC_INIT

#if defined(__AROS__)
D(bug("[AROSTCP.MIAMI] miami_api.c: SetSysLogPort()\n"));
#endif

	DSYSCALLS(__log(LOG_DEBUG,"SetSysLogPort() called");)
	ExtLogPort = FindPort("SysLog");

	AROS_LIBFUNC_EXIT
}

AROS_LH2(int, Miami_sethostname,
         AROS_LHA(const char *, name, A0),
         AROS_LHA(size_t, namelen, D0),
         struct MiamiBase *, MiamiBase, 23, Miami
)
{
	AROS_LIBFUNC_INIT

#if defined(__AROS__)
D(bug("[AROSTCP.MIAMI] miami_api.c: Miami_sethostname()\n"));
#endif

	return sethostname(name, namelen);

	AROS_LIBFUNC_EXIT
}

AROS_LH2(void, Miami_QueryInterfaceTagList,
         AROS_LHA(STRPTR, interface_name, A0),
         AROS_LHA(struct TagItem *, tags, A1),
         struct MiamiBase *, MiamiBase, 7, Miami
)
{
	AROS_LIBFUNC_INIT

#if defined(__AROS__)
D(bug("[AROSTCP.MIAMI] miami_api.c: Miami_QueryInterfaceTagList()\n"));
#endif
	// "return" removed, because we are a void function
	__QueryInterfaceTagList(interface_name, tags, SocketBase);

	AROS_LIBFUNC_EXIT
}

AROS_LH0(struct UserGroupCredentials *, MiamiGetCredentials,
         struct MiamiBase *, MiamiBase, 58, Miami)
{
	AROS_LIBFUNC_INIT

#if defined(__AROS__)
D(bug("[AROSTCP.MIAMI] miami_api.c: MiamiGetCredentials()\n"));
#endif

	/* We don't want to have this library at all, so we open it only if we really need it */
	if (!UserGroupBase)
		UserGroupBase = OpenLibrary("usergroup.library",4);

	DSYSCALLS(__log(LOG_DEBUG,"MiamiGetCredentials(): UserGroupBase = 0x%p", (IPTR)UserGroupBase);)
/* TODO: uncomment the following lines once we have a working usergroups.library implemenetation */
//		if (UserGroupBase)
//		return getcredentials(NULL);

	return NULL;

	AROS_LIBFUNC_EXIT
}
