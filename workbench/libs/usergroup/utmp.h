#ifndef	UTMP_H
#define	UTMP_H
/*
 * Copyright (c) 1988 The Regents of the University of California.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by the University of
 *	California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 *	@(#)utmp.h	5.11 (Berkeley) 4/3/91
 */

#define _PATH_UTMP              "T:log/utmp"
#define _PATH_WTMP              "T:log/wtmp"
#define _PATH_LASTLOG           "T:log/lastlog"

#define UT_NAMESIZE             32
#define UT_LINESIZE             32
#define UT_HOSTSIZE             64

struct lastlog {
  long  ll_time;                /* the login time */
  uid_t ll_uid;                 /* user ID */
  char  ll_name[UT_NAMESIZE];   /* the login name */
  char  ll_host[UT_HOSTSIZE];   /* where the login originated */
};

#define ll_line ll_host

struct utmp {
  long  ut_time;                /* the login time */
  long  ut_sid;                 /* session ID */
  char  ut_name[UT_NAMESIZE];   /* the login name */
  char  ut_host[UT_HOSTSIZE];   /* where the login originated */
};

#define ut_line ut_host

#endif /* !UTMP_H */
