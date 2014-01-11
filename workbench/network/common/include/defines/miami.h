#ifndef DEFINES_MIAMI_PROTOS_H
#define DEFINES_MIAMI_PROTOS_H

/*
    Copyright © 1995-2007, The AROS Development Team. All rights reserved.
*/

/*
    Desc: Defines for miami
*/

#include <aros/libcall.h>
#include <exec/types.h>
#include <aros/preprocessor/variadic/cast2iptr.hpp>

#define __MiamiSysCtl_WB(__MiamiBase) \
        AROS_LC7NR(void, MiamiSysCtl, \
                  AROS_LCA(LONG *, (__arg1), A0), \
                  AROS_LCA(ULONG, (__arg2), D0), \
                  AROS_LCA(void *, (__arg3), A1), \
                  AROS_LCA(LONG *, (__arg4), A2), \
                  AROS_LCA(void *, (__arg5), A3), \
                  AROS_LCA(LONG, (__arg6), D1), \
                  AROS_LCA(int, (__arg7), D2), \
        struct Library *, (__MiamiBase), 5, Miami)

#define MiamiSysCtl(arg1, arg2, arg3, arg4, arg5, arg6, arg7) \
    __MiamiSysCtl_WB(MiamiBase, (arg1), (arg2), (arg3), (arg4), (arg5), (arg6), (arg7))

#define __SetSysLogPort_WB(__MiamiBase) \
        AROS_LC0NR(void, SetSysLogPort, \
        struct Library *, (__MiamiBase), 6, Miami)

#define SetSysLogPort() \
    __SetSysLogPort_WB(MiamiBase)

#define __QueryInterfaceTagList_WB(__MiamiBase, __arg1, __arg2) \
        AROS_LC2NR(void, QueryInterfaceTagList, \
                  AROS_LCA(STRPTR, (__arg1), A0), \
                  AROS_LCA(struct TagItem *, (__arg2), A1), \
        struct Library *, (__MiamiBase), 7, Miami)

#define QueryInterfaceTagList(arg1, arg2) \
    __QueryInterfaceTagList_WB(MiamiBase, (arg1), (arg2))

#define __ClearDynNameServ_WB(__MiamiBase) \
        AROS_LC0NR(void, ClearDynNameServ, \
        struct Library *, (__MiamiBase), 9, Miami)

#define ClearDynNameServ() \
    __ClearDynNameServ_WB(MiamiBase)

#define __gethostent_WB(__MiamiBase) \
        AROS_LC0(struct hostent *, gethostent, \
        struct Library *, (__MiamiBase), 10, Miami)

#define gethostent() \
    __gethostent_WB(MiamiBase)

#define __MiamiDisallowDNS_WB(__MiamiBase, __arg1) \
        AROS_LC1NR(void, MiamiDisallowDNS, \
                  AROS_LCA(LONG, (__arg1), D0), \
        struct Library *, (__MiamiBase), 11, Miami)

#define MiamiDisallowDNS(arg1) \
    __MiamiDisallowDNS_WB(MiamiBase, (arg1))

#define __endhostent_WB(__MiamiBase) \
        AROS_LC0NR(void, endhostent, \
        struct Library *, (__MiamiBase), 12, Miami)

#define endhostent() \
    __endhostent_WB(MiamiBase)

#define __MiamiGetPid_WB(__MiamiBase) \
        AROS_LC0(void *, MiamiGetPid, \
        struct Library *, (__MiamiBase), 13, Miami)

#define MiamiGetPid() \
    __MiamiGetPid_WB(MiamiBase)

#define __getprotoent_WB(__MiamiBase) \
        AROS_LC0(struct protoent *, getprotoent, \
        struct Library *, (__MiamiBase), 14, Miami)

#define getprotoent() \
    __getprotoent_WB(MiamiBase)

#define __endprotoent_WB(__MiamiBase) \
        AROS_LC0NR(void, endprotoent, \
        struct Library *, (__MiamiBase), 15, Miami)

#define endprotoent() \
    __endprotoent_WB(MiamiBase)

#define __MiamiPFAddHook_WB(__MiamiBase, __arg1, __arg2, __arg3) \
        AROS_LC3(APTR, MiamiPFAddHook, \
                  AROS_LCA(struct Hook *, (__arg1), A0), \
                  AROS_LCA(UBYTE *, (__arg2), A1), \
                  AROS_LCA(struct TagItem *, (__arg3), A2), \
        struct Library *, (__MiamiBase), 16, Miami)

#define MiamiPFAddHook(arg1, arg2, arg3) \
    __MiamiPFAddHook_WB(MiamiBase, (arg1), (arg2), (arg3))

