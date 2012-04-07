#ifndef CLIB_BSDSOCKET_PROTOS_H
#define CLIB_BSDSOCKET_PROTOS_H

/*
    Copyright © 1995-2007, The AROS Development Team. All rights reserved.
*/

#include <aros/libcall.h>
#include <sys/types.h>
#include <sys/select.h>
/* Stub macros for 'emulation' of some functions */
#define select(nfds,rfds,wfds,efds,timeout) WaitSelect(nfds,rfds,wfds,efds,timeout,NULL)
#define inet_ntoa(addr) Inet_NtoA(((struct in_addr)addr).s_addr)
AROS_LP3(int, socket,
         AROS_LPA(int, domain, D0),
         AROS_LPA(int, type, D1),
         AROS_LPA(int, protocol, D2),
         LIBBASETYPEPTR, SocketBase, 5, BSDSocket
);
AROS_LP3(int, bind,
         AROS_LPA(int, s, D0),
         AROS_LPA(struct sockaddr *, name, A0),
         AROS_LPA(int, namelen, D1),
         LIBBASETYPEPTR, SocketBase, 6, BSDSocket
);
AROS_LP2(int, listen,
         AROS_LPA(int, s, D0),
         AROS_LPA(int, backlog, D1),
         LIBBASETYPEPTR, SocketBase, 7, BSDSocket
);
AROS_LP3(int, accept,
         AROS_LPA(int, s, D0),
         AROS_LPA(struct sockaddr *, addr, A0),
         AROS_LPA(socklen_t *, addrlen, A1),
         LIBBASETYPEPTR, SocketBase, 8, BSDSocket
);
AROS_LP3(int, connect,
         AROS_LPA(int, s, D0),
         AROS_LPA(struct sockaddr *, name, A0),
         AROS_LPA(int, namelen, D1),
         LIBBASETYPEPTR, SocketBase, 9, BSDSocket
);
AROS_LP6(int, sendto,
         AROS_LPA(int, s, D0),
         AROS_LPA(const void *, msg, A0),
         AROS_LPA(int, len, D1),
         AROS_LPA(int, flags, D2),
         AROS_LPA(const struct sockaddr *, to, A1),
         AROS_LPA(int, tolen, D3),
         LIBBASETYPEPTR, SocketBase, 10, BSDSocket
);
AROS_LP4(int, send,
         AROS_LPA(int, s, D0),
         AROS_LPA(const void *, msg, A0),
         AROS_LPA(int, len, D1),
         AROS_LPA(int, flags, D2),
         LIBBASETYPEPTR, SocketBase, 11, BSDSocket
);
AROS_LP6(int, recvfrom,
         AROS_LPA(int, s, D0),
         AROS_LPA(void *, buf, A0),
         AROS_LPA(int, len, D1),
         AROS_LPA(int, flags, D2),
         AROS_LPA(struct sockaddr *, from, A1),
         AROS_LPA(int *, fromlen, A2),
         LIBBASETYPEPTR, SocketBase, 12, BSDSocket
);
AROS_LP4(int, recv,
         AROS_LPA(int, s, D0),
         AROS_LPA(void *, buf, A0),
         AROS_LPA(int, len, D1),
         AROS_LPA(int, flags, D2),
         LIBBASETYPEPTR, SocketBase, 13, BSDSocket
);
AROS_LP2(int, shutdown,
         AROS_LPA(int, s, D0),
         AROS_LPA(int, how, D1),
         LIBBASETYPEPTR, SocketBase, 14, BSDSocket
);
AROS_LP5(int, setsockopt,
         AROS_LPA(int, s, D0),
         AROS_LPA(int, level, D1),
         AROS_LPA(int, optname, D2),
         AROS_LPA(void *, optval, A0),
         AROS_LPA(int, optlen, D3),
         LIBBASETYPEPTR, SocketBase, 15, BSDSocket
);
AROS_LP5(int, getsockopt,
         AROS_LPA(int, s, D0),
         AROS_LPA(int, level, D1),
         AROS_LPA(int, optname, D2),
         AROS_LPA(void *, optval, A0),
         AROS_LPA(void *, optlen, A1),
         LIBBASETYPEPTR, SocketBase, 16, BSDSocket
);
AROS_LP3(int, getsockname,
         AROS_LPA(int, s, D0),
         AROS_LPA(struct sockaddr *, name, A0),
         AROS_LPA(int *, namelen, A1),
         LIBBASETYPEPTR, SocketBase, 17, BSDSocket
);
AROS_LP3(int, getpeername,
         AROS_LPA(int, s, D0),
         AROS_LPA(struct sockaddr *, name, A0),
         AROS_LPA(int *, namelen, A1),
         LIBBASETYPEPTR, SocketBase, 18, BSDSocket
);
AROS_LP3(int, IoctlSocket,
         AROS_LPA(int, s, D0),
         AROS_LPA(unsigned long, request, D1),
         AROS_LPA(char *, argp, A0),
         LIBBASETYPEPTR, SocketBase, 19, BSDSocket
);
AROS_LP1(int, CloseSocket,
         AROS_LPA(int, s, D0),
         LIBBASETYPEPTR, SocketBase, 20, BSDSocket
);
AROS_LP6(int, WaitSelect,
         AROS_LPA(int, nfds, D0),
         AROS_LPA(fd_set *, readfds, A0),
         AROS_LPA(fd_set *, writefds, A1),
         AROS_LPA(fd_set *, exceptfds, A2),
         AROS_LPA(struct timeval *, timeout, A3),
         AROS_LPA(ULONG *, sigmask, D1),
         LIBBASETYPEPTR, SocketBase, 21, BSDSocket
);
AROS_LP3(void, SetSocketSignals,
         AROS_LPA(ULONG, intrmask, D0),
         AROS_LPA(ULONG, iomask, D1),
         AROS_LPA(ULONG, urgmask, D2),
         LIBBASETYPEPTR, SocketBase, 22, BSDSocket
);
AROS_LP0(int, getdtablesize,
         LIBBASETYPEPTR, SocketBase, 23, BSDSocket
);
AROS_LP4(LONG, ObtainSocket,
         AROS_LPA(LONG, id, D0),
         AROS_LPA(LONG, domain, D1),
         AROS_LPA(LONG, type, D2),
         AROS_LPA(LONG, protocol, D3),
         LIBBASETYPEPTR, SocketBase, 24, BSDSocket
);
AROS_LP2(LONG, ReleaseSocket,
         AROS_LPA(LONG, sd, D0),
         AROS_LPA(LONG, id, D1),
         LIBBASETYPEPTR, SocketBase, 25, BSDSocket
);
AROS_LP2(LONG, ReleaseCopyOfSocket,
         AROS_LPA(LONG, sd, D0),
         AROS_LPA(LONG, id, D1),
         LIBBASETYPEPTR, SocketBase, 26, BSDSocket
);
AROS_LP0(LONG, Errno,
         LIBBASETYPEPTR, SocketBase, 27, BSDSocket
);
AROS_LP2(void, SetErrnoPtr,
         AROS_LPA(void *, ptr, A0),
         AROS_LPA(int, size, D0),
         LIBBASETYPEPTR, SocketBase, 28, BSDSocket
);
AROS_LP1(char *, Inet_NtoA,
         AROS_LPA(unsigned long, in, D0),
         LIBBASETYPEPTR, SocketBase, 29, BSDSocket
);
AROS_LP1(unsigned long, inet_addr,
         AROS_LPA(const char *, cp, A0),
         LIBBASETYPEPTR, SocketBase, 30, BSDSocket
);
AROS_LP1(unsigned long, Inet_LnaOf,
         AROS_LPA(unsigned long, in, D0),
         LIBBASETYPEPTR, SocketBase, 31, BSDSocket
);
AROS_LP1(unsigned long, Inet_NetOf,
         AROS_LPA(unsigned long, in, D0),
         LIBBASETYPEPTR, SocketBase, 32, BSDSocket
);
AROS_LP2(unsigned long, Inet_MakeAddr,
         AROS_LPA(int, net, D0),
         AROS_LPA(int, lna, D1),
         LIBBASETYPEPTR, SocketBase, 33, BSDSocket
);
AROS_LP1(unsigned long, inet_network,
         AROS_LPA(const char *, cp, A0),
         LIBBASETYPEPTR, SocketBase, 34, BSDSocket
);
AROS_LP1(struct hostent *, gethostbyname,
         AROS_LPA(const char *, name, A0),
         LIBBASETYPEPTR, SocketBase, 35, BSDSocket
);
AROS_LP3(struct hostent *, gethostbyaddr,
         AROS_LPA(const void *, addr, A0),
         AROS_LPA(int, len, D0),
         AROS_LPA(int, type, D1),
         LIBBASETYPEPTR, SocketBase, 36, BSDSocket
);
AROS_LP1(struct netent *, getnetbyname,
         AROS_LPA(const char *, name, A0),
         LIBBASETYPEPTR, SocketBase, 37, BSDSocket
);
AROS_LP2(struct netent *, getnetbyaddr,
         AROS_LPA(long, net, D0),
         AROS_LPA(int, type, D1),
         LIBBASETYPEPTR, SocketBase, 38, BSDSocket
);
AROS_LP2(struct servent *, getservbyname,
         AROS_LPA(char *, name, A0),
         AROS_LPA(char *, proto, A1),
         LIBBASETYPEPTR, SocketBase, 39, BSDSocket
);
AROS_LP2(struct servent *, getservbyport,
         AROS_LPA(int, port, D0),
         AROS_LPA(char *, proto, A0),
         LIBBASETYPEPTR, SocketBase, 40, BSDSocket
);
AROS_LP1(struct protoent *, getprotobyname,
         AROS_LPA(char *, name, A0),
         LIBBASETYPEPTR, SocketBase, 41, BSDSocket
);
AROS_LP1(struct protoent *, getprotobynumber,
         AROS_LPA(int, proto, D0),
         LIBBASETYPEPTR, SocketBase, 42, BSDSocket
);
AROS_LP3(void, vsyslog,
         AROS_LPA(int, level, D0),
         AROS_LPA(const char *, format, A0),
         AROS_LPA(IPTR *, args, A1),
         LIBBASETYPEPTR, SocketBase, 43, BSDSocket
);
AROS_LP2(int, Dup2Socket,
         AROS_LPA(int, fd1, D0),
         AROS_LPA(int, fd2, D1),
         LIBBASETYPEPTR, SocketBase, 44, BSDSocket
);
AROS_LP3(int, sendmsg,
         AROS_LPA(int, s, D0),
         AROS_LPA(const struct msghdr *, msg, A0),
         AROS_LPA(int, flags, D1),
         LIBBASETYPEPTR, SocketBase, 45, BSDSocket
);
AROS_LP3(int, recvmsg,
         AROS_LPA(int, s, D0),
         AROS_LPA(struct msghdr *, msg, A0),
         AROS_LPA(int, flags, D1),
         LIBBASETYPEPTR, SocketBase, 46, BSDSocket
);
AROS_LP2(int, gethostname,
         AROS_LPA(char *, name, A0),
         AROS_LPA(int, namelen, D0),
         LIBBASETYPEPTR, SocketBase, 47, BSDSocket
);
AROS_LP0(long, gethostid,
         LIBBASETYPEPTR, SocketBase, 48, BSDSocket
);
AROS_LP1(ULONG, SocketBaseTagList,
         AROS_LPA(struct TagItem *, tagList, A0),
         LIBBASETYPEPTR, SocketBase, 49, BSDSocket
);
AROS_LP1(LONG, GetSocketEvents,
         AROS_LPA(ULONG *, eventsp, A0),
         LIBBASETYPEPTR, SocketBase, 50, BSDSocket
);

