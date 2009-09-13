#ifndef DEFINES_BSDSOCKET_PROTOS_H
#define DEFINES_BSDSOCKET_PROTOS_H

/*
    Copyright © 1995-2007, The AROS Development Team. All rights reserved.
*/

/*
    Desc: Defines for bsdsocket
*/

#include <aros/libcall.h>
#include <exec/types.h>
#include <aros/preprocessor/variadic/cast2iptr.hpp>


#define __socket_WB(__SocketBase, __arg1, __arg2, __arg3) \
        AROS_LC3(int, socket, \
                  AROS_LCA(int,(__arg1),D0), \
                  AROS_LCA(int,(__arg2),D1), \
                  AROS_LCA(int,(__arg3),D2), \
        struct Library *, (__SocketBase), 5, BSDSocket)

#define socket(arg1, arg2, arg3) \
    __socket_WB(SocketBase, (arg1), (arg2), (arg3))

#define __bind_WB(__SocketBase, __arg1, __arg2, __arg3) \
        AROS_LC3(int, bind, \
                  AROS_LCA(int,(__arg1),D0), \
                  AROS_LCA(struct sockaddr *,(__arg2),A0), \
                  AROS_LCA(int,(__arg3),D1), \
        struct Library *, (__SocketBase), 6, BSDSocket)

#define bind(arg1, arg2, arg3) \
    __bind_WB(SocketBase, (arg1), (arg2), (arg3))

#define __listen_WB(__SocketBase, __arg1, __arg2) \
        AROS_LC2(int, listen, \
                  AROS_LCA(int,(__arg1),D0), \
                  AROS_LCA(int,(__arg2),D1), \
        struct Library *, (__SocketBase), 7, BSDSocket)

#define listen(arg1, arg2) \
    __listen_WB(SocketBase, (arg1), (arg2))

#define __accept_WB(__SocketBase, __arg1, __arg2, __arg3) \
        AROS_LC3(int, accept, \
                  AROS_LCA(int,(__arg1),D0), \
                  AROS_LCA(struct sockaddr *,(__arg2),A0), \
                  AROS_LCA(socklen_t *,(__arg3),A1), \
        struct Library *, (__SocketBase), 8, BSDSocket)

#define accept(arg1, arg2, arg3) \
    __accept_WB(SocketBase, (arg1), (arg2), (arg3))

#define __connect_WB(__SocketBase, __arg1, __arg2, __arg3) \
        AROS_LC3(int, connect, \
                  AROS_LCA(int,(__arg1),D0), \
                  AROS_LCA(struct sockaddr *,(__arg2),A0), \
                  AROS_LCA(int,(__arg3),D1), \
        struct Library *, (__SocketBase), 9, BSDSocket)

#define connect(arg1, arg2, arg3) \
    __connect_WB(SocketBase, (arg1), (arg2), (arg3))

#define __sendto_WB(__SocketBase, __arg1, __arg2, __arg3, __arg4, __arg5, __arg6) \
        AROS_LC6(int, sendto, \
                  AROS_LCA(int,(__arg1),D0), \
                  AROS_LCA(const void *,(__arg2),A0), \
                  AROS_LCA(int,(__arg3),D1), \
                  AROS_LCA(int,(__arg4),D2), \
                  AROS_LCA(const struct sockaddr *,(__arg5),A1), \
                  AROS_LCA(int,(__arg6),D3), \
        struct Library *, (__SocketBase), 10, BSDSocket)

#define sendto(arg1, arg2, arg3, arg4, arg5, arg6) \
    __sendto_WB(SocketBase, (arg1), (arg2), (arg3), (arg4), (arg5), (arg6))

#define __send_WB(__SocketBase, __arg1, __arg2, __arg3, __arg4) \
        AROS_LC4(int, send, \
                  AROS_LCA(int,(__arg1),D0), \
                  AROS_LCA(const void *,(__arg2),A0), \
                  AROS_LCA(int,(__arg3),D1), \
                  AROS_LCA(int,(__arg4),D2), \
        struct Library *, (__SocketBase), 11, BSDSocket)

#define send(arg1, arg2, arg3, arg4) \
    __send_WB(SocketBase, (arg1), (arg2), (arg3), (arg4))

