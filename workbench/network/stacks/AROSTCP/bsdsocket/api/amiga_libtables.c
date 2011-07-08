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
extern VOID AROS_SLIB_ENTRY(Null, LIB, 0)(VOID);

/*
 * "declarations" for ExecLibraryList_funcTable functions.
 */ 

extern VOID AROS_SLIB_ENTRY(Open, ELL, 1)();
extern VOID AROS_SLIB_ENTRY(Expunge, ELL, 3)();

f_void ExecLibraryList_funcTable[] = {
#ifdef __MORPHOS__
  (f_void)FUNCARRAY_32BIT_NATIVE,
#endif
  AROS_SLIB_ENTRY(Open, ELL, 1),
  AROS_SLIB_ENTRY(Null, LIB, 0),	     /* ELL_Close() is never called */
  AROS_SLIB_ENTRY(Expunge, ELL, 3),
  AROS_SLIB_ENTRY(Null, LIB, 0),       /* ELL_Reserved() */
  (f_void)-1
};

/*
 * "declarations" for userLibrary_funcTable functions.
 */ 
void AROS_SLIB_ENTRY(Close, UL, 2)(void);
void AROS_SLIB_ENTRY(socket, UL, 5)(void);
void AROS_SLIB_ENTRY(bind, UL, 6)(void);
void AROS_SLIB_ENTRY(listen, UL, 7)(void);
void AROS_SLIB_ENTRY(accept, UL, 8)(void);
void AROS_SLIB_ENTRY(connect, UL, 9)(void);
void AROS_SLIB_ENTRY(sendto, UL, 10)(void);
void AROS_SLIB_ENTRY(send, UL, 11)(void);
void AROS_SLIB_ENTRY(recvfrom, UL, 12)(void);
void AROS_SLIB_ENTRY(recv, UL, 13)(void);
void AROS_SLIB_ENTRY(shutdown, UL, 14)(void);
void AROS_SLIB_ENTRY(setsockopt, UL, 15)(void);
void AROS_SLIB_ENTRY(getsockopt, UL, 16)(void);
void AROS_SLIB_ENTRY(getsockname, UL, 17)(void);
void AROS_SLIB_ENTRY(getpeername, UL, 18)(void);

void AROS_SLIB_ENTRY(IoctlSocket, UL, 19)(void);
void AROS_SLIB_ENTRY(CloseSocket, UL, 20)(void);
void AROS_SLIB_ENTRY(WaitSelect, UL, 21)(void);
void AROS_SLIB_ENTRY(SetSocketSignals, UL, 22)(void);
void AROS_SLIB_ENTRY(getdtablesize, UL, 23)(void);

void AROS_SLIB_ENTRY(ObtainSocket, UL, 24)(void);
void AROS_SLIB_ENTRY(ReleaseSocket, UL, 25)(void);
void AROS_SLIB_ENTRY(ReleaseCopyOfSocket, UL, 26)(void);
void AROS_SLIB_ENTRY(Errno, UL, 27)(void);
void AROS_SLIB_ENTRY(SetErrnoPtr, UL, 28)(void);

void AROS_SLIB_ENTRY(Inet_NtoA, UL, 29)(void);
void AROS_SLIB_ENTRY(inet_addr, UL, 30)(void);
void AROS_SLIB_ENTRY(Inet_LnaOf, UL, 31)(void);
void AROS_SLIB_ENTRY(Inet_NetOf, UL, 32)(void);
void AROS_SLIB_ENTRY(Inet_MakeAddr, UL, 33)(void);
void AROS_SLIB_ENTRY(inet_network, UL, 34)(void);

void AROS_SLIB_ENTRY(gethostbyname, UL, 35)(void);
void AROS_SLIB_ENTRY(gethostbyaddr, UL, 36)(void);
void AROS_SLIB_ENTRY(getnetbyname, UL, 37)(void);
void AROS_SLIB_ENTRY(getnetbyaddr, UL, 38)(void);
void AROS_SLIB_ENTRY(getservbyname, UL, 39)(void);
void AROS_SLIB_ENTRY(getservbyport, UL, 40)(void);
void AROS_SLIB_ENTRY(getprotobyname, UL, 41)(void);
void AROS_SLIB_ENTRY(getprotobynumber, UL, 42)(void);
void AROS_SLIB_ENTRY(Syslog, UL, 43)(void);
  
  /* bsdsocket.library 2 extensions */
