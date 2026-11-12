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

/*
 * Copyright (C) 1985, 1988 Regents of the University of California.
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
 * The host's name and identity: gethostname(), gethostid() (vectors 47,
 * 48), sethostname(), the configured host name, and the configuration
 * variable USENAMESERVER (usens).
 *
 * Moved here unchanged from api/gethostnamadr.c (Stage 7, W0): these are
 * not name look-ups and stay in the core when the resolver becomes a
 * module of its own.
 */

#include <conf.h>

#include <aros/libcall.h>

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/socket.h>
#include <sys/malloc.h>
#include <netinet/in.h>
#include <net/if.h> /* for the gethostid() needs */
#include <arpa/inet.h>
#include <netdb.h>
#include <ctype.h>
#include <sys/errno.h>

#include <arpa/nameser.h>
#include <api/resolv.h>
#include <kern/amiga_includes.h>
#include <api/amiga_api.h>
#include <api/amiga_libcallentry.h>
#include <api/amiga_raf.h>
#include <api/allocdatabuffer.h>
#include <kern/amiga_subr.h>

#include <api/gethtbynamadr.h>     /* prototypes (NO MORE BUGS HERE) */
#include <api/apicalls.h>
#include <api/dns_cache.h>         /* dns_cache_lookup/insert, dns_cache_enabled */
#include <kern/amiga_netdb.h>      /* NDB, LOCK_R_NDB, HostentNode */

#include <stdio.h>

#include <proto/bsdsocket.h>

/*
 * USENAMESERVER: whether, and in which order, the name server and the
 * hosts table are asked (NO, FIRST, SECOND); read by the look-ups.
 */
LONG usens = 2;

/*
 * id_addr variable is used by both the gethostname() and gethostid().
 *
 * host_name is the host_name configuration variable.
 */
static ULONG id_addr = 0;

void findid(ULONG *); /* defined in net/if.c */

/*
 * Global host name and name length
 *
 * NOTE: host_name/host_namelen (and the global usens above) are shared,
 * unsynchronised globals. Concurrent sethostname()/gethostname() callers
 * can race on them. Locking is intentionally not added here to avoid
 * introducing a deadlock against the callers' higher-level locks.
 */
char host_name[MAXHOSTNAMELEN + 1] = { 0 };
size_t host_namelen = 0;

/****i* AmiTCP/sethostname *********************************************
*
*   NAME
*       sethostname -- set the name of the host
*
*   SYNOPSIS
*       error = sethostname(name, namelen);
*
*       int sethostname(const char *, size_t);
*
*   FUNCTION
*       Set the name of the host to the given 'name' of length 'namelen'.
*
*   INPUTS
*       name    - Pointer to the name string.
*       namelen - Length of the name.
*
*   RESULT
*       error   - 0 on success.
*
*   EXAMPLE
*
*   NOTES
*       This function is not intended to be provided to the applications,
*       this is for AmiTCP internal use only (at least for now).
*
*   BUGS
*
*   SEE ALSO
*****************************************************************************
*
*/
int sethostname(const char *name, size_t namelen)
{
#if defined(__AROS__)
    D(bug("[AROSTCP](gethostnameadr.c) sethostname()\n"));
#endif

    if(namelen > MAXHOSTNAMELEN)
        namelen = MAXHOSTNAMELEN;

    memcpy(host_name, name, namelen);
    host_name[namelen] = '\0';
    host_namelen = namelen;
    SetVar("HOSTNAME", name, namelen, GVF_GLOBAL_ONLY);

    return 0;
}

