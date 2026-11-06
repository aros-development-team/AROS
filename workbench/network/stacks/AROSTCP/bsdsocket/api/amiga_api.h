/*
 * Copyright (C) 1993 AmiTCP/IP Group, <amitcp-group@hut.fi>
 *                    Helsinki University of Technology, Finland.
 *                    All rights reserved.
 * Copyright (C) 2005-2026 The AROS Dev Team
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
extern struct Library *MasterNetServicesBase;

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

/*
 * What a task sleeping in the stack (tsleep(), WaitSelect()) needs: the
 * sleep queue link, the timer for timeouts, and the select buffer. The
 * opener uses the one in its base. On a base shared by the tasks of a
 * program (SBTC_CAN_SHARE_LIBRARY_BASES) every other thread borrows one for
 * the duration of a sleep (waiter_obtain()), so nothing outlives the call.
 */
struct SocketWaiter {
  struct MinNode	node;           /* on the base's free or busy list */
  struct Task *		task;           /* the task asleep */
  const char *		wmesg;
  queue_chain_t		sleep_link;
  caddr_t		wchan;                 /* event the task is awaiting */
  struct timerequest *	timer;
  struct MsgPort *	timerPort;
  struct newselbuf *	sb;          /* select buffer during WaitSelect() */
  struct DataBuffer	selitems;    /* WaitSelect()'s fd masks and items */
};

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
  ULONG			sigReconfigMask;   /* SBTC_SIG_RECONFIG_MASK: reload begin/end */
  ULONG			sigAddrChangeMask; /* SBTC_SIG_ADDRESS_CHANGE_MASK: address change */
/* -- the opener's tsleep()/wakeup()/WaitSelect() state -- */
  struct SocketWaiter	waiter;
/* -- per process fields used by various 'library' functions -- */
/* buffer for inet_ntoa */
  char			inet_ntoa[20]; /* xxx.xxx.xxx.xxx\0 */
/* -- pointers for data buffers that MAY be used -- */
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
  struct NetentNode *NetentNode;
  struct ServentNode *ServentNode;
/* -- set under syscall_semaphore -- */
  UBYTE			closing;      /* UL_Close is closing the table */
  struct Task *		closer;       /* the task closing a shared base */
  UBYTE			hookBase;     /* api_hookbase_open(), sleeping only */
/*
 * -- sharing among the tasks of a program (SBTC_CAN_SHARE_LIBRARY_BASES) --
 * A shared base belongs to the program, not to the task that opened it,
 * which may end first: thisTask is NULL and every caller sleeps on a lent
 * waiter.
 */
  UBYTE			shared;       /* any task may call through the base */
  struct Hook *		errorHook;    /* SBTC_ERROR_HOOK, called by the caller */
/* tasks that set the signal masks; the masks' signals go to them */
  struct Task *		sigIOTask;
  struct Task *		sigUrgTask;
  struct Task *		sigEventTask;
/* waiters lent to threads other than the opener (waiter_obtain()) */
  struct SignalSemaphore WaiterLock;
  struct MinList	freeWaiters;
  struct MinList	busyWaiters;
/* the resolver state and host buffers above, for one caller at a time */
  struct SignalSemaphore ResolverLock;
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
extern struct SignalSemaphore baselist_semaphore;
extern struct List releasedSocketList;

/* A releasedSocketList entry */
struct SocketNode {
    struct MinNode	sn_Node;
    LONG			sn_Id;
    struct socket 	*sn_Socket;
};

/*
 *  Functions to put and remove application library to/from exec library list
 */
BOOL api_init(VOID);
struct SocketBase *api_hookbase_open(VOID);
VOID api_hookbase_close(struct SocketBase *);
BOOL api_show(VOID);
VOID api_hide(VOID);
VOID api_setfunctions(VOID);
VOID api_sendbreaktotasks(VOID);
VOID api_logopeners(VOID);

/*
 * Reconfigure notification: signal every consumer that set
 * SBTC_SIG_RECONFIG_MASK.  begin=TRUE marks the start of an in-place reload
 * (state -> NETRC_RECONFIGURING); begin=FALSE marks the end (generation bumped,
 * state -> NETRC_ONLINE).  State and generation are read via SocketBaseTagList.
 */
VOID api_sendreconfig(BOOL begin);
VOID api_sendaddrchange(VOID);
extern ULONG api_reconfig_state;        /* NETRC_ONLINE / NETRC_RECONFIGURING */
extern ULONG api_reconfig_generation;   /* bumped after each completed reload  */
extern ULONG api_reconfig_expected;     /* consumers signalled at reconfigure-begin */
extern volatile ULONG api_reconfig_acked; /* consumers that acked (SBTC_RECONFIG_ACK) */
VOID api_deinit(VOID);

/* SBTC_CAN_SHARE_LIBRARY_BASES: the base becomes the program's, no task's */
VOID api_sharebase(struct SocketBase *);

/* Function which sets Errno value */

VOID writeErrnoValue(struct SocketBase *, int);
/* Hands h_errno to the caller's SBTC_ERROR_HOOK, if one is installed */
VOID writeHErrnoValue(struct SocketBase *);

/*
 * inline functions which changes (raises) task priority while it is
 * executing library functions
 */

/*
 * The priority change is the calling task's, which on a shared base need not
 * be the opener. myPri and libCallPri are only used under the semaphore.
 */
static inline void ObtainSyscallSemaphore(struct SocketBase *libPtr)
{
  extern struct Task *AROSTCP_Task;

  ObtainSemaphore(&syscall_semaphore);
  libPtr->myPri = SetTaskPri(FindTask(NULL),
  libPtr->libCallPri = AROSTCP_Task->tc_Node.ln_Pri);
}

static inline void ReleaseSyscallSemaphore(struct SocketBase *libPtr)
{
  struct Task *me = FindTask(NULL);

  if (libPtr->libCallPri != (libPtr->myPri = SetTaskPri(me, libPtr->myPri)))
    SetTaskPri(me, libPtr->myPri);
  ReleaseSemaphore(&syscall_semaphore);
}

/*
 * The waiter the calling task sleeps with (NULL when none can be set up),
 * and its return. See struct SocketWaiter.
 */
struct SocketWaiter *waiter_obtain(struct SocketBase *libPtr);
void waiter_release(struct SocketBase *libPtr, struct SocketWaiter *w);

/*
 * The resolver calls use per-base buffers and resolver state, and sleep
 * while they wait for a name server, so the tasks sharing a base make them
 * one at a time.
 */
static inline void ObtainResolver(struct SocketBase *libPtr)
{
  ObtainSemaphore(&libPtr->ResolverLock);
}

static inline void ReleaseResolver(struct SocketBase *libPtr)
{
  ReleaseSemaphore(&libPtr->ResolverLock);
}

/*
 * inline function for searching library base when taskpointer is known
 */

static inline struct SocketBase *FindSocketBase(struct Task *task)
{
  extern struct List socketBaseList;
  struct Node *libNode;

  ObtainSemaphoreShared(&baselist_semaphore);
  for (libNode = socketBaseList.lh_Head; libNode->ln_Succ;
       libNode = libNode->ln_Succ)
    if (((struct SocketBase *)libNode)->thisTask == task) {
      ReleaseSemaphore(&baselist_semaphore);
      return (struct SocketBase *)libNode;
    }
  /* here if Task wasn't in socketBaseList */
  ReleaseSemaphore(&baselist_semaphore);
  return NULL;
}

#endif /* !AMIGA_API_H */
