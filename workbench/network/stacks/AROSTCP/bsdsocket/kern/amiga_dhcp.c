#include <conf.h>

#include <dos/dos.h>
#include <proto/dos.h>
#include <kern/amiga_gui.h>
#include <kern/amiga_dhcp.h>
#include <net/if.h>
#include <net/if_sana.h>
#include <net/sana2arp.h>   /* for autoip_start() */

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
 * reads back through SIOCGIFDHCP.  These remain as no-ops so existing callers
 * are undisturbed; autoip (IPv4LL) stays in the stack.
 */

void run_dhclient(struct ifnet *ifp)        { (void)ifp; }
void kill_dhclient(struct ifnet *ifp)       { (void)ifp; }
#if INET6 && DHCP6
void run_dhclient6(struct ifnet *ifp)       { (void)ifp; }
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
        if(ifp->if_flags & IFF_DELAYUP) {
            ifp->if_flags &= ~IFF_DELAYUP;
            if(!ifp->if_data.ifi_aros_usedhcp &&
                    ((struct sana_softc *)ifp)->ss_ipaddr.s_addr == INADDR_ANY)
                autoip_start(ifp);  /* no DHCPv4 and no address assigned */
        }
    }
}
