#include <conf.h>

#include <dos/dos.h>
#include <dos/dostags.h>
#include <proto/dos.h>
#include <kern/amiga_gui.h>
#include <net/if.h>
#include <net/if_sana.h>

extern UBYTE dhclient_path[];
extern struct ifnet *ifnet;
extern struct Library *logDOSBase;

#define DOSBase logDOSBase

void run_dhclient(struct ifnet *ifp)
{
	char ifname[IFNAMSIZ+5];
	BPTR seglist;

	if (!ifp->if_data.ifi_aros_dhcp_pid) {
		DDHCP(KPrintF("Starting DHCP client for %s%u\n", ifp->if_name, ifp->if_unit);)
		sprintf(ifname, "-q %s%u", ifp->if_name, ifp->if_unit);
		seglist = LoadSeg(dhclient_path);
		DDHCP(KPrintF("seglist = 0x%08lx\n", seglist);)
		if (seglist) {
			ifp->if_data.ifi_aros_dhcp_pid = (pid_t)CreateNewProcTags(NP_Seglist, seglist,
									 NP_Arguments, ifname,
									 NP_Cli, TRUE,
									 NP_Name, "AROSTCP DHCP client",
									 NP_CommandName, "dhclient",
									 NP_ConsoleTask, NULL,
									 TAG_DONE);
			DDHCP(KPrintF("dhclient pid = 0x%08lx\n", ifp->if_data.ifi_aros_dhcp_pid);)
			if (!ifp->if_data.ifi_aros_dhcp_pid) {
				UnLoadSeg(seglist);
				seglist = NULL;
			}
		}
		if (!seglist)
			error_request("Unable to start DHCP client for the interface %s", ifname);
	}
}

void kill_dhclient(struct ifnet *ifp)
{
	if (ifp->if_data.ifi_aros_dhcp_pid) {
		Signal(ifp->if_data.ifi_aros_dhcp_pid, SIGBREAKF_CTRL_C);
		ifp->if_data.ifi_aros_dhcp_pid = NULL;
	}
}

void run_dhcp(void)
{
	struct ifnet *ifp;

	for (ifp = ifnet; ifp; ifp = ifp->if_next) {
		if (ifp->if_flags & IFF_DELAYUP) {
			DDHCP(KPrintF("Executing delayed DHCP start for %s%u\n", ifp->if_name, ifp->if_unit);)
			ifp->if_flags &= ~IFF_DELAYUP;
			run_dhclient(ifp);
		}
	}
}

