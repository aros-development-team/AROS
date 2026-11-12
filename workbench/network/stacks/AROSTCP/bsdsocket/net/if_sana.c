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

#include <exec/errors.h>

#include <sys/param.h>
#include <sys/cdefs.h>
#include <sys/malloc.h>
#include <sys/mbuf.h>
#include <sys/socket.h>
#include <sys/errno.h>
#include <sys/sockio.h>
#include <sys/systm.h>
#include <sys/syslog.h>

#include <kern/amiga_includes.h>
#include <kern/amiga_gui.h>
#include <kern/amiga_dhcp.h>
#include <kern/amiga_netdb.h>

#include <sys/synch.h>

#include <net/if.h>
#include <net/netisr.h>
#include <net/route.h>

#define NDEBUG
#include <assert.h>

#if INET
#include <netinet/in.h>		/* struct in_addr in struct sana_softc */
#endif

#if NS
#include <netns/ns.h>
#include <netns/ns_if.h>
#endif

#if INET6
#include <netinet/ip6.h>
#include <netinet6/in6_var.h>
#include <netinet6/nd6.h>
#endif

#include <stdio.h>
#include <net/if_sana.h>
#include <net/netmodule.h>
#include <net/if_types.h>
#include "bpf.h"
#include <net/packet_ops.h>      /* the capture tap */

#include <net/sana2config.h>
#include <net/sana2request.h>
#include <net/sana2errno.h>
#include <net/rtsock_protos.h>

#include <libraries/bsdsocket.h>
#include <libraries/miamipanel.h>
#include <proto/dos.h>

int debug_sana = 1;

/* Global port for all SANA-II network interfaces */
struct MsgPort *SanaPort = NULL;

/* queue for sana network interfaces */
struct sana_softc *ssq = NULL;

/* These are wire type dependant parameters of
 * Sana-II Network Interface
 */
//extern struct wiretype_parameters wiretype_table[];

/*
 * Local prototypes
 */
extern void if_detach(struct ifnet *ifp);       /* net/if.c */
static void sana_run(struct sana_softc *ssc, int requests, struct ifaddr *ifa);
static void sana_up(struct sana_softc *ssc);
static struct mbuf *
sana_read(struct sana_softc *ssc, struct IOIPReq *req,
          UWORD  flags, UWORD *sent, const char *banner, size_t mtu);
static void sana_linktype_builtin(void);
static void sana_ip_read(struct sana_softc *ssc, struct IOIPReq *req);
static void sana_arp_read(struct sana_softc *ssc, struct IOIPReq *req);
#if INET6
static void sana_ip6_read(struct sana_softc *ssc, struct IOIPReq *req);
#endif
static void sana_online(struct sana_softc *ssc, struct IOIPReq *req);
static void sana_connect(struct sana_softc *ssc, struct IOIPReq *req);
static void free_written_packet(struct sana_softc *ssc, struct IOIPReq *req);
static void sana_start(struct sana_softc *ssc);
static int sana_linktype_wanted(int i);

/*
 * Whether the interface has been brought online by sana_up(): it has read
 * requests with its driver. Of any link type - which families are part of
 * the running stack decides which types are read.
 */
static inline int
sana_reading(struct sana_softc *ssc)
{
    int i;

    for(i = 0; i < SANA_PT_MAX; i++)
        if(ssc->ss_pt[i].sent != 0)
            return 1;
    return 0;
}

/*
 * Prepended (as a short mbuf) to a packet held on the interface send queue so
 * that its resolved destination and framing survive until a request is free to
 * carry the packet to the SANA-II driver.
 */
struct sana_sendtag {
    ULONG st_type;                 /* SANA-II packet type              */
    UWORD st_ioflags;              /* ios2 io_Flags (e.g. SANA2IOF_RAW) */
    BYTE  st_pri;                  /* request message priority         */
    UBYTE st_dstaddr[MAXADDRSANA]; /* resolved destination address     */
};

/*
 * Initialize Sana-II interface
 *
 * This routine creates needed message port for Sana-II IO
 * It returns our signal mask, or 0L on an error.
 */
ULONG
sana_init(void)
{
    assert(!SanaPort);

    SanaPort = CreateMsgPort();	/* V36 function, creates a PA_SIGNAL port */

    if(SanaPort) {
        SanaPort->mp_Node.ln_Name = (void *)"sana_if.port";
        sana_linktype_builtin();
        loattach();
        return (ULONG) 1 << SanaPort->mp_SigBit;
    }

    return 0L;
}

/*
 * Clean up Sana-II Interfaces
 *
 * Note: main interface queue is SNAFU after deinitializing
 */
void
sana_deinit(void)
{
    struct sana_softc *ssc;
    struct IOSana2Req *req;

    assert(SanaPort);

    while(ssq) {
        sana_down(ssq);
        if(ssq->ss_if.if_flags & IFF_DRV_RUNNING) {
            sana_unrun(ssq);
        }
        ssc = ssq;
        ssq = ssc->ss_next;
        /* Close device */
        req = CreateIOSana2Req(ssc);
        if(req) {
            CloseDevice((struct IORequest *)req);
            DeleteIOSana2Req(req);
        } else {
            __log(LOG_ERR, "sana_deinit(): Couldn't close device %s\n",
                  ssc->ss_name);
        }
    }

    if(SanaPort) {
        /* Clear possible pending signals */
        SetSignal(1 << SanaPort->mp_SigBit, 0L);
        DeleteMsgPort(SanaPort);
        SanaPort = NULL;
    }
}

/*
 * Drop all of an interface's live layer-3 configuration - DNS, autoip, every
 * route, and every address (IPv6 then IPv4) - WITHOUT closing the device or
 * freeing the ifnet.  Shared by the reload teardown (which re-applies the
 * config afterwards) and the dropped-interface path (which leaves it bare;
 * sana_remove_unseen() of the configuration reader, kern/netconfig_iface.c).
 *
 * rt_purgeif() removes every route that resolves through the interface first -
 * including the IPv6 RTF_CLONING prefix route that in6_ifscrub() alone did not
 * reliably delete.  With the routes already gone, in6_purgeaddr() can free each
 * IPv6 address without leaving a route's rt_ifa dangling.
 *
 * Returns the number of addresses removed (for logging).
 */
int
sana_if_deconfigure(struct ifnet *ifp)
{
    int deleted = 0;

    ifnet_clear_dns(ifp);

    /* Each family first drops what must go before the routes do (IPv4 its
     * link-local claim; IPv6 the routers heard on the interface, the routes
     * they gave and the prefixes - an entry left marked installed, or its
     * address made, would never be put back when the router advertises
     * again after a reload). */
    (void)netfamily_if_event(NFE_UNCONFIGURE_PRE, ifp);

    /* Flush all routes on the interface before dropping its addresses, so no
     * live route is left referencing an ifaddr we are about to free. */
    rt_purgeif(ifp, AF_UNSPEC);

    deleted = netfamily_if_event(NFE_UNCONFIGURE, ifp);
    return deleted;
}

/*
 * Tear down every interface's live configuration for an in-place reload.  The
 * config is re-applied afterwards by re-reading the interface database
 * (addifent), so an interface is reconfigured without a link bounce - and
 * because the routes/addresses are fully dropped first, an IPv6 address can
 * change across a reload (addifent() re-adds the new address and its route; an
 * unchanged one is simply re-added identically).  DHCP is stopped separately
 * (dhcp service) before this runs.
 */
void
sana_reconfig_teardown(void)
{
    struct sana_softc *ssc;

    for(ssc = ssq; ssc; ssc = ssc->ss_next) {
        struct ifnet *ifp = (struct ifnet *)ssc;
        int deleted = sana_if_deconfigure(ifp);

        D(bug("[AROSTCP](if_sana.c) sana_reconfig_teardown: %s%d dropped %d address(es)\n",
              ifp->if_name, ifp->if_unit, deleted));
    }
}

/*
 * In-place reload: the generation of the configuration read last. The
 * configuration reader (module "netconfig", kern/netconfig_iface.c) raises
 * it at the start of a reload, stamps every interface a line names with it
 * (ss_reloadseen, and the tunnels' own records) and removes the interfaces
 * left with an older one. It is kept here, not in the reader, because the
 * reader is unloaded between two reads and the stamps outlive it.
 */
ULONG sana_reload_gen = 0;

/*
 * sana_poll()
 *  This routine polls SanaPort and processes replied
 *  requests appropriately
 */
/*
 * Replies taken from SanaPort before the protocols run (net_poll()), as
 * Linux NAPI's budget: each read reply posts its request again at once,
 * so while a device keeps answering, the loop below would only queue
 * packets, and a burst would overflow ipintrq (ipqmaxlen) and be dropped
 * there. Run in batches, the protocols keep up, and a device with no
 * request posted holds what follows (tap.device in the host's queue).
 */
