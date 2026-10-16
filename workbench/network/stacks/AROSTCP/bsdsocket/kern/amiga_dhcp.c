#include <conf.h>

#include <dos/dos.h>
#include <proto/dos.h>
#include <kern/amiga_gui.h>
#include <kern/amiga_dhcp.h>
#include <net/if.h>
#include <net/if_sana.h>
#include <net/sana2arp.h>   /* for autoip_start() */
#include <api/netservice_api.h>

extern struct ifnet *ifnet;

/*
 * DHCP client launching has been EXTERNALISED to the dhcp service daemon
 * (workbench/network/stacks/AROSTCP/services/dhcp, installed as
 * SYS:System/Network/Services/dhcpd and configured in db/services.d/dhcp).  The daemon
 * iterates interfaces (SIOCGIFDHCP), launches/manages the ISC dhclient, and
 * cycles it on reconfigure via netservices.library.
 *
 * The in-stack entry points below are now thin: addifent() and the interface
 * teardown still call run_dhclient()/kill_dhclient() etc. to mark intent, but
 * the actual process management is the daemon's job - the stack only records
 * the per-interface ifi_aros_usedhcp flag (set by addifent), which the daemon
 * reads back through SIOCGIFDHCP.  run_dhclient() only tells the daemon to
 * relaunch its clients - a link that came up (S2EVENT_CONNECT) or an
 * interface added at run time needs a new lease; the kill calls stay no-ops.
 * autoip (IPv4LL) stays in the stack.
 */

void run_dhclient(struct ifnet *ifp)        { (void)ifp; netservice_resume("dhcp"); }
void kill_dhclient(struct ifnet *ifp)       { (void)ifp; }
#if INET6 && DHCP6
void run_dhclient6(struct ifnet *ifp)       { (void)ifp; netservice_resume("dhcp"); }
void kill_dhclient6(struct ifnet *ifp)      { (void)ifp; }
#endif
void dhcp_stop_all(void)                    { }

/*
 * Called once the API is up (from the log/NETTRACE task) to deal with
 * interfaces that were configured before the stack was visible (IFF_DELAYUP).
 * DHCP for those is now handled by the dhcp daemon when it starts; here we only
 * clear the deferral flag and bring up IPv4LL (autoip) for AUTO interfaces that
 * neither use DHCP nor have a static address.
 */
void run_dhcp(void)
{
    struct ifnet *ifp;

    for(ifp = ifnet; ifp; ifp = ifp->if_next) {
        struct sana_softc *ssc = sana_softc_of(ifp);

        /* only a SANA-II interface is ever marked */
        if(ssc == NULL)
            continue;
        if(ssc->ss_cflags & SSF_DELAYUP) {
            ssc->ss_cflags &= ~SSF_DELAYUP;
            if(!ifp->if_data.ifi_aros_usedhcp &&
                    ssc->ss_ipaddr.s_addr == INADDR_ANY)
                autoip_start(ifp);  /* no DHCPv4 and no address assigned */
        }
    }
}