/* RoadShow Extensions .. */
#if defined(__CONFIG_ROADSHOW__)
AROS_LP1(long, bpf_open,
         AROS_LPA(long, channel, D0),
         LIBBASETYPEPTR, SocketBase, 57, BSDSocket
);
AROS_LP1(long, bpf_close,
         AROS_LPA(long, handle, D0),
         LIBBASETYPEPTR, SocketBase, 58, BSDSocket
);
AROS_LP3(long, bpf_read,
         AROS_LPA(long, handle, D0),
         AROS_LPA(void *, buffer, A0),
         AROS_LPA(long, len, D1),
         LIBBASETYPEPTR, SocketBase, 59, BSDSocket
);
AROS_LP3(long, bpf_write,
         AROS_LPA(long, handle, D0),
         AROS_LPA(void *, buffer, A0),
         AROS_LPA(long, len, D1),
         LIBBASETYPEPTR, SocketBase, 60, BSDSocket
);
AROS_LP2(long, bpf_set_notify_mask,
         AROS_LPA(long, handle, D0),
         AROS_LPA(unsigned long, mask, D1),
         LIBBASETYPEPTR, SocketBase, 61, BSDSocket
);
AROS_LP2(long, bpf_set_interrupt_mask,
         AROS_LPA(long, handle, D0),
         AROS_LPA(unsigned long, mask, D1),
         LIBBASETYPEPTR, SocketBase, 62, BSDSocket
);
AROS_LP3(long, bpf_ioctl,
         AROS_LPA(long, handle, D0),
         AROS_LPA(unsigned long, request, D1),
         AROS_LPA(char *, argp, A0),
         LIBBASETYPEPTR, SocketBase, 63, BSDSocket
);
AROS_LP1(long, bpf_data_waiting,
         AROS_LPA(long, handle, D0),
         LIBBASETYPEPTR, SocketBase, 64, BSDSocket
);
AROS_LP1(long, AddRouteTagList,
         AROS_LPA(struct TagItem *, tags, A0),
         LIBBASETYPEPTR, SocketBase, 65, BSDSocket
);
AROS_LP1(long, DeleteRouteTagList,
         AROS_LPA(struct TagItem *, tags, A0),
         LIBBASETYPEPTR, SocketBase, 66, BSDSocket
);
AROS_LP1(long, ChangeRouteTagList,
         AROS_LPA(struct TagItem *, tags, A0),
         LIBBASETYPEPTR, SocketBase, 67, BSDSocket
);
AROS_LP1I(void, FreeRouteInfo,
         AROS_LPA(struct rt_msghdr *, table, A0),
         LIBBASETYPEPTR, SocketBase, 68, BSDSocket
);
AROS_LP2(struct rt_msghdr *, GetRouteInfo,
         AROS_LPA(LONG, address_family, D0),
         AROS_LPA(LONG, flags, D1),
         LIBBASETYPEPTR, SocketBase, 69, BSDSocket
);
AROS_LP4(long, AddInterfaceTagList,
         AROS_LPA(STRPTR, name, A0),
         AROS_LPA(STRPTR, device, A1),
         AROS_LPA(long, unit, D0),
         AROS_LPA(struct TagItem *, tags, A2),
         LIBBASETYPEPTR, SocketBase, 70, BSDSocket
);
AROS_LP2(long, ConfigureInterfaceTagList,
         AROS_LPA(STRPTR, name, A0),
         AROS_LPA(struct TagItem *, tags, A1),
         LIBBASETYPEPTR, SocketBase, 71, BSDSocket
);
AROS_LP1I(void, ReleaseInterfaceList,
         AROS_LPA(struct List *, list, A0),
         LIBBASETYPEPTR, SocketBase, 72, BSDSocket
);
AROS_LP0I(struct List *, ObtainInterfaceList,
         LIBBASETYPEPTR, SocketBase, 73, BSDSocket
);
AROS_LP2(long, QueryInterfaceTagList,
         AROS_LPA(STRPTR, name, A0),
         AROS_LPA(struct TagItem *, tags, A1),
         LIBBASETYPEPTR, SocketBase, 74, BSDSocket
);
AROS_LP5I(LONG, CreateAddrAllocMessageA,
         AROS_LPA(LONG, version, D0),
         AROS_LPA(LONG, protocol, D1),
         AROS_LPA(STRPTR, interface_name, A0),
         AROS_LPA(struct AddressAllocationMessage *, result_ptr, A1),
         AROS_LPA(struct TagItem *, tags, A2),
         LIBBASETYPEPTR, SocketBase, 75, BSDSocket
);
AROS_LP1I(void, DeleteAddrAllocMessage,
         AROS_LPA(struct AddressAllocationMessage *, message, A0),
         LIBBASETYPEPTR, SocketBase, 76, BSDSocket
);
AROS_LP1I(void, BeginInterfaceConfig,
         AROS_LPA(struct AddressAllocationMessage *, message, A0),
         LIBBASETYPEPTR, SocketBase, 77, BSDSocket
);
AROS_LP1I(void, AbortInterfaceConfig,
         AROS_LPA(struct AddressAllocationMessage *, message, A0),
         LIBBASETYPEPTR, SocketBase, 78, BSDSocket
);
AROS_LP3(long, AddNetMonitorHookTagList,
         AROS_LPA(long, type, D0),
         AROS_LPA(struct Hook *, hook, A0),
         AROS_LPA(struct TagItem *, tags, A1),
         LIBBASETYPEPTR, SocketBase, 79, BSDSocket
);
AROS_LP1I(void, RemoveNetMonitorHook,
         AROS_LPA(struct Hook *, hook, A0),
         LIBBASETYPEPTR, SocketBase, 80, BSDSocket
);
AROS_LP4(LONG, GetNetworkStatistics,
         AROS_LPA(LONG, type, D0),
         AROS_LPA(LONG, version, D1),
         AROS_LPA(APTR, destination, A0),
         AROS_LPA(LONG, size, D2),
         LIBBASETYPEPTR, SocketBase, 81, BSDSocket
);
AROS_LP1(LONG, AddDomainNameServer,
         AROS_LPA(STRPTR, address, A0),
         LIBBASETYPEPTR, SocketBase, 82, BSDSocket
);
AROS_LP1(LONG, RemoveDomainNameServer,
         AROS_LPA(STRPTR, address, A0),
         LIBBASETYPEPTR, SocketBase, 83, BSDSocket
);
AROS_LP1I(void, ReleaseDomainNameServerList,
         AROS_LPA(struct List *, list, A0),
         LIBBASETYPEPTR, SocketBase, 84, BSDSocket
);
AROS_LP0I(struct List *, ObtainDomainNameServerList,
         LIBBASETYPEPTR, SocketBase, 85, BSDSocket
);
AROS_LP1I(void, setnetent,
         AROS_LPA(int, stayopen, D0),
         LIBBASETYPEPTR, SocketBase, 86, BSDSocket
);
AROS_LP0I(void, endnetent,
         LIBBASETYPEPTR, SocketBase, 87, BSDSocket
);
AROS_LP0I(struct netent *, getnetent,
         LIBBASETYPEPTR, SocketBase, 88, BSDSocket
);
AROS_LP1I(void, setprotoent,
         AROS_LPA(int, stayopen, D0),
         LIBBASETYPEPTR, SocketBase, 89, BSDSocket
);
AROS_LP0(void, endprotoent,
         LIBBASETYPEPTR, SocketBase, 90, BSDSocket
);
AROS_LP0(struct protoent *, getprotoent,
         LIBBASETYPEPTR, SocketBase, 91, BSDSocket
);
AROS_LP1I(void, setservent,
         AROS_LPA(int, stayopen, D0),
         LIBBASETYPEPTR, SocketBase, 92, BSDSocket
);
AROS_LP0I(void, endservent,
         LIBBASETYPEPTR, SocketBase, 93, BSDSocket
);
AROS_LP0I(struct servent *, getservent,
         LIBBASETYPEPTR, SocketBase, 94, BSDSocket
);
AROS_LP2(LONG, inet_aton,
         AROS_LPA(STRPTR, cp, A0),
         AROS_LPA(struct in_addr *, addr, A1),
         LIBBASETYPEPTR, SocketBase, 95, BSDSocket
);
#endif /* __CONFIG_ROADSHOW__ */
#endif /* CLIB_BSDSOCKET_PROTOS_H */
