/*
 * Copyright (C) 1993 AmiTCP/IP Group, <amitcp-group@hut.fi>
 *                    Helsinki University of Technology, Finland.
 *                    All rights reserved.
 * Copyright (C) 2005 - 2007 The AROS Dev Team
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston,
 * MA 02111-1307, USA.
 *
 */

#include <conf.h>

#include <aros/libcall.h>
#include <exec/types.h>
#include <sys/param.h>
#include <api/amiga_raf.h>

typedef VOID (* f_void)();

/*
 * Null used in both function tables
 */
extern VOID AROS_SLIB_ENTRY(Null, LIB)(VOID);

/*
 * "declarations" for ExecLibraryList_funcTable functions.
 */ 

extern VOID AROS_SLIB_ENTRY(Open, ELL)();
extern VOID AROS_SLIB_ENTRY(Expunge, ELL)();

f_void ExecLibraryList_funcTable[] = {
#ifdef __MORPHOS__
  (f_void)FUNCARRAY_32BIT_NATIVE,
#endif
  AROS_SLIB_ENTRY(Open, ELL),
#warning "TODO: NicJA - LIB_Null??"
#if defined(__AROS__)
   NULL,
#else
  AROS_SLIB_ENTRY(Null, LIB),	     /* ELL_Close() is never called */
#endif
  AROS_SLIB_ENTRY(Expunge, ELL),
#warning "TODO: NicJA - LIB_Null??"
#if defined(__AROS__)
   NULL,
#else
  AROS_SLIB_ENTRY(Null, LIB),       /* ELL_Reserved() */
#endif
  (f_void)-1
};

/*
 * "declarations" for userLibrary_funcTable functions.
 */ 
extern VOID AROS_SLIB_ENTRY(Close, UL)();

extern VOID AROS_SLIB_ENTRY(socket, UL)();
extern VOID AROS_SLIB_ENTRY(bind, UL)();
extern VOID AROS_SLIB_ENTRY(listen, UL)();
extern VOID AROS_SLIB_ENTRY(accept, UL)();
extern VOID AROS_SLIB_ENTRY(connect, UL)();
extern VOID AROS_SLIB_ENTRY(sendto, UL)();
extern VOID AROS_SLIB_ENTRY(send, UL)();
extern VOID AROS_SLIB_ENTRY(recvfrom, UL)();
extern VOID AROS_SLIB_ENTRY(recv, UL)();
extern VOID AROS_SLIB_ENTRY(shutdown, UL)();
extern VOID AROS_SLIB_ENTRY(setsockopt, UL)();
extern VOID AROS_SLIB_ENTRY(getsockopt, UL)();
extern VOID AROS_SLIB_ENTRY(getsockname, UL)();
extern VOID AROS_SLIB_ENTRY(getpeername, UL)();

extern VOID AROS_SLIB_ENTRY(IoctlSocket, UL)();
extern VOID AROS_SLIB_ENTRY(CloseSocket, UL)();
extern VOID AROS_SLIB_ENTRY(WaitSelect, UL)();
extern VOID AROS_SLIB_ENTRY(SetSocketSignals, UL)();
extern VOID AROS_SLIB_ENTRY(getdtablesize, UL)();  /* from V3 on */
/*extern REGARGFUN VOID SetDTableSize(); */
extern VOID AROS_SLIB_ENTRY(ObtainSocket, UL)();
extern VOID AROS_SLIB_ENTRY(ReleaseSocket, UL)();
extern VOID AROS_SLIB_ENTRY(ReleaseCopyOfSocket, UL)();
extern VOID AROS_SLIB_ENTRY(Errno, UL)();
extern VOID AROS_SLIB_ENTRY(SetErrnoPtr, UL)();

extern VOID AROS_SLIB_ENTRY(Inet_NtoA, UL)();
extern VOID AROS_SLIB_ENTRY(inet_addr, UL)();
extern VOID AROS_SLIB_ENTRY(Inet_LnaOf, UL)();
extern VOID AROS_SLIB_ENTRY(Inet_NetOf, UL)();
extern VOID AROS_SLIB_ENTRY(Inet_MakeAddr, UL)();
extern VOID AROS_SLIB_ENTRY(inet_network, UL)();

extern VOID AROS_SLIB_ENTRY(gethostbyname, UL)();
extern VOID AROS_SLIB_ENTRY(gethostbyaddr, UL)();
extern VOID AROS_SLIB_ENTRY(getnetbyname, UL)();
extern VOID AROS_SLIB_ENTRY(getnetbyaddr, UL)();
extern VOID AROS_SLIB_ENTRY(getservbyname, UL)();
extern VOID AROS_SLIB_ENTRY(getservbyport, UL)();
extern VOID AROS_SLIB_ENTRY(getprotobyname, UL)();
extern VOID AROS_SLIB_ENTRY(getprotobynumber, UL)();