#define __recvfrom_WB(__SocketBase, __arg1, __arg2, __arg3, __arg4, __arg5, __arg6) \
        AROS_LC6(int, recvfrom, \
                  AROS_LCA(int,(__arg1),D0), \
                  AROS_LCA(void *,(__arg2),A0), \
                  AROS_LCA(int,(__arg3),D1), \
                  AROS_LCA(int,(__arg4),D2), \
                  AROS_LCA(struct sockaddr *,(__arg5),A1), \
                  AROS_LCA(int *,(__arg6),A2), \
        struct Library *, (__SocketBase), 12, BSDSocket)

#define recvfrom(arg1, arg2, arg3, arg4, arg5, arg6) \
    __recvfrom_WB(SocketBase, (arg1), (arg2), (arg3), (arg4), (arg5), (arg6))

#define __recv_WB(__SocketBase, __arg1, __arg2, __arg3, __arg4) \
        AROS_LC4(int, recv, \
                  AROS_LCA(int,(__arg1),D0), \
                  AROS_LCA(void *,(__arg2),A0), \
                  AROS_LCA(int,(__arg3),D1), \
                  AROS_LCA(int,(__arg4),D2), \
        struct Library *, (__SocketBase), 13, BSDSocket)

#define recv(arg1, arg2, arg3, arg4) \
    __recv_WB(SocketBase, (arg1), (arg2), (arg3), (arg4))

#define __shutdown_WB(__SocketBase, __arg1, __arg2) \
        AROS_LC2(int, shutdown, \
                  AROS_LCA(int,(__arg1),D0), \
                  AROS_LCA(int,(__arg2),D1), \
        struct Library *, (__SocketBase), 14, BSDSocket)

#define shutdown(arg1, arg2) \
    __shutdown_WB(SocketBase, (arg1), (arg2))

#define __setsockopt_WB(__SocketBase, __arg1, __arg2, __arg3, __arg4, __arg5) \
        AROS_LC5(int, setsockopt, \
                  AROS_LCA(int,(__arg1),D0), \
                  AROS_LCA(int,(__arg2),D1), \
                  AROS_LCA(int,(__arg3),D2), \
                  AROS_LCA(void *,(__arg4),A0), \
                  AROS_LCA(int,(__arg5),D3), \
        struct Library *, (__SocketBase), 15, BSDSocket)

#define setsockopt(arg1, arg2, arg3, arg4, arg5) \
    __setsockopt_WB(SocketBase, (arg1), (arg2), (arg3), (arg4), (arg5))

#define __getsockopt_WB(__SocketBase, __arg1, __arg2, __arg3, __arg4, __arg5) \
        AROS_LC5(int, getsockopt, \
                  AROS_LCA(int,(__arg1),D0), \
                  AROS_LCA(int,(__arg2),D1), \
                  AROS_LCA(int,(__arg3),D2), \
                  AROS_LCA(void *,(__arg4),A0), \
                  AROS_LCA(void *,(__arg5),A1), \
        struct Library *, (__SocketBase), 16, BSDSocket)

#define getsockopt(arg1, arg2, arg3, arg4, arg5) \
    __getsockopt_WB(SocketBase, (arg1), (arg2), (arg3), (arg4), (arg5))

#define __getsockname_WB(__SocketBase, __arg1, __arg2, __arg3) \
        AROS_LC3(int, getsockname, \
                  AROS_LCA(int,(__arg1),D0), \
                  AROS_LCA(struct sockaddr *,(__arg2),A0), \
                  AROS_LCA(int *,(__arg3),A1), \
        struct Library *, (__SocketBase), 17, BSDSocket)

#define getsockname(arg1, arg2, arg3) \
    __getsockname_WB(SocketBase, (arg1), (arg2), (arg3))

#define __getpeername_WB(__SocketBase, __arg1, __arg2, __arg3) \
        AROS_LC3(int, getpeername, \
                  AROS_LCA(int,(__arg1),D0), \
                  AROS_LCA(struct sockaddr *,(__arg2),A0), \
                  AROS_LCA(int *,(__arg3),A1), \
        struct Library *, (__SocketBase), 18, BSDSocket)