void AROS_SLIB_ENTRY(Dup2Socket, UL, 44)(void);

  /* bsdsocket.library 3 extensions */
void AROS_SLIB_ENTRY(sendmsg, UL, 45)(void);
void AROS_SLIB_ENTRY(recvmsg, UL, 46)(void);
void AROS_SLIB_ENTRY(gethostname, UL, 47)(void);
void AROS_SLIB_ENTRY(gethostid, UL, 48)(void);
void AROS_SLIB_ENTRY(SocketBaseTagList, UL, 49)(void);
  
  /* bsdsocket.library 4 extensions */
void AROS_SLIB_ENTRY(GetSocketEvents, UL, 50)(void);

#if defined(__CONFIG_ROADSHOW__)
  /* Roadshow extensions  */
void AROS_SLIB_ENTRY(bpf_open, UL, 61)(void);
void AROS_SLIB_ENTRY(bpf_close, UL, 62)(void);
void AROS_SLIB_ENTRY(bpf_read, UL, 63)(void);
void AROS_SLIB_ENTRY(bpf_write, UL, 64)(void);
void AROS_SLIB_ENTRY(bpf_set_notify_mask, UL, 65)(void);
void AROS_SLIB_ENTRY(bpf_set_interrupt_mask, UL, 66)(void);
void AROS_SLIB_ENTRY(bpf_ioctl, UL, 67)(void);
void AROS_SLIB_ENTRY(bpf_data_waiting, UL, 68)(void);
void AROS_SLIB_ENTRY(AddRouteTagList, UL, 69)(void);
void AROS_SLIB_ENTRY(DeleteRouteTagList, UL, 70)(void);
void AROS_SLIB_ENTRY(ChangeRouteTagList, UL, 71)(void);
void AROS_SLIB_ENTRY(FreeRouteInfo, UL, 72)(void);
void AROS_SLIB_ENTRY(GetRouteInfo, UL, 73)(void);
void AROS_SLIB_ENTRY(AddInterfaceTagList, UL, 74)(void);
void AROS_SLIB_ENTRY(ConfigureInterfaceTagList, UL, 75)(void);
void AROS_SLIB_ENTRY(ReleaseInterfaceList, UL, 76)(void);
void AROS_SLIB_ENTRY(ObtainInterfaceList, UL, 77)(void);
void AROS_SLIB_ENTRY(QueryInterfaceTagList, UL, 78)(void);
void AROS_SLIB_ENTRY(CreateAddrAllocMessageA, UL, 79)(void);
void AROS_SLIB_ENTRY(DeleteAddrAllocMessage, UL, 80)(void);
void AROS_SLIB_ENTRY(BeginInterfaceConfig, UL, 81)(void);
void AROS_SLIB_ENTRY(AbortInterfaceConfig, UL, 82)(void);
void AROS_SLIB_ENTRY(AddNetMonitorHookTagList, UL, 83)(void);
void AROS_SLIB_ENTRY(RemoveNetMonitorHook, UL, 84)(void);
void AROS_SLIB_ENTRY(GetNetworkStatistics, UL, 85)(void);
void AROS_SLIB_ENTRY(AddDomainNameServer, UL, 86)(void);
void AROS_SLIB_ENTRY(RemoveDomainNameServer, UL, 87)(void);
void AROS_SLIB_ENTRY(ReleaseDomainNameServerList, UL, 88)(void);
void AROS_SLIB_ENTRY(ObtainDomainNameServerList, UL, 89)(void);
void AROS_SLIB_ENTRY(setnetent, UL, 90)(void);
void AROS_SLIB_ENTRY(endnetent, UL, 91)(void);
void AROS_SLIB_ENTRY(getnetent, UL, 92)(void);
void AROS_SLIB_ENTRY(setprotoent, UL, 93)(void);
void AROS_SLIB_ENTRY(endprotoent, UL, 94)(void);
void AROS_SLIB_ENTRY(getprotoent, UL, 95)(void);
void AROS_SLIB_ENTRY(setservent, UL, 96)(void);
void AROS_SLIB_ENTRY(endservent, UL, 97)(void);
void AROS_SLIB_ENTRY(getservent, UL, 98)(void);
void AROS_SLIB_ENTRY(inet_aton, UL, 99)(void);
#endif