#define SANA_POLL_BUDGET 32

BOOL
sana_poll(void)
{
    struct IOIPReq *io;
    spl_t s = splnet();
    int n = 0;

    while(io = (struct IOIPReq *)GetMsg(SanaPort)) {
        /* touch the network interface */
        GetSysTime(&io->ioip_if->ss_if.if_lastchange);
        if(io->ioip_dispatch) {
            (*io->ioip_dispatch)(io->ioip_if, io);
        } else {
            __log(LOG_ERR, "No dispatch function in request for %s\n",
                  io->ioip_if->ss_name);
        }
        if(++n == SANA_POLL_BUDGET) {
            net_poll();
            n = 0;
        }
    }

    net_poll();

    splx(s);

    return FALSE;
}

#ifdef COMPAT_AMITCP2
/*
 * Name points to the full device name.
 * Device name is a legal DOS file name,
 * appended with a slash and a decimal unit number
 *
 * Some explanation on the device names:
 * There is a DOS wrapper around Exec OpenDevice() function.
 * The device is first searched from the Exec list, if that fails
 * DOS tries to load the segment file with the device name.
 * If that fails too, the filename is catenated to string "DEVS:" and
 * DOS tries again.
 *
 * AmiTCP uses internally only the Exec device name (ie. device name
 * without pathpart)
 */

/*
 * Map exec device name to
 * interface structure pointer.
 */
struct ifnet *aifunit(register char *name)
{
    register char *cp;
    register struct ifnet *ifp;
    long unit;
    unsigned len;
    char *ep, c;

    /* AmigaTCP/IP uses the slash as unit number separator
     * because Exec device name may contain digits.
     */
    char *up;
    cp = ep = name - 1;
    /* Find pathpart */
    for(up = name; *up; up++)
        if(*up == '/' || *up == ':') {
            cp = ep;
            ep = up;
        }
    /* Name is too long, or there is no unit number */
    if(up >= cp + IFNAMSIZ || cp == ep)
        return ((struct ifnet *)0);
    cp++;

    /*
     * cp points first char in device name,
     * ep to unit number separator ('/')
     * and up to NUL ('\0') at the end of string
     */
    len = ep - cp;
    c = *ep;
    *ep = '\0';			/* sentinel */
    for(unit = 0, up--; *up >= '0' && *up <= '9'; up--)
        unit = unit * 10 + *up - '0';
    if(up != ep) {
        *ep = c;
        return NULL;
    }

    /* Pathpart is not included in search */
    for(ifp = ifnet; ifp; ifp = ifp->if_next) {
        if(bcmp(ifp->if_name, cp, len))
            continue;
        if(unit == ifp->if_unit)
            break;
    }
    {
        extern struct ifnet *aiface_find(char *, long unit);
        *ep = '\0';			/* sentinel */
        if(ifp == 0)
            ifp = aiface_find(name, unit);
        *ep = c;
    }
    return (ifp);
}

struct ifnet *
aiface_find(char *name, long unit)
{
    struct interface_parameters *sifp = sana2tag_find_exec(name, unit);

    /* No alias found, use defaults */
    if(sifp == NULL) {
        static short sana_units = 0;
        struct interface_parameters sifp[1];
        const static long tag_end = TAG_END;

        sifp->ifname = "sana";
        sifp->unit = sana_units++;
        sifp->execname = name;
        sifp->execunit = unit;
        sifp->tags = (struct TagItem *)&tag_end;
        return make_iface(sifp, sifp->unit);
    }
    return make_iface(sifp, sifp->unit);
}
#endif

/*
 * The interfaces of the configuration are made by iface_make() of the
 * configuration reader (kern/netconfig_iface.c), with sana_output,
 * sana_ioctl and sana_query below as their functions.
 */

/*
 * Allocate Sana-II IORequests for TCP/IP process
 */
static void
sana_run(struct sana_softc *ssc, int requests, struct ifaddr *ifa)
{
    spl_t s = splimp();
    struct IOIPReq *req, *next = ssc->ss_reqs;

    DSANA(__log(LOG_DEBUG, "sana_run(%s%d) called", ssc->ss_if.if_name, ssc->ss_if.if_unit);)
    /*
     * Configure the Sana-II device driver
     * (now with factory address)
     */
    if((ssc->ss_if.if_flags & IFF_DRV_RUNNING) == 0) {
        struct IOSana2Req *req;

        if(req = CreateIOSana2Req(ssc)) {
            req->ios2_Req.io_Command = S2_CONFIGINTERFACE;
            bcopy(ssc->ss_hwaddr, req->ios2_SrcAddr, ssc->ss_if.if_addrlen);

            DoIO((struct IORequest *)req);

            if(req->ios2_Req.io_Error == 0 ||
                    req->ios2_WireError == S2WERR_IS_CONFIGURED) {
                /* Mark us as running */
                ssc->ss_if.if_flags |= IFF_DRV_RUNNING;
                if(ssc->ss_cflags & SSF_TRACK) {
                    /* Ask for packet type specific statistics */
                    int i;

                    for(i = 0; i < SANA_PT_MAX; i++) {
                        if(sana_linktypes[i] == NULL ||
                                (ssc->ss_pt[i].reqno == 0 && i != SANA_PT_IP))
                            continue;
                        req->ios2_Req.io_Command = S2_TRACKTYPE;
                        req->ios2_PacketType = ssc->ss_pt[i].type;
                        DoIO((struct IORequest *)req);
                        /* It is *not* safe to turn tracking off */
                        if(req->ios2_Req.io_Error &&
                                req->ios2_WireError != S2WERR_ALREADY_TRACKED) {
                            char what[40];

                            snprintf(what, sizeof(what), "S2_TRACKTYPE for %s",
                                     sana_linktypes[i]->lt_name);
                            sana2perror(what, req);
                        }
                    }
                }
            } else {
                sana2perror("S2_CONFIGINTERFACE", req);
            }
            DeleteIOSana2Req(req);
        }
    }

    if((ssc->ss_if.if_flags & IFF_DRV_RUNNING)) {
        /* Initialize ioRequests, add them into free queue */
        for(int i = 0; i < requests ; i++) {
            if(!(req = CreateIORequest(SanaPort, sizeof(*req)))) break;
            req->ioip_s2.ios2_Req.io_Device    = ssc->ss_dev;
            req->ioip_s2.ios2_Req.io_Unit      = ssc->ss_unit;
            req->ioip_s2.ios2_BufferManagement = ssc->ss_bufmgnt;
            aligned_bcopy(ssc->ss_hwaddr, req->ioip_s2.ios2_SrcAddr, ssc->ss_if.if_addrlen);
            req->ioip_s2.ios2_Req.io_Message.mn_Node.ln_Type = NT_REPLYMSG;
            req->ioip_s2.ios2_Data = req;
            req->ioip_if = ssc;
            req->ioip_next = next;
            AddTail((struct List *)&ssc->ss_freereq, (struct Node *)req);
            next = req;
        }
        ssc->ss_reqs = next;

        /* Order a notify when driver connects to a (wireless) network.
         * Register for all interfaces — DHCP and IPv4LL both need reconnect handling. */
        {
            if((req = CreateIORequest(SanaPort, sizeof(*req))) != NULL) {
                ssc->ss_eventsent++;
                req->ioip_s2.ios2_Req.io_Device    = ssc->ss_dev;
                req->ioip_s2.ios2_Req.io_Unit      = ssc->ss_unit;
                req->ioip_s2.ios2_BufferManagement = ssc->ss_bufmgnt;
                req->ioip_s2.ios2_Req.io_Message.mn_Node.ln_Type = NT_REPLYMSG;
                req->ioip_if = ssc;
                req->ioip_next = NULL;
                req->ioip_s2.ios2_Req.io_Command = S2_ONEVENT;
                req->ioip_s2.ios2_WireError = S2EVENT_OFFLINE | S2EVENT_CONNECT
                                              | S2EVENT_DISCONNECT;
                req->ioip_dispatch = sana_connect;
                BeginIO((struct IORequest *)req);
                ssc->ss_connectreq = req;
            }
        }
    }
    splx(s);
}

#if __SASC
/*
 * "Fix" for numerous sana2 drivers, which expect to get Unit * in the
 * register A3 when their AbortIO function is called.
 * Note that Exec AbortIO() does NOT put it there.
 */
extern VOID _AbortSanaIO(struct IORequest *, struct Unit *);
#pragma libcall DeviceBase _AbortSanaIO 24 B902