#define getpeername(arg1, arg2, arg3) \
    __getpeername_WB(SocketBase, (arg1), (arg2), (arg3))

#define __IoctlSocket_WB(__SocketBase, __arg1, __arg2, __arg3) \
        AROS_LC3(int, IoctlSocket, \
                  AROS_LCA(int,(__arg1),D0), \
                  AROS_LCA(unsigned long,(__arg2),D1), \
                  AROS_LCA(char *,(__arg3),A0), \
        struct Library *, (__SocketBase), 19, BSDSocket)

#define IoctlSocket(arg1, arg2, arg3) \
    __IoctlSocket_WB(SocketBase, (arg1), (arg2), (arg3))

#define __CloseSocket_WB(__SocketBase, __arg1) \
        AROS_LC1(int, CloseSocket, \
                  AROS_LCA(int,(__arg1),D0), \
        struct Library *, (__SocketBase), 20, BSDSocket)

#define CloseSocket(arg1) \
    __CloseSocket_WB(SocketBase, (arg1))

#define __WaitSelect_WB(__SocketBase, __arg1, __arg2, __arg3, __arg4, __arg5, __arg6) \
        AROS_LC6(int, WaitSelect, \
                  AROS_LCA(int,(__arg1),D0), \
                  AROS_LCA(fd_set *,(__arg2),A0), \
                  AROS_LCA(fd_set *,(__arg3),A1), \
                  AROS_LCA(fd_set *,(__arg4),A2), \
                  AROS_LCA(struct timeval *,(__arg5),A3), \
                  AROS_LCA(ULONG *,(__arg6),D1), \
        struct Library *, (__SocketBase), 21, BSDSocket)

#define WaitSelect(arg1, arg2, arg3, arg4, arg5, arg6) \
    __WaitSelect_WB(SocketBase, (arg1), (arg2), (arg3), (arg4), (arg5), (arg6))

#define __SetSocketSignals_WB(__SocketBase, __arg1, __arg2, __arg3) \
        AROS_LC3NR(void, SetSocketSignals, \
                  AROS_LCA(ULONG,(__arg1),D0), \
                  AROS_LCA(ULONG,(__arg2),D1), \
                  AROS_LCA(ULONG,(__arg3),D2), \
        struct Library *, (__SocketBase), 22, BSDSocket)

#define SetSocketSignals(arg1, arg2, arg3) \
    __SetSocketSignals_WB(SocketBase, (arg1), (arg2), (arg3))

#define __getdtablesize_WB(__SocketBase) \
        AROS_LC0(int, getdtablesize, \
        struct Library *, (__SocketBase), 23, BSDSocket)

#define getdtablesize() \
    __getdtablesize_WB(SocketBase)

#define __ObtainSocket_WB(__SocketBase, __arg1, __arg2, __arg3, __arg4) \
        AROS_LC4(LONG, ObtainSocket, \
                  AROS_LCA(LONG,(__arg1),D0), \
                  AROS_LCA(LONG,(__arg2),D1), \
                  AROS_LCA(LONG,(__arg3),D2), \
                  AROS_LCA(LONG,(__arg4),D3), \
        struct Library *, (__SocketBase), 24, BSDSocket)

#define ObtainSocket(arg1, arg2, arg3, arg4) \
    __ObtainSocket_WB(SocketBase, (arg1), (arg2), (arg3), (arg4))

#define __ReleaseSocket_WB(__SocketBase, __arg1, __arg2) \
        AROS_LC2(LONG, ReleaseSocket, \
                  AROS_LCA(LONG,(__arg1),D0), \
                  AROS_LCA(LONG,(__arg2),D1), \
        struct Library *, (__SocketBase), 25, BSDSocket)

#define ReleaseSocket(arg1, arg2) \
    __ReleaseSocket_WB(SocketBase, (arg1), (arg2))

#define __ReleaseCopyOfSocket_WB(__SocketBase, __arg1, __arg2) \
        AROS_LC2(LONG, ReleaseCopyOfSocket, \
                  AROS_LCA(LONG,(__arg1),D0), \
                  AROS_LCA(LONG,(__arg2),D1), \
        struct Library *, (__SocketBase), 26, BSDSocket)

