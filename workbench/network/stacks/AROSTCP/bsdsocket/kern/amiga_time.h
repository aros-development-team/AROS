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

#ifndef AMIGA_TIME_H
#define AMIGA_TIME_H

#ifndef SYS_CDEFS_H
#include <sys/cdefs.h>
#endif

#ifndef AMIGA_INCLUDES_H
#include <kern/amiga_includes.h>
#endif

/*
 * Globals defined in amiga_time.c
 */
#ifdef __MORPHOS__
extern struct Library    *TimerBase;
#else
extern struct Device     *TimerBase;
#endif

/*
 * Define an extended timerequest to make implementing the UNIX kernel function
 * timeout() easier.
 */

typedef void (*TimerCallback_t)(void);

struct timeoutRequest {
  struct timerequest timeout_request;	/* timer.device sees only this */
  struct timeval     timeout_timeval;   /* timeout interval */
  TimerCallback_t    timeout_function;  /* timeout function to be called */
};


/*
 * Command field must be TR_ADDREQUEST before this is called!
 * A request may be sent again ONLY AFTER PREVIOUS REQUEST HAS BEEN RETURNED!
 */
static inline void
sendTimeoutRequest(struct timeoutRequest *tr)
{
  tr->timeout_request.tr_time = tr->timeout_timeval;
  SendIO((struct IORequest *)&(tr->timeout_request));
}

/*
 * This MUST be called at splsoftclock()
 */
static inline void
handleTimeoutRequest(struct timeoutRequest *tr)
{
  /*
   * call the function
   */
  (*(tr->timeout_function))();
}

/*
 * prototypes for functions defined in kern/amiga_time.c
 */
ULONG timer_init(void);
void timer_deinit(void);
void timer_send(void);
struct timeoutRequest * createTimeoutRequest(TimerCallback_t fun,
					     ULONG seconds, ULONG micros);
void deleteTimeoutRequest(struct timeoutRequest *tr);
BOOL timer_poll(VOID);

#endif /* AMIGA_TIME_H */
