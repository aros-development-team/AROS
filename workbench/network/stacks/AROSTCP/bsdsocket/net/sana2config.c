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

#include <conf.h>

#include <sys/param.h>
#include <sys/socket.h>
#include <sys/socketvar.h>
#include <net/if.h>
#include <net/if_types.h>
#include <assert.h>
#include <kern/amiga_subr.h>

#if NS
#error NS is not supported
#endif

#include "kern/amiga_includes.h"
#include <proto/dos.h>

#include <netinet/in.h>
#include <devices/sana2.h>
#include <utility/tagitem.h>
#include "net/netdbpaths.h"
#include <net/sana2tags.h>
#include <net/sana2config.h>
#include <net/if_arp.h>
#include <net/if_sana.h>

#include <stdio.h>

#define CONFIGLINELEN 1024

/*
 * Nothing of this file is part of the stack program any more: the interface
 * line's arguments are parsed (ssconfig_parse()) and freed again by the
 * configuration reader (kern/netconfig_module.c), and the interface is made
 * of them there too (iface_make() -> ssconfig(), kern/netconfig_iface.c),
 * which is only in memory while a configuration is read. Only the old
 * line reader below is left, disabled.
 */

#if 0
static int
getconfs(BPTR iffh, UBYTE *buf)
{
    LONG i, quoted = 0, escaped = 0;

    D(bug("[AROSTCP] getconfs()\n"));

    if(FGets(iffh, buf, CONFIGLINELEN - 1)) {
        for(i = 0; buf[i]; i++) {
            UBYTE c = buf[i];
            if(c == '\n') {
                if(quoted) {
                    return ERROR_UNMATCHED_QUOTES;
                }
                if(i > 0 && buf[i - 1] == '+') {
                    i--;
                    if(i < CONFIGLINELEN - 2) {
                        if(FGets(iffh, buf + i, CONFIGLINELEN - 1 - i))
                            continue;
                        return IoErr();
                    }
                    return ERROR_LINE_TOO_LONG;
                }
                return 0;
            } else if(quoted) {
                if(escaped) {
                    escaped = 0;
                } else if(c == '*') {
                    escaped = 1;
                    continue;
                } else if(c == '"') {
                    quoted = 0;
                }
            } else if(c == ';' || c == '#') {
                buf[i++] = '\n';
                buf[i] = '\0';
                return 0;
            } else if(escaped) {
                escaped = 0;
            } else if(c == '*') {
                escaped = 1;
            } else if(c == '"') {
                quoted = 1;
            }
        }
        return ERROR_LINE_TOO_LONG;
    }

    buf[0] = '\0';
    return IoErr();
}
#endif
