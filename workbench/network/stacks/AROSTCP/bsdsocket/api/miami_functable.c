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
extern VOID AROS_SLIB_ENTRY(Null, LIB, 0)(VOID);

/*
 * "declarations" for ExecLibraryList_funcTable functions.
 */ 

extern VOID AROS_SLIB_ENTRY(Open, Miami, 1)();
extern VOID AROS_SLIB_ENTRY(Close, Miami, 2)();

extern VOID AROS_SLIB_ENTRY(Expunge, ELL, 3)();

f_void Miami_InitFuncTable[]=
{
#ifdef __MORPHOS__
	FUNCARRAY_32BIT_NATIVE,
#endif
	AROS_SLIB_ENTRY(Open, Miami, 1),
    AROS_SLIB_ENTRY(Close, Miami, 2),
	AROS_SLIB_ENTRY(Expunge, ELL, 3),
/* TODO: NicJA - LIB_Null?? */
#if defined(__AROS__)
   NULL,
#else
	AROS_SLIB_ENTRY(Null, LIB, 0),	     /* ELL_Reserved() is never called */
#endif
  (f_void)-1
};

/*
 * "declarations" for userLibrary_funcTable functions.
 */ 
void AROS_SLIB_ENTRY(MiamiSysCtl, Miami, 5)(void);
void AROS_SLIB_ENTRY(SetSysLogPort, Miami, 6)(void);
void AROS_SLIB_ENTRY(Miami_QueryInterfaceTagList, Miami, 7)(void);
void AROS_SLIB_ENTRY(ClearDynNameServ, Miami, 9)(void);
void AROS_SLIB_ENTRY(Miami_gethostent, Miami, 10)(void);
void AROS_SLIB_ENTRY(MiamiDisallowDNS, Miami, 11)(void);
void AROS_SLIB_ENTRY(Miami_endhostent, Miami, 12)(void);
void AROS_SLIB_ENTRY(MiamiGetPid, Miami, 13)(void);
void AROS_SLIB_ENTRY(Miami_getprotoent, Miami, 14)(void);
void AROS_SLIB_ENTRY(Miami_endprotoent, Miami, 15)(void);
void AROS_SLIB_ENTRY(MiamiPFAddHook, Miami, 16)(void);
void AROS_SLIB_ENTRY(MiamiPFRemoveHook, Miami, 17)(void);
void AROS_SLIB_ENTRY(MiamiGetHardwareLen, Miami, 18)(void);
void AROS_SLIB_ENTRY(EndDynDomain, Miami, 19)(void);
void AROS_SLIB_ENTRY(EndDynNameServ, Miami, 20)(void);
void AROS_SLIB_ENTRY(AddDynNameServ, Miami, 21)(void);
void AROS_SLIB_ENTRY(AddDynDomain, Miami, 22)(void);
void AROS_SLIB_ENTRY(Miami_sethostname, Miami, 23)(void);
void AROS_SLIB_ENTRY(ClearDynDomain, Miami, 24)(void);
void AROS_SLIB_ENTRY(MiamiOpenSSL, Miami, 25)(void);
void AROS_SLIB_ENTRY(MiamiCloseSSL, Miami, 26)(void);
void AROS_SLIB_ENTRY(MiamiSetSocksConn, Miami, 33)(void);
void AROS_SLIB_ENTRY(MiamiIsOnline, Miami, 35)(void);
void AROS_SLIB_ENTRY(MiamiOnOffline, Miami, 36)(void);
void AROS_SLIB_ENTRY(inet_ntop, Miami, 38)(void);
void AROS_SLIB_ENTRY(Miami_inet_aton, Miami, 39)(void);
void AROS_SLIB_ENTRY(inet_pton, Miami, 40)(void);
void AROS_SLIB_ENTRY(gethostbyname2, Miami, 41)(void);
void AROS_SLIB_ENTRY(gai_strerror, Miami, 42)(void);
void AROS_SLIB_ENTRY(freeaddrinfo, Miami, 43)(void);
void AROS_SLIB_ENTRY(getaddrinfo, Miami, 44)(void);
void AROS_SLIB_ENTRY(getnameinfo, Miami, 45)(void);
void AROS_SLIB_ENTRY(if_nametoindex, Miami, 46)(void);
void AROS_SLIB_ENTRY(if_indextoname, Miami, 47)(void);
void AROS_SLIB_ENTRY(if_nameindex, Miami, 48)(void);
void AROS_SLIB_ENTRY(if_freenameindex, Miami, 49)(void);
void AROS_SLIB_ENTRY(MiamiSupportsIPV6, Miami, 50)(void);
void AROS_SLIB_ENTRY(MiamiGetResOptions, Miami, 51)(void);
void AROS_SLIB_ENTRY(MiamiSetResOptions, Miami, 52)(void);
void AROS_SLIB_ENTRY(sockatmark, Miami, 53)(void);
void AROS_SLIB_ENTRY(MiamiSupportedCPUs, Miami, 54)(void);
void AROS_SLIB_ENTRY(MiamiGetFdCallback, Miami, 55)(void);
void AROS_SLIB_ENTRY(MiamiSetFdCallback, Miami, 56)(void);
void AROS_SLIB_ENTRY(MiamiGetCredentials, Miami, 58)(void);
void AROS_SLIB_ENTRY(FindKernelVar, Miami, 59)(void);