extern VOID AROS_SLIB_ENTRY(Syslog, UL)();

/* bsdsocket.library 2 extensions */
extern VOID AROS_SLIB_ENTRY(Dup2Socket, UL)();

/* bsdsocket.library 3 extensions */
extern VOID AROS_SLIB_ENTRY(sendmsg, UL)();
extern VOID AROS_SLIB_ENTRY(recvmsg, UL)();
extern VOID AROS_SLIB_ENTRY(gethostname, UL)();
extern VOID AROS_SLIB_ENTRY(gethostid, UL)();
extern VOID AROS_SLIB_ENTRY(SocketBaseTagList, UL)();

/* bsdsocket.library 4 extensions */
extern VOID AROS_SLIB_ENTRY(GetSocketEvents, UL)();

#if defined(__CONFIG_ROADSHOW__)
/* Roadshow extensions */
extern VOID AROS_SLIB_ENTRY(bpf_open, UL)();
extern VOID AROS_SLIB_ENTRY(bpf_close, UL)();
extern VOID AROS_SLIB_ENTRY(bpf_read, UL)();
extern VOID AROS_SLIB_ENTRY(bpf_write, UL)();
extern VOID AROS_SLIB_ENTRY(bpf_set_notify_mask, UL)();
extern VOID AROS_SLIB_ENTRY(bpf_set_interrupt_mask, UL)();
extern VOID AROS_SLIB_ENTRY(bpf_ioctl, UL)();
extern VOID AROS_SLIB_ENTRY(bpf_data_waiting, UL)();
extern VOID AROS_SLIB_ENTRY(AddRouteTagList, UL)();
extern VOID AROS_SLIB_ENTRY(DeleteRouteTagList, UL)();
extern VOID AROS_SLIB_ENTRY(ChangeRouteTagList, UL)();
extern VOID AROS_SLIB_ENTRY(FreeRouteInfo, UL)();
extern VOID AROS_SLIB_ENTRY(GetRouteInfo, UL)();
extern VOID AROS_SLIB_ENTRY(AddInterfaceTagList, UL)();
extern VOID AROS_SLIB_ENTRY(ConfigureInterfaceTagList, UL)();
extern VOID AROS_SLIB_ENTRY(ReleaseInterfaceList, UL)();
extern VOID AROS_SLIB_ENTRY(ObtainInterfaceList, UL)();
extern VOID AROS_SLIB_ENTRY(QueryInterfaceTagList, UL)();
extern VOID AROS_SLIB_ENTRY(CreateAddrAllocMessageA, UL)();
extern VOID AROS_SLIB_ENTRY(DeleteAddrAllocMessage, UL)();
extern VOID AROS_SLIB_ENTRY(BeginInterfaceConfig, UL)();
extern VOID AROS_SLIB_ENTRY(AbortInterfaceConfig, UL)();
extern VOID AROS_SLIB_ENTRY(AddNetMonitorHookTagList, UL)();
extern VOID AROS_SLIB_ENTRY(RemoveNetMonitorHook, UL)();
extern VOID AROS_SLIB_ENTRY(GetNetworkStatistics, UL)();
extern VOID AROS_SLIB_ENTRY(AddDomainNameServer, UL)();
extern VOID AROS_SLIB_ENTRY(RemoveDomainNameServer, UL)();
extern VOID AROS_SLIB_ENTRY(ReleaseDomainNameServerList, UL)();
extern VOID AROS_SLIB_ENTRY(ObtainDomainNameServerList, UL)();
extern VOID AROS_SLIB_ENTRY(setnetent, UL)();
extern VOID AROS_SLIB_ENTRY(endnetent, UL)();
extern VOID AROS_SLIB_ENTRY(getnetent, UL)();
extern VOID AROS_SLIB_ENTRY(setprotoent, UL)();
extern VOID AROS_SLIB_ENTRY(endprotoent, UL)();
extern VOID AROS_SLIB_ENTRY(getprotoent, UL)();
extern VOID AROS_SLIB_ENTRY(setservent, UL)();
extern VOID AROS_SLIB_ENTRY(endservent, UL)();
extern VOID AROS_SLIB_ENTRY(getservent, UL)();
extern VOID AROS_SLIB_ENTRY(inet_aton, UL)();
#endif
/* TODO: following functions are not implemented yet */

