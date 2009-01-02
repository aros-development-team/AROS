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

#ifndef AMIGA_API_H
#define AMIGA_API_H

#ifndef EXEC_TYPES_H
#error <exec/types.h> not included.
#endif

#ifndef EXEC_LIBRARIES_H
#error <exec/libraries.h> not included.
#endif

#ifndef EXEC_SEMAPHORES_H
#error <exec/semaphores.h> not included.
#endif

#ifndef EXEC_TASKS_H
#include <exec/tasks.h>
#endif

#ifndef SYS_CDEFS_H
#include <sys/cdefs.h>
#endif

#ifndef SYS_TYPES_H
#include <sys/types.h>
#endif

#ifndef SYS_QUEUE_H
#include "sys/queue2.h"
#endif

#ifndef API_RESOLV_H
#include <api/resolv.h>
#endif

#include <proto/exec.h>
#include <kern/amiga_config.h>

enum apistate {
	API_SCRATCH,                /* api not initialized */
	API_INITIALIZED,            /* librarybase created */
	API_SHOWN,                  /* librarybase made visible */
	API_HIDDEN,                 /* librarybase hidden */
	API_FUNCTIONPATCHED         /* Api functions set to return -1 */
};
extern enum apistate api_state;

#ifdef __MORPHOS__
#pragma pack(2)
#endif

struct LibInitTable {
  UBYTE byte1; UBYTE offset1; UBYTE ln_type; UBYTE pad0;
  UBYTE byte2; UBYTE offset2; UBYTE lib_flags; UBYTE pad1;
  UBYTE long3; UBYTE offset3; ULONG ln_Name;
  UBYTE word4; UBYTE offset4; UWORD lib_Version;
  UBYTE word5; UBYTE offset5; UWORD lib_Revision;
  UBYTE long6; UBYTE offset6; ULONG lib_IdString;
  UBYTE end7;
};

#ifdef __MORPHOS__
#pragma pack()
#endif

extern struct LibInitTable Miami_initTable;
extern struct Library *MasterMiamiBase;

struct newselbuf;

/*
 * structure for holding size and address of some dynamically allocated buffers
 * such as selitems for WaitSelect() and netdatabase entry structures
 */
struct DataBuffer {
  int		db_Size;
  void *	db_Addr;
};

typedef int (* ASM fdCallback_t)(REG(d0, int fd), REG(d1, int action));

struct SocketBase {
  struct Library	libNode;
/* -- "Global" Errno -- */
  BYTE			flags;
  BYTE			errnoSize;                             /* 1, 2 or 4 */
 /* -- now we are longword aligned -- */
  UBYTE *		errnoPtr;                   /* this points to errno */
  LONG			defErrno;
/* Task pointer of owner task */
  struct Task *		thisTask;
/* task priority changes (WORDS so we keep structure longword aligned) */  
  BYTE			myPri;        /* task's priority just after libcall */
  BYTE			libCallPri;  /* task's priority during library call */
/* note: not long word aligned at this point */
/* -- descriptor sets -- */
  WORD			dTableSize; /* long word aligned again */
  struct socket	**	dTable;
  fdCallback_t		fdCallback;
/* AmiTCP signal masks */
  ULONG			sigIntrMask;
  ULONG			sigIOMask;
  ULONG			sigUrgMask;
  ULONG			sigEventMask;
/* -- these are used by tsleep()/wakeup() -- */
  const char *		p_wmesg;
  queue_chain_t 	p_sleep_link;
  caddr_t		p_wchan;               /* event process is awaiting */
  struct timerequest *	tsleep_timer;
  struct MsgPort *	timerPort;
/* -- pointer to select buffer during Select() -- */
  struct newselbuf *	p_sb;
/* -- per process fields used by various 'library' functions -- */
/* buffer for inet_ntoa */
  char			inet_ntoa[20]; /* xxx.xxx.xxx.xxx\0 */
/* -- pointers for data buffers that MAY be used -- */
  struct DataBuffer	selitems;
  struct DataBuffer	hostents;
  struct DataBuffer	netents;
  struct DataBuffer	protoents;
  struct DataBuffer	servents;
/* -- variables for the syslog (see netinclude:sys/syslog.h) -- */
  UBYTE			LogStat;                                  /* status */
  UBYTE			LogMask;                     /* mask for log events */
  UWORD			LogFacility;                       /* facility code */
  const char *		LogTag;	           /* tag string for the log events */
/* -- resolver variables -- */
  LONG *		hErrnoPtr;
  LONG			defHErrno;
  LONG			res_socket;       /* socket used for resolver comm. */
  struct state          res_state;
/* -- socket events -- */
  struct SignalSemaphore EventLock;
  struct MinList	EventList;
/* -- buffer for string returns -- */
  UBYTE			result_str[REPLYBUFLEN + 1];
/* -- NetDB pointers for getXXXent() and friends -- */
  struct HostentNode *HostentNode;
  struct ProtoentNode *ProtoentNode;
};

/* 
 * Socket base flags 
 */
#define SBFB_COMPAT43	0L	    /* compat 43 code (without sockaddr_len) */

#define SBFF_COMPAT43   1L

/*
 * macro for getting error value pointed by the library base. All but
 * the lowest byte of the errno are assumed to stay zero. 
 */
int readErrnoValue(struct SocketBase *);

extern struct SignalSemaphore syscall_semaphore;
extern struct List releasedSocketList;

/*
 *  Functions to put and remove application library to/from exec library list
 */
BOOL api_init(VOID);
BOOL api_show(VOID);
VOID api_hide(VOID);
VOID api_setfunctions(VOID);
VOID api_sendbreaktotasks(VOID);
VOID api_deinit(VOID);

/* Function which sets Errno value */

VOID writeErrnoValue(struct SocketBase *, int);

/*
 * inline functions which changes (raises) task priority while it is
 * executing library functions
 */

static inline void ObtainSyscallSemaphore(struct SocketBase *libPtr)
{
  extern struct Task *AROSTCP_Task;

  ObtainSemaphore(&syscall_semaphore);
  libPtr->myPri = SetTaskPri(libPtr->thisTask,
  libPtr->libCallPri = AROSTCP_Task->tc_Node.ln_Pri);
}

static inline void ReleaseSyscallSemaphore(struct SocketBase *libPtr)
{
  if (libPtr->libCallPri != (libPtr->myPri = SetTaskPri(libPtr->thisTask,
							libPtr->myPri)))
    SetTaskPri(libPtr->thisTask, libPtr->myPri);
  ReleaseSemaphore(&syscall_semaphore);
}

/*
 * inline function for searching library base when taskpointer is known
 */

static inline struct SocketBase *FindSocketBase(struct Task *task)
{
  extern struct List socketBaseList;
  struct Node *libNode;

  Forbid();
  for (libNode = socketBaseList.lh_Head; libNode->ln_Succ;
       libNode = libNode->ln_Succ)
    if (((struct SocketBase *)libNode)->thisTask == task) {
      Permit();
      return (struct SocketBase *)libNode;
    }
  /* here if Task wasn't in socketBaseList */
  Permit();
  return NULL;
}

#endif /* !AMIGA_API_H */