/****** bsdsocket.library/gethostname *************************************
*
*   NAME
*       gethostname -- get the name of the host
*
*   SYNOPSIS
*       error = gethostname(name, namelen);
*
*       long gethostname(char *, long);
*
*   FUNCTION
*       Get the name of the host to the buffer name of length namelen.
*       The name is queried from the netdb and/or the name server if
*       it is not explicitly configured (configuration variable
*       HOSTNAME).
*
*   INPUTS
*       name    - Pointer to the buffer where the name should be
*                 stored.
*       namelen - Length of the buffer name.
*
*   RESULT
*       error   - 0 on success.
*
*   EXAMPLE
*       char hostname[MAXHOSTNAMELEN];
*       long error;
*
*       error = gethostname(hostname, sizeof(hostname));
*       if (error < 0)
*         exit(10);
*
*       printf("My name is \"%s\".\n", hostname);
*
*   NOTES
*
*   BUGS
*       Unlike the Unix version, this version assures that the
*       resulting string is always NULL-terminated.
*
*   SEE ALSO
*       gethostid()
*****************************************************************************
*
*/
LONG __gethostname(STRPTR name, LONG namelen, struct SocketBase *libPtr)
{
    size_t host_namelen = 0;

#if defined(__AROS__)
    D(bug("[AROSTCP](gethostnameadr.c) __gethostname()\n"));
#endif

    CHECK_TASK();

    /*
     * Get the name with the gethostbyaddr(), if the name is not set yet.
     */
    if(*host_name == '\0') {
        struct hostent *hent;
        /* gethostid() */
        if(id_addr == 0)
            findid(&id_addr);

        if(id_addr != 0) {
            /* query if we have an address */
            hent = __gethostbyaddr((UBYTE *)&id_addr,
                                   sizeof(id_addr), AF_INET, libPtr);

            if(hent != NULL) {
                host_namelen = strnlen(hent->h_name, MAXHOSTNAMELEN);
                sethostname(hent->h_name, host_namelen);
            } else {
#if defined(__AROS__)
                D(bug("[AROSTCP](gethostnameadr.c) __gethostname: BUG!?! No Host ??\n"));
#endif
            }
        }
    } else host_namelen = strnlen(host_name, MAXHOSTNAMELEN);

#if defined(__AROS__)
    D(bug("[AROSTCP](gethostnameadr.c) __gethostname: namelen: %d host_namelen: %d\n", (int)namelen, (int)host_namelen));
#endif

    /*
     * Copy the name to the user buffer. stccpy() ensures that the buffer
     * is not written over and that it will be null-terminated.
     */
    if(namelen > host_namelen)
        namelen = host_namelen;
    else
        namelen--;			/* make space for the trailing '\0' */

    memcpy(name, host_name, namelen);
    name[namelen] = '\0';

    API_STD_RETURN(0, 0);
}

AROS_LH2(LONG, gethostname,
         AROS_LHA(STRPTR, name, A0),
         AROS_LHA(LONG, namelen, D0),
         struct SocketBase *, libPtr, 47, UL)
{
    AROS_LIBFUNC_INIT
#if defined(__AROS__)
    D(bug("[AROSTCP](gethostnameadr.c) gethostname()\n"));
#endif
    return __gethostname(name, namelen, libPtr);
    AROS_LIBFUNC_EXIT
}

/****** bsdsocket.library/gethostid ***************************************
*
*   NAME
*       gethostid -- get an unique 32-bit id to this host
*
*   SYNOPSIS
*       id = gethostid();
*
*       ULONG gethostid(void);
*
*   FUNCTION
*       Return the 32-bit unique id for this host. The Internet
*       address if the primary interface is used as the unique id.
*       This means that this function is also a supported way to get
*       the hosts IP address in AmiTCP/IP. If no interfaces are
*       configured, zero is returned. Any non-loobpack interface with
*       is preferred. Only if no other interfaces are present, is the
*       loopback address returned.
*
*   INPUTS
*
*   RESULT
*       id  - non-zero on success.
*
*   EXAMPLE
*       ULONG id;
*
*       id = gethostid();
*       if (id == 0)
*         exit(10);
*
*       printf("My primary IP address is: %s.\n", Inet_NtoA(id));
*
*   NOTES
*       Non-zero id is returned as soon as a interface is configured.
*       After that the id will not change, not even if the id is the
*       address of the loopback interface.
*
*   BUGS
*
*   SEE ALSO
*****************************************************************************
*
*/
/*ULONG SAVEDS gethostid(
   REG(a6, struct SocketBase * libPtr))*/
AROS_LH0(ULONG, gethostid,
         struct SocketBase *, libPtr, 48, UL)
{
    AROS_LIBFUNC_INIT

#if defined(__AROS__)
    D(bug("[AROSTCP](gethostnameadr.c) gethostid()\n"));
#endif

    if(id_addr == 0)
        findid(&id_addr);
    return id_addr;
    AROS_LIBFUNC_EXIT
}
