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

#ifndef AMIGA_LIBCALLENTRY_H
#define AMIGA_LIBCALLENTRY_H

#ifndef AMIGA_API_H
#error include amiga_api.h before this (libcallentry.h)
#endif

#ifndef SYS_CDEFS_H
#include <sys/cdefs.h>
#endif

#ifndef SYS_ERRNO_H
#include <sys/errno.h>
#endif

#ifndef SYS_SYSLOG_H
#include <sys/syslog.h>
#endif

/*
 * The following macros are written in each socket library functions
 * (execpt Errno()). they makes sure that the task that calls library
 * functions is the opener task of the socketbase it is using.
 */

extern const char wrongTaskErrorFmt[];

#define CHECK_TASK()					\
  if (libPtr->thisTask != FindTask(NULL)) {		\
    struct Task * wTask = FindTask(NULL);		\
    __log(LOG_CRIT, wrongTaskErrorFmt, wTask,		\
	wTask->tc_Node.ln_Name,	libPtr->thisTask,	\
	libPtr->thisTask->tc_Node.ln_Name);		\
    return -1;						\
  }

#define CHECK_TASK_NULL()				\
  if (libPtr->thisTask != FindTask(NULL)) {		\
    struct Task * wTask = FindTask(NULL);		\
    __log(LOG_CRIT, wrongTaskErrorFmt, wTask,		\
	wTask->tc_Node.ln_Name,	libPtr->thisTask,	\
	libPtr->thisTask->tc_Node.ln_Name);		\
    return NULL;					\
  }

#define CHECK_TASK2() CHECK_TASK_NULL()

#define CHECK_TASK_VOID()				\
  if (libPtr->thisTask != FindTask(NULL)) {		\
    struct Task * wTask = FindTask(NULL);		\
    __log(LOG_CRIT, wrongTaskErrorFmt, wTask,		\
	wTask->tc_Node.ln_Name,	libPtr->thisTask,	\
	libPtr->thisTask->tc_Node.ln_Name);		\
    return;						\
  }

#define API_STD_RETURN(error, ret)	\
  if (error == 0)			\
    return ret;				\
  writeErrnoValue(libPtr, error);	\
  return -1;
						
/*
 * getSock() gets a socket referenced by given filedescriptor if exists,
 * returns EBADF (bad file descriptor) if not. (because this now uses
 * struct socket * pointer and those are often register variables, perhaps
 * some kind of change is to be done here).
 */

static inline LONG getSock(struct SocketBase *p, int fd, struct socket **sop)
{
  register struct socket *so;
  
  if ((unsigned)fd >= p->dTableSize || (so = p->dTable[(short)fd]) == NULL)
    return (EBADF);
  *sop = so;
  return 0;
}

/*
 * Prototype for sdFind. This is located in amiga_syscalls.c and replaces
 * fdAlloc there. libPtr->nextDToSearch is dumped.
 */
LONG sdFind(struct SocketBase * libPtr, LONG *fdp);

#ifndef AMIGA_RAF_H
#include <api/amiga_raf.h>
#endif

#endif /* !AMIGA_LIBCALLENTRY_H */