#define ReleaseCopyOfSocket(arg1, arg2) \
    __ReleaseCopyOfSocket_WB(SocketBase, (arg1), (arg2))

#define __Errno_WB(__SocketBase) \
        AROS_LC0(LONG, Errno, \
        struct Library *, (__SocketBase), 27, BSDSocket)

#define Errno() \
    __Errno_WB(SocketBase)

#define __SetErrnoPtr_WB(__SocketBase, __arg1, __arg2) \
        AROS_LC2NR(void, SetErrnoPtr, \
                  AROS_LCA(void *,(__arg1),A0), \
                  AROS_LCA(int,(__arg2),D0), \
        struct Library *, (__SocketBase), 28, BSDSocket)

#define SetErrnoPtr(arg1, arg2) \
    __SetErrnoPtr_WB(SocketBase, (arg1), (arg2))

#define __Inet_NtoA_WB(__SocketBase, __arg1) \
        AROS_LC1(char *, Inet_NtoA, \
                  AROS_LCA(unsigned long,(__arg1),D0), \
        struct Library *, (__SocketBase), 29, BSDSocket)

#define Inet_NtoA(arg1) \
    __Inet_NtoA_WB(SocketBase, (arg1))

#if !defined(NO_INLINE_STDARG) && !defined(BSDSOCKET_NO_INLINE_STDARG)
#define Inet_Nto(...) \
({ \
    IPTR __args[] = { AROS_PP_VARIADIC_CAST2IPTR(__VA_ARGS__) }; \
    Inet_NtoA((unsigned long)__args); \
})
#endif /* !NO_INLINE_STDARG */

#define __inet_addr_WB(__SocketBase, __arg1) \
        AROS_LC1(unsigned long, inet_addr, \
                  AROS_LCA(const char *,(__arg1),A0), \
        struct Library *, (__SocketBase), 30, BSDSocket)

#define inet_addr(arg1) \
    __inet_addr_WB(SocketBase, (arg1))

#define __Inet_LnaOf_WB(__SocketBase, __arg1) \
        AROS_LC1(unsigned long, Inet_LnaOf, \
                  AROS_LCA(unsigned long,(__arg1),D0), \
        struct Library *, (__SocketBase), 31, BSDSocket)

#define Inet_LnaOf(arg1) \
    __Inet_LnaOf_WB(SocketBase, (arg1))

#define __Inet_NetOf_WB(__SocketBase, __arg1) \
        AROS_LC1(unsigned long, Inet_NetOf, \
                  AROS_LCA(unsigned long,(__arg1),D0), \
        struct Library *, (__SocketBase), 32, BSDSocket)

#define Inet_NetOf(arg1) \
    __Inet_NetOf_WB(SocketBase, (arg1))

#define __Inet_MakeAddr_WB(__SocketBase, __arg1, __arg2) \
        AROS_LC2(unsigned long, Inet_MakeAddr, \
                  AROS_LCA(int,(__arg1),D0), \
                  AROS_LCA(int,(__arg2),D1), \
        struct Library *, (__SocketBase), 33, BSDSocket)

#define Inet_MakeAddr(arg1, arg2) \
    __Inet_MakeAddr_WB(SocketBase, (arg1), (arg2))

#define __inet_network_WB(__SocketBase, __arg1) \
        AROS_LC1(unsigned long, inet_network, \
                  AROS_LCA(const char *,(__arg1),A0), \
        struct Library *, (__SocketBase), 34, BSDSocket)

#define inet_network(arg1) \
    __inet_network_WB(SocketBase, (arg1))

#define __gethostbyname_WB(__SocketBase, __arg1) \
        AROS_LC1(struct hostent *, gethostbyname, \
                  AROS_LCA(char *,(__arg1),A0), \
        struct Library *, (__SocketBase), 35, BSDSocket)

#define gethostbyname(arg1) \
    __gethostbyname_WB(SocketBase, (arg1))