f_void	Miami_UserFuncTable[] =
{
#ifdef __MORPHOS__
	FUNCARRAY_BEGIN,
	FUNCARRAY_32BIT_NATIVE,
#endif
	AROS_SLIB_ENTRY(Null, LIB, 0),	     /* ELL_Open() is never called */
	AROS_SLIB_ENTRY(Close, Miami, 2),
	AROS_SLIB_ENTRY(Null, LIB, 0),	     /* ELL_Expunge() is never called */
	AROS_SLIB_ENTRY(Null, LIB, 0),	     /* ELL_Reserved() is never called */
	AROS_SLIB_ENTRY(MiamiSysCtl, Miami, 5),
	AROS_SLIB_ENTRY(SetSysLogPort, Miami, 6),
	AROS_SLIB_ENTRY(Miami_QueryInterfaceTagList, Miami, 7),
	AROS_SLIB_ENTRY(Null, LIB, 0),	     /* ELL_Reserved2() is never called */
	AROS_SLIB_ENTRY(ClearDynNameServ, Miami, 9),
	AROS_SLIB_ENTRY(Miami_gethostent, Miami, 10),
	AROS_SLIB_ENTRY(MiamiDisallowDNS, Miami, 11),
	AROS_SLIB_ENTRY(Miami_endhostent, Miami, 12),
	AROS_SLIB_ENTRY(MiamiGetPid, Miami, 13),
	AROS_SLIB_ENTRY(Miami_getprotoent, Miami, 14),
	AROS_SLIB_ENTRY(Miami_endprotoent, Miami, 15),
	AROS_SLIB_ENTRY(MiamiPFAddHook, Miami, 16),
	AROS_SLIB_ENTRY(MiamiPFRemoveHook, Miami, 17),
	AROS_SLIB_ENTRY(MiamiGetHardwareLen, Miami, 18),
	AROS_SLIB_ENTRY(EndDynDomain, Miami, 19),
	AROS_SLIB_ENTRY(EndDynNameServ, Miami, 20),
	AROS_SLIB_ENTRY(AddDynNameServ, Miami, 21),
	AROS_SLIB_ENTRY(AddDynDomain, Miami, 22),
	AROS_SLIB_ENTRY(Miami_sethostname, Miami, 23),
	AROS_SLIB_ENTRY(ClearDynDomain, Miami, 24),
	AROS_SLIB_ENTRY(MiamiOpenSSL, Miami, 25),
	AROS_SLIB_ENTRY(MiamiCloseSSL, Miami, 26),
	AROS_SLIB_ENTRY(Null, LIB, 0),	     /* ELL_Close() is never called */
	AROS_SLIB_ENTRY(Null, LIB, 0),	     /* ELL_Close() is never called */
	AROS_SLIB_ENTRY(Null, LIB, 0),	     /* ELL_Close() is never called */
	AROS_SLIB_ENTRY(Null, LIB, 0),	     /* ELL_Close() is never called */
	AROS_SLIB_ENTRY(Null, LIB, 0),	     /* ELL_Close() is never called */
	AROS_SLIB_ENTRY(Null, LIB, 0),	     /* ELL_Close() is never called */
	AROS_SLIB_ENTRY(MiamiSetSocksConn, Miami, 33),
	AROS_SLIB_ENTRY(Null, LIB, 0),	     /* ELL_Close() is never called */
	AROS_SLIB_ENTRY(MiamiIsOnline, Miami, 35),
	AROS_SLIB_ENTRY(MiamiOnOffline, Miami, 36),
	AROS_SLIB_ENTRY(Null, LIB, 0),	     /* ELL_Close() is never called */
	AROS_SLIB_ENTRY(inet_ntop, Miami, 38),
	AROS_SLIB_ENTRY(Miami_inet_aton, Miami, 39),
	AROS_SLIB_ENTRY(inet_pton, Miami, 40),
	AROS_SLIB_ENTRY(gethostbyname2, Miami, 41),
	AROS_SLIB_ENTRY(gai_strerror, Miami, 42),
	AROS_SLIB_ENTRY(freeaddrinfo, Miami, 43),
	AROS_SLIB_ENTRY(getaddrinfo, Miami, 44),
	AROS_SLIB_ENTRY(getnameinfo, Miami, 45),
	AROS_SLIB_ENTRY(if_nametoindex, Miami, 46),
	AROS_SLIB_ENTRY(if_indextoname, Miami, 47),
	AROS_SLIB_ENTRY(if_nameindex, Miami, 48),
	AROS_SLIB_ENTRY(if_freenameindex, Miami, 49),
	AROS_SLIB_ENTRY(MiamiSupportsIPV6, Miami, 50),
	AROS_SLIB_ENTRY(MiamiGetResOptions, Miami, 51),
	AROS_SLIB_ENTRY(MiamiSetResOptions, Miami, 52),
	AROS_SLIB_ENTRY(sockatmark, Miami, 53),
	AROS_SLIB_ENTRY(MiamiSupportedCPUs, Miami, 54),
	AROS_SLIB_ENTRY(MiamiGetFdCallback, Miami, 55),
	AROS_SLIB_ENTRY(MiamiSetFdCallback, Miami, 56),
	AROS_SLIB_ENTRY(Null, LIB, 0),	     /* ELL_Close() is never called */
	AROS_SLIB_ENTRY(MiamiGetCredentials, Miami, 58),
#ifdef __MORPHOS__
	-1,
	FUNCARRAY_32BIT_SYSTEMV,
#endif
	AROS_SLIB_ENTRY(FindKernelVar, Miami, 59),
#ifdef __MORPHOS__
	(f_void)-1,
	FUNCARRAY_END
#else
    (f_void)-1
#endif
};

