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
 * The SANA-II interfaces as the configuration reader makes and drops them:
 * part of module "netconfig" (kern/netconfig_ops.h, conf/group-netconfig.cfg),
 * so in memory only while a configuration is read.
 *
 * Moved, unchanged but for the names they reach the core by, from
 *
 *   iface_make()                           net/if_sana.c
 *   ssconfig(), its wire-type defaults     net/sana2config.c
 *   the reload generation: sana_reload_begin(), sana_mark_seen(),
 *   sana_remove_unseen()                   net/if_sana.c
 *   iface_fail_request()                   kern/amiga_request.c
 *
 * Every one of them is called only by the reader (addifent() and
 * netdb_reload() in kern/netconfig_module.c) - at start-up, at a reload,
 * for an ARexx ADD and for a DEFER line, which the core's timer retries
 * only while the reader runs (netconfig_running()). What they make and
 * set is the core's and outlives the reader: the sana_softc and its place
 * in ssq, the functions an interface is run by (sana_output, sana_ioctl,
 * sana_query, all in the core), the generation number sana_reload_gen.
 * The generation is the core's variable, not the module's, because the
 * module is unloaded between two reads and an interface keeps the number
 * of the read that last named it.
 */

#include <conf.h>

#include <exec/errors.h>
#include <exec/memory.h>
#include <proto/exec.h>
#include <proto/intuition.h>
#include <intuition/intuition.h>

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/syslog.h>
#include <sys/socket.h>
#include <sys/malloc.h>
#include <sys/mbuf.h>
#include <sys/errno.h>

#include <kern/amiga_includes.h>
#include <kern/amiga_gui.h>
#include <kern/amiga_dhcp.h>

#include <net/if.h>
#include <net/if_types.h>
#include <net/if_arp.h>
#include <netinet/in.h>
#include <devices/sana2.h>
#include <net/sana2tags.h>          /* the wire types' packet types */
#include <net/if_sana.h>
#include <net/sana2config.h>
#include <net/sana2request.h>
#include <net/netmodule.h>
#include <kern/netconfig_ops.h>

#include <net/netcore.h>            /* the core's names, as the module's table has them */
#include <net/netcore_map.h>        /* always the last include */

#ifndef __mc68000__
#define SANA2_LARGEREQALLOCS
#define SANA2_DEF_WRITENO 64
#else
#define SANA2_DEF_WRITENO 16
#endif