#define __gethostbyaddr_WB(__SocketBase, __arg1, __arg2, __arg3) \
        AROS_LC3(struct hostent *, gethostbyaddr, \
                  AROS_LCA(char *,(__arg1),A0), \
                  AROS_LCA(int,(__arg2),D0), \
                  AROS_LCA(int,(__arg3),D1), \
        struct Library *, (__SocketBase), 36, BSDSocket)

#define gethostbyaddr(arg1, arg2, arg3) \
    __gethostbyaddr_WB(SocketBase, (arg1), (arg2), (arg3))

#define __getnetbyname_WB(__SocketBase, __arg1) \
        AROS_LC1(struct netent *, getnetbyname, \
                  AROS_LCA(char *,(__arg1),A0), \
        struct Library *, (__SocketBase), 37, BSDSocket)

#define getnetbyname(arg1) \
    __getnetbyname_WB(SocketBase, (arg1))

#define __getnetbyaddr_WB(__SocketBase, __arg1, __arg2) \
        AROS_LC2(struct netent *, getnetbyaddr, \
                  AROS_LCA(long,(__arg1),D0), \
                  AROS_LCA(int,(__arg2),D1), \
        struct Library *, (__SocketBase), 38, BSDSocket)

#define getnetbyaddr(arg1, arg2) \
    __getnetbyaddr_WB(SocketBase, (arg1), (arg2))

#define __getservbyname_WB(__SocketBase, __arg1, __arg2) \
        AROS_LC2(struct servent *, getservbyname, \
                  AROS_LCA(char *,(__arg1),A0), \
                  AROS_LCA(char *,(__arg2),A1), \
        struct Library *, (__SocketBase), 39, BSDSocket)

#define getservbyname(arg1, arg2) \
    __getservbyname_WB(SocketBase, (arg1), (arg2))

#define __getservbyport_WB(__SocketBase, __arg1, __arg2) \
        AROS_LC2(struct servent *, getservbyport, \
                  AROS_LCA(int,(__arg1),D0), \
                  AROS_LCA(char *,(__arg2),A0), \
        struct Library *, (__SocketBase), 40, BSDSocket)

#define getservbyport(arg1, arg2) \
    __getservbyport_WB(SocketBase, (arg1), (arg2))

#define __getprotobyname_WB(__SocketBase, __arg1) \
        AROS_LC1(struct protoent *, getprotobyname, \
                  AROS_LCA(char *,(__arg1),A0), \
        struct Library *, (__SocketBase), 41, BSDSocket)

#define getprotobyname(arg1) \
    __getprotobyname_WB(SocketBase, (arg1))

#define __getprotobynumber_WB(__SocketBase, __arg1) \
        AROS_LC1(struct protoent *, getprotobynumber, \
                  AROS_LCA(int,(__arg1),D0), \
        struct Library *, (__SocketBase), 42, BSDSocket)

#define getprotobynumber(arg1) \
    __getprotobynumber_WB(SocketBase, (arg1))

#define __vsyslog_WB(__SocketBase, __arg1, __arg2, __arg3) \
        AROS_LC3NR(void, vsyslog, \
                  AROS_LCA(int,(__arg1),D0), \
                  AROS_LCA(const char *,(__arg2),A0), \
                  AROS_LCA(LONG *,(__arg3),A1), \
        struct Library *, (__SocketBase), 43, BSDSocket)

#define vsyslog(arg1, arg2, arg3) \
    __vsyslog_WB(SocketBase, (arg1), (arg2), (arg3))

#if !defined(NO_INLINE_STDARG) && !defined(BSDSOCKET_NO_INLINE_STDARG)
#define syslog(arg1, arg2, ...) \
({ \
    IPTR __args[] = { AROS_PP_VARIADIC_CAST2IPTR(__VA_ARGS__) }; \
    vsyslog(arg1, arg2, __args); \
})
#endif /* !NO_INLINE_STDARG */


#define __Dup2Socket_WB(__SocketBase, __arg1, __arg2) \
        AROS_LC2(int, Dup2Socket, \
                  AROS_LCA(int,(__arg1),D0), \
                  AROS_LCA(int,(__arg2),D1), \
        struct Library *, (__SocketBase), 44, BSDSocket)

#define Dup2Socket(arg1, arg2) \
    __Dup2Socket_WB(SocketBase, (arg1), (arg2))

