/*
 * Copyright (C) 1993,1994 AmiTCP/IP Group, <amitcp-group@hut.fi>
 *                         Helsinki University of Technology, Finland.
 *                         All rights reserved.
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

#include <exec/types.h>

#include <sys/systm.h>
#include <sys/malloc.h>
#include <kern/amiga_config.h>
#include <kern/amiga_netdb.h>
#include <kern/accesscontrol.h>

#include <sys/syslog.h>

int controlaccess(struct in_addr shost, unsigned short dport)
{
    int i;

    /*
     * NOTE: matching is intentionally first-match-wins. The ACF_CONTINUE
     * flag is NOT honoured here as a "keep scanning" directive: addaccessent()
     * sets ACF_CONTINUE on *every* access entry (it is used as a non-zero
     * validity/sentinel bit so that the ai_flags == 0 terminator below marks
     * the end of the table). Treating ACF_CONTINUE as "continue scanning"
     * would cause every match to fall through to the default-allow below,
     * which would silently disable all "deny" rules. Do not change this
     * without also changing how the table sentinel is encoded.
     */
    LOCK_R_NDB(NDB);
    for(i = 0; NDB->ndb_AccessTable[i].ai_flags; i++)
#define AT NDB->ndb_AccessTable[i]
#define host (*(ULONG *)&shost)				/* XXX */
        if((AT.ai_port == 0 && (!(AT.ai_flags & ACF_PRIVONLY) || dport < 1024)
                || AT.ai_port == dport) &&
                ((host ^ AT.ai_host) & AT.ai_mask) == 0) {
            /*
             * match
             */
            int allow = AT.ai_flags & ACF_ALLOW;

            if(AT.ai_flags & ACF_LOG)
                __log(allow ? LOG_INFO : LOG_NOTICE,
                      "Access from host %ld.%ld.%ld.%ld to port %ld %s\n",
                      host >> 24 & 0xff, host >> 16 & 0xff, host >> 8 & 0xff, host & 0xff,
                      (long)dport, allow ? "allowed" : "denied");

            UNLOCK_NDB(NDB);
            return allow;
#undef allow
#undef host
#undef AT
        }
    /*
     * No match. allow by default.
     */
    UNLOCK_NDB(NDB);
    return ACF_ALLOW;
}