f_void UserLibrary_funcTable[] = {
#ifdef __MORPHOS__
  (f_void)FUNCARRAY_32BIT_NATIVE,
#endif
#warning "TODO: NicJA - LIB_Null??"
#if defined(__AROS__)
   NULL,
#else
  AROS_SLIB_ENTRY(Null, LIB),	     /* Open() */
#endif
  AROS_SLIB_ENTRY(Close, UL),
#warning "TODO: NicJA - LIB_Null??"
#if defined(__AROS__)
   NULL,
#else
  AROS_SLIB_ENTRY(Null, LIB),	     /* Expunge() */
#endif
#warning "TODO: NicJA - LIB_Null??"
#if defined(__AROS__)
   NULL,
#else
  AROS_SLIB_ENTRY(Null, LIB),	     /* Reserved() */
#endif

  AROS_SLIB_ENTRY(socket, UL),
  AROS_SLIB_ENTRY(bind, UL),
  AROS_SLIB_ENTRY(listen, UL),
  AROS_SLIB_ENTRY(accept, UL),
  AROS_SLIB_ENTRY(connect, UL),
  AROS_SLIB_ENTRY(sendto, UL),
  AROS_SLIB_ENTRY(send, UL),
  AROS_SLIB_ENTRY(recvfrom, UL),
  AROS_SLIB_ENTRY(recv, UL),
  AROS_SLIB_ENTRY(shutdown, UL),
  AROS_SLIB_ENTRY(setsockopt, UL),
  AROS_SLIB_ENTRY(getsockopt, UL),
  AROS_SLIB_ENTRY(getsockname, UL),
  AROS_SLIB_ENTRY(getpeername, UL),

  AROS_SLIB_ENTRY(IoctlSocket, UL),
  AROS_SLIB_ENTRY(CloseSocket, UL),
  AROS_SLIB_ENTRY(WaitSelect, UL),
  AROS_SLIB_ENTRY(SetSocketSignals, UL),
  AROS_SLIB_ENTRY(getdtablesize, UL),	     /* from V3 on */
/*  SetDTableSize, */
  AROS_SLIB_ENTRY(ObtainSocket, UL),
  AROS_SLIB_ENTRY(ReleaseSocket, UL),
  AROS_SLIB_ENTRY(ReleaseCopyOfSocket, UL),
  AROS_SLIB_ENTRY(Errno, UL),
  AROS_SLIB_ENTRY(SetErrnoPtr, UL),

  AROS_SLIB_ENTRY(Inet_NtoA, UL),
  AROS_SLIB_ENTRY(inet_addr, UL),
  AROS_SLIB_ENTRY(Inet_LnaOf, UL),
  AROS_SLIB_ENTRY(Inet_NetOf, UL),
  AROS_SLIB_ENTRY(Inet_MakeAddr, UL),
  AROS_SLIB_ENTRY(inet_network, UL),

  AROS_SLIB_ENTRY(gethostbyname, UL),
  AROS_SLIB_ENTRY(gethostbyaddr, UL),
  AROS_SLIB_ENTRY(getnetbyname, UL),
  AROS_SLIB_ENTRY(getnetbyaddr, UL),
  AROS_SLIB_ENTRY(getservbyname, UL),
  AROS_SLIB_ENTRY(getservbyport, UL),
  AROS_SLIB_ENTRY(getprotobyname, UL),
  AROS_SLIB_ENTRY(getprotobynumber, UL),
  AROS_SLIB_ENTRY(Syslog, UL),
  
  /* bsdsocket.library 2 extensions */
  AROS_SLIB_ENTRY(Dup2Socket, UL),

  /* bsdsocket.library 3 extensions */
  AROS_SLIB_ENTRY(sendmsg, UL),
  AROS_SLIB_ENTRY(recvmsg, UL),
  AROS_SLIB_ENTRY(gethostname, UL),
  AROS_SLIB_ENTRY(gethostid, UL),
  AROS_SLIB_ENTRY(SocketBaseTagList, UL),
  
  /* bsdsocket.library 4 extensions */
  AROS_SLIB_ENTRY(GetSocketEvents, UL),

#if defined(__CONFIG_ROADSHOW__)
  /* Roadshow extensions  */
#warning "TODO: NicJA - LIB_Null??"
#if defined(__AROS__)
   NULL,
#else
  AROS_SLIB_ENTRY(Null, LIB),	    /* Reserved1()  */
#endif
#warning "TODO: NicJA - LIB_Null??"
#if defined(__AROS__)
   NULL,
#else
  AROS_SLIB_ENTRY(Null, LIB),	    /* Reserved2()  */
#endif
#warning "TODO: NicJA - LIB_Null??"
#if defined(__AROS__)
   NULL,
#else
  AROS_SLIB_ENTRY(Null, LIB),	    /* Reserved3()  */
#endif
#warning "TODO: NicJA - LIB_Null??"
#if defined(__AROS__)
   NULL,
#else
  AROS_SLIB_ENTRY(Null, LIB),	    /* Reserved4()  */
#endif
#warning "TODO: NicJA - LIB_Null??"
#if defined(__AROS__)
   NULL,
#else
  AROS_SLIB_ENTRY(Null, LIB),	    /* Reserved5()  */
#endif
#warning "TODO: NicJA - LIB_Null??"
#if defined(__AROS__)
   NULL,
#else
  AROS_SLIB_ENTRY(Null, LIB),	    /* Reserved6()  */
#endif
#warning "TODO: NicJA - LIB_Null??"
#if defined(__AROS__)
   NULL,
#else
  AROS_SLIB_ENTRY(Null, LIB),	    /* Reserved7()  */
#endif
#warning "TODO: NicJA - LIB_Null??"
#if defined(__AROS__)
   NULL,
#else
  AROS_SLIB_ENTRY(Null, LIB),	    /* Reserved8()  */
#endif
#warning "TODO: NicJA - LIB_Null??"
#if defined(__AROS__)
   NULL,
#else
  AROS_SLIB_ENTRY(Null, LIB),	    /* Reserved9()  */
#endif
#warning "TODO: NicJA - LIB_Null??"
#if defined(__AROS__)
   NULL,
#else
  AROS_SLIB_ENTRY(Null, LIB),	    /* Reserved10() */
#endif
  AROS_SLIB_ENTRY(bpf_open, UL),
  AROS_SLIB_ENTRY(bpf_close, UL),
  AROS_SLIB_ENTRY(bpf_read, UL),
  AROS_SLIB_ENTRY(bpf_write, UL),
  AROS_SLIB_ENTRY(bpf_set_notify_mask, UL),
  AROS_SLIB_ENTRY(bpf_set_interrupt_mask, UL),
  AROS_SLIB_ENTRY(bpf_ioctl, UL),
  AROS_SLIB_ENTRY(bpf_data_waiting, UL),
  AROS_SLIB_ENTRY(AddRouteTagList, UL),
  AROS_SLIB_ENTRY(DeleteRouteTagList, UL),
  AROS_SLIB_ENTRY(ChangeRouteTagList, UL),
  AROS_SLIB_ENTRY(FreeRouteInfo, UL),
  AROS_SLIB_ENTRY(GetRouteInfo, UL),
  AROS_SLIB_ENTRY(AddInterfaceTagList, UL),
  AROS_SLIB_ENTRY(ConfigureInterfaceTagList, UL),
  AROS_SLIB_ENTRY(ReleaseInterfaceList, UL),
  AROS_SLIB_ENTRY(ObtainInterfaceList, UL),
  AROS_SLIB_ENTRY(QueryInterfaceTagList, UL),
  AROS_SLIB_ENTRY(CreateAddrAllocMessageA, UL),
  AROS_SLIB_ENTRY(DeleteAddrAllocMessage, UL),
  AROS_SLIB_ENTRY(BeginInterfaceConfig, UL),
  AROS_SLIB_ENTRY(AbortInterfaceConfig, UL),
  AROS_SLIB_ENTRY(AddNetMonitorHookTagList, UL),
  AROS_SLIB_ENTRY(RemoveNetMonitorHook, UL),
  AROS_SLIB_ENTRY(GetNetworkStatistics, UL),
  AROS_SLIB_ENTRY(AddDomainNameServer, UL),
  AROS_SLIB_ENTRY(RemoveDomainNameServer, UL),
  AROS_SLIB_ENTRY(ReleaseDomainNameServerList, UL),
  AROS_SLIB_ENTRY(ObtainDomainNameServerList, UL),
  AROS_SLIB_ENTRY(setnetent, UL),
  AROS_SLIB_ENTRY(endnetent, UL),
  AROS_SLIB_ENTRY(getnetent, UL),
  AROS_SLIB_ENTRY(setprotoent, UL),
  AROS_SLIB_ENTRY(endprotoent, UL),
  AROS_SLIB_ENTRY(getprotoent, UL),
  AROS_SLIB_ENTRY(setservent, UL),
  AROS_SLIB_ENTRY(endservent, UL),
  AROS_SLIB_ENTRY(getservent, UL),
  AROS_SLIB_ENTRY(inet_aton, UL),
#endif
  /* TODO: Following functions are not implemented yet */

  (f_void)-1
};