static inline __asm VOID
AbortSanaIO(register __a1 struct IORequest *ioRequest)
{
#define DeviceBase ioRequest->io_Device
    _AbortSanaIO(ioRequest, ioRequest->io_Unit);
#undef DeviceBase
}
#else /* implement later for other compilers */
#define AbortSanaIO AbortIO
#endif

/*
 * Free Sana-II IO Requests
 * Note: this is protected by splimp();
 */
void
sana_unrun(struct sana_softc *ssc)
{
    struct IOIPReq *req, *next;

    for(next = ssc->ss_reqs; req = next ;) {
        next = req -> ioip_next;
        WaitIO((struct IORequest *)req);
        DeleteIORequest((struct IORequest *)req);
    }
    ssc->ss_reqs = next;

    if(ssc->ss_connectreq) {
        /*
         * Abort before waiting. sana_down() only issues S2_OFFLINE when we are
         * the device's last opener, so with a wireless device - where
         * WirelessManager holds it open too - nothing ever replies this parked
         * S2_ONEVENT and the wait never returns, taking the whole stack
         * shutdown with it.
         */
        if(!CheckIO((struct IORequest *)ssc->ss_connectreq))
            AbortSanaIO((struct IORequest *)ssc->ss_connectreq);
        WaitIO((struct IORequest *)ssc->ss_connectreq);
        DeleteIORequest((struct IORequest *)ssc->ss_connectreq);
    }

    ssc->ss_if.if_flags &= ~IFF_DRV_RUNNING;
}

/*
 * Generic SANA-II interface ioctl
 *
 * Interface setup is thru IOCTL.
 */
int
sana_ioctl(register struct ifnet *ifp, int cmd, caddr_t data)
{
    register struct sana_softc *ssc = (struct sana_softc *)ifp;
    register struct ifaddr *ifa = (struct ifaddr *)data;
    register struct ifreq *ifr = (struct ifreq *)data;

    spl_t s = splimp();
    int error = 0;

    D(bug("[AROSTCP:SANA] %s()\n", __func__));

    switch(cmd) {

    case SIOCSIFFLAGS:
        D(bug("[AROSTCP:SANA] %s: SIOCSIFFLAGS - \n", __func__));

        /*
         * Bring the device online if the user wants IFF_UP, the driver
         * has been initialised (IFF_DRV_RUNNING), but sana_up() hasn't
         * actually been called yet (no read requests queued).
         */
        if((ifr->ifr_flags & IFF_UP) &&
                (ssc->ss_if.if_flags & IFF_DRV_RUNNING) &&
                !sana_reading(ssc)) {
            D(bug("[AROSTCP:SANA] %s: SIFFLAGS bringing interface up .. \n", __func__));
            sana_up(ssc);
        }
        /* Call sana_down() in every case */
        if((ifr->ifr_flags & IFF_UP) == 0) {
            D(bug("[AROSTCP:SANA] %s: SIFFLAGS bringing interface DOWN .. \n", __func__));
            sana_down(ssc);
        }
        /* (What a family keeps per interface for its address resolution -
         * IPv4 its ARP table - it makes itself: on NFE_UP, with its first
         * address, or when it first resolves one.) */
        break;

    case SIOCSIFMTU:		/* Set Interface MTU */
        /*
         * The frame buffers were sized for ss_maxmtu (the MTU the device
         * was configured with at init, from S2_DEVICEQUERY), so the MTU
         * may be lowered freely but never raised above that ceiling.  The
         * floor is 1280, the IPv6 minimum, so both address families keep
         * working at any accepted value.  mb_autosize (grow-only) keeps
         * the cluster pool adequate.
         */
        D(bug("[AROSTCP:SANA] %s: SIOCSIFMTU %ld (max %ld)\n", __func__,
              (long)ifr->ifr_mtu, (long)ssc->ss_maxmtu));
        if(ifr->ifr_mtu < 1280 || ifr->ifr_mtu > ssc->ss_maxmtu)
            error = EINVAL;
        else {
            ssc->ss_if.if_mtu = ifr->ifr_mtu;
            mb_autosize(ifr->ifr_mtu);
        }
        break;

    /*
     * Set interface address (and mark interface up).
     */
    case SIOCSIFADDR:		/* Set Interface Address */
        D(bug("[AROSTCP:SANA] %s: SIOCSIFADDR - Set Interface Address\n", __func__));
        if(!(ssc->ss_if.if_flags & IFF_DRV_RUNNING)) {
            D(bug("[AROSTCP:SANA] %s: SIFADDR set interface as running .. \n", __func__));
            sana_run(ssc, ssc->ss_reqno, ifa);
        }
        /*
         * Use "no read requests sent" instead of !IFF_UP to detect whether
         * the device has actually been brought online.  IFF_UP may already be
         * set by a prior SIOCSIFFLAGS from the DHCP client's PREINIT phase, but
         * that does not mean sana_up() (CmdOnline + CmdRead) has been called.
         */
        if((ssc->ss_if.if_flags & IFF_DRV_RUNNING) && !sana_reading(ssc)) {
            if(ssc->ss_cflags & SSF_NOUP) {
                D(bug("[AROSTCP:SANA] %s: SIFADDR Clearing interface NOUP flag .. \n", __func__));
                ssc->ss_cflags &= ~SSF_NOUP;
            } else {
                D(bug("[AROSTCP:SANA] %s: SIFADDR bringing interface UP .. \n", __func__));
                sana_up(ssc);
            }
        }
        /* (The family that sets the address keeps what it needs of it
         * itself: IPv4 its copy in ss_ipaddr and its ARP table.) */

    case SIOCAIFADDR:		/* Alter Interface Address */
        D(bug("[AROSTCP:SANA] %s: SIOCAIFADDR - Alter Interface Address\n", __func__));
        break;

#if INET6
    case SIOCAIFADDR_IN6:		/* Alter IPv6 Interface Address */
        D(bug("[AROSTCP:SANA] %s: SIOCAIFADDR_IN6 - Alter IPv6 Interface Address\n", __func__));
        /* Replicate IPv4 SIOCSIFADDR behaviour: start hardware and bring UP */
        if(!(ssc->ss_if.if_flags & IFF_DRV_RUNNING)) {
            D(bug("[AROSTCP:SANA] %s: SIOCAIFADDR_IN6 set interface as running ..\n", __func__));
            sana_run(ssc, ssc->ss_reqno, ifa);
        }
        if((ssc->ss_if.if_flags & IFF_DRV_RUNNING) && !sana_reading(ssc)) {
            if(ssc->ss_cflags & SSF_NOUP) {
                D(bug("[AROSTCP:SANA] %s: SIOCAIFADDR_IN6 Clearing interface NOUP flag ..\n", __func__));
                ssc->ss_cflags &= ~SSF_NOUP;
            } else {
                D(bug("[AROSTCP:SANA] %s: SIOCAIFADDR_IN6 bringing interface UP ..\n", __func__));
                sana_up(ssc);
            }
        }
        break;
#endif /* INET6 */

    case SIOCSIFDSTADDR:		/* Sets P-P-link destination address */
        D(bug("[AROSTCP:SANA] %s: SIOCSIFDSTADDR - [*] Set P-P-link destination address\n", __func__));
        break;

    default:
        D(bug("[AROSTCP:SANA] %s: UNKNOWN SIOC\n", __func__));
        error = EINVAL;
        break;
    }
    splx(s);
    return (error);
}

/*
 * Start an interface for the configuration, whatever families the running
 * stack has: configure its device and make its requests (sana_run()) and,
 * with `online', put it online and post the reads (sana_up(), which raises
 * NFE_UP - IPv6 makes its link-local address there). Until this existed
 * only a family setting an address did it (SIOCSIFADDR, SIOCAIFADDR_IN6
 * above), so an interface without an address of a family that is part of
 * the stack never came up. Does nothing that is already done. EOPNOTSUPP
 * for an interface that is not a SANA-II one, ENXIO when the device does
 * not configure.
 */
int
sana_bringup(struct ifnet *ifp, int online)
{
    struct sana_softc *ssc = sana_softc_of(ifp);
    spl_t s;
    int error;

    if(ssc == NULL)
        return (EOPNOTSUPP);
    s = splimp();
    if(!(ssc->ss_if.if_flags & IFF_DRV_RUNNING))
        sana_run(ssc, ssc->ss_reqno, NULL);
    if(online && (ssc->ss_if.if_flags & IFF_DRV_RUNNING) && !sana_reading(ssc)) {
        /* the line said UP: nothing is left for a first address to hold back */
        ssc->ss_cflags &= ~SSF_NOUP;
        sana_up(ssc);
    }
    error = (ssc->ss_if.if_flags & IFF_DRV_RUNNING) ? 0 : ENXIO;
    splx(s);
    return (error);
}

