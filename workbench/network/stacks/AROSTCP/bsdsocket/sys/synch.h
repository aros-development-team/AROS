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

#ifndef SYS_SYNCH_H
#define SYS_SYNCH_H

#ifndef SYS_TYPES_H
#include <sys/types.h>
#endif

#ifndef EXEC_TYPES_H
#include <exec/types.h>
#endif

#ifndef SYS_TIME_H
#include <sys/time.h>
#endif

#if !defined(AMIGA_INCLUDES_H) && defined(DEBUG)
#include <kern/amiga_includes.h> /* for the inline spl_n() */
#endif

#include <proto/exec.h>

extern BOOL sleep_init(void);
extern void tsleep_send_timeout(struct SocketBase *, const struct timeval *);
extern void tsleep_abort_timeout(struct SocketBase *, const struct timeval *);
extern void tsleep_enter(struct SocketBase *, caddr_t, const char *);
extern int  tsleep_main(struct SocketBase *, ULONG blockmask);
extern int  tsleep(struct SocketBase *, caddr_t, const char *,const struct timeval *);
extern void wakeup(caddr_t);

/*
 * Spl-levels used in this implementation
 */
#define SPL0         0
#define SPLSOFTCLOCK 1
#define SPLNET       2
#define SPLIMP       3

/*
 * Spl-function prototypes and definitions.
 *
 * spl_t is the return type of the spl_n(). It should be used when defining
 * storage to store the return value, using int may be little slower :-) 
 */

extern BOOL spl_init(void);

/*#ifdef DEBUG*/ /* NC */
#if 1

typedef int spl_t;

extern spl_t spl_n(spl_t newlevel);

#define spl0()          spl_n(SPL0)
#define splsoftclock()  spl_n(SPLSOFTCLOCK)
#define splnet()        spl_n(SPLNET)
#define splimp()        spl_n(SPLIMP)
#define splx(s)         spl_n(s)

#else

typedef BYTE spl_t;		/* the type of SysBase->TDNestCnt */

extern struct ExecBase * SysBase;

extern spl_t spl_level;

static inline spl_t
splx(spl_t new_level)
{
  spl_t old_level;

  old_level = spl_level;
  if(spl_level == SPL0 && new_level > SPL0)
    Forbid();
  spl_level = new_level;
  if(old_level > SPL0 && new_level == SPL0)
    Permit();

  return old_level;
}

#define spl0()          splx(SPL0)
#define splsoftclock()  splx(SPLSOFTCLOCK)
#define splnet()        splx(SPLNET)
#define splimp()        splx(SPLIMP)

#endif /* DEBUG */

#endif /* !SYS_SYNCH_H */