#define __MiamiPFRemoveHook_WB(__MiamiBase, __arg1) \
        AROS_LC1NR(void, MiamiPFRemoveHook, \
                  AROS_LCA(APTR, (__arg1), A0), \
        struct Library *, (__MiamiBase), 17, Miami)

#define MiamiPFRemoveHook(arg1) \
    __MiamiPFRemoveHook_WB(MiamiBase, (arg1))

#define __MiamiGetHardwareLen_WB(__MiamiBase, __arg1) \
        AROS_LC1(int, MiamiGetHardwareLen, \
                  AROS_LCA(char *, (__arg1), A0), \
        struct Library *, (__MiamiBase), 18, Miami)

#define MiamiGetHardwareLen(arg1) \
    __MiamiGetHardwareLen_WB(MiamiBase, (arg1))

#define __EndDynDomain_WB(__MiamiBase) \
        AROS_LC0NR(void, EndDynDomain, \
        struct Library *, (__MiamiBase), 19, Miami)

#define EndDynDomain() \
    __EndDynDomain_WB(MiamiBase)

#define __EndDynNameServ_WB(__MiamiBase) \
        AROS_LC0NR(void, EndDynNameServ, \
        struct Library *, (__MiamiBase), 20, Miami)

#define EndDynNameServ() \
    __EndDynNameServ_WB(MiamiBase)

#define __AddDynNameServ_WB(__MiamiBase, __arg1) \
        AROS_LC1(LONG, AddDynNameServ, \
                  AROS_LCA(struct sockaddr *, (__arg1), A0), \
        struct Library *, (__MiamiBase), 21, Miami)

#define AddDynNameServ(arg1) \
    __AddDynNameServ_WB(MiamiBase, (arg1))

#define __AddDynDomain_WB(__MiamiBase, __arg1) \
        AROS_LC1(LONG, AddDynDomain, \
                  AROS_LCA(STRPTR, (__arg1), A0), \
        struct Library *, (__MiamiBase), 22, Miami)

#define AddDynDomain(arg1) \
    __AddDynDomain_WB(MiamiBase, (arg1))

#define __sethostname_WB(__MiamiBase, __arg1, __arg2) \
        AROS_LC2(int, sethostname, \
                  AROS_LCA(const char *, (__arg1), A0), \
                  AROS_LCA(size_t, (__arg2), D0), \
        struct Library *, (__MiamiBase), 23, Miami)

#define sethostname(arg1, arg2) \
    __sethostname_WB(MiamiBase, (arg1), (arg2))

#define __ClearDynDomain_WB(__MiamiBase) \
        AROS_LC0NR(void, ClearDynDomain, \
        struct Library *, (__MiamiBase), 24, Miami)

#define ClearDynDomain() \
    __ClearDynDomain_WB(MiamiBase)

#define __MiamiOpenSSL_WB(__MiamiBase, __arg1) \
        AROS_LC1(struct Library *, MiamiOpenSSL, \
                  AROS_LCA(struct TagItem *, (__arg1), A0), \
        struct Library *, (__MiamiBase), 25, Miami)

#define MiamiOpenSSL(arg1) \
    __MiamiOpenSSL_WB(MiamiBase, (arg1))

#define __MiamiCloseSSL_WB(__MiamiBase) \
        AROS_LC0NR(void, MiamiCloseSSL, \
        struct Library *, (__MiamiBase), 26, Miami)

#define MiamiCloseSSL() \
    __MiamiCloseSSL_WB(MiamiBase)

#define __MiamiSetSocksConn_WB(__MiamiBase, __arg1, __arg2) \
        AROS_LC2(int, MiamiSetSocksConn, \
                  AROS_LCA(struct sockaddr *, (__arg1), A0), \
                  AROS_LCA(int, (__arg2), D0), \
        struct Library *, (__MiamiBase), 33, Miami)

#define MiamiSetSocksConn(arg1, arg2) \
    __MiamiSetSocksConn_WB(MiamiBase, (arg1), (arg2))

#define __MiamiIsOnline_WB(__MiamiBase, __arg1) \
        AROS_LC1(int, MiamiIsOnline, \
                  AROS_LCA(char *, (__arg1), A0), \
        struct Library *, (__MiamiBase), 35, Miami)

#define MiamiIsOnline(arg1) \
    __MiamiIsOnline_WB(MiamiBase, (arg1))

#define __MiamiOnOffline_WB(__MiamiBase, __arg1, __arg2) \
        AROS_LC2NR(void, MiamiOnOffline, \
                  AROS_LCA(char *, (__arg1), A0), \
                  AROS_LCA(int, (__arg2), D0), \
        struct Library *, (__MiamiBase), 36, Miami)

#define MiamiOnOffline(arg1, arg2) \
    __MiamiOnOffline_WB(MiamiBase, (arg1), (arg2))

