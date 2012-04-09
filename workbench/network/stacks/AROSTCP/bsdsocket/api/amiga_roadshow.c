/* This file contains Roadshow-specific functions which are not required for MorphOS build */

#include <conf.h>

#ifdef __CONFIG_ROADSHOW__

#include <aros/libcall.h>
#include <exec/lists.h>
#include <libraries/bsdsocket.h>
#include <utility/tagitem.h>
//#include <if/route.h>
#include <sys/types.h>
#include <syslog.h>
#include <api/amiga_api.h>

AROS_LH1(long, bpf_open,
	AROS_LHA(long, channel, D0),
	struct SocketBase *, libPtr, 61, UL)
{
    AROS_LIBFUNC_INIT
    __log(LOG_CRIT,"bpf_open() is not implemented");
    writeErrnoValue(libPtr, ENOSYS);
    return -1;
    AROS_LIBFUNC_EXIT
}

AROS_LH1(long, bpf_close,
	AROS_LHA(long, handle, D0),
	struct SocketBase *, libPtr, 62, UL)
{
    AROS_LIBFUNC_INIT
    writeErrnoValue(libPtr, ENOSYS);
    return -1;
    AROS_LIBFUNC_EXIT
}

AROS_LH3(long, bpf_read,
	AROS_LHA(long, handle, D0),
	AROS_LHA(void *, buffer, A0),
	AROS_LHA(long, len, D1),
	struct SocketBase *, libPtr, 63, UL)
{
    AROS_LIBFUNC_INIT
    writeErrnoValue(libPtr, ENOSYS);
    return -1;
    AROS_LIBFUNC_EXIT
}

AROS_LH3(long, bpf_write,
	AROS_LHA(long, handle, D0),
	AROS_LHA(void *, buffer, A0),
	AROS_LHA(long, len, D1),
	struct SocketBase *, libPtr, 64, UL)
{
    AROS_LIBFUNC_INIT
    writeErrnoValue(libPtr, ENOSYS);
    return -1;
    AROS_LIBFUNC_EXIT
}

AROS_LH2(long, bpf_set_notify_mask,
	AROS_LHA(long, handle, D0),
	AROS_LHA(unsigned long, mask, D1),
	struct SocketBase *, libPtr, 65, UL)
{
    AROS_LIBFUNC_INIT
    writeErrnoValue(libPtr, ENOSYS);
    return -1;
    AROS_LIBFUNC_EXIT
}

AROS_LH2(long, bpf_set_interrupt_mask,
	AROS_LHA(long, handle, D0),
	AROS_LHA(unsigned long, mask, D1),
	struct SocketBase *, libPtr, 66, UL)
{
    AROS_LIBFUNC_INIT
    writeErrnoValue(libPtr, ENOSYS);
    return -1;
    AROS_LIBFUNC_EXIT
}

AROS_LH3(long, bpf_ioctl,
	AROS_LHA(long, handle, D0),
	AROS_LHA(unsigned long, request, D1),
	AROS_LHA(char *, argp, A0),
	struct SocketBase *, libPtr, 67, UL)
{
    AROS_LIBFUNC_INIT
    writeErrnoValue(libPtr, ENOSYS);
    return -1;
    AROS_LIBFUNC_EXIT
}

AROS_LH1(long, bpf_data_waiting,
	AROS_LHA(long, handle, D0),
	struct SocketBase *, libPtr, 68, UL)
{
    AROS_LIBFUNC_INIT
    writeErrnoValue(libPtr, ENOSYS);
    return -1;
    AROS_LIBFUNC_EXIT
}

AROS_LH1(long, AddRouteTagList,
	AROS_LHA(struct TagItem *, tags, A0),
	struct SocketBase *, libPtr, 69, UL)
{
    AROS_LIBFUNC_INIT
    __log(LOG_CRIT,"AddRouteTagList() is not implemented");
    writeErrnoValue(libPtr, ENOSYS);
    return -1;
    AROS_LIBFUNC_EXIT
}

AROS_LH1(long, DeleteRouteTagList,
	AROS_LHA(struct TagItem *, tags, A0),
	struct SocketBase *, libPtr, 70, UL)
{
    AROS_LIBFUNC_INIT
    __log(LOG_CRIT,"DeleteRouteTagList() is not implemented");
    writeErrnoValue(libPtr, ENOSYS);
    return -1;
    AROS_LIBFUNC_EXIT
}