/*
 * The registered link types (net/if_sana.h).
 */
const struct sana_linktype *sana_linktypes[SANA_PT_MAX];

#if INET
static struct sana_linktype sana_lt_ip  = { "IP",  AF_INET,   sana_ip_read,  NULL, 0, SANA_PT_IP };
/* ARP is spoken by IPv4, which gives its input and packet size: sana_linktype_handler() */
static struct sana_linktype sana_lt_arp = { "ARP", AF_UNSPEC, sana_arp_read, NULL, 0, SANA_PT_ARP };
#endif
#if INET6
static struct sana_linktype sana_lt_ip6 = { "IPv6", AF_INET6, sana_ip6_read, NULL, 0, SANA_PT_IP6 };
#endif

static void
sana_linktype_builtin(void)
{
#if INET
    sana_linktypes[SANA_PT_IP] = &sana_lt_ip;
    sana_linktypes[SANA_PT_ARP] = &sana_lt_arp;
#endif
#if INET6
    sana_linktypes[SANA_PT_IP6] = &sana_lt_ip6;
#endif
}

/*
 * Register a link type; its place is returned in lt->lt_index. The three
 * built-in places are not given out.
 */
int
sana_linktype_register(struct sana_linktype *lt)
{
    int i;

    if(lt == NULL || lt->lt_read == NULL || lt->lt_config == NULL)
        return (EINVAL);
    for(i = SANA_PT_IP6 + 1; i < SANA_PT_MAX; i++)
        if(sana_linktypes[i] == NULL) {
            lt->lt_index = i;
            sana_linktypes[i] = lt;
            return (0);
        }
    return (ENOSPC);
}

/*
 * Take a link type out. The caller has brought down, or reloaded, the
 * interfaces that had requests of this type with their drivers.
 */
int
sana_linktype_unregister(struct sana_linktype *lt)
{
    struct sana_softc *ssc;
    int i = lt ? lt->lt_index : -1;

    if(i <= SANA_PT_IP6 || i >= SANA_PT_MAX || sana_linktypes[i] != lt)
        return (ENOENT);
    for(ssc = ssq; ssc; ssc = ssc->ss_next)
        if(ssc->ss_pt[i].sent != 0)
            return (EBUSY);
    sana_linktypes[i] = NULL;
    return (0);
}

int
sana_linktype_resolver(int family,
                       int (*resolve)(struct ifnet *, struct rtentry *, struct mbuf *,
                                      struct sockaddr *, u_char *))
{
    int i;

    for(i = 0; i < SANA_PT_MAX; i++)
        if(sana_linktypes[i] != NULL && sana_linktypes[i]->lt_family == family &&
                family != AF_UNSPEC) {
            ((struct sana_linktype *)sana_linktypes[i])->lt_resolve = resolve;
            return (0);
        }
    return (ENOENT);
}

int
sana_linktype_handler(int index,
                      void (*input)(struct sana_softc *, struct mbuf *, caddr_t), ULONG mtu)
{
    struct sana_linktype *lt;
    spl_t s;

    if(index < 0 || index >= SANA_PT_MAX ||
            (lt = (struct sana_linktype *)sana_linktypes[index]) == NULL ||
            lt->lt_family != AF_UNSPEC)
        return (ENOENT);
    if(input != NULL && lt->lt_input != NULL && lt->lt_input != input)
        return (EEXIST);
    s = splimp();
    lt->lt_input = input;
    if(input != NULL)
        lt->lt_mtu = mtu;
    splx(s);
    return (0);
}

/*
 * A packet of link type `i' was read: put it on the input queue of the
 * family the type carries (looked up once), for the stack's task.
 *
 * What is kept is a pointer into the table of input queues
 * (net/netisr.c), which is kept sorted: an entry MOVES when another queue
 * is registered or unregistered, and one that is unregistered is gone
 * while the slot it had now holds another family's queue. So the table's
 * owner empties this cache whenever the table changes
 * (sana_linktype_uncache(), called by netinput_register(),
 * netinput_unregister() and netinput_forget_all()), and the next packet
 * of each type looks its queue up again. A packet costs what it did: one
 * pointer loaded and tested.
 */
static const struct netinput *sana_lt_input[SANA_PT_MAX];

void
sana_linktype_uncache(void)
{
    spl_t s = splimp();
    int i;

    for(i = 0; i < SANA_PT_MAX; i++)
        sana_lt_input[i] = NULL;
    splx(s);
}

static void
sana_linktype_enqueue(struct sana_softc *ssc, struct mbuf *m, int i)
{
    const struct netinput *ni = sana_lt_input[i];
    spl_t s;

    if(ni == NULL &&
            (ni = sana_lt_input[i] = netinput_find(sana_linktypes[i]->lt_family)) == NULL) {
        m_freem(m);             /* the family is not part of the running stack */
        return;
    }
    s = splimp();
    if(IF_QFULL(ni->ni_queue)) {
        IF_DROP(ni->ni_queue);
        m_freem(m);
    } else {
        /* Set interface pointer (needed for broadcasts) */
        m->m_pkthdr.rcvif = (struct ifnet *)ssc;
        IF_ENQUEUE(ni->ni_queue, m);
        /* A signal might be needed if we use PA_EXCEPTION port */
        schednetisr_nosignal(ni->ni_isr);
    }
    splx(s);
}

/* An interface is being configured: the types the configuration does not set itself. */
LONG
sana_linktype_config(struct sana_softc *ssc)
{
    LONG total = 0;
    int i;

    for(i = SANA_PT_IP6 + 1; i < SANA_PT_MAX; i++) {
        ssc->ss_pt[i].reqno = ssc->ss_pt[i].sent = 0;
        ssc->ss_pt[i].type = 0;
        if(sana_linktypes[i] != NULL) {
            (*sana_linktypes[i]->lt_config)(ssc, &ssc->ss_pt[i]);
            total += ssc->ss_pt[i].reqno;
        }
    }
    return (total);
}

/*
 * sana_send_read():
 * send read requests with given types, dispatcher & c
 * MUST be called at splimp()
 */
static inline WORD
sana_send_read(struct sana_softc *ssc, WORD count, ULONG type, ULONG mtu,
               void (*dispatch)(struct sana_softc *, struct IOIPReq *),
               UWORD command, UBYTE flags)
{
    struct IOIPReq *req = NULL;
    WORD i;

    for(i = 0; i < count; i++) {
        if(!(req = (struct IOIPReq *)RemHead((struct List *)&ssc->ss_freereq)))
            return i;
        req->ioip_dispatch = dispatch;
        req->ioip_s2.ios2_PacketType = type;
        req->ioip_Command = command;
        req->ioip_s2.ios2_Req.io_Flags = flags;
        if(!ioip_alloc_mbuf(req, mtu))
            goto no_resources;
        BeginIO((struct IORequest *)req);
    }
    return i;

no_resources:
    if(req)
        AddHead((struct List *)&ssc->ss_freereq, (struct Node *)req);
    __log(LOG_ERR, "sana_send_read: could not queue enough read requests\n");
    return i;
}

/*
 * Whether read requests of link type `i' are to be with the drivers: none
 * for a family that is not part of the running stack, nor of a built-in
 * type nobody takes.
 */
static int
sana_linktype_wanted(int i)
{
    const struct sana_linktype *lt = sana_linktypes[i];

    if(lt == NULL)
        return 0;
    if(lt->lt_family != AF_UNSPEC)
        return netinput_find(lt->lt_family) != NULL;
    return lt->lt_input != NULL || i > SANA_PT_IP6;
}

/*
 * The read requests of one link type taken back on every interface, and
 * sent again (net/if_sana.h).
 *
 * A read request of the type is one whose completion is the type's
 * lt_read and whose packet type is the interface's for it. It is with the
 * driver, or replied and waiting on SanaPort - never taken off the port
 * and not yet handled, because this runs in the stack's task outside
 * sana_poll(), which finishes with each reply (sends the request again, or
 * puts it with the free ones) before it takes the next. AbortIO() and
 * WaitIO() therefore collect it in either state, WaitIO() taking a replied
 * one off the port, as sana_down() and sana_unrun() do for all of an
 * interface's requests at once.
 */
