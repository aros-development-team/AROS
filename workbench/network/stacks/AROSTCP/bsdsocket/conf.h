/*
 * Copyright (C) 1993 AmiTCP/IP Group, <amitcp-group@hut.fi>
 *                    Helsinki University of Technology, Finland.
 *                    All rights reserved.
 * Copyright (C) 2005 Neil Cafferkey
 * Copyright (C) 2005 Pavel Fedin
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

/*
 * This is include file, which includes most of AmiTCP/IP include files
 * for creation of an GST file for SASC.
 *
 * This file must be maintained to be consistent with the project itself
 * (ie. remember to add new include files when they are defined).
 */

/* NC */

#ifndef SAVEDS /* NC: in case our own cdefs.h is included */
#define SAVEDS
#define REGARGFUN
#define STKARGFUN
#define ALIGNED
#define ASM
#ifndef REG
#if defined(__mc68000) && !defined(__AROS__)
#define REG(A, B) B __asm(#A)
#else
#define REG(A, B) B
#endif
#endif
#endif

/* __PPC__ definition is dropped in gcc 4.4.2, but our code relies on it. Fix this up. */
#ifdef __powerpc__
#ifndef __PPC__
#define __PPC__
#endif
#endif

/*
 * For m68k, AmigaOS4 and AROS we build Roadshow-compatible version
 */
#ifdef __mc68000
#define __CONFIG_ROADSHOW__
#endif
#ifdef __AMIGAOS4__
#define __CONFIG_ROADSHOW__
#endif
#if defined(__AROS__)
#define __CONFIG_ROADSHOW__
#endif
/* 
 * Include first configuration information
 *
 * This file does not include any other include files and can be compiled
 * anytime. Include it first for the definitions to be effective during
 * following includes, which may depend on preprocessor symbols defined here.
 */
#include <conf/conf.h>

#include <sys/types.h>

#if __SASC
#define USE_BUILTIN_MATH
#ifndef _STRING_H
#include <string.h>
#endif
#endif

#ifdef __MORPHOS__
#define VA68K __attribute((varargs68k))
#else
#define VA68K
#endif

/*
 * sys/param.h includes basic information about system and C-compiler
 * environment.  It also contains for example incomplete structure
 * definitions for all prototype files in protos/ (structure pointers
 * only).  This is why this file must be included first, since SASC
 * don't like incomplete declarations _after_ complete ones.
 */
#include <sys/param.h>

/*
 * following files are included instead of #include <kern/amiga_includes.h>
 */
#ifndef EXEC_TYPES_H
#include <exec/types.h>
#endif

#ifndef EXEC_LISTS_H
#include <exec/lists.h>
#endif

#ifndef EXEC_MEMORY_H
#include <exec/memory.h>
#endif

#ifndef EXEC_SEMAPHORES_H
#include <exec/semaphores.h>
#endif

#ifndef EXEC_LIBRARIES_H
#include <exec/libraries.h>
#endif

#ifndef EXEC_DEVICES_H
#include <exec/devices.h>
#endif

#ifndef EXEC_EXECBASE_H
#include <exec/execbase.h>
#endif

#ifndef DEVICES_TIMER_H
#include <devices/timer.h>
#endif

#ifndef DOS_DOS_H
#include <dos/dos.h>
#endif

#ifndef SYS_CDEFS_H
#include <sys/cdefs.h>
#endif

#ifndef CLIB_EXEC_PROTOS_H
#include <clib/exec_protos.h>
#endif
extern struct ExecBase *SysBase;
/*#include <pragmas/exec_sysbase_pragmas.h>*/ /* NC */

#ifndef PROTO_TIMER_H
#include <proto/timer.h>
#endif

/*
 * undef math log, because it conflicts with log() used for logging.
 */
#undef log

#include <sys/time.h>
#include "kern/uio.h"

#include <sys/errno.h>
#include <sys/ioctl.h>
#include <sys/kernel.h>
#include <sys/mbuf.h>
#include "sys/queue2.h"
#include <sys/domain.h>
#include <sys/socket.h>
#include <sys/socketvar.h>
#include <sys/protosw.h>   /* Was just before socket.h. NC */
#include <sys/syslog.h>
#include <sys/queue.h>