#define __inet_ntop_WB(__MiamiBase, __arg1, __arg2, __arg3, __arg4) \
        AROS_LC4(STRPTR, inet_ntop, \
                  AROS_LCA(LONG, (__arg1), D0), \
                  AROS_LCA(void *, (__arg2), A0), \
                  AROS_LCA(char *, (__arg3), A1), \
                  AROS_LCA(LONG, (__arg4), D1), \
        struct Library *, (__MiamiBase), 38, Miami)

#define inet_ntop(arg1, arg2, arg3, arg4) \
    __inet_ntop_WB(MiamiBase, (arg1), (arg2), (arg3), (arg4))

#define __inet_aton_WB(__MiamiBase, __arg1, __arg2) \
        AROS_LC2(int, inet_aton, \
                  AROS_LCA(const char *, (__arg1), A0), \
                  AROS_LCA(void *, (__arg2), A2), \
        struct Library *, (__MiamiBase), 39, Miami)

#define inet_aton(arg1, arg2) \
    __inet_aton_WB(MiamiBase, (arg1), (arg2))

#define __inet_pton_WB(__MiamiBase, __arg1, __arg2, __arg3) \
        AROS_LC3(int, inet_pton, \
                  AROS_LCA(LONG, (__arg1), D0), \
                  AROS_LCA(char *, (__arg2), A0), \
                  AROS_LCA(void *, (__arg3), A1), \
        struct Library *, (__MiamiBase), 40, Miami)

#define inet_pton(arg1, arg2, arg3) \
    __inet_pton_WB(MiamiBase, (arg1), (arg2), (arg3))

#define __gethostbyname2_WB(__MiamiBase, __arg1, __arg2) \
        AROS_LC2(struct hostent *, gethostbyname2, \
                  AROS_LCA(const char *, (__arg1), A0), \
                  AROS_LCA(LONG, (__arg2), D0), \
        struct Library *, (__MiamiBase), 41, Miami)

#define gethostbyname2(arg1, arg2) \
    __gethostbyname2_WB(MiamiBase, (arg1), (arg2))

#define __gai_strerror_WB(__MiamiBase, __arg1) \
        AROS_LC1(char *, gai_strerror, \
                  AROS_LCA(LONG, (__arg1), D0), \
        struct Library *, (__MiamiBase), 42, Miami)

#define gai_strerror(arg1) \
    __gai_strerror_WB(MiamiBase, (arg1))

#define __freeaddrinfo_WB(__MiamiBase, __arg1) \
        AROS_LC1NR(void, freeaddrinfo, \
                  AROS_LCA(struct addrinfo *, (__arg1), A0), \
        struct Library *, (__MiamiBase), 43, Miami)

#define freeaddrinfo(arg1) \
    __freeaddrinfo_WB(MiamiBase, (arg1))

#define __getaddrinfo_WB(__MiamiBase, __arg1, __arg2, __arg3, __arg4) \
        AROS_LC4(LONG, getaddrinfo, \
                  AROS_LCA(char *, (__arg1), A0), \
                  AROS_LCA(char *, (__arg2), A1), \
                  AROS_LCA(struct addrinfo *, (__arg3), A2), \
                  AROS_LCA(struct addrinfo **, (__arg4), A3), \
        struct Library *, (__MiamiBase), 44, Miami)

#define getaddrinfo(arg1, arg2, arg3, arg4) \
    __getaddrinfo_WB(MiamiBase, (arg1), (arg2), (arg3), (arg4))

#define __getnameinfo_WB(__MiamiBase, __arg1, __arg2, __arg3, __arg4, __arg5, __arg6, __arg7) \
        AROS_LC7(LONG, getnameinfo, \
                  AROS_LCA(struct sockaddr *, (__arg1), A0), \
                  AROS_LCA(LONG, (__arg2), D0), \
                  AROS_LCA(char *, (__arg3), A1), \
                  AROS_LCA(LONG, (__arg4), D1), \
                  AROS_LCA(char *, (__arg5), A2), \
                  AROS_LCA(LONG, (__arg6), D2), \
                  AROS_LCA(LONG, (__arg7), D3), \
        struct Library *, (__MiamiBase), 45, Miami)

#define getnameinfo(arg1, arg2, arg3, arg4, arg5, arg6, arg7) \
    __getnameinfo_WB(MiamiBase, (arg1), (arg2), (arg3), (arg4), (arg5), (arg6), (arg7))

#define __if_nametoindex_WB(__MiamiBase, __arg1) \
        AROS_LC1(LONG, if_nametoindex, \
                  AROS_LCA(char *, (__arg1), A0), \
        struct Library *, (__MiamiBase), 46, Miami)