int
sana_linktype_quiesce(int index)
{
    const struct sana_linktype *lt;
    struct sana_softc *ssc;
    struct IOIPReq *req;
    int n = 0;
    spl_t s;

    if(index < 0 || index >= SANA_PT_MAX || (lt = sana_linktypes[index]) == NULL)
        return 0;
    s = splimp();
    for(ssc = ssq; ssc; ssc = ssc->ss_next) {
        struct sana_ptype *pt = &ssc->ss_pt[index];

        for(req = ssc->ss_reqs; req != NULL && pt->sent != 0; req = req->ioip_next) {
            if(req->ioip_dispatch != lt->lt_read || req->ioip_Command != CMD_READ ||
                    req->ioip_s2.ios2_PacketType != pt->type)
                continue;
            if(!CheckIO((struct IORequest *)req))
                AbortSanaIO((struct IORequest *)req);
            WaitIO((struct IORequest *)req);
            /* a packet that had arrived, and the buffer it was read into */
            if(req->ioip_packet != NULL) {
                m_freem(req->ioip_packet);
                req->ioip_packet = NULL;
            }
            if(req->ioip_reserved != NULL) {
                m_freem(req->ioip_reserved);
                req->ioip_reserved = NULL;
            }
            req->ioip_dispatch = NULL;
            AddHead((struct List *)&ssc->ss_freereq, (struct Node *)req);
            pt->sent--;
            n++;
        }
        /* requests are free: what waited for one to be sent with goes out */
        if((ssc->ss_if.if_flags & (IFF_UP | IFF_DRV_RUNNING)) == (IFF_UP | IFF_DRV_RUNNING))
            sana_start(ssc);
    }
    splx(s);
    return n;
}

int
sana_linktype_resume(int index)
{
    const struct sana_linktype *lt;
    struct sana_softc *ssc;
    int n = 0;
    spl_t s;

    if(index < 0 || index >= SANA_PT_MAX || (lt = sana_linktypes[index]) == NULL ||
            !sana_linktype_wanted(index))
        return 0;
    s = splimp();
    for(ssc = ssq; ssc; ssc = ssc->ss_next) {
        struct sana_ptype *pt = &ssc->ss_pt[index];
        WORD sent;

        /* online: put there by sana_up(), and reading */
        if((ssc->ss_if.if_flags & (IFF_UP | IFF_DRV_RUNNING)) != (IFF_UP | IFF_DRV_RUNNING) ||
                !sana_reading(ssc) || pt->reqno <= pt->sent)
            continue;
        sent = sana_send_read(ssc, pt->reqno - pt->sent, pt->type,
                              lt->lt_mtu ? lt->lt_mtu : ssc->ss_if.if_mtu,
                              lt->lt_read, CMD_READ, 0);
        pt->sent += sent;
        n += sent;
    }
    splx(s);
    return n;
}

/*
 * Called when interface goes online
 */
static void
sana_restore(struct sana_softc *ssc)
{
    spl_t s;
    struct timeval now;

    DSANA(__log(LOG_DEBUG, "sana_restore(%s%d) called", ssc->ss_if.if_name, ssc->ss_if.if_unit);)
    D(bug("[AROSTCP:SANA] %s('%s%d')\n", __func__, ssc->ss_if.if_name, ssc->ss_if.if_unit));

    s = splimp();
    ssc->ss_if.if_flags |= IFF_UP;
    GetSysTime(&now);
    ssc->ss_if.if_data.ifi_aros_ontime.tv_secs = now.tv_secs;
    ssc->ss_if.if_data.ifi_aros_ontime.tv_micro = now.tv_micro;
    ssc->ss_if.if_data.ifi_aros_lasttotal = ssc->ss_if.if_ibytes + ssc->ss_if.if_obytes;
    gui_set_interface_state(&ssc->ss_if, MIAMIPANELV_AddInterface_State_Online);
    rt_ifmsg(&ssc->ss_if);
    /* Send read requests to device driver */
    /* every registered link type that has requests on this interface */
    {
        int i;

        for(i = 0; i < SANA_PT_MAX; i++) {
            const struct sana_linktype *lt = sana_linktypes[i];
            struct sana_ptype *pt = &ssc->ss_pt[i];

            if(lt == NULL || pt->reqno <= pt->sent)
                continue;
            /* no reads for a family that is not part of the running stack,
             * nor of a type nobody takes */
            if(!sana_linktype_wanted(i))
                continue;
            pt->sent += sana_send_read(ssc, pt->reqno - pt->sent, pt->type,
                                       lt->lt_mtu ? lt->lt_mtu : ssc->ss_if.if_mtu,
                                       lt->lt_read, CMD_READ, 0);
        }
    }
#if	ISO
#endif /* ISO */
#if	CCITT
#endif /* CCITT */
#if	NS
#endif /* NS */
#if 0
    ssc->ss_rawsent +=
        sana_send_read(ssc, ssc->ss_rawreqno - ssc->ss_rawsent, 0,
                       ssc->ss_if.if_mtu, sana_raw_read,
                       S2_READORPHAN, SANA2_IOF_RAW);
#endif
    splx(s);

    /* The interface is up with its device running: each family makes what
     * it makes itself (IPv6 its link-local address). */
    (void)netfamily_if_event(NFE_UP, &ssc->ss_if);
    return;
}

/*
 * sana_up():
 * send read requests
 */
static void
sana_up(struct sana_softc *ssc)
{
    struct IOSana2Req *req;
    DSANA(__log(LOG_DEBUG, "sana_up(%s%d) called", ssc->ss_if.if_name, ssc->ss_if.if_unit);)
    D(bug("[AROSTCP:SANA] %s('%s%d')\n", __func__, ssc->ss_if.if_name, ssc->ss_if.if_unit));

    gui_set_interface_state(&ssc->ss_if, MIAMIPANELV_AddInterface_State_GoingOnline);

    if(req = CreateIOSana2Req(ssc)) {
        req->ios2_Req.io_Command = S2_ONLINE;
        req->ios2_Req.io_Error   = S2ERR_NO_ERROR;

        DoIO((struct IORequest *)req);

        if((req->ios2_Req.io_Error) && (req->ios2_WireError != S2WERR_UNIT_ONLINE)) {
            sana2perror("S2_ONLINE", req);
            gui_set_interface_state(&ssc->ss_if, MIAMIPANELV_AddInterface_State_Offline);
        } else {
            __log(LOG_NOTICE, "%s%d is now online.", ssc->ss_name, ssc->ss_if.if_unit);
            sana_restore(ssc);
        }
        DeleteIOSana2Req(req);
    }
}

/*
 * sana_down(): Mark interface as down, abort all pending requests
 */
BOOL
sana_down(struct sana_softc *ssc)
{
    struct IOSana2Req *sreq;
    spl_t s = splimp();
    struct IOIPReq *req = ssc->ss_reqs;
    BOOL success = TRUE;	/* default when no S2_OFFLINE request is issued */

    DSANA(__log(LOG_DEBUG, "sana_down(%s%d) called", ssc->ss_if.if_name, ssc->ss_if.if_unit);)
    gui_set_interface_state(&ssc->ss_if, MIAMIPANELV_AddInterface_State_GoingOffline);
    /* Completed, Remove()'d requests are not aborted */
    while(req) {
        if(!CheckIO((struct IORequest *)req)) {
            AbortSanaIO((struct IORequest *)req);
        }
        req = req->ioip_next;
    }

    /* Discard packets still parked on the transmit queue */
    for(;;) {
        struct mbuf *tag;
        IF_DEQUEUE(&ssc->ss_if.if_snd, tag);
        if(tag == NULL)
            break;
        m_freem(tag);		/* frees the tag and the packet behind it */
    }
    if(ssc->ss_dev && (ssc->ss_dev->dd_Library.lib_OpenCnt == 1)) {
        if(sreq = CreateIOSana2Req(ssc)) {
            sreq->ios2_Req.io_Command = S2_OFFLINE;

            DoIO((struct IORequest *)sreq);
            if(sreq->ios2_Req.io_Error) {
                sana2perror("S2_OFFLINE", sreq);
                success = FALSE;
            } else
                success = TRUE;
            DeleteIOSana2Req(sreq);
        }
    } else
        success = TRUE;
    if(success) {
        __log(LOG_NOTICE, "%s%d is now offline.", ssc->ss_name, ssc->ss_if.if_unit);
        gui_set_interface_state(&ssc->ss_if, MIAMIPANELV_AddInterface_State_Offline);
    } else
        gui_set_interface_state(&ssc->ss_if, MIAMIPANELV_AddInterface_State_Online);

    splx(s);

    return(TRUE);
}

/*
 * sana_read: deattach a packet from IORequest
 *            resend the IORequest
 */