#define __sendmsg_WB(__SocketBase, __arg1, __arg2, __arg3) \
        AROS_LC3(int, sendmsg, \
                  AROS_LCA(int,(__arg1),D0), \
                  AROS_LCA(const struct msghdr *,(__arg2),A0), \
                  AROS_LCA(int,(__arg3),D1), \
        struct Library *, (__SocketBase), 45, BSDSocket)

#define sendmsg(arg1, arg2, arg3) \
    __sendmsg_WB(SocketBase, (arg1), (arg2), (arg3))

#define __recvmsg_WB(__SocketBase, __arg1, __arg2, __arg3) \
        AROS_LC3(int, recvmsg, \
                  AROS_LCA(int,(__arg1),D0), \
                  AROS_LCA(struct msghdr *,(__arg2),A0), \
                  AROS_LCA(int,(__arg3),D1), \
        struct Library *, (__SocketBase), 46, BSDSocket)

#define recvmsg(arg1, arg2, arg3) \
    __recvmsg_WB(SocketBase, (arg1), (arg2), (arg3))

#define __gethostname_WB(__SocketBase, __arg1, __arg2) \
        AROS_LC2(int, gethostname, \
                  AROS_LCA(char *,(__arg1),A0), \
                  AROS_LCA(int,(__arg2),D0), \
        struct Library *, (__SocketBase), 47, BSDSocket)

#define gethostname(arg1, arg2) \
    __gethostname_WB(SocketBase, (arg1), (arg2))

#define __gethostid_WB(__SocketBase) \
        AROS_LC0(long, gethostid, \
        struct Library *, (__SocketBase), 48, BSDSocket)

#define gethostid() \
    __gethostid_WB(SocketBase)

#define __SocketBaseTagList_WB(__SocketBase, __arg1) \
        AROS_LC1(ULONG, SocketBaseTagList, \
                  AROS_LCA(struct TagItem *,(__arg1),A0), \
        struct Library *, (__SocketBase), 49, BSDSocket)

#define SocketBaseTagList(arg1) \
    __SocketBaseTagList_WB(SocketBase, (arg1))

#if !defined(NO_INLINE_STDARG) && !defined(BSDSOCKET_NO_INLINE_STDARG)
#define SocketBaseTags(...) \
({ \
    IPTR __args[] = { AROS_PP_VARIADIC_CAST2IPTR(__VA_ARGS__) }; \
    SocketBaseTagList((struct TagItem *)__args); \
})
#endif /* !NO_INLINE_STDARG */

#define __GetSocketEvents_WB(__SocketBase, __arg1) \
        AROS_LC1(LONG, GetSocketEvents, \
                  AROS_LCA(ULONG *,(__arg1),A0), \
        struct Library *, (__SocketBase), 50, BSDSocket)

#define GetSocketEvents(arg1) \
    __GetSocketEvents_WB(SocketBase, (arg1))

#if defined(__CONFIG_ROADSHOW__)
/* RoadShow Extensions .. */

