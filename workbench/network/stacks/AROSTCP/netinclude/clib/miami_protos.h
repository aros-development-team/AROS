#ifndef CLIB_MIAMI_PROTOS_H
#define CLIB_MIAMI_PROTOS_H

/*
    Copyright © 1995-2007, The AROS Development Team. All rights reserved.
*/

#include <aros/libcall.h>

AROS_LP7(int, MiamiSysCtl,
         AROS_LPA(LONG *, name, A0),
         AROS_LPA(ULONG, namelen, D0),
         AROS_LPA(void *, oldp, A1),
         AROS_LPA(LONG *, oldlenp, A2),
         AROS_LPA(void *, newp, A3),
         AROS_LPA(LONG, newlen, D1),
         AROS_LPA(int, len, D2),
         LIBBASETYPEPTR, MiamiBase, 5, Miami
);
AROS_LP0(void, SetSysLogPort,
         LIBBASETYPEPTR, MiamiBase, 6, Miami
);
AROS_LP2(void, QueryInterfaceTagList,
         AROS_LPA(STRPTR, name, A0),
         AROS_LPA(struct TagItem *, tags, A1),
         LIBBASETYPEPTR, MiamiBase, 7, Miami
);
AROS_LP0(void, ClearDynNameServ,
         LIBBASETYPEPTR, MiamiBase, 9, Miami
);
AROS_LP0(struct hostent *, gethostent,
         LIBBASETYPEPTR, MiamiBase, 10, Miami
);
AROS_LP1(void, MiamiDisallowDNS,
         AROS_LPA(LONG, value, D0),
         LIBBASETYPEPTR, MiamiBase, 11, Miami
);
AROS_LP0(void, endhostent,
         LIBBASETYPEPTR, MiamiBase, 12, Miami
);
AROS_LP0(void *, MiamiGetPid,
         LIBBASETYPEPTR, MiamiBase, 13, Miami
);
AROS_LP0(struct protoent *, getprotoent,
         LIBBASETYPEPTR, MiamiBase, 14, Miami
);
AROS_LP0(void, endprotoent,
         LIBBASETYPEPTR, MiamiBase, 15, Miami
);
AROS_LP3(APTR, MiamiPFAddHook,
         AROS_LPA(struct Hook *, hook, A0),
         AROS_LPA(UBYTE *, interface, A1),
         AROS_LPA(struct TagItem *, taglist, A2),
         LIBBASETYPEPTR, MiamiBase, 16, Miami
);
AROS_LP1(void, MiamiPFRemoveHook,
         AROS_LPA(APTR, handle, A0),
         LIBBASETYPEPTR, MiamiBase, 17, Miami
);
AROS_LP1(int, MiamiGetHardwareLen,
         AROS_LPA(char *, name, A0),
         LIBBASETYPEPTR, MiamiBase, 18, Miami
);
AROS_LP0(void, EndDynDomain,
         LIBBASETYPEPTR, MiamiBase, 19, Miami
);
AROS_LP0(void, EndDynNameServ,
         LIBBASETYPEPTR, MiamiBase, 20, Miami
);
AROS_LP1(LONG, AddDynNameServ,
         AROS_LPA(struct sockaddr *, entry, A0),
         LIBBASETYPEPTR, MiamiBase, 21, Miami
);
AROS_LP1(LONG, AddDynDomain,
         AROS_LPA(STRPTR, entry, A0),
         LIBBASETYPEPTR, MiamiBase, 22, Miami
);
AROS_LP2(int, sethostname,
         AROS_LPA(const char *, name, A0),
         AROS_LPA(size_t, namelen, D0),
         LIBBASETYPEPTR, MiamiBase, 23, Miami
);
AROS_LP0(void, ClearDynDomain,
         LIBBASETYPEPTR, MiamiBase, 24, Miami
);
AROS_LP1(struct Library *, MiamiOpenSSL,
         AROS_LPA(struct TagItem *, taglist, A0),
         LIBBASETYPEPTR, MiamiBase, 25, Miami
);
AROS_LP0(void, MiamiCloseSSL,
         LIBBASETYPEPTR, MiamiBase, 26, Miami
);
AROS_LP2(int, MiamiSetSocksConn,
         AROS_LPA(struct sockaddr *, sa, A0),
         AROS_LPA(int, len, D0),
         LIBBASETYPEPTR, MiamiBase, 33, Miami
);
AROS_LP1(int, MiamiIsOnline,
         AROS_LPA(char *, name, A0),
         LIBBASETYPEPTR, MiamiBase, 35, Miami
);
AROS_LP2(void, MiamiOnOffline,
         AROS_LPA(char *, name, A0),
         AROS_LPA(int, online, D0),
         LIBBASETYPEPTR, MiamiBase, 36, Miami
);
AROS_LP4(STRPTR, inet_ntop,
         AROS_LPA(LONG, family, D0),
         AROS_LPA(void *, addrptr, A0),
         AROS_LPA(char *, strptr, A1),
         AROS_LPA(LONG, len, D1),
         LIBBASETYPEPTR, MiamiBase, 38, Miami
);
AROS_LP2(int, inet_aton,
         AROS_LPA(const char *, strptr, A0),
         AROS_LPA(void *, addrptr, A2),
         LIBBASETYPEPTR, MiamiBase, 39, Miami
);
AROS_LP3(int, inet_pton,
         AROS_LPA(LONG, family, D0),
         AROS_LPA(const char *, strptr, A0),
         AROS_LPA(void *, addrptr, A1),
         LIBBASETYPEPTR, MiamiBase, 40, Miami
);
AROS_LP2(struct hostent *, gethostbyname2,
         AROS_LPA(const char *, name, A0),
         AROS_LPA(LONG, family, D0),
         LIBBASETYPEPTR, MiamiBase, 41, Miami
);
AROS_LP1(char *, gai_strerror,
         AROS_LPA(LONG, error, D0),
         LIBBASETYPEPTR, MiamiBase, 42, Miami
);
AROS_LP1(void, freeaddrinfo,
         AROS_LPA(struct addrinfo *, addrinfo, A0),
         LIBBASETYPEPTR, MiamiBase, 43, Miami
);
AROS_LP4(LONG, getaddrinfo,
         AROS_LPA(char *, hostname, A0),
         AROS_LPA(char *, servicename, A1),
         AROS_LPA(struct addrinfo *, hintsp, A2),
         AROS_LPA(struct addrinfo **, result, A3),
         LIBBASETYPEPTR, MiamiBase, 44, Miami
);
AROS_LP7(LONG, getnameinfo,
         AROS_LPA(struct sockaddr *, sockaddr, A0),
         AROS_LPA(LONG, addrlen, D0),
         AROS_LPA(char *, hostname, A1),
         AROS_LPA(LONG, hostlen, D1),
         AROS_LPA(char *, servicename, A2),
         AROS_LPA(LONG, servicelen, D2),
         AROS_LPA(LONG, flags, D3),
         LIBBASETYPEPTR, MiamiBase, 45, Miami
);
AROS_LP1(LONG, if_nametoindex,
         AROS_LPA(char *, name, A0),
         LIBBASETYPEPTR, MiamiBase, 46, Miami
);
AROS_LP2(char *, if_indextoname,
         AROS_LPA(LONG, index, D0),
         AROS_LPA(char *, name, A0),
         LIBBASETYPEPTR, MiamiBase, 47, Miami
);
AROS_LP0(struct if_nameindex *, if_nameindex,
         LIBBASETYPEPTR, MiamiBase, 48, Miami
);
AROS_LP1(void, if_freenameindex,
         AROS_LPA(struct if_nameindex *, nameindex, D0),
         LIBBASETYPEPTR, MiamiBase, 49, Miami
);
AROS_LP0(LONG, MiamiSupportsIPV6,
         LIBBASETYPEPTR, MiamiBase, 50, Miami
);
AROS_LP0(LONG, MiamiGetResOptions,
         LIBBASETYPEPTR, MiamiBase, 51, Miami
);
AROS_LP1(void, MiamiSetResOptions,
         AROS_LPA(LONG, options, D0),
         LIBBASETYPEPTR, MiamiBase, 52, Miami
);
AROS_LP1(LONG, sockatmark,
         AROS_LPA(LONG, sockfd, D0),
         LIBBASETYPEPTR, MiamiBase, 53, Miami
);
AROS_LP3(void, MiamiSupportedCPUs,
         AROS_LPA(ULONG *, apis, A0),
         AROS_LPA(ULONG *, callbacks, A1),
         AROS_LPA(ULONG *, kernel, A2),
         LIBBASETYPEPTR, MiamiBase, 54, Miami
);
AROS_LP1(LONG, MiamiGetFdCallback,
         AROS_LPA(void **, cbptr, A0),
         LIBBASETYPEPTR, MiamiBase, 55, Miami
);
AROS_LP2(LONG, MiamiSetFdCallback,
         AROS_LPA(void *, cbptr, A0),
         AROS_LPA(LONG, cputype, D0),
         LIBBASETYPEPTR, MiamiBase, 56, Miami
);
AROS_LP0(struct UserGroupCredentials *, MiamiGetCredentials,
         LIBBASETYPEPTR, MiamiBase, 58, Miami
);
AROS_LP1(void *, FindKernelVar,
         AROS_LPA(STRPTR, name, A0),
         LIBBASETYPEPTR, MiamiBase, 59, Miami
);
#endif /* CLIB_MIAMI_PROTOS_H */