#include <net/route.h>
#include <net/if.h>
#include <net/netisr.h>
#include <net/if_types.h>
#include <net/raw_cb.h>
#include <net/radix.h>

#include <netinet/in_systm.h>
#include <netinet/in.h>
#include <netinet/ip.h>

#include <netinet/ip_var.h>
#include <netinet/ip_icmp.h>
#include <netinet/icmp_var.h>

#include <netinet/in_pcb.h>
#include <netinet/in_var.h>

#include <net/if_arp.h>
#include <net/if_sana.h>
#include <net/sana2arp.h>
#include <net/sana2tags.h>
#include <net/sana2config.h> /* NC */

#include <netinet/tcp.h>
#include <netinet/tcp_timer.h>
#include <netinet/tcp_var.h>
#include <netinet/tcpip.h>
#include <netinet/tcp_fsm.h>
#include <netinet/tcp_seq.h>
#include <netinet/tcp_debug.h>

#include <netinet/udp.h>
#include <netinet/udp_var.h>

#include <api/amiga_raf.h>
#include <kern/amiga_log.h>
#include <kern/amiga_subr.h> /* NC */

#include <ctype.h>
#include <stdarg.h>
#include <limits.h>
#include <stddef.h>

#ifdef DEBUG
#define D(x) x
#else
#define D(x)
#endif
#ifdef DEBUG_NETDB
#define DNETDB(x) x
#else
#define DNETDB(x)
#endif
#ifdef DEBUG_SANA
#define DSANA(x) x
#else
#define DSANA(x)
#endif
#ifdef DEBUG_NETTRACE
#define DNETTRACE(x) x
#else
#define DNETTRACE(x)
#endif
#ifdef RES_DEBUG
#define DRES(x) x
#else
#define DRES(x)
#endif
#ifdef DEBUG_TSLEEP
#define DTSLEEP(x) x
#else
#define DTSLEEP(x)
#endif
#ifdef DEBUG_EVENTS
#define DEVENTS(x) x
#else
#define DEVENTS(x)
#endif
#ifdef DEBUG_SYSLOG
#define DSYSLOG(x) x
#else
#define DSYSLOG(x)
#endif
#ifdef DEBUG_SYSCALLS
#define DSYSCALLS(x) x
#else
#define DSYSCALLS(x)
#endif
#ifdef DEBUG_OPTERR
#define DOPTERR(x) x
#else
#define DOPTERR(x)
#endif
#ifdef DEBUG_IFCONFIG
#define DIFCONF(x) x
#else
#define DIFCONF(x)
#endif
#ifdef DEBUG_GUI
#define DGUI(x) x
#else
#define DGUI(x)
#endif
#ifdef DEBUG_PF
#define DPF(x) x
#else
#define DPF(x)
#endif
#ifdef DEBUG_ROUTE
#define DROUTE(x) x
#else
#define DROUTE(x)
#endif
#ifdef DEBUG_DHCP
#define DDHCP(x) x
#else
#define DDHCP(x)
#endif

#if 0
GLOBAL struct IntuitionBase *IntuitionBase;
GLOBAL struct Library *GadToolsBase;
GLOBAL struct GfxBase *GfxBase;
#endif
GLOBAL struct DosLibrary *DOSBase;
#ifdef __MORPHOS__
GLOBAL struct Library *UtilityBase;
GLOBAL struct Library *RexxSysBase;
#else
GLOBAL struct UtilityBase *UtilityBase;
GLOBAL struct RxsLib *RexxSysBase;
#endif

#define ENABLE_PACKET_FILTER

/* This option is now obsolete
#define ENABLE_TTCP_SHUTUP */

/* Global define to enable Debug output for AROS */
#if defined(__AROS__)
#define DEBUG 0
#include <aros/debug.h>
#endif

// ## "TODO: NicJA - Would it be better to use differing pools for different data types?"
/* Defines used by the stack to Allocate sufficient memory space for _ALL_ allocated blocks */
/*       see kern_malloc.c                                                                  */
#define __MALLOC_POOLSIZE            (128*1024)               // original Value was (16x1024)bytes
#define __MALLOC_POOLSIZE_THRESHOLD  (16*1024)