static struct mbuf *
sana_read(struct sana_softc *ssc, struct IOIPReq *req,
          UWORD  flags, UWORD *sent, const char *banner, size_t mtu)
{
    register struct mbuf *m = req->ioip_packet;
    register spl_t s = splimp();

    req->ioip_packet = NULL;

    switch(req->ioip_Error) {
    case 0:
        if(req->ioip_s2.ios2_Req.io_Flags & SANA2IOF_BCAST)
            m->m_flags |= M_BCAST;
        if(req->ioip_s2.ios2_Req.io_Flags & SANA2IOF_MCAST)
            m->m_flags |= M_MCAST;
        ssc->ss_if.if_ibytes += req->ioip_s2.ios2_DataLength;
        break;
    case S2ERR_OUTOFSERVICE:
        /*
         * Somebody put Sana-II driver offline.
         * We put down also the network interface
         */
        if(ssc->ss_if.if_flags & IFF_UP) {
            /* Show a log message */
            sana2perror(ssc->ss_if.if_name, (struct IOSana2Req *)req);

            /* tell it to protocols */
            if_down((struct ifnet *)ssc);

            /* Free mbufs allocated for packet */
            m_freem(req->ioip_reserved);
            req->ioip_reserved = NULL;

            /* Order an notify when driver is put back online */
            ssc->ss_eventsent++;
            req->ioip_s2.ios2_Req.io_Command = S2_ONEVENT;
            req->ioip_s2.ios2_WireError = S2EVENT_ONLINE;
            req->ioip_dispatch = sana_online;
            BeginIO((struct IORequest *)req);
            req = NULL;
            ssc->ss_if.if_flags &= ~IFF_UP;
            gui_set_interface_state(&ssc->ss_if, MIAMIPANELV_AddInterface_State_Offline);
        }
        m_freem(m);
        m = NULL;
        break;
    default:
        if(debug_sana && req->ioip_Error != IOERR_ABORTED)
            sana2perror(banner, (struct IOSana2Req *)req);
        m_freem(m);
        m = NULL;
    }

    if(ssc->ss_if.if_flags & IFF_UP) {
        /* Return request to the Sana-II driver */
        if(ioip_alloc_mbuf(req, mtu)) {
            req->ioip_s2.ios2_Req.io_Flags = flags;
            BeginIO((struct IORequest *)req);
            splx(s);
            return m;
        }
        __log(LOG_ERR, "sana_read (%s): not enough mbufs\n", ssc->ss_name);
    }

    /* do not resend, free used resources */
    (*sent)--;
    if(req) {
        m_freem(req->ioip_reserved);
        req->ioip_reserved = NULL;
        req->ioip_dispatch = NULL;
        AddHead((struct List *)&ssc->ss_freereq, (struct Node *)req);
    }

    if(m) {
        m_freem(m);
    }

    splx(s);
    return NULL;
}

/*
 * BPF sees what is on the wire. The SANA-II driver adds and strips the link
 * header itself, so the stack never holds it: on Ethernet it is rebuilt
 * here from the destination and source addresses and the packet type, and
 * the capture is DLT_EN10MB. No other wire type has a header the stack can
 * rebuild in general; there the capture is DLT_RAW, which holds IPv4 and
 * IPv6 alone, so another packet (ARP) is not shown. bpf_dlt_for_ifp() in
 * bpf.c reports the same choice.
 *
 * Capture is part of the packet services module (net/packet_ops.h): the
 * tap is called through the core's two pointers, which the module sets
 * while it runs - and a capture can be open (if_bpf set) only then.
 */
static void
sana_bpf_tap(struct sana_softc *ssc, struct mbuf *m, int direction,
             const UBYTE *dsthw, const UBYTE *srchw, ULONG type, int ip)
{
    if(ssc->ss_hwtype == S2WireType_Ethernet) {
        UBYTE eh[SANA_ETHER_HDRLEN];

        bcopy(dsthw, eh, SANA_ETHER_ADDRLEN);
        bcopy(srchw, eh + SANA_ETHER_ADDRLEN, SANA_ETHER_ADDRLEN);
        eh[12] = (UBYTE)(type >> 8);
        eh[13] = (UBYTE)type;
        (*nettap_mtap2)(&ssc->ss_if, eh, sizeof(eh), m, direction);
    } else if(ip) {
        (*nettap_mtap)(&ssc->ss_if, m, direction);
    }
}

/*
 * The link addresses of a received frame, for sana_bpf_tap(). They are
 * taken before sana_read(), which hands the request back to the driver.
 */
struct sana_rxaddr {
    UBYTE ra_dst[MAXADDRSANA];
    UBYTE ra_src[MAXADDRSANA];
};

static inline void
sana_rxaddr_save(struct sana_softc *ssc, struct IOIPReq *req,
                 struct sana_rxaddr *ra)
{
    bcopy(req->ioip_s2.ios2_DstAddr, ra->ra_dst, ssc->ss_if.if_addrlen);
    bcopy(req->ioip_s2.ios2_SrcAddr, ra->ra_src, ssc->ss_if.if_addrlen);
}

/*
 * sana_ip_read(): feed a packet to the IP queue
 * (This routine is called from sana_poll)
 */
static void
sana_ip_read(struct sana_softc *ssc, struct IOIPReq *req)
{
    struct mbuf *m;
    struct sana_rxaddr ra;
    int tap = ssc->ss_if.if_bpf != NULL;

    D(bug("[AROSTCP:SANA] %s: %s%d ioError=%d wireError=%ld len=%ld\n",
          __func__, ssc->ss_if.if_name, ssc->ss_if.if_unit,
          req->ioip_s2.ios2_Req.io_Error,
          (long)req->ioip_s2.ios2_WireError,
          (long)req->ioip_s2.ios2_DataLength));

    if(tap)
        sana_rxaddr_save(ssc, req, &ra);
    m = sana_read(ssc, req, 0, &ssc->ss_ip.sent, "sana_ip_read",
                  ssc->ss_if.if_mtu);

    if(m) {
        D(bug("[AROSTCP:SANA] %s: %s%d pktlen=%d flags=0x%x mbuf=0x%p\n",
              __func__, ssc->ss_if.if_name, ssc->ss_if.if_unit,
              m->m_pkthdr.len, m->m_flags, m));
        /* BPF tap: let packet filters see incoming IPv4 packets */
        if(tap)
            sana_bpf_tap(ssc, m, BPF_D_IN, ra.ra_dst, ra.ra_src,
                         ssc->ss_ip.type, TRUE);
        sana_linktype_enqueue(ssc, m, SANA_PT_IP);
    } else {
        D(bug("[AROSTCP:SANA] %s: sana_read returned NULL\n", __func__));
    }
}

/*
 * sana_arp_read(): process an ARP packet
 * (This routine is called from sana_poll)
 */
static void
sana_arp_read(struct sana_softc *ssc, struct IOIPReq *req)
{
    const struct sana_linktype *lt = sana_linktypes[SANA_PT_ARP];
    struct mbuf *m;
    UBYTE hwaddr[MAXADDRSANA];
    struct sana_rxaddr ra;
    int tap = ssc->ss_if.if_bpf != NULL;

    bcopy(req->ioip_s2.ios2_SrcAddr, hwaddr, ssc->ss_if.if_addrlen);
    if(tap)
        sana_rxaddr_save(ssc, req, &ra);

    m = sana_read(ssc, req, 0, &ssc->ss_arp.sent, "sana_arp_read",
                  lt->lt_mtu ? lt->lt_mtu : ssc->ss_if.if_mtu);

    if(m) {
        /* BPF tap: let packet filters see incoming ARP packets */
        if(tap)
            sana_bpf_tap(ssc, m, BPF_D_IN, ra.ra_dst, ra.ra_src,
                         ssc->ss_arp.type, FALSE);
        /* to the module that speaks ARP, if it is part of the running stack */
        if(lt->lt_input != NULL)
            (*lt->lt_input)(ssc, m, (caddr_t)hwaddr);
        else
            m_freem(m);
    }
}

#if INET6
/*
 * sana_ip6_read(): feed an IPv6 packet to the ip6 input queue.
 * (Called from sana_poll when an IPv6-typed packet arrives.)
 */
static void
sana_ip6_read(struct sana_softc *ssc, struct IOIPReq *req)
{
    struct mbuf *m;
    struct sana_rxaddr ra;
    int tap = ssc->ss_if.if_bpf != NULL;

    D(bug("[AROSTCP:SANA] %s: %s%d ioError=%d wireError=%ld len=%ld\n",
          __func__, ssc->ss_if.if_name, ssc->ss_if.if_unit,
          req->ioip_s2.ios2_Req.io_Error,
          (long)req->ioip_s2.ios2_WireError,
          (long)req->ioip_s2.ios2_DataLength));

    if(tap)
        sana_rxaddr_save(ssc, req, &ra);
    m = sana_read(ssc, req, 0, &ssc->ss_ip6.sent, "sana_ip6_read",
                  ssc->ss_if.if_mtu);

    if(m) {
        D(bug("[AROSTCP:SANA] %s: %s%d pktlen=%d flags=0x%x\n",
              __func__, ssc->ss_if.if_name, ssc->ss_if.if_unit,
              m->m_pkthdr.len, m->m_flags));
        /* BPF tap: let packet filters see incoming IPv6 packets */
        if(tap)
            sana_bpf_tap(ssc, m, BPF_D_IN, ra.ra_dst, ra.ra_src,
                         ssc->ss_ip6.type, TRUE);
        sana_linktype_enqueue(ssc, m, SANA_PT_IP6);
    } else {
        D(bug("[AROSTCP:SANA] %s: sana_read returned NULL\n", __func__));
    }
}
#endif /* INET6 */