AROS_LH1(long, ChangeRouteTagList,
	AROS_LHA(struct TagItem *, tags, A0),
	struct SocketBase *, libPtr, 71, UL)
{
    AROS_LIBFUNC_INIT
    __log(LOG_CRIT,"ChangeRouteTagList() is not implemented");
    writeErrnoValue(libPtr, ENOSYS);
    return -1;
    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, FreeRouteInfo,
	AROS_LHA(struct rt_msghdr *, table, A0),
	struct SocketBase *, libPtr, 72, UL)
{
    AROS_LIBFUNC_INIT
    __log(LOG_CRIT,"FreeRouteInfo() is not implemented");
    AROS_LIBFUNC_EXIT
}

AROS_LH2(struct rt_msghdr *, GetRouteInfo,
	AROS_LHA(LONG, address_family, D0),
	AROS_LHA(LONG, flags, D1),
	struct SocketBase *, libPtr, 73, UL)
{
    AROS_LIBFUNC_INIT
    __log(LOG_CRIT,"GetRouteInfo() is not implemented\n");
    writeErrnoValue(libPtr, ENOSYS);
    return NULL;
    AROS_LIBFUNC_EXIT
}

AROS_LH4(long, AddInterfaceTagList,
	AROS_LHA(STRPTR, name, A0),
	AROS_LHA(STRPTR, device, A1),
	AROS_LHA(long, unit, D0),
	AROS_LHA(struct TagItem *, tags, A2),
	struct SocketBase *, libPtr, 74, UL)
{
    AROS_LIBFUNC_INIT
    __log(LOG_CRIT,"AddInterfaceTagList() is not implemented\n");
    writeErrnoValue(libPtr, ENOSYS);
    return -1;
    AROS_LIBFUNC_EXIT
}

AROS_LH2(long, ConfigureInterfaceTagList,
	AROS_LHA(STRPTR, name, A0),
	AROS_LHA(struct TagItem *, tags, A1),
	struct SocketBase *, libPtr, 75, UL)
{
    AROS_LIBFUNC_INIT
    __log(LOG_CRIT,"ConfigureInterfaceTagList() is not implemented\n");
    writeErrnoValue(libPtr, ENOSYS);
    return -1;
    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, ReleaseInterfaceList,
	AROS_LHA(struct List *, list, A0),
	struct SocketBase *, libPtr, 76, UL)
{
    AROS_LIBFUNC_INIT
    __log(LOG_CRIT,"ReleaseInterfaceList() is not implemented");
    AROS_LIBFUNC_EXIT
}

AROS_LH0(struct List *, ObtainInterfaceList,
	struct SocketBase *, libPtr, 77, UL)
{
    AROS_LIBFUNC_INIT
    __log(LOG_CRIT,"ObtainInterfaceList() is not implemented");
    return NULL;
    AROS_LIBFUNC_EXIT
}

// QueryInterfaceTagList in amiga_netstat.c

AROS_LH5(LONG, CreateAddrAllocMessageA,
	AROS_LHA(LONG, version, D0),
	AROS_LHA(LONG, protocol, D1),
	AROS_LHA(STRPTR, interface_name, A0),
	AROS_LHA(struct AddressAllocationMessage *, result_ptr, A1),
	AROS_LHA(struct TagItem *, tags, A2),
	struct SocketBase *, libPtr, 79, UL)
{
    AROS_LIBFUNC_INIT
    __log(LOG_CRIT,"CreateAddrAllocMessageA() is not implemented\n");
    return 0;
    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, DeleteAddrAllocMessage,
	AROS_LHA(struct AddressAllocationMessage *, message, A0),
	struct SocketBase *, libPtr, 80, UL)
{
    AROS_LIBFUNC_INIT
    __log(LOG_CRIT,"DeleteAddrAllocMessage() is not implemented");
    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, BeginInterfaceConfig,
	AROS_LHA(struct AddressAllocationMessage *, message, A0),
	struct SocketBase *, libPtr, 81, UL)
{
    AROS_LIBFUNC_INIT
    __log(LOG_CRIT,"BeginInterfaceConfig() is not implemented");
    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, AbortInterfaceConfig,
	AROS_LHA(struct AddressAllocationMessage *, message, A0),
	struct SocketBase *, libPtr, 82, UL)
{
    AROS_LIBFUNC_INIT
    __log(LOG_CRIT,"AbortInterfaceConfig() is not implemented");
    AROS_LIBFUNC_EXIT
}