/* ======================================================================
 * The interface's parameters (from net/sana2config.c)
 * ====================================================================== */

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
    UWORD wd_ifflags;		/* Interface flags */
} wire_defaults[] = {
    {
        S2WireType_Ethernet,
        ETHERTYPE_IP, 16, SANA2_DEF_WRITENO,
        ETHERTYPE_ARP, 4, 1,
        ETHERTYPE_IPV6, 16,
        IFF_BROADCAST | IFF_SIMPLEX | IFF_MULTICAST,
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
    /* Use ethernet as default; no IFF_MULTICAST, see sana_ether_mcast() */
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
 * Initialize sana_softc (the caller has just allocated it: never NULL)
 */
static void
ssconfig(struct sana_softc *ifp, struct ssconfig *ifc)
{
    const struct ssc_args *args = ifc->args;
    const struct wire_defaults *wd;
    LONG reqtotal = 0;
#ifdef SANA2_LARGEREQALLOCS
    ULONG bps = ifp->ss_if.if_baudrate;

    D(bug("[AROSTCP] ssconfig(bps=%lu)\n", (u_long)bps));
#else
    D(bug("[AROSTCP] ssconfig()\n"));
#endif

    for(wd = wire_defaults; wd->wd_wiretype != 0; wd++) {
        LONG wt = ifp->ss_hwtype;

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
    ifp->ss_arp_hrd = args->a_arphdr ? *args->a_arphdr : wd->wd_arphdr;

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

    /* link types registered by others */
    reqtotal += sana_linktype_config(ifp);

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

/* ======================================================================
 * Making an interface (from net/if_sana.c)
 * ====================================================================== */

/*
 * Open the device of an interface line, ask it what it is, and make the
 * interface: a sana_softc in ssq, attached. NULL when the device or unit
 * does not open or does not answer (the caller decides what then).
 */
struct ifnet *
iface_make(struct ssconfig *ifc)
{
    register struct sana_softc *ssc = NULL;
    register struct IOSana2Req *req;
    struct Sana2DeviceQuery devicequery;

    /* Allocate the request for opening the device */
    if((req = CreateIOSana2Req(NULL)) == NULL) {
        __log(LOG_ERR, "iface_find(): CreateIOSana2Req failed\n");
    } else {
        req->ios2_BufferManagement = buffermanagement;

        DSANA(__log(LOG_DEBUG, "Opening device %s unit %ld", ifc->args->a_dev, (long)*ifc->args->a_unit);)
        if(OpenDevice(ifc->args->a_dev, *ifc->args->a_unit,
                      (struct IORequest *)req, 0L)) {
            /* OpenDevice() returns non-zero on FAILURE: only report the
             * error and clean up.  ssc stays NULL so this function returns
             * NULL; req is released by DeleteIOSana2Req() below. */
            sana2perror("OpenDevice", req);

            D(bug("[AROSTCP:SANA] %s: device '%s' unit %u", __func__, ifc->args->a_dev, *ifc->args->a_unit));
        } else {
            /* Ask for our type, address length, MTU
            * Obl. bitch: nobody tells, WHO is supplying
            * DevQueryFormat and DeviceLevel
            */
            req->ios2_Req.io_Command   = S2_DEVICEQUERY;
            req->ios2_StatData         = &devicequery;
            devicequery.SizeAvailable  = sizeof(devicequery);
            devicequery.DevQueryFormat = 0L;

            DoIO((struct IORequest *)req);
            if(req->ios2_Req.io_Error) {
                sana2perror("S2_DEVICEQUERY", req);
            } else {
                /* Get Our Station address */
                req->ios2_StatData = NULL;
                req->ios2_Req.io_Command = S2_GETSTATIONADDRESS;
                DoIO((struct IORequest *)req);

                if(req->ios2_Req.io_Error) {
                    sana2perror("S2_GETSTATIONADDRESS", req);
                } else {
                    req->ios2_Req.io_Command = 0;

                    /* Allocate the interface structure */
                    ssc = (struct sana_softc *)
                          bsd_malloc(sizeof(*ssc) + strnlen(ifc->args->a_dev, FILENAME_MAX) + 1,
                                     M_IFNET, M_WAITOK);

                    if(!ssc) {
                        __log(LOG_ERR, "iface_find: out of memory\n");
                    } else {
                        aligned_bzero_const(ssc, sizeof(*ssc));

                        /* Save request pointers */
                        ssc->ss_dev     = req->ios2_Req.io_Device;
                        ssc->ss_unit    = req->ios2_Req.io_Unit;
                        ssc->ss_bufmgnt = req->ios2_BufferManagement;

                        /* Address must be full bytes */
                        ssc->ss_if.if_addrlen  = (devicequery.AddrFieldSize + 7) >> 3;
                        bcopy(req->ios2_DstAddr, ssc->ss_hwaddr, ssc->ss_if.if_addrlen);
                        ssc->ss_if.if_mtu      = devicequery.MTU;
                        ssc->ss_maxmtu         = devicequery.MTU;
                        ssc->ss_if.if_baudrate = devicequery.BPS;
                        ssc->ss_hwtype         = devicequery.HardwareType;

                        /* A jumbo interface needs proportionally more mbuf
                         * clusters; grow the pool cap for the advertised MTU
                         * (bounded by free memory). */
                        mb_autosize(devicequery.MTU);

                        /* These might be different on different hwtypes.
                         * The core's functions: they run the interface
                         * long after this module has gone. */
                        ssc->ss_if.if_output = sana_output;
                        ssc->ss_if.if_ioctl  = sana_ioctl;
                        ssc->ss_if.if_query  = sana_query;

                        /* Map SANA-II hardware types to RFC1573 standard */
                        switch(ssc->ss_hwtype) {
                        case S2WireType_Ethernet:
                            ssc->ss_if.if_type = IFT_ETHER;
                            break;
                        case S2WireType_IEEE802:
                            ssc->ss_if.if_type = IFT_IEEE80211;
                            break;
                        case S2WireType_Arcnet:
                            ssc->ss_if.if_type = IFT_ARCNET;
                            break;
                        case S2WireType_LocalTalk:
                            ssc->ss_if.if_type = IFT_LOCALTALK;
                            break;
                        case S2WireType_PPP:
                            ssc->ss_if.if_type = IFT_PPP;
                            break;
                        case S2WireType_SLIP:
                        case S2WireType_CSLIP:
                            ssc->ss_if.if_type = IFT_SLIP;
                            break;
                        case S2WireType_PLIP:
                            ssc->ss_if.if_type = IFT_PARA;
                            break;
                        default:
                            ssc->ss_if.if_type = IFT_OTHER;
                        }

                        /* Initialize */
                        ssconfig(ssc, ifc);

                        NewList((struct List *)&ssc->ss_freereq);

                        /* depth of the transmit queue that sana_start() drains */
                        ssc->ss_if.if_snd.ifq_maxlen = IFQ_MAXLEN;

                        if_attach((struct ifnet *)ssc);
                        ifinit();

                        ssc->ss_next = ssq;
                        ssq = ssc;
                    }
                }
            }
            if(!ssc)
                CloseDevice((struct IORequest *)req);
        }
        DeleteIOSana2Req(req);
    }

    return (struct ifnet *)ssc;
}

/* ======================================================================
 * In-place reload: the interfaces dropped from the configuration
 * (from net/if_sana.c)
 * ====================================================================== */

/*
 * sana_reload_begin() bumps a generation; addifent() marks each interface it
 * (re)configures via sana_mark_seen(); sana_remove_unseen() then disables any
 * interface still carrying an older generation (i.e. no longer in the config).
 *
 * Removal is a full teardown: stop DHCP, release DNS/autoip, flush routes and
 * delete every address (sana_if_deconfigure()), take the device offline and
 * drain all its I/O requests (sana_down() + sana_unrun() - every request buffer
 * is chained in ss_reqs, so this clears SanaPort of anything that references the
 * softc), detach the per-interface nd6 state, unlink the ifnet from the global
 * list and free its link-level ifaddr (if_detach()), unlink from ssq,
 * CloseDevice() and finally free the softc.  Order mirrors sana_deinit(), with
 * the list-unlink + free added for a single live interface.
 */
void sana_reload_begin(void)
{
    sana_reload_gen++;
}

void sana_mark_seen(struct ifnet *ifp)
{
    struct sana_softc *ssc;

    /* SANA interfaces (those in ssq) are tracked here; a tunnel/pseudo ifnet
     * is not a sana_softc and must not be cast/written as one: the kind that
     * made it tracks it (nettunnel_seen()). */
    for(ssc = ssq; ssc; ssc = ssc->ss_next) {
        if((struct ifnet *)ssc == ifp) {
            ssc->ss_reloadseen = sana_reload_gen;
            return;
        }
    }
    nettunnel_seen(ifp, sana_reload_gen);
}

void sana_remove_unseen(void)
{
    struct sana_softc *ssc, *next;
    struct sana_softc **prev = &ssq;

    for(ssc = ssq; ssc; ssc = next) {
        struct ifnet *ifp = (struct ifnet *)ssc;
        struct IOSana2Req *req;

        next = ssc->ss_next;

        if(ssc->ss_reloadseen == sana_reload_gen) {
            prev = &ssc->ss_next;       /* kept: advance the unlink cursor */
            continue;
        }

        /* interface no longer in the config: full teardown and free */
        ifp->if_data.ifi_aros_usedhcp = 0;
#if INET6 && DHCP6
        ifp->if_data.ifi_aros_usedhcp6 = 0;
#endif
        (void)netfamily_if_event(NFE_REMOVING, ifp);
        sana_if_deconfigure(ifp);       /* DNS, autoip, routes, all addresses */

        /* take the device offline and drain/free all of its I/O requests
         * before anything that references the softc is torn down.  (A compliant
         * SANA-II driver must return parked CMD_READs on AbortIO; tap.device
         * now does.) */
        sana_down(ssc);
        if(ssc->ss_if.if_flags & IFF_DRV_RUNNING)
            sana_unrun(ssc);
        ifp->if_flags &= ~IFF_UP;

        (void)netfamily_if_event(NFE_DETACHING, ifp);
        if_detach(ifp);                 /* global-list unlink + free link ifaddr */

        *prev = next;                   /* unlink from ssq (prev still points here) */

        /* close the device (ss_dev/ss_unit are still valid in the softc) */
        req = CreateIOSana2Req(ssc);
        if(req) {
            CloseDevice((struct IORequest *)req);
            DeleteIOSana2Req(req);
        } else {
            __log(LOG_ERR, "sana_remove_unseen: couldn't close device %s\n",
                  ssc->ss_name);
        }

        __log(LOG_NOTICE, "%s%d: removed from configuration, interface detached",
              ifp->if_name, ifp->if_unit);

        bsd_free((caddr_t)ssc, M_IFNET);
        /* prev is unchanged: it now holds `next`, the new current node */
    }

    /* the tunnels whose line is gone, by the kind that made them */
    nettunnel_prune(sana_reload_gen);
}

/* ======================================================================
 * The requester for an interface that does not start
 * (from kern/amiga_request.c)
 * ====================================================================== */

/*
 * An interface could not be started (its device or unit did not open): say
 * so and let the user decide between trying again, going on without it, and
 * shutting the stack down.
 *
 * The text is formatted by the requester itself (RawDoFmt() of exec, from
 * an argument array laid out as RawDoFmt() reads it: a pointer for each %s,
 * a LONG for %ld), not by snprintf(): a module library does not carry the
 * C library's formatter.
 */
LONG iface_fail_request(CONST_STRPTR name, CONST_STRPTR device, LONG unit)
{
    struct IntuitionBase *IntuitionBase =
        (struct IntuitionBase *)OpenLibrary("intuition.library", 37);
    struct EasyStruct es = {
        sizeof(struct EasyStruct),
        0,
        "AROSTCP",
        "Interface %s failed to start.\n%s unit %ld could not be opened.",
        "Retry|Skip|Shutdown"
    };
    struct {
        CONST_STRPTR name;
        CONST_STRPTR device;
        LONG         unit;
    } args;
    struct Window *win;
    LONG choice;

    args.name   = name;
    args.device = device ? device : (CONST_STRPTR)"(no device)";
    args.unit   = unit;

    if(IntuitionBase == NULL)
        return IFFAIL_SKIP;
    win = BuildEasyRequestArgs(NULL, &es, 0, (RAWARG)&args);
    if(win == NULL || win == (struct Window *)1) {
        CloseLibrary((struct Library *)IntuitionBase);
        return IFFAIL_SKIP;
    }

    while((choice = SysReqHandler(win, NULL, TRUE)) < 0)
        ;
    FreeSysRequest(win);
    CloseLibrary((struct Library *)IntuitionBase);

    /* gadgets answer 1, 2, ... from the left, the rightmost 0 */
    if(choice == 1)
        return IFFAIL_RETRY;
    if(choice == 2)
        return IFFAIL_SKIP;
    return IFFAIL_SHUTDOWN;
}
