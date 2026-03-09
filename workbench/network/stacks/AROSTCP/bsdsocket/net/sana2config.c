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

static const char template[] = SSC_TEMPLATE;

#define CONFIGLINELEN 1024
#ifndef __mc68000__
#define SANA2_LARGEREQALLOCS
#define SANA2_DEF_WRITENO 64
#else
#define SANA2_DEF_WRITENO 16
#endif

/*
 * Parse the configuration
 */
struct ssconfig *
ssconfig_parse(struct RDArgs *rdargs)
{
    struct ssconfig *config = AllocVec(sizeof(*config), MEMF_CLEAR | MEMF_PUBLIC);

    D(bug("[AROSTCP] ssconfig_parse()\n"));

    if(config != NULL) {

        if(ReadArgs(template, (IPTR *)config->args, rdargs)) {
            config->rdargs = rdargs;
            config->flags |= SSCF_RDARGS;
            return config;
        } else {
            FreeVec(config);
        }
    }
    return NULL;
}

/*
 * Free the configuration
 */
void
ssconfig_free(struct ssconfig config[])
{
    D(bug("[AROSTCP] ssconfig_free()\n"));
    if(config->flags & SSCF_RDARGS)
        FreeArgs(config->rdargs);
    FreeVec(config);
}

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

/*
 * Default configuration as per hardware type
 */
static const struct wire_defaults {
    LONG  wd_wiretype;
    LONG  wd_iptype;		/* IPv4 packet type */
    WORD  wd_ipno;		/* minimum IPv4 read requests */
    WORD  wd_writeno;		/* minimum write requests */
    LONG  wd_arptype;		/* ARP packet type */
    WORD  wd_arpno;		/* minimum ARP read requests */
    WORD  wd_arphdr;		/* ARP hardware type */
    LONG  wd_ip6type;		/* IPv6 packet type (0 = no IPv6) */
    WORD  wd_ip6no;		/* minimum IPv6 read requests */
    WORD  wd_ifflags;		/* Interface flags */
} wire_defaults[] = {
    {
        S2WireType_Ethernet,
        ETHERTYPE_IP, 16, SANA2_DEF_WRITENO,
        ETHERTYPE_ARP, 4, 1,
        ETHERTYPE_IPV6, 16,
        IFF_BROADCAST | IFF_SIMPLEX,
    },
    {
        S2WireType_Arcnet,
        ARCOTYPE_IP, 16, 16,
        ARCOTYPE_ARP, 4, 7,
        0, 0,
        IFF_BROADCAST | IFF_SIMPLEX,
    },
    {
        S2WireType_SLIP,
        SLIPTYPE_IP, 8, 8,
        0, 0, 0,
        0, 0,
        IFF_POINTOPOINT | IFF_NOARP,
    },
    {
        S2WireType_CSLIP,
        SLIPTYPE_IP, 8, 8,
        0, 0, 0,
        0, 0,
        IFF_POINTOPOINT | IFF_NOARP,
    },
    {
        S2WireType_PPP,
        PPPTYPE_IP, 8, 8,
        0, 0, 0,
        0, 0,
        IFF_POINTOPOINT | IFF_NOARP,
    },
    /* Use ethernet as default */
    {
        0,
        ETHERTYPE_IP, 16, SANA2_DEF_WRITENO,
        ETHERTYPE_ARP, 4, 1,
        ETHERTYPE_IPV6, 16,
        IFF_BROADCAST | IFF_SIMPLEX,
    },
};

#ifdef SANA2_LARGEREQALLOCS
/*
 * Scale a SANA-II request count based on the link speed reported by
 * S2_DEVICEQUERY.  Each read request can receive exactly one frame, so
 * the pool must cover the driver-to-stack round-trip latency.
 *
 * Approximate minimum in-flight requests for 1500-byte frames at
 * ~100 us turnaround:
 *     10 Mbps  ->   ~1    (16 is plenty)
 *    100 Mbps  ->   ~8    (32 comfortable)
 *      1 Gbps  ->  ~81    (128 with burst headroom)
 *     10 Gbps  -> ~812    (256 minimum, 512 with headroom)
 *
 * SANA-II BPS is ULONG and overflows above ~4.3 Gbps.  A BPS of 0 is
 * treated as "unknown / very fast" and gets the maximum allocation.
 *
 * Memory cost per request: ~2.2 KB (IOIPReq + mbuf cluster).
 */
