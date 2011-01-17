/*
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

extern VOID AROS_SLIB_ENTRY(Open, Miami)();
extern VOID AROS_SLIB_ENTRY(Close, Miami)();

extern VOID AROS_SLIB_ENTRY(Expunge, ELL)();

f_void Miami_InitFuncTable[]=
{
#ifdef __MORPHOS__
	FUNCARRAY_32BIT_NATIVE,
#endif
	AROS_SLIB_ENTRY(Open, Miami),
    AROS_SLIB_ENTRY(Close, Miami),
	AROS_SLIB_ENTRY(Expunge, ELL),
#warning "TODO: NicJA - LIB_Null??"
#if defined(__AROS__)
   NULL,
#else
  AROS_SLIB_ENTRY(Null, LIB),	     /* ELL_Reserved() is never called */
#endif
  (f_void)-1
};

/*
 * "declarations" for userLibrary_funcTable functions.
 */ 

extern VOID AROS_SLIB_ENTRY(MiamiSysCtl, Miami)();
//extern VOID AROS_SLIB_ENTRY(MiamiSetSysLogPort, Miami)();
extern VOID AROS_SLIB_ENTRY(MiamiDisallowDNS, Miami)();
extern VOID AROS_SLIB_ENTRY(MiamiGetPid, Miami)();
extern VOID AROS_SLIB_ENTRY(MiamiPFAddHook, Miami)();
extern VOID AROS_SLIB_ENTRY(MiamiPFRemoveHook, Miami)();
extern VOID AROS_SLIB_ENTRY(MiamiGetHardwareLen, Miami)();
extern VOID AROS_SLIB_ENTRY(MiamiOpenSSL, Miami)();
extern VOID AROS_SLIB_ENTRY(MiamiCloseSSL, Miami)();
extern VOID AROS_SLIB_ENTRY(MiamiSetSocksConn, Miami)();
extern VOID AROS_SLIB_ENTRY(MiamiIsOnline, Miami)();
extern VOID AROS_SLIB_ENTRY(MiamiOnOffline, Miami)();
extern VOID AROS_SLIB_ENTRY(inet_ntop, Miami)();
extern VOID AROS_SLIB_ENTRY(Miami_inet_aton, Miami)();
extern VOID AROS_SLIB_ENTRY(inet_pton, Miami)();
extern VOID AROS_SLIB_ENTRY(gethostbyname2, Miami)();
extern VOID AROS_SLIB_ENTRY(gai_strerror, Miami)();
extern VOID AROS_SLIB_ENTRY(freeaddrinfo, Miami)();
extern VOID AROS_SLIB_ENTRY(getaddrinfo, Miami)();
extern VOID AROS_SLIB_ENTRY(getnameinfo, Miami)();
extern VOID AROS_SLIB_ENTRY(if_nametoindex, Miami)();
extern VOID AROS_SLIB_ENTRY(if_indextoname, Miami)();
extern VOID AROS_SLIB_ENTRY(if_nameindex, Miami)();
extern VOID AROS_SLIB_ENTRY(if_freenameindex, Miami)();
extern VOID AROS_SLIB_ENTRY(MiamiSupportsIPV6, Miami)();
extern VOID AROS_SLIB_ENTRY(MiamiGetResOptions, Miami)();
extern VOID AROS_SLIB_ENTRY(MiamiSetResOptions, Miami)();
extern VOID AROS_SLIB_ENTRY(sockatmark, Miami)();
extern VOID AROS_SLIB_ENTRY(MiamiSupportedCPUs, Miami)();
extern VOID AROS_SLIB_ENTRY(MiamiGetFdCallback, Miami)();
extern VOID AROS_SLIB_ENTRY(MiamiSetFdCallback, Miami)();
extern VOID AROS_SLIB_ENTRY(MiamiGetCredentials, Miami)();
extern VOID AROS_SLIB_ENTRY(SetSysLogPort, Miami)();
extern VOID AROS_SLIB_ENTRY(Miami_gethostent, Miami)();
extern VOID AROS_SLIB_ENTRY(Miami_endhostent, Miami)();
extern VOID AROS_SLIB_ENTRY(Miami_getprotoent, Miami)();
extern VOID AROS_SLIB_ENTRY(Miami_endprotoent, Miami)();
extern VOID AROS_SLIB_ENTRY(ClearDynNameServ, Miami)();
extern VOID AROS_SLIB_ENTRY(ClearDynDomain, Miami)();
extern VOID AROS_SLIB_ENTRY(AddDynNameServ, Miami)();
extern VOID AROS_SLIB_ENTRY(AddDynDomain, Miami)();
extern VOID AROS_SLIB_ENTRY(EndDynNameServ, Miami)();
extern VOID AROS_SLIB_ENTRY(EndDynDomain, Miami)();
extern VOID AROS_SLIB_ENTRY(Miami_sethostname, Miami)();
extern VOID AROS_SLIB_ENTRY(Miami_QueryInterfaceTagList, Miami)();

