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
#include <kern/amiga_log.h>
#include <net/route.h>
#include <net/if.h>
#include <net/if_protos.h>
#include <net/pfil.h>
#include <errno.h>
#include <netdb.h>
#include <string.h>
#include <syslog.h>
#include <version.h>

#include "amiga_api.h"
#include "gethtbynamadr.h"
#include "miami_api.h"

#include <proto/bsdsocket.h>

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
        struct Library *, MasterBase, 0, Miami)
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
					     NULL);
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
		SocketBase = OpenLibrary("bsdsocket.library", VERSION);
		if (SocketBase) {
			D(__log(LOG_DEBUG,"miami.library opened: SocketBase = 0x%p, MiamiBase = 0x%p", (IPTR)SocketBase, (IPTR)MiamiBase);)
			return(MiamiBase);
		}
		D(else kprintf("Unable to open bsdsocket.library\n");)
		__MiamiLIB_Cleanup(MiamiBase);
	}
	return(NULL);

	AROS_LIBFUNC_EXIT
}

AROS_LH0(ULONG *, Close, struct MiamiBase *, MiamiBase, 1, Miami)
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
	return NULL;

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
	if (family == AF_INET)
	{
		sprintf(strptr, "%u.%u.%u.%u", addrptr[0], addrptr[1], addrptr[2], addrptr[3]);
		return addrptr;
	} else {
		__log(LOG_CRIT,"inet_ntop(): address family %ld is not implemented", family);
		return NULL;
	}

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
	if (family == AF_INET)
		return __inet_aton(strptr, addrptr);
	else {
		__log(LOG_CRIT,"inet_pton(): address family %ld is not implemented", family);
		return 0;
	}

	AROS_LIBFUNC_EXIT
}

AROS_LH2(struct hostent *, gethostbyname2,
         AROS_LHA(char *, name, A0),
         AROS_LHA(LONG, family, D0),
         struct MiamiBase *, MiamiBase, 41, Miami
)
{
	AROS_LIBFUNC_INIT

#if defined(__AROS__)
D(bug("[AROSTCP.MIAMI] miami_api.c: gethostbyname2()\n"));
#endif

	DSYSCALLS(__log(LOG_DEBUG,"gethostbyname2(%s, %ld) called", (ULONG)name, family);)
	if (family == AF_INET)
		return __gethostbyname(name, SocketBase);
	else {
		syslog(LOG_CRIT,"gethostbyname2(): address family %ld is not implemented", family);
		return NULL;
	}

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
	if (error > 12)
		error = 12;
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

	AROS_LIBFUNC_EXIT
}

AROS_LH4(LONG, getaddrinfo,
         AROS_LHA(char *, hostname, A0),
         AROS_LHA(char *, servicename, A1),
         AROS_LHA(struct addrinfo *, hintsp, A2),
         AROS_LHA(struct addrinfo **, result, A3),
         struct MiamiBase *, MiamiBase, 44, Miami
)
{
	AROS_LIBFUNC_INIT

#if defined(__AROS__)
D(bug("[AROSTCP.MIAMI] miami_api.c: getaddrinfo()\n"));
#endif

	__log(LOG_CRIT,"getaddrinfo() is not implemented");
	writeErrnoValue(SocketBase, ENOSYS);
	return EAI_SYSTEM;

	AROS_LIBFUNC_EXIT
}

AROS_LH7(LONG, getnameinfo,
         AROS_LHA(struct sockaddr *, sockaddr, A0),
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
D(bug("[AROSTCP.MIAMI] miami_api.c: getnameinfo()\n"));
#endif

	__log(LOG_CRIT,"getnameinfo() is not implemented");
	return ENOSYS;

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
	return FALSE;

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

	return __QueryInterfaceTagList(interface_name, tags, SocketBase);

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
#warning "TODO: uncomment the following lines once we have a working usergroups.library implemenetation"
//		if (UserGroupBase)
//		return getcredentials(NULL);

	return NULL;

	AROS_LIBFUNC_EXIT
}