static WORD
sana_speed_reqno(ULONG bps, WORD minimum)
{
    WORD reqno;

    if(bps == 0) {
        /* Unknown or wrapped (>4.3 Gbps) -- assume 10+ GbE */
        reqno = 512;
    } else if(bps > 1000000000UL) {
        /* 1-4.3 Gbps (ULONG representable range) */
        reqno = 256;
    } else if(bps > 100000000UL) {
        /* 100 Mbps - 1 Gbps */
        reqno = 128;
    } else if(bps > 10000000UL) {
        /* 10 - 100 Mbps */
        reqno = 32;
    } else {
        /* <= 10 Mbps (serial, slow Ethernet) */
        reqno = 16;
    }

    /* Never go below the configured/wire minimum */
    return (reqno > minimum) ? reqno : minimum;
}
#endif /* SANA2_LARGEREQALLOCS */

/*
 * Initialize sana_softc
 */
void
ssconfig(struct sana_softc *ifp, struct ssconfig *ifc)
{
    const struct ssc_args *args = ifc->args;
    const struct wire_defaults *wd;
    LONG wt = ifp->ss_hwtype;
    LONG reqtotal = 0;
#ifdef SANA2_LARGEREQALLOCS
    ULONG bps = ifp->ss_if.if_baudrate;

    D(bug("[AROSTCP] ssconfig(bps=%lu)\n", bps));
#else
    D(bug("[AROSTCP] ssconfig()\n"));
#endif

    assert(ifp != NULL);

    for(wd = wire_defaults; wd->wd_wiretype != 0; wd++) {
        if(wt == wd->wd_wiretype)
            break;
    }

    ifp->ss_ip.type = args->a_iptype ? *args->a_iptype : wd->wd_iptype;
    reqtotal += ifp->ss_ip.reqno =
#ifdef SANA2_LARGEREQALLOCS
                    args->a_ipno ? *args->a_ipno : sana_speed_reqno(bps, wd->wd_ipno);
#else
                    args->a_ipno ? *args->a_ipno : wd->wd_ipno;
#endif

    ifp->ss_arp.type = args->a_arptype ? *args->a_arptype : wd->wd_arptype;
    reqtotal += ifp->ss_arp.reqno = args->a_arpno ? *args->a_arpno : wd->wd_arpno;
    ifp->ss_arp.hrd = args->a_arphdr ? *args->a_arphdr : wd->wd_arphdr;

#if INET6
    /* IPv6: wire_defaults supplies the ethertype and minimum read count */
    ifp->ss_ip6.type  = wd->wd_ip6type;
    ifp->ss_ip6.reqno = (wd->wd_ip6no > 0) ?
#ifdef SANA2_LARGEREQALLOCS
                        sana_speed_reqno(bps, wd->wd_ip6no) : 0;
#else
                        wd->wd_ip6no : 0;
#endif
    reqtotal += ifp->ss_ip6.reqno;
#endif

    reqtotal += args->a_writeno ? *args->a_writeno :
#ifdef SANA2_LARGEREQALLOCS
                sana_speed_reqno(bps, wd->wd_writeno);
#else
                wd->wd_writeno;
#endif

    if(reqtotal > 65535)
        reqtotal = 65535;
    ifp->ss_reqno = reqtotal;

    {
        UWORD ifflags = wd->wd_ifflags;

        if(args->a_noarp)
            ifflags |= IFF_NOARP;
        if(args->a_point2point) {
            ifflags |= IFF_POINTOPOINT;
            ifflags &= ~IFF_BROADCAST;
        }
        if(args->a_nosimplex)
            ifflags &= ~IFF_SIMPLEX;
        if(args->a_loopback)
            ifflags |= IFF_LOOPBACK;

        ifp->ss_if.if_flags = ifflags;
    }

    /* Flags for soft_sanac */
    ifp->ss_cflags = SS_CFLAGS;

    if(args->a_notrack)
        ifp->ss_cflags &= ~(SSF_TRACK);

    /* Set up name */
    ifp->ss_if.if_name = ifp->ss_name;
    {
        size_t nlen = strnlen(ifc->name, IFNAMSIZ - 1);
        memcpy(ifp->ss_name, ifc->name, nlen);
        ifp->ss_name[nlen] = '\0';
    }
    ifp->ss_if.if_unit = ifc->unit;
    ifp->ss_execname = (char *)(ifp + 1);
    /* Buffer after the struct is allocated to strnlen(a_dev, FILENAME_MAX)+1, NOT FILENAME_MAX.
     * Use memcpy with the exact source length to avoid overflowing the allocation. */
    {
        size_t devlen = strnlen(ifc->args->a_dev, FILENAME_MAX);
        memcpy(ifp->ss_execname, ifc->args->a_dev, devlen + 1);
    }
    ifp->ss_execunit = *ifc->args->a_unit;
}