/* TODO: following functions are not implemented yet */

f_void UserLibrary_funcTable[] = {
#ifdef __MORPHOS__
  (f_void)FUNCARRAY_32BIT_NATIVE,
#endif
  AROS_SLIB_ENTRY(Null, LIB, 0),	     /* Open() */
  AROS_SLIB_ENTRY(Close, UL, 2),
  AROS_SLIB_ENTRY(Null, LIB, 0),	     /* Expunge() */
  AROS_SLIB_ENTRY(Null, LIB, 0),	     /* Reserved() */
  AROS_SLIB_ENTRY(socket, UL, 5),
  AROS_SLIB_ENTRY(bind, UL, 6),
  AROS_SLIB_ENTRY(listen, UL, 7),
  AROS_SLIB_ENTRY(accept, UL, 8),
  AROS_SLIB_ENTRY(connect, UL, 9),
  AROS_SLIB_ENTRY(sendto, UL, 10),
  AROS_SLIB_ENTRY(send, UL, 11),
  AROS_SLIB_ENTRY(recvfrom, UL, 12),
  AROS_SLIB_ENTRY(recv, UL, 13),
  AROS_SLIB_ENTRY(shutdown, UL, 14),
  AROS_SLIB_ENTRY(setsockopt, UL, 15),
  AROS_SLIB_ENTRY(getsockopt, UL, 16),
  AROS_SLIB_ENTRY(getsockname, UL, 17),
  AROS_SLIB_ENTRY(getpeername, UL, 18),

  AROS_SLIB_ENTRY(IoctlSocket, UL, 19),
  AROS_SLIB_ENTRY(CloseSocket, UL, 20),
  AROS_SLIB_ENTRY(WaitSelect, UL, 21),
  AROS_SLIB_ENTRY(SetSocketSignals, UL, 22),
  AROS_SLIB_ENTRY(getdtablesize, UL, 23),	     /* from V3 on */
/*  SetDTableSize, */
  AROS_SLIB_ENTRY(ObtainSocket, UL, 24),
  AROS_SLIB_ENTRY(ReleaseSocket, UL, 25),
  AROS_SLIB_ENTRY(ReleaseCopyOfSocket, UL, 26),
  AROS_SLIB_ENTRY(Errno, UL, 27),
  AROS_SLIB_ENTRY(SetErrnoPtr, UL, 28),

  AROS_SLIB_ENTRY(Inet_NtoA, UL, 29),
  AROS_SLIB_ENTRY(inet_addr, UL, 30),
  AROS_SLIB_ENTRY(Inet_LnaOf, UL, 31),
  AROS_SLIB_ENTRY(Inet_NetOf, UL, 32),
  AROS_SLIB_ENTRY(Inet_MakeAddr, UL, 33),
  AROS_SLIB_ENTRY(inet_network, UL, 34),

  AROS_SLIB_ENTRY(gethostbyname, UL, 35),
  AROS_SLIB_ENTRY(gethostbyaddr, UL, 36),
  AROS_SLIB_ENTRY(getnetbyname, UL, 37),
  AROS_SLIB_ENTRY(getnetbyaddr, UL, 38),
  AROS_SLIB_ENTRY(getservbyname, UL, 39),
  AROS_SLIB_ENTRY(getservbyport, UL, 40),
  AROS_SLIB_ENTRY(getprotobyname, UL, 41),
  AROS_SLIB_ENTRY(getprotobynumber, UL, 42),
  AROS_SLIB_ENTRY(Syslog, UL, 43),
  
  /* bsdsocket.library 2 extensions */
  AROS_SLIB_ENTRY(Dup2Socket, UL, 44),

  /* bsdsocket.library 3 extensions */
  AROS_SLIB_ENTRY(sendmsg, UL, 45),
  AROS_SLIB_ENTRY(recvmsg, UL, 46),
  AROS_SLIB_ENTRY(gethostname, UL, 47),
  AROS_SLIB_ENTRY(gethostid, UL, 48),
  AROS_SLIB_ENTRY(SocketBaseTagList, UL, 49),
  
  /* bsdsocket.library 4 extensions */
  AROS_SLIB_ENTRY(GetSocketEvents, UL, 50),

#if defined(__CONFIG_ROADSHOW__)
  /* Roadshow extensions  */
  AROS_SLIB_ENTRY(Null, LIB, 0),	    /* Reserved1()  */
  AROS_SLIB_ENTRY(Null, LIB, 0),	    /* Reserved2()  */
  AROS_SLIB_ENTRY(Null, LIB, 0),	    /* Reserved3()  */
  AROS_SLIB_ENTRY(Null, LIB, 0),	    /* Reserved4()  */
  AROS_SLIB_ENTRY(Null, LIB, 0),	    /* Reserved5()  */
  AROS_SLIB_ENTRY(Null, LIB, 0),	    /* Reserved6()  */
  AROS_SLIB_ENTRY(Null, LIB, 0),	    /* Reserved7()  */
  AROS_SLIB_ENTRY(Null, LIB, 0),	    /* Reserved8()  */
  AROS_SLIB_ENTRY(Null, LIB, 0),	    /* Reserved9()  */
  AROS_SLIB_ENTRY(Null, LIB, 0),	    /* Reserved10() */
  AROS_SLIB_ENTRY(bpf_open, UL, 61),
  AROS_SLIB_ENTRY(bpf_close, UL, 62),
  AROS_SLIB_ENTRY(bpf_read, UL, 63),
  AROS_SLIB_ENTRY(bpf_write, UL, 64),
  AROS_SLIB_ENTRY(bpf_set_notify_mask, UL, 65),
  AROS_SLIB_ENTRY(bpf_set_interrupt_mask, UL, 66),
  AROS_SLIB_ENTRY(bpf_ioctl, UL, 67),
  AROS_SLIB_ENTRY(bpf_data_waiting, UL, 68),
  AROS_SLIB_ENTRY(AddRouteTagList, UL, 69),
  AROS_SLIB_ENTRY(DeleteRouteTagList, UL, 70),
  AROS_SLIB_ENTRY(ChangeRouteTagList, UL, 71),
  AROS_SLIB_ENTRY(FreeRouteInfo, UL, 72),
  AROS_SLIB_ENTRY(GetRouteInfo, UL, 73),
  AROS_SLIB_ENTRY(AddInterfaceTagList, UL, 74),
  AROS_SLIB_ENTRY(ConfigureInterfaceTagList, UL, 75),
  AROS_SLIB_ENTRY(ReleaseInterfaceList, UL, 76),
  AROS_SLIB_ENTRY(ObtainInterfaceList, UL, 77),
  AROS_SLIB_ENTRY(QueryInterfaceTagList, UL, 78),
  AROS_SLIB_ENTRY(CreateAddrAllocMessageA, UL, 79),
  AROS_SLIB_ENTRY(DeleteAddrAllocMessage, UL, 80),
  AROS_SLIB_ENTRY(BeginInterfaceConfig, UL, 81),
  AROS_SLIB_ENTRY(AbortInterfaceConfig, UL, 82),
  AROS_SLIB_ENTRY(AddNetMonitorHookTagList, UL, 83),
  AROS_SLIB_ENTRY(RemoveNetMonitorHook, UL, 84),
  AROS_SLIB_ENTRY(GetNetworkStatistics, UL, 85),
  AROS_SLIB_ENTRY(AddDomainNameServer, UL, 86),
  AROS_SLIB_ENTRY(RemoveDomainNameServer, UL, 87),
  AROS_SLIB_ENTRY(ReleaseDomainNameServerList, UL, 88),
  AROS_SLIB_ENTRY(ObtainDomainNameServerList, UL, 89),
  AROS_SLIB_ENTRY(setnetent, UL, 90),
  AROS_SLIB_ENTRY(endnetent, UL, 91),
  AROS_SLIB_ENTRY(getnetent, UL, 92),
  AROS_SLIB_ENTRY(setprotoent, UL, 93),
  AROS_SLIB_ENTRY(endprotoent, UL, 94),
  AROS_SLIB_ENTRY(getprotoent, UL, 95),
  AROS_SLIB_ENTRY(setservent, UL, 96),
  AROS_SLIB_ENTRY(endservent, UL, 97),
  AROS_SLIB_ENTRY(getservent, UL, 98),
  AROS_SLIB_ENTRY(inet_aton, UL, 99),
#endif
  /* TODO: Following functions are not implemented yet */

  (f_void)-1
};