/*
 * sana_online(): process an ONLINE event
 */
static void
sana_online(struct sana_softc *ssc, struct IOIPReq *req)
{
    LONG events = req->ioip_s2.ios2_WireError;

    if(req->ioip_s2.ios2_Req.io_Error == 0 &&
            events & S2EVENT_ONLINE) {
        ssc->ss_eventsent--;
        req->ioip_dispatch = NULL;
        AddHead((struct List *)&ssc->ss_freereq, (struct Node *)req);
        __log(LOG_NOTICE, "%s is online again.", ssc->ss_name);
        sana_restore(ssc);
        return;
    }

    /* An error? */
    if(debug_sana && req->ioip_Error != IOERR_ABORTED) {
        sana2perror("sana_online", (struct IOSana2Req *)req);
        req->ioip_s2.ios2_Req.io_Command = S2_ONEVENT;
        req->ioip_s2.ios2_WireError = S2EVENT_ONLINE;
        BeginIO((struct IORequest *)req);
    } else {
        /* Aborted -- probably because "ifconfig xxx/0 down" */
        ssc->ss_eventsent--;
        req->ioip_dispatch = NULL;
        AddHead((struct List *)&ssc->ss_freereq, (struct Node *)req);
    }
}

/*
 * sana_linkdown(): the link went away under an interface that is still online,
 * as happens when a wireless device loses its association. The lease and the
 * routes belonged to that network, so give them up instead of leaving a stale
 * address behind. IFF_UP is deliberately left alone: sana_output() refuses to
 * transmit without it, and the DHCP client needs to send when the link returns.
 */
static void
sana_linkdown(struct sana_softc *ssc)
{
    struct ifnet *ifp = (struct ifnet *)ssc;
    int deleted;

    ifnet_clear_dns(ifp);       /* drop this interface's DNS servers with its addresses */
    /* each family gives up what belonged to the network that went */
    deleted = netfamily_if_event(NFE_LINKDOWN, ifp);

    __log(LOG_NOTICE, "%s%d: link lost, dropped %d address(es).",
          ifp->if_name, ifp->if_unit, deleted);
    gui_set_interface_state(ifp, MIAMIPANELV_AddInterface_State_Offline);
}

/*
 * sana_online(): process a CONNECT event
 */
static void
sana_connect(struct sana_softc *ssc, struct IOIPReq *req)
{
    LONG events = req->ioip_s2.ios2_WireError;

    if(req->ioip_s2.ios2_Req.io_Error == 0 &&
            events == S2EVENT_CONNECT) {

        /* New network -> new address */
        (void)netfamily_if_event(NFE_LINKUP, (struct ifnet *) ssc);
    } else if(req->ioip_s2.ios2_Req.io_Error == 0 &&
              (events & S2EVENT_DISCONNECT)) {
        sana_linkdown(ssc);
    }

    /* Send request back for next event */
    if(req->ioip_s2.ios2_Req.io_Error == 0 && !(events & S2EVENT_OFFLINE)) {
        req->ioip_s2.ios2_Req.io_Command = S2_ONEVENT;
        req->ioip_s2.ios2_WireError =  S2EVENT_OFFLINE | S2EVENT_CONNECT
                                       | S2EVENT_DISCONNECT;
        BeginIO((struct IORequest *)req);
    } else {
        /*
         * No more events from the driver: it does not do S2_ONEVENT (as
         * tap.device), or the interface went offline. The request is done,
         * and off every list - sana_poll()'s GetMsg() took it - but still
         * NT_REPLYMSG, with the links it had on the port: a WaitIO() on it
         * later (sana_unrun()) would Remove() it from the port a second time,
         * writing through them into whatever they point at by then. Free it
         * now.
         */
        ssc->ss_eventsent--;
        if(ssc->ss_connectreq == req)
            ssc->ss_connectreq = NULL;
        DeleteIORequest((struct IORequest *)req);
    }
}

/*
 * sana_start(): hand queued packets to the SANA-II driver, one per free
 * request.  Submission (sana_output) and completion (free_written_packet) both
 * call it, decoupling the two: a burst that outruns the requests waits on the
 * send queue instead of being dropped, and each completion immediately pulls
 * the next packet through.  Must be called at splimp.
 */
static void
sana_start(struct sana_softc *ssc)
{
    struct ifnet *ifp = &ssc->ss_if;
    struct IOIPReq *req;
    struct mbuf *tag, *m;
    struct sana_sendtag *st;

    while(ifp->if_snd.ifq_head != NULL) {
        if(!(req = (struct IOIPReq *)RemHead((struct List *)&ssc->ss_freereq)))
            break;			/* no free request: leave packets queued */

        IF_DEQUEUE(&ifp->if_snd, tag);
        st = mtod(tag, struct sana_sendtag *);
        m  = tag->m_next;
        tag->m_next = NULL;

        req->ioip_s2.ios2_Req.io_Flags   = st->st_ioflags;
        req->ioip_s2.ios2_PacketType     = st->st_type;
        bcopy(st->st_dstaddr, req->ioip_s2.ios2_DstAddr, ifp->if_addrlen);
        req->ioip_s2.ios2_Req.io_Message.mn_Node.ln_Pri = st->st_pri;
        /* a group's frame is not an ordinary write to a SANA-II driver */
        req->ioip_Command  = (m->m_flags & M_BCAST) ? S2_BROADCAST :
                             (m->m_flags & M_MCAST) ? S2_MULTICAST : CMD_WRITE;
        req->ioip_dispatch = free_written_packet;
        req->ioip_packet   = m;
        req->ioip_s2.ios2_DataLength = m->m_pkthdr.len;

        m_free(tag);			/* metadata consumed; the packet lives on */

        ifp->if_obytes += m->m_pkthdr.len;
        if(m->m_flags & (M_BCAST | M_MCAST))
            ifp->if_omcasts++;

        BeginIO((struct IORequest *)req);
    }
}

/*
 * sana_output: send a packet to Sana-II driver
 */
int
sana_output(struct ifnet *ifp, struct mbuf *m0,
            struct sockaddr *dst, struct rtentry *rt)
{
    register struct sana_softc *ssc = (struct sana_softc *)ifp;
    ULONG type;
    int error = 0;

    register struct mbuf *m = m0, *tag;
    struct sana_sendtag *st;
    UBYTE dst_hw[MAXADDRSANA] = { 0 };	/* a broadcast leaves it unset */
    UWORD ioflags = 0;
    BYTE  pri = 0;
    int   ip = FALSE;			/* a family's packet, as DLT_RAW holds */

    spl_t s = splimp();

    ifp->if_opackets++;		/* stats */

    /* Check if we are up and running... */
    if((ssc->ss_if.if_flags & (IFF_UP | IFF_DRV_RUNNING)) != (IFF_UP | IFF_DRV_RUNNING)) {
        error = ENETDOWN;
        goto bad;
    }

    GetSysTime(&ssc->ss_if.if_lastchange);

    /*
     * Resolve the destination into local storage, then hand the packet to
     * sana_start() via the send queue.  A request is only claimed there, when
     * one is free.
     */

