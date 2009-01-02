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

#include <conf.h>
#if !defined(__AROS__)
#include <proto/sysdebug.h>
#endif
#include <sys/param.h>
#include <sys/systm.h>
#include <sys/syslog.h>

#include <devices/sana2.h>
#include <net/sana2errno.h>

extern const char * const io_errlist[];
extern const short io_nerr;
extern const char * const sana2io_errlist[];
extern const short sana2io_nerr;
extern const char * const sana2wire_errlist[];
extern const short sana2wire_nerr;

void 
sana2perror(const char *banner, struct IOSana2Req *ios2)
{
  register WORD err = ios2->ios2_Req.io_Error;
  register ULONG werr = ios2->ios2_WireError;
  const char *errstr;

  if (err >= sana2io_nerr || -err > io_nerr) {
    errstr = io_errlist[0];
  } else { 
    if (err < 0) 
      /* Negative error codes are common with all IO devices */
      errstr = io_errlist[-err];
    else 
      /* Positive error codes are SANA-II specific */ 
      errstr = sana2io_errlist[err];
  }

  if (werr == 0 || werr >= sana2wire_nerr) {
    __log(LOG_ERR, "%s: %s\n", banner, errstr);
  } else {
    __log(LOG_ERR, "%s: %s (%s)\n", banner, errstr, sana2wire_errlist[werr]);
  }
}