extern VOID AROS_SLIB_ENTRY(FindKernelVar, Miami)();

f_void	Miami_UserFuncTable[] =
{
#ifdef __MORPHOS__
	FUNCARRAY_BEGIN,
	FUNCARRAY_32BIT_NATIVE,
#endif
#warning "TODO: NicJA - LIB_Null??"
#if defined(__AROS__)
   NULL,
#else
  AROS_SLIB_ENTRY(Null, LIB),	     /* ELL_Open() is never called */
#endif
	AROS_SLIB_ENTRY(Close, Miami),
#warning "TODO: NicJA - LIB_Null??"
#if defined(__AROS__)
   NULL,
#else
  AROS_SLIB_ENTRY(Null, LIB),	     /* ELL_Expunge() is never called */
#endif
#warning "TODO: NicJA - LIB_Null??"
#if defined(__AROS__)
   NULL,
#else
  AROS_SLIB_ENTRY(Null, LIB),	     /* ELL_Reserved() is never called */
#endif
	AROS_SLIB_ENTRY(MiamiSysCtl, Miami),
	AROS_SLIB_ENTRY(SetSysLogPort, Miami),
	AROS_SLIB_ENTRY(Miami_QueryInterfaceTagList, Miami),
#warning "TODO: NicJA - LIB_Null??"
#if defined(__AROS__)
   NULL,
#else
  AROS_SLIB_ENTRY(Null, LIB),	     /* ELL_Reserved2() is never called */
#endif
	AROS_SLIB_ENTRY(ClearDynNameServ, Miami),
	AROS_SLIB_ENTRY(Miami_gethostent, Miami),
	AROS_SLIB_ENTRY(MiamiDisallowDNS, Miami),
	AROS_SLIB_ENTRY(Miami_endhostent, Miami),
	AROS_SLIB_ENTRY(MiamiGetPid, Miami),
	AROS_SLIB_ENTRY(Miami_getprotoent, Miami),
	AROS_SLIB_ENTRY(Miami_endprotoent, Miami),
	AROS_SLIB_ENTRY(MiamiPFAddHook, Miami),
	AROS_SLIB_ENTRY(MiamiPFRemoveHook, Miami),
	AROS_SLIB_ENTRY(MiamiGetHardwareLen, Miami),
	AROS_SLIB_ENTRY(EndDynDomain, Miami),
	AROS_SLIB_ENTRY(EndDynNameServ, Miami),
	AROS_SLIB_ENTRY(AddDynNameServ, Miami),
	AROS_SLIB_ENTRY(AddDynDomain, Miami),
	AROS_SLIB_ENTRY(Miami_sethostname, Miami),
	AROS_SLIB_ENTRY(ClearDynDomain, Miami),
	AROS_SLIB_ENTRY(MiamiOpenSSL, Miami),
	AROS_SLIB_ENTRY(MiamiCloseSSL, Miami),
#warning "TODO: NicJA - LIB_Null??"
#if defined(__AROS__)
   NULL,
#else
  AROS_SLIB_ENTRY(Null, LIB),	     /* ELL_Close() is never called */
#endif
#warning "TODO: NicJA - LIB_Null??"
#if defined(__AROS__)
   NULL,
#else
  AROS_SLIB_ENTRY(Null, LIB),	     /* ELL_Close() is never called */
#endif
#warning "TODO: NicJA - LIB_Null??"
#if defined(__AROS__)
   NULL,
#else
  AROS_SLIB_ENTRY(Null, LIB),	     /* ELL_Close() is never called */
#endif
#warning "TODO: NicJA - LIB_Null??"
#if defined(__AROS__)
   NULL,
#else
  AROS_SLIB_ENTRY(Null, LIB),	     /* ELL_Close() is never called */
#endif
#warning "TODO: NicJA - LIB_Null??"
#if defined(__AROS__)
   NULL,
#else
  AROS_SLIB_ENTRY(Null, LIB),	     /* ELL_Close() is never called */
#endif
#warning "TODO: NicJA - LIB_Null??"
#if defined(__AROS__)
   NULL,
#else
  AROS_SLIB_ENTRY(Null, LIB),	     /* ELL_Close() is never called */
#endif
	AROS_SLIB_ENTRY(MiamiSetSocksConn, Miami),
#warning "TODO: NicJA - LIB_Null??"
#if defined(__AROS__)
   NULL,
#else
  AROS_SLIB_ENTRY(Null, LIB),	     /* ELL_Close() is never called */
#endif
	AROS_SLIB_ENTRY(MiamiIsOnline, Miami),
	AROS_SLIB_ENTRY(MiamiOnOffline, Miami),
#warning "TODO: NicJA - LIB_Null??"
#if defined(__AROS__)
   NULL,
#else
  AROS_SLIB_ENTRY(Null, LIB),	     /* ELL_Close() is never called */
#endif
	AROS_SLIB_ENTRY(inet_ntop, Miami),
	AROS_SLIB_ENTRY(Miami_inet_aton, Miami),
	AROS_SLIB_ENTRY(inet_pton, Miami),
	AROS_SLIB_ENTRY(gethostbyname2, Miami),
	AROS_SLIB_ENTRY(gai_strerror, Miami),
	AROS_SLIB_ENTRY(freeaddrinfo, Miami),
	AROS_SLIB_ENTRY(getaddrinfo, Miami),
	AROS_SLIB_ENTRY(getnameinfo, Miami),
	AROS_SLIB_ENTRY(if_nametoindex, Miami),
	AROS_SLIB_ENTRY(if_indextoname, Miami),
	AROS_SLIB_ENTRY(if_nameindex, Miami),
	AROS_SLIB_ENTRY(if_freenameindex, Miami),
	AROS_SLIB_ENTRY(MiamiSupportsIPV6, Miami),
	AROS_SLIB_ENTRY(MiamiGetResOptions, Miami),
	AROS_SLIB_ENTRY(MiamiSetResOptions, Miami),
	AROS_SLIB_ENTRY(sockatmark, Miami),
	AROS_SLIB_ENTRY(MiamiSupportedCPUs, Miami),
	AROS_SLIB_ENTRY(MiamiGetFdCallback, Miami),
	AROS_SLIB_ENTRY(MiamiSetFdCallback, Miami),
#warning "TODO: NicJA - LIB_Null??"
#if defined(__AROS__)
   NULL,
#else
  AROS_SLIB_ENTRY(Null, LIB),	     /* ELL_Close() is never called */
#endif
	AROS_SLIB_ENTRY(MiamiGetCredentials, Miami),
#ifdef __MORPHOS__
	-1,
	FUNCARRAY_32BIT_SYSTEMV,
#endif
	AROS_SLIB_ENTRY(FindKernelVar, Miami),
#ifdef __MORPHOS__
	(f_void)-1,
	FUNCARRAY_END
#else
    (f_void)-1
#endif
};

