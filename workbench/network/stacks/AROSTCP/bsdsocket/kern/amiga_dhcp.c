#include <conf.h>

#include <dos/dos.h>
#include <dos/dostags.h>
#include <proto/dos.h>
#include <kern/amiga_gui.h>
#include <kern/amiga_dhcp.h>
#include <net/if.h>
#include <net/if_sana.h>

#include <stdio.h>	// for sprintf

extern UBYTE dhclient_path[];
extern struct ifnet *ifnet;
extern struct Library *logDOSBase;

#define DOSBase logDOSBase

static const TEXT dhclient_proc_name[] = "AROSTCP DHCP client";
static const TEXT dhclient_cmd_name[] = "dhclient";
#if INET6 && DHCP6
static const TEXT dhclient6_proc_name[] = "AROSTCP DHCPv6 client";
#endif

void run_dhclient(struct ifnet *ifp)
{
	BPTR seglist;

	if (!aros_dhcpv4.pid) {
		DDHCP(KPrintF("Starting DHCP client for %s%u\n", ifp->if_name, ifp->if_unit);)
		snprintf(aros_dhcpv4.args, sizeof(aros_dhcpv4.args), "-q %s%u",
			ifp->if_name, ifp->if_unit);
		aros_dhcpv4.args[sizeof(aros_dhcpv4.args)-1] = 0;
		seglist = LoadSeg(dhclient_path);
		DDHCP(KPrintF("seglist = 0x%p\n", seglist);)
		if (seglist) {
			aros_dhcpv4.pid =
				(pid_t)CreateNewProcTags(NP_Seglist, seglist,
									 NP_Arguments, aros_dhcpv4.args,
									 NP_Cli, TRUE,
									 NP_Name, dhclient_proc_name,
									 NP_CommandName, dhclient_cmd_name,
									 NP_ConsoleTask, NULL,
									 TAG_DONE);
			DDHCP(KPrintF("dhclient pid = 0x%p\n", aros_dhcpv4.pid);)
			if (!aros_dhcpv4.pid) {
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
	if (aros_dhcpv4.pid) {
		Signal((APTR)aros_dhcpv4.pid, SIGBREAKF_CTRL_C);
		aros_dhcpv4.pid = (pid_t)NULL;
	}
}

#if INET6 && DHCP6
void run_dhclient6(struct ifnet *ifp)
{
	BPTR seglist;

	if (!aros_dhcpv6.pid) {
		DDHCP(KPrintF("Starting DHCPv6 client for %s%u\n", ifp->if_name, ifp->if_unit);)
		snprintf(aros_dhcpv6.args, sizeof(aros_dhcpv6.args),
			 "-6 -q %s%u", ifp->if_name, ifp->if_unit);
		aros_dhcpv6.args[sizeof(aros_dhcpv6.args)-1] = 0;
		seglist = LoadSeg(dhclient_path);
		DDHCP(KPrintF("dhclient6 seglist = 0x%p\n", seglist);)
		if (seglist) {
			aros_dhcpv6.pid =
				(pid_t)CreateNewProcTags(NP_Seglist, seglist,
									 NP_Arguments, aros_dhcpv6.args,
									 NP_Cli, TRUE,
									 NP_Name, dhclient6_proc_name,
									 NP_CommandName, dhclient_cmd_name,
									 NP_ConsoleTask, NULL,
									 TAG_DONE);
			DDHCP(KPrintF("dhclient6 pid = 0x%p\n", aros_dhcpv6.pid);)
			if (!aros_dhcpv6.pid) {
				UnLoadSeg(seglist);
				seglist = BNULL;
			}
		}
		if (!seglist)
			error_request("Unable to start DHCPv6 client for the interface %s%u",
				(IPTR)ifp->if_name, (IPTR)ifp->if_unit);
	}
}

void kill_dhclient6(struct ifnet *ifp)
{
	if (aros_dhcpv6.pid) {
		Signal((APTR)aros_dhcpv6.pid, SIGBREAKF_CTRL_C);
		aros_dhcpv6.pid = (pid_t)NULL;
	}
}
#endif /* INET6 && DHCP6 */

void run_dhcp(void)
{
	struct ifnet *ifp;

	for (ifp = ifnet; ifp; ifp = ifp->if_next) {
		if (ifp->if_flags & IFF_DELAYUP) {
			DDHCP(KPrintF("Executing delayed DHCP start for %s%u\n", ifp->if_name, ifp->if_unit);)
			ifp->if_flags &= ~IFF_DELAYUP;
			if (ifp->if_data.ifi_aros_usedhcp)
				run_dhclient(ifp);
#if INET6 && DHCP6
			if (ifp->if_data.ifi_aros_usedhcp6)
				run_dhclient6(ifp);
#endif
		}
	}
}