    switch(dst->sa_family) {
#if NS
#error NS unimplemented!!!
    case AF_NS:
        type = ssc->ss_nstype;
        /* There is hardware address straight in socket */
        /* Dunno how this works, if we have a P-to-P device */
        bcopy((caddr_t) & (((struct sockaddr_ns *)dst)->sns_addr.x_host),
              (caddr_t)req->ioip_s2.ios2_DestAddr, ssc->ss_if.if_addrlen);
        /* Local send */
        if(!bcmp((caddr_t)req->ioip_s2.ios2_DestAddr,
                 (caddr_t)&ns_thishost, ssc->ss_if.if_addrlen)) {
            AddHead(&ssc->ss_freereq, req);
            return (looutput(ifp, m, dst, rt));
        }
        req->ioip_s2.ios2_Req.io_Message.mn_Node.ln_Pri = 0;
        break;
#endif
    case AF_UNSPEC:
        /* Raw packets. Sana-II address (a tuple of type and host)
         * specifies the destination
         */
        if(type = ((struct sockaddr_sana2 *)dst)->ss2_type) {
            bcopy(((struct sockaddr_sana2 *)dst)->ss2_host,
                  dst_hw,
                  ssc->ss_if.if_addrlen);
        } else {
            ioflags = SANA2IOF_RAW;
            type = 0L;
        }
        break;

#if	ISO
#endif /* ISO */
#if RMP
    case AF_RMP:
#endif

    default: {
        /*
         * A family that registered a link type with a resolver (IPv4 with
         * ARP, IPv6 with neighbour discovery): its wire type on this
         * interface, and the link address of the next hop from its
         * resolver. What else a family does for a packet on its way out
         * (IPv4: a copy of its own broadcast for itself where the device
         * does not hear it) its resolver does.
         */
        const struct sana_linktype *lt = NULL;
        int i, lt_error;

        for(i = 0; i < SANA_PT_MAX; i++)
            if((lt = sana_linktypes[i]) != NULL && lt->lt_family == dst->sa_family &&
                    lt->lt_resolve != NULL)
                break;
        if(i < SANA_PT_MAX) {
            type = ssc->ss_pt[i].type;
            lt_error = (*lt->lt_resolve)(ifp, rt, m, dst, dst_hw);
            if(lt_error == EAGAIN) {
                /* kept by the resolver, sent when the address is known */
                splx(s);
                return 0;
            }
            if(lt_error) {
                error = lt_error;
                goto bad;
            }
            /* the resolver may ask for the packet to go ahead of others */
            if(m->m_flags & M_LINKPRI) {
                m->m_flags &= ~M_LINKPRI;
                pri = 1;
            }
            ip = TRUE;
            break;
        }
        __log(LOG_ERR, "%s%d: can't handle af%d\n",
              ssc->ss_if.if_name, ssc->ss_if.if_unit, dst->sa_family);
        error = EAFNOSUPPORT;
        goto bad;
    }
    }

    /*
     * Queue packet to Sana-II driver
     */

    /*
     * BPF tap: let packet filters see outgoing packets. A raw packet (from
     * bpf_write()) is a whole frame already. Otherwise the frame goes to
     * dst_hw, which a broadcast leaves unset: the driver broadcasts it.
     */
    if(ifp->if_bpf) {
        if(ioflags & SANA2IOF_RAW) {
            if(ssc->ss_hwtype == S2WireType_Ethernet)
                (*nettap_mtap)(ifp, m, BPF_D_OUT);
        } else {
            static const UBYTE bcast_hw[SANA_ETHER_ADDRLEN] =
                { 0xff, 0xff, 0xff, 0xff, 0xff, 0xff };

            sana_bpf_tap(ssc, m, BPF_D_OUT,
                         (m->m_flags & M_BCAST) ? bcast_hw : dst_hw,
                         ssc->ss_hwaddr, type, ip);
        }
    }

    /*
     * Park the packet on the send queue behind a metadata tag and pump the
     * queue; sana_start() binds it to a request when one becomes free.
     */
    MGET(tag, M_DONTWAIT, MT_DATA);
    if(tag == NULL) {
        error = ENOBUFS;
        goto bad;
    }
    st = mtod(tag, struct sana_sendtag *);
    st->st_type    = type;
    st->st_ioflags = ioflags;
    st->st_pri     = pri;
    bcopy(dst_hw, st->st_dstaddr, ssc->ss_if.if_addrlen);
    tag->m_len  = sizeof(struct sana_sendtag);
    tag->m_next = m;

    if(IF_QFULL(&ifp->if_snd)) {
        IF_DROP(&ifp->if_snd);
        ifp->if_oerrors++;
        splx(s);
        m_freem(tag);		/* frees the tag and the packet behind it */
        return ENOBUFS;
    }
    IF_ENQUEUE(&ifp->if_snd, tag);
    sana_start(ssc);

    splx(s);
    return 0;

bad:
    ifp->if_oerrors++;
    splx(s);
    if(m)
        m_freem(m);
    return error;
}

/*
 * free_written_packet(): free mbufs of written packet,
 *                        queue IOrequest for reuse
 * (This routine is called from sana_poll)
 */
static void
free_written_packet(struct sana_softc *ssc, struct IOIPReq *req)
{
    spl_t s = splimp();

    if(req->ioip_packet) {
        m_freem(req->ioip_packet);
        req->ioip_packet = NULL;
    }
    req->ioip_dispatch = NULL;
    if(debug_sana && req->ioip_Error)
        sana2perror("sana_output", (struct IOSana2Req *)req);
    AddHead((struct List *)&ssc->ss_freereq, (struct Node *)req);
    sana_start(ssc);		/* a request is free: pump the next queued packet */
    splx(s);
}

/*
 * SANA-II-dependent part of QueryInterfaceTagList()
 */
int sana_query(struct ifnet *ifn, struct TagItem *tag)
{
    struct sana_softc *ssc = (struct sana_softc *)ifn;

    switch(tag->ti_Tag) {
    case IFQ_DeviceName:
        *((STRPTR *)tag->ti_Data) = ssc->ss_execname;
        break;
    case IFQ_DeviceUnit:
        *((ULONG *)tag->ti_Data) = ssc->ss_execunit;
        break;
    case IFQ_HardwareAddress: /* Temporary, we should extract it from if_addrlist instead */
        memcpy((void *)tag->ti_Data, ssc->ss_hwaddr, ssc->ss_if.if_addrlen);
        break;
    case IFQ_HardwareType:
        *((ULONG *)tag->ti_Data) = ssc->ss_hwtype;
        break;
    case IFQ_NumReadRequests:
        __log(LOG_CRIT, "IFQ_NumReadRequests is not implemented");
        return -1;
    /*		*((LONG *)tag->ti_Data) = *** TODO ***
    		break;*/
    case IFQ_MaxReadRequests:
        __log(LOG_CRIT, "IFQ_MaxReadRequests is not implemented");
        return -1;
    /*		*((LONG *)tag->ti_Data) =
    		break;*/
    case IFQ_NumWriteRequests:
        __log(LOG_CRIT, "IFQ_NumWriteRequests is not implemented");
        return -1;
    /*		*((LONG *)tag->ti_Data) =
    		break;*/
    case IFQ_MaxWriteRequests:
        __log(LOG_CRIT, "IFQ_MaxWriteRequests is not implemented");
        return -1;
    /*		*((LONG *)tag->ti_Data) =
    		break;*/
    case IFQ_GetDebugMode:
        *((LONG *)tag->ti_Data) = debug_sana;
        break;
    case IFQ_GetSANA2CopyStats:
        __log(LOG_CRIT, "IFQ_GetSANA2CopyStats is not implemented");
        return -1;
    /*	        (struct Sana2CopyStats *)tag->ti_Data
    		break;*/
    case IFQ_NumReadRequestsPending:
        __log(LOG_CRIT, "IFQ_NumReadRequestsPending is not implemented");
        return -1;
    /*		*((LONG *)tag->ti_Data =
    		break;*/
    case IFQ_NumWriteRequestsPending:
        __log(LOG_CRIT, "IFQ_NumWriteRequestsPending is not implemented");
        return -1;
    /*		*((LONG *)tag->ti_Data =
    		break;*/
    default:
        return -1;
    }
    return 0;
}

/*
 * See net/if_sana.h. The one place a family's multicast group reaches the
 * device's address filter: IPv4 and IPv6 differ only in the mapping.
 */
int
sana_mcast(struct ifnet *ifp, const u_int8_t *lladdr, int add)
{
    struct sana_softc *ssc;
    struct IOSana2Req *req;
    int error = 0;

    if(!sana_ether_mcast(ifp))
        return 0;
    ssc = (struct sana_softc *)ifp;

    D(bug("[AROSTCP:SANA] %s: %s%d %s %02x:%02x:%02x:%02x:%02x:%02x\n",
          __func__, ifp->if_name, ifp->if_unit, add ? "add" : "delete",
          lladdr[0], lladdr[1], lladdr[2], lladdr[3], lladdr[4], lladdr[5]));

    if((req = CreateIOSana2Req(ssc)) == NULL)
        return ENOBUFS;
    req->ios2_Req.io_Command = add ? S2_ADDMULTICASTADDRESS : S2_DELMULTICASTADDRESS;
    bcopy(lladdr, req->ios2_SrcAddr, 6);
    DoIO((struct IORequest *)req);
    if(req->ios2_Req.io_Error)
        error = EIO;
    DeleteIOSana2Req(req);
    return error;
}