/*
        AROS_LC1(long, bpf_open, \
                  AROS_LCA(long, (__arg1), D0), \
        struct Library *, (__SocketBase), 57, BSDSocket)

        AROS_LC1(long, bpf_close, \
                  AROS_LCA(long, (__arg1), D0), \
        struct Library *, (__SocketBase), 58, BSDSocket)

        AROS_LC3(long, bpf_read, \
                  AROS_LCA(long, (__arg1), D0), \
                  AROS_LCA(void *, (__arg2), A0), \
                  AROS_LCA(long, (__arg3), D1), \
        struct Library *, (__SocketBase), 59, BSDSocket)

        AROS_LC3(long, bpf_write, \
                  AROS_LCA(long, (__arg1), D0), \
                  AROS_LCA(void *, (__arg2), A0), \
                  AROS_LCA(long, (__arg3), D1), \
        struct Library *, (__SocketBase), 60, BSDSocket)

        AROS_LC2(long, bpf_set_notify_mask, \
                  AROS_LCA(long, (__arg1), D0), \
                  AROS_LCA(unsigned long, (__arg2), D1), \
        struct Library *, (__SocketBase), 61, BSDSocket)

        AROS_LC2(long, bpf_set_interrupt_mask, \
                  AROS_LCA(long, (__arg1), D0), \
                  AROS_LCA(unsigned long, (__arg2), D1), \
        struct Library *, (__SocketBase), 62, BSDSocket)

        AROS_LC3(long, bpf_ioctl, \
                  AROS_LCA(long, (__arg1), D0), \
                  AROS_LCA(unsigned long, (__arg2), D1), \
                  AROS_LCA(char *, (__arg3), A0), \
        struct Library *, (__SocketBase), 63, BSDSocket)

        AROS_LC1(long, bpf_data_waiting, \
                  AROS_LCA(long, (__arg1), D0), \
        struct Library *, (__SocketBase), 64, BSDSocket)

        AROS_LC1(long, AddRouteTagList, \
                  AROS_LCA(struct TagItem *, (__arg1), A0), \
        struct Library *, (__SocketBase), 65, BSDSocket)

        AROS_LC1(long, DeleteRouteTagList, \
                  AROS_LCA(struct TagItem *, (__arg1), A0), \
        struct Library *, (__SocketBase), 66, BSDSocket)

        AROS_LC1(long, ChangeRouteTagList, \
                  AROS_LCA(struct TagItem *, (__arg1), A0), \
        struct Library *, (__SocketBase), 67, BSDSocket)

        AROS_LC1NR(void, FreeRouteInfo, \
                  AROS_LCA(struct rt_msghdr *, (__arg1), A0), \
        struct Library *, (__SocketBase), 68, BSDSocket)

        AROS_LC2(struct rt_msghdr *, GetRouteInfo, \
                  AROS_LCA(LONG, (__arg1), D0), \
                  AROS_LCA(LONG, (__arg2), D1), \
        struct Library *, (__SocketBase), 69, BSDSocket)

        AROS_LC4(long, AddInterfaceTagList, \
                  AROS_LCA(STRPTR, (__arg1), A0), \
                  AROS_LCA(STRPTR, (__arg2), A1), \
                  AROS_LCA(long, (__arg3), D0), \
                  AROS_LCA(struct TagItem *, (__arg4), A2), \
        struct Library *, (__SocketBase), 70, BSDSocket)

        AROS_LC2(long, ConfigureInterfaceTagList, \
                  AROS_LCA(STRPTR, (__arg1), A0), \
                  AROS_LCA(struct TagItem *, (__arg2), A1), \
        struct Library *, (__SocketBase), 71, BSDSocket)

        AROS_LC1NR(void, ReleaseInterfaceList, \
                  AROS_LCA(struct List *, (__arg1), A0), \
        struct Library *, (__SocketBase), 72, BSDSocket)

        AROS_LC0(struct List *, ObtainInterfaceList, \
        struct Library *, (__SocketBase), 73, BSDSocket)
*/
#define __QueryInterfaceTagList_WB(__SocketBase, __arg1, __arg2) \
        AROS_LC2(long, QueryInterfaceTagList, \
                  AROS_LCA(STRPTR, (__arg1), A0), \
                  AROS_LCA(struct TagItem *, (__arg2), A1), \
        struct Library *, (__SocketBase), 74, BSDSocket)

#define QueryInterfaceTagList(arg1, arg2) \
    __QueryInterfaceTagList_WB(SocketBase, (arg1), (arg2))
