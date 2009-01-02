/*
 * Copyright (C) 1993 AmiTCP/IP Group, <amitcp-group@hut.fi>
 *                    Helsinki University of Technology, Finland.
 *                    All rights reserved.
 * Copyright (C) 2005 Neil Cafferkey
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

#ifndef AMIGA_INCLUDES_H
#define AMIGA_INCLUDES_H

/*
 * Standard Amiga includes
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

#ifndef DOS_DOS_H
#include <dos/dos.h>
#endif

#ifndef DOS_RDARGS_H
#include <dos/rdargs.h>
#endif

#ifndef SYS_CDEFS_H
#include <sys/cdefs.h>
#endif

#ifndef SYS_TYPES_H
#include <sys/types.h>
#endif

#if !defined(SYS_TIME_H)
#include <sys/time.h>
#endif

#ifndef PROTO_EXEC_H
#include <proto/exec.h>
#endif

#ifndef PROTO_DOS_H
#include <proto/dos.h>
#endif

#include <clib/alib_protos.h>

/*
 * for built in functions in SASC
 */
#if __SASC
#define USE_BUILTIN_MATH
#ifndef _STRING_H
#include <string.h>
#endif
#else

#include <string.h>

#if 0 /* NC */
#define _SIZE_T_        unsigned int            /* sizeof() */

#ifdef _SIZE_T_
typedef _SIZE_T_ size_t;
#undef _SIZE_T_
#endif

#ifndef NULL
#define NULL 0
#endif

#endif
#endif

extern struct ExecBase *SysBase;
/*
 * Amiga shared library prototypes
 */

#if 0
#if __GNUC__

#ifndef _INLINE_EXEC_H
#define Remove garbageRemove	/* we have inline version */
#define Forbid garbageForbid	/* we have inline version */
#define AbortIO garbageAbortIO	/* we have inline version */
#define NewList garbageNewList	/* conflicts with amiga.lib */
#include <inline/exec.h>
#undef NewList
#undef AbortIO
#undef Forbid
#undef Remove
#endif

#ifndef _INLINE_TIMER_H
/*
 * predefine TimerBase to Library to follow SASC convention.
 */
#define BASE_EXT_DECL extern struct Library * TimerBase;
#define BASE_NAME (struct Device *)TimerBase
#include <inline/timer.h>
#endif

static inline VOID  
BeginIO(register struct IORequest *ioRequest)
{
  register struct IORequest *a1 __asm("a1") = ioRequest;
  register struct Device *a6 __asm("a6") = ioRequest->io_Device;
  __asm __volatile ("jsr a6@(-0x1e)"
  : /* no output */
  : "r" (a6), "r" (a1)
  : "a0","a1","d0","d1", "memory");
}

static inline VOID  
AbortIO(register struct IORequest *ioRequest)
{
  register struct IORequest *a1 __asm("a1") = ioRequest;
  register struct Device *a6 __asm("a6") = ioRequest->io_Device;
  __asm __volatile ("jsr a6@(-0x24)"
  : /* no output */
  : "r" (a6), "r" (a1)
  : "a0","a1","d0","d1", "memory");
}

static inline VOID 
Remove(register struct Node *node)
{
  register struct Node *node2;

  node2 = node->ln_Succ;
  node = node->ln_Pred;
  node->ln_Succ = node2;
  node2->ln_Pred = node;
}

static inline VOID
Forbid(void)
{
  SysBase->TDNestCnt++;
}

#elif __SASC

#ifndef CLIB_EXEC_PROTOS_H
#include <clib/exec_protos.h>
#endif
#include <pragmas/exec_sysbase_pragmas.h>
#ifndef PROTO_TIMER_H
#include <proto/timer.h>
#endif

#pragma msg 93 ignore push

#if 0

extern VOID pragmaed_AbortIO(struct IORequest *);
#pragma libcall DeviceBase pragmaed_AbortIO 24 901

static inline __asm VOID 
AbortIO(register __a1 struct IORequest *ioRequest)
{
#define DeviceBase ioRequest->io_Device
  pragmaed_AbortIO(ioRequest);
#undef DeviceBase
}

#endif

extern VOID pragmaed_BeginIO(struct IORequest *);
#pragma libcall DeviceBase pragmaed_BeginIO 1E 901

static inline __asm VOID 
BeginIO(register __a1 struct IORequest *ioRequest)
{
#define DeviceBase ioRequest->io_Device
  pragmaed_BeginIO(ioRequest);
#undef DeviceBase
}

#pragma msg 93 pop

#endif
#endif


#if 0 /* NC */
/*
 * common inlines for both compilers
 */

static inline VOID
NewList(register struct List *list)
{
  list->lh_Head = (struct Node *)&list->lh_Tail;
  list->lh_Tail = NULL;
  list->lh_TailPred = (struct Node *)list;
}
#endif

/*
 * undef math log, because it conflicts with log() used for logging.
 */
#undef log

#endif /* !AMIGA_INCLUDES_H */

