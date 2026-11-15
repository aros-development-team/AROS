/*
 * Copyright (C) 1993 AmiTCP/IP Group, <amitcp-group@hut.fi>
 *                    Helsinki University of Technology, Finland.
 *                    All rights reserved.
 * Copyright (C) 2005 - 2026 The AROS Dev Team
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
 * Copyright (C) 1983, 1988 Regents of the University of California.
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
 */

/*
 * Number formatting and the reply buffer of the configuration and ARexx
 * variables: ultoa(), ltoa(), CS_Alloc().
 *
 * Moved here unchanged from kern/amiga_cstat.c (Stage 7, W0): the
 * configuration parser uses them, and they stay in the core when the ARexx
 * status reports become part of the administration module.
 */

#include <conf.h>

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/socket.h>
#include <sys/socketvar.h>
#include <sys/synch.h>
#include <sys/malloc.h>
#include <sys/syslog.h>

#include <kern/amiga_includes.h>
#include <kern/amiga_rexx.h>
#include <kern/amiga_config.h>

#include <dos/rdargs.h>

#include <proto/dos.h>

#include <api/apicalls.h>

int
ultoa(unsigned long ul, char *buffer)
{
    static char buf[10];
    char *p;
    int len;

    p = buf;
    *p = '\0';

    do {
        *++p = (ul % 10) + '0';
    } while(ul /= 10);

    len = p - buf;

    while(*buffer++ = *p--)
        ;
    return(len);
}

int
ltoa(long l, char *buffer)
{
    int len = 0;

    if(l < 0) {
        *buffer++ = '-';
        l = -l;
        len++;
    }
    return (len + ultoa((unsigned long)l, buffer));
}

/*
 * Allocate big enough buffer for reply
 */
LONG
CS_Alloc(struct CSource *reply, size_t size)
{
    /* do we have enough space? */
    if(reply->CS_Length < size) {
        char *buffer;
        if((buffer = bsd_malloc(size, M_TEMP, 0)) == NULL) {
            return 0;
        }
        /*
         * Old buffer will be freed by caller
         */
        reply->CS_Buffer = buffer;
        reply->CS_Length = size;
    }

    return 1;
}