/*
        AROS_LC5(LONG, CreateAddrAllocMessageA, \
                  AROS_LCA(LONG, (__arg1), D0), \
                  AROS_LCA(LONG, (__arg2), D1), \
                  AROS_LCA(STRPTR, (__arg3), A0), \
                  AROS_LCA(struct AddressAllocationMessage *, (__arg4), A1), \
                  AROS_LCA(struct TagItem *, (__arg5), A2), \
        struct Library *, (__SocketBase), 75, BSDSocket)

        AROS_LC1NR(void, DeleteAddrAllocMessage, \
                  AROS_LCA(struct AddressAllocationMessage *, (__arg1), A0), \
        struct Library *, (__SocketBase), 76, BSDSocket)

        AROS_LC1NR(void, BeginInterfaceConfig, \
                  AROS_LCA(struct AddressAllocationMessage *, (__arg1), A0), \
        struct Library *, (__SocketBase), 77, BSDSocket)

        AROS_LC1NR(void, AbortInterfaceConfig, \
                  AROS_LCA(struct AddressAllocationMessage *, (__arg1), A0), \
        struct Library *, (__SocketBase), 78, BSDSocket)

        AROS_LC3(long, AddNetMonitorHookTagList, \
                  AROS_LCA(long, (__arg1), D0), \
                  AROS_LCA(struct Hook *, (__arg2), A0), \
                  AROS_LCA(struct TagItem *, (__arg3), A1), \
        struct Library *, (__SocketBase), 79, BSDSocket)

        AROS_LC1NR(void, RemoveNetMonitorHook, \
                  AROS_LCA(struct Hook *, (__arg1), A0), \
        struct Library *, (__SocketBase), 80, BSDSocket)

        AROS_LC4(LONG, GetNetworkStatistics, \
                  AROS_LCA(LONG, (__arg1), D0), \
                  AROS_LCA(LONG, (__arg2), D1), \
                  AROS_LCA(APTR, (__arg3), A0), \
                  AROS_LCA(LONG, (__arg4), D2), \
        struct Library *, (__SocketBase), 81, BSDSocket)

        AROS_LC1(LONG, AddDomainNameServer, \
                  AROS_LCA(STRPTR, (__arg1), A0), \
        struct Library *, (__SocketBase), 82, BSDSocket)

        AROS_LC1(LONG, RemoveDomainNameServer, \
                  AROS_LCA(STRPTR, (__arg1), A0), \
        struct Library *, (__SocketBase), 83, BSDSocket)

        AROS_LC1NR(void, ReleaseDomainNameServerList, \
                  AROS_LCA(struct List *, (__arg1), A0), \
        struct Library *, (__SocketBase), 84, BSDSocket)

        AROS_LC0(struct List *, ObtainDomainNameServerList, \
        struct Library *, (__SocketBase), 85, BSDSocket)

        AROS_LC1NR(void, setnetent, \
                  AROS_LCA(int, (__arg1), D0), \
        struct Library *, (__SocketBase), 86, BSDSocket)

        AROS_LC0NR(void, endnetent, \
        struct Library *, (__SocketBase), 87, BSDSocket)

        AROS_LC0(struct netent *, getnetent, \
        struct Library *, (__SocketBase), 88, BSDSocket)

        AROS_LC1NR(void, setprotoent, \
                  AROS_LCA(int, (__arg1), D0), \
        struct Library *, (__SocketBase), 89, BSDSocket)
*/

#define __endprotoent_WB(__SocketBase) \
        AROS_LC0NR(void, endprotoent, \
        struct Library *, (__SocketBase), 90, BSDSocket)

#define endprotoent() \
    __endprotoent_WB(SocketBase)

#define __getprotoent_WB(__SocketBase) \
        AROS_LC0(struct protoent *, getprotoent, \
        struct Library *, (__SocketBase), 91, BSDSocket)

#define getprotoent() \
    __getprotoent_WB(SocketBase)

/*
        AROS_LC1NR(void, setservent, \
                  AROS_LCA(int, (__arg1), D0), \
        struct Library *, (__SocketBase), 92, BSDSocket)

        AROS_LC0NR(void, endservent, \
        struct Library *, (__SocketBase), 93, BSDSocket)

        AROS_LC0(struct servent *, getservent, \
        struct Library *, (__SocketBase), 94, BSDSocket)

*/

#define __inet_aton_WB(__SocketBase, __arg1, __arg2) \
        AROS_LC2(LONG, inet_aton, \
                  AROS_LCA(STRPTR,(__arg1),A0), \
                  AROS_LCA(struct in_addr *,(__arg2),A1), \
        struct Library *, (__SocketBase), 95, BSDSocket)

#define inet_aton(arg1, arg2) \
    __inet_aton_WB(SocketBase, (arg1), (arg2))
#endif /* __CONFIG_ROADSHOW__ */
#endif /* DEFINES_BSDSOCKET_PROTOS_H*/