AROS_LH3(long, AddNetMonitorHookTagList,
	AROS_LHA(long, type, D0),
	AROS_LHA(struct Hook *, hook, A0),
	AROS_LHA(struct TagItem *, tags, A1),
	struct SocketBase *, libPtr, 83, UL)
{
    AROS_LIBFUNC_INIT
    __log(LOG_CRIT,"AddNetMonitorHookTagList() is not implemented");
    writeErrnoValue(libPtr, ENOSYS);
    return -1;
    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, RemoveNetMonitorHook,
	AROS_LHA(struct Hook *, hook, A0),
	struct SocketBase *, libPtr, 84, UL)
{
    AROS_LIBFUNC_INIT
    __log(LOG_CRIT,"RemoveNetMonitorHook() is not implemented");
    AROS_LIBFUNC_EXIT
}

AROS_LH4(LONG, GetNetworkStatistics,
	AROS_LHA(LONG, type, D0),
	AROS_LHA(LONG, version, D1),
	AROS_LHA(APTR, destination, A0),
	AROS_LHA(LONG, size, D2),
	struct SocketBase *, libPtr, 85, UL)
{
    AROS_LIBFUNC_INIT
    __log(LOG_CRIT,"GetNetworkStatistics() is not implemented");
    writeErrnoValue(libPtr, ENOSYS);
    return -1;
    AROS_LIBFUNC_EXIT
}

AROS_LH1(LONG, AddDomainNameServer,
	AROS_LHA(STRPTR, address, A0),
	struct SocketBase *, libPtr, 86, UL)
{
    AROS_LIBFUNC_INIT
    __log(LOG_CRIT, "AddDomainNameServer() is not implemented");
    writeErrnoValue(libPtr, ENOSYS);
    return -1;
    AROS_LIBFUNC_EXIT
}

AROS_LH1(LONG, RemoveDomainNameServer,
	AROS_LHA(STRPTR, address, A0),
	struct SocketBase *, libPtr, 87, UL)
{
    AROS_LIBFUNC_INIT
    __log(LOG_CRIT, "RemoveDomainNameServer() is not implemented");
    writeErrnoValue(libPtr, ENOSYS);
    return -1;
    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, ReleaseDomainNameServerList,
	AROS_LHA(struct List *, list, A0),
	struct SocketBase *, libPtr, 88, UL)
{
    AROS_LIBFUNC_INIT
    __log(LOG_CRIT, "ReleaseDomainNameServerList() is not implemented");
    AROS_LIBFUNC_EXIT
}

AROS_LH0(struct List *, ObtainDomainNameServerList,
	struct SocketBase *, libPtr, 89, UL)
{
    AROS_LIBFUNC_INIT
    __log(LOG_CRIT, "ObtainDomainNameServerList() is not implemented");
    return NULL;
    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, setnetent,
	AROS_LHA(int, stayopen, D0),
	struct SocketBase *, libPtr, 90, UL)
{
    AROS_LIBFUNC_INIT
    __log(LOG_CRIT, "setnetent() is not implemented");
    AROS_LIBFUNC_EXIT
}

AROS_LH0(void, endnetent,
	struct SocketBase *, libPtr, 91, UL)
{
    AROS_LIBFUNC_INIT
    __log(LOG_CRIT, "endnetent() is not implemented");
    AROS_LIBFUNC_EXIT
}

AROS_LH0(struct netent *, getnetent,
	struct SocketBase *, libPtr, 92, UL)
{
    AROS_LIBFUNC_INIT
    __log(LOG_CRIT, "getnetent() is not implemented");
    return NULL;
    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, setprotoent,
	AROS_LHA(int, stayopen, D0),
	struct SocketBase *, libPtr, 93, UL)
{
    AROS_LIBFUNC_INIT
    __log(LOG_CRIT, "setprotoent() is not implemented");
    AROS_LIBFUNC_EXIT
}

// endprotoent defined in amiga_ndbent.c
// getprotoent defined in amiga_ndbent.c

AROS_LH1(void, setservent,
	AROS_LHA(int, stayopen, D0),
	struct SocketBase *, libPtr, 96, UL)
{
    AROS_LIBFUNC_INIT
    __log(LOG_CRIT, "setservent() is not implemented");
    AROS_LIBFUNC_EXIT
}

AROS_LH0(void, endservent,
	struct SocketBase *, libPtr, 97, UL)
{
    AROS_LIBFUNC_INIT
    __log(LOG_CRIT, "endservent() is not implemented");
    AROS_LIBFUNC_EXIT
}

AROS_LH0(struct servent *, getservent,
	struct SocketBase *, libPtr, 98, UL)
{
    AROS_LIBFUNC_INIT
    __log(LOG_CRIT, "getservent() is not implemented");
    return NULL;
    AROS_LIBFUNC_EXIT
}

// inet_aton defined in amiga_libcalls.c

#endif

