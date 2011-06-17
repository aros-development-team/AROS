#include <conf.h>

#include <dos/dos.h>
#include <dos/dostags.h>
#include <proto/dos.h>
#include <kern/amiga_gui.h>
#include <net/if.h>
#include <net/if_sana.h>

#include <stdio.h>	// for sprintf

extern UBYTE dhclient_path[];
extern struct ifnet *ifnet;
extern struct Library *logDOSBase;

#define DOSBase logDOSBase

static const TEXT dhclient_proc_name[] = "AROSTCP DHCP client";
static const TEXT dhclient_cmd_name[] = "dhclient";

void run_dhclient(struct ifnet *ifp)
{
	BPTR seglist;

	if (!ifp->if_data.ifi_aros_dhcp_pid) {
		DDHCP(KPrintF("Starting DHCP client for %s%u\n", ifp->if_name, ifp->if_unit);)
		snprintf(ifp->if_data.ifi_aros_dhcp_args, sizeof(ifp->if_data.ifi_aros_dhcp_args), "-q %s%u", ifp->if_name,
			ifp->if_unit);
		ifp->if_data.ifi_aros_dhcp_args[sizeof(ifp->if_data.ifi_aros_dhcp_args)-1] = 0;
		seglist = LoadSeg(dhclient_path);
		DDHCP(KPrintF("seglist = 0x%08lx\n", seglist);)
		if (seglist) {
			ifp->if_data.ifi_aros_dhcp_pid =
				(pid_t)CreateNewProcTags(NP_Seglist, seglist,
									 NP_Arguments, ifp->if_data.ifi_aros_dhcp_args,
									 NP_Cli, TRUE,
									 NP_Name, dhclient_proc_name,
									 NP_CommandName, dhclient_cmd_name,
									 NP_ConsoleTask, NULL,
									 TAG_DONE);
			DDHCP(KPrintF("dhclient pid = 0x%08lx\n", ifp->if_data.ifi_aros_dhcp_pid);)
			if (!ifp->if_data.ifi_aros_dhcp_pid) {
				UnLoadSeg(seglist);
				seglist = BNULL;
			}
		}
		if (!seglist)
			error_request("Unable to start DHCP client for the interface %s%u",
				(IPTR)ifp->if_name, (IPTR)ifp->if_unit);
	}
}

void kill_dhclient(struct ifnet *ifp)
{
	if (ifp->if_data.ifi_aros_dhcp_pid) {
		Signal((APTR)ifp->if_data.ifi_aros_dhcp_pid, SIGBREAKF_CTRL_C);
		ifp->if_data.ifi_aros_dhcp_pid = (pid_t)NULL;
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