#define if_nametoindex(arg1) \
    __if_nametoindex_WB(MiamiBase, (arg1))

#define __if_indextoname_WB(__MiamiBase, __arg1, __arg2) \
        AROS_LC2(char *, if_indextoname, \
                  AROS_LCA(LONG, (__arg1), D0), \
                  AROS_LCA(char *, (__arg2), A0), \
        struct Library *, (__MiamiBase), 47, Miami)

#define if_indextoname(arg1, arg2) \
    __if_indextoname_WB(MiamiBase, (arg1), (arg2))

#define __if_nameindex_WB(__MiamiBase) \
        AROS_LC0(struct if_nameindex *, if_nameindex, \
        struct Library *, (__MiamiBase), 48, Miami)

#define if_nameindex() \
    __if_nameindex_WB(MiamiBase)

#define __if_freenameindex_WB(__MiamiBase, __arg1) \
        AROS_LC1NR(void, if_freenameindex, \
                  AROS_LCA(struct if_nameindex *, (__arg1), D0), \
        struct Library *, (__MiamiBase), 49, Miami)

#define if_freenameindex(arg1) \
    __if_freenameindex_WB(MiamiBase, (arg1))

#define __MiamiSupportsIPV6_WB(__MiamiBase) \
        AROS_LC0(LONG, MiamiSupportsIPV6, \
        struct Library *, (__MiamiBase), 50, Miami)

#define MiamiSupportsIPV6() \
    __MiamiSupportsIPV6_WB(MiamiBase)

#define __MiamiGetResOptions_WB(__MiamiBase) \
        AROS_LC0(LONG, MiamiGetResOptions, \
        struct Library *, (__MiamiBase), 51, Miami)

#define MiamiGetResOptions() \
    __MiamiGetResOptions_WB(MiamiBase)

#define __MiamiSetResOptions_WB(__MiamiBase, __arg1) \
        AROS_LC1NR(void, MiamiSetResOptions, \
                  AROS_LCA(LONG, (__arg1), D0), \
        struct Library *, (__MiamiBase), 52, Miami)

#define MiamiSetResOptions(arg1) \
    __MiamiSetResOptions_WB(MiamiBase, (arg1))

#define __sockatmark_WB(__MiamiBase, __arg1) \
        AROS_LC1(LONG, sockatmark, \
                  AROS_LCA(LONG, (__arg1), D0), \
        struct Library *, (__MiamiBase), 53, Miami)

#define sockatmark(arg1) \
    __sockatmark_WB(MiamiBase, (arg1))

#define __MiamiSupportedCPUs_WB(__MiamiBase, __arg1, __arg2, __arg3) \
        AROS_LC3NR(void, MiamiSupportedCPUs, \
                  AROS_LCA(ULONG *, (__arg1), A0), \
                  AROS_LCA(ULONG *, (__arg2), A1), \
                  AROS_LCA(ULONG *, (__arg3), A2), \
        struct Library *, (__MiamiBase), 54, Miami)

#define MiamiSupportedCPUs(arg1, arg2, arg3) \
    __MiamiSupportedCPUs_WB(MiamiBase, (arg1), (arg2), (arg3))

#define __MiamiGetFdCallback_WB(__MiamiBase, __arg1) \
        AROS_LC1(LONG, MiamiGetFdCallback, \
                  AROS_LCA(void **, (__arg1), A0), \
        struct Library *, (__MiamiBase), 55, Miami)

#define MiamiGetFdCallback(arg1) \
    __MiamiGetFdCallback_WB(MiamiBase, (arg1))

#define __MiamiSetFdCallback_WB(__MiamiBase, __arg1, __arg2) \
        AROS_LC2(LONG, MiamiSetFdCallback, \
                  AROS_LCA(void *, (__arg1), A0), \
                  AROS_LCA(LONG, (__arg2), D0), \
        struct Library *, (__MiamiBase), 56, Miami)

#define MiamiSetFdCallback(arg1, arg2) \
    __MiamiSetFdCallback_WB(MiamiBase, (arg1), (arg2))

#define __MiamiGetCredentials_WB(__MiamiBase) \
        AROS_LC0(struct UserGroupCredentials *, MiamiGetCredentials, \
        struct Library *, (__MiamiBase), 58, Miami)

#define MiamiGetCredentials() \
    __MiamiGetCredentials_WB(MiamiBase)

#define __FindKernelVar_WB(__MiamiBase, __arg1) \
        AROS_LC1(void *, FindKernelVar, \
                  AROS_LCA(STRPTR, (__arg1), A0), \
        struct Library *, (__MiamiBase), 59, Miami)

#define FindKernelVar(arg1) \
    __FindKernelVar_WB(MiamiBase, (arg1))

#endif /* DEFINES_MIAMI_PROTOS_H*/
