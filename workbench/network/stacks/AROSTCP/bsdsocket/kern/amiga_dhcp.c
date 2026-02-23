#include <conf.h>

#include <dos/dos.h>
#include <dos/dostags.h>
#include <proto/dos.h>
#include <kern/amiga_gui.h>
#include <kern/amiga_dhcp.h>
#include <net/if.h>
#include <net/if_sana.h>

#include <stdio.h>
#include <string.h>

extern UBYTE dhclient_path[];
extern struct ifnet *ifnet;
extern struct Library *logDOSBase;

#define DOSBase logDOSBase

static const TEXT dhclient_proc_name[] = "AROSTCP DHCP client";
static const TEXT dhclient_cmd_name[] = "dhclient";
#if INET6 && DHCP6
static const TEXT dhclient6_proc_name[] = "AROSTCP DHCPv6 client";
#endif

/*
 * Build the dhclient argument string for all interfaces that have
 * ifi_aros_usedhcp set, optionally excluding one interface (e.g. one
 * that is going offline).  Returns the number of interfaces included.
 */
static int
build_dhclient_args(char *buf, size_t len, struct ifnet *exclude_ifp, int v6)
{
	struct ifnet *ifp;
	int count = 0;
	char tmp[IFNAMSIZ + 8];

	if (v6)
		snprintf(buf, len, "-6 -q");
	else
		snprintf(buf, len, "-q");

	for (ifp = ifnet; ifp; ifp = ifp->if_next) {
		if (ifp == exclude_ifp)
			continue;
		if (v6) {
#if INET6 && DHCP6
			if (!ifp->if_data.ifi_aros_usedhcp6)
				continue;
#else
			continue;
#endif
		} else {
			if (!ifp->if_data.ifi_aros_usedhcp)
				continue;
		}
		snprintf(tmp, sizeof(tmp), " %s%u", ifp->if_name, ifp->if_unit);
		strncat(buf, tmp, len - strlen(buf) - 1);
		count++;
	}
	return count;
}

/*
 * Internal: kill any running dhclient for IPv4, then restart with args
 * built from all DHCP-enabled interfaces, excluding exclude_ifp.
 * If no interfaces remain, just kill and do not restart.
 */
static void
_update_dhclient(struct ifnet *exclude_ifp)
{
	BPTR seglist;
	int count;

	/* Kill existing client if running */
	if (aros_dhcpv4.pid) {
		Signal((APTR)aros_dhcpv4.pid, SIGBREAKF_CTRL_C);
		aros_dhcpv4.pid = (pid_t)NULL;
	}

	count = build_dhclient_args(aros_dhcpv4.args, sizeof(aros_dhcpv4.args),
	                            exclude_ifp, 0);
	if (count == 0)
		return; /* no interfaces need DHCPv4 */

	DDHCP(KPrintF("Starting DHCP client: %s\n", aros_dhcpv4.args);)
	seglist = LoadSeg(dhclient_path);
	DDHCP(KPrintF("dhclient seglist = 0x%p\n", seglist);)
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
		error_request("Unable to start DHCP client (%s)", (IPTR)aros_dhcpv4.args, 0);
}

/* Start (or restart) the DHCPv4 client including ifp. */
void run_dhclient(struct ifnet *ifp)
{
	char new_args[AROS_DHCP_ARGS_LEN];

	if (build_dhclient_args(new_args, sizeof(new_args), NULL, 0) == 0)
		return;

	/* Already running for the same set of interfaces — don't disturb it */
	if (aros_dhcpv4.pid && strcmp(aros_dhcpv4.args, new_args) == 0)
		return;

	_update_dhclient(NULL);
}

/* Kill DHCPv4 client for ifp going offline; restart for remaining interfaces. */
void kill_dhclient(struct ifnet *ifp)
{
	_update_dhclient(ifp);
}

#if INET6 && DHCP6
/*
 * Internal: kill any running dhclient6, then restart with args built from
 * all DHCPv6-enabled interfaces, excluding exclude_ifp.
 */
static void
_update_dhclient6(struct ifnet *exclude_ifp)
{
	BPTR seglist;
	int count;

	if (aros_dhcpv6.pid) {
		Signal((APTR)aros_dhcpv6.pid, SIGBREAKF_CTRL_C);
		aros_dhcpv6.pid = (pid_t)NULL;
	}

	count = build_dhclient_args(aros_dhcpv6.args, sizeof(aros_dhcpv6.args),
	                            exclude_ifp, 1);
	if (count == 0)
		return;

	DDHCP(KPrintF("Starting DHCPv6 client: %s\n", aros_dhcpv6.args);)
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
		error_request("Unable to start DHCPv6 client (%s)", (IPTR)aros_dhcpv6.args, 0);
}

void run_dhclient6(struct ifnet *ifp)
{
	char new_args[AROS_DHCP_ARGS_LEN];

	if (build_dhclient_args(new_args, sizeof(new_args), NULL, 1) == 0)
		return;

	/* Already running for the same set of interfaces — don't disturb it */
	if (aros_dhcpv6.pid && strcmp(aros_dhcpv6.args, new_args) == 0)
		return;

	_update_dhclient6(NULL);
}

void kill_dhclient6(struct ifnet *ifp)
{
	_update_dhclient6(ifp);
}
#endif /* INET6 && DHCP6 */

/*
 * Called during AROSTCP startup to start DHCP for all interfaces that had
 * IFF_DELAYUP set (i.e. DHCP was configured but the GUI wasn't up yet).
 * Clears the flag for all pending interfaces first, then starts a single
 * dhclient process covering all of them.
 */
void run_dhcp(void)
{
	struct ifnet *ifp;
	int need4 = 0;
#if INET6 && DHCP6
	int need6 = 0;
#endif

	for (ifp = ifnet; ifp; ifp = ifp->if_next) {
		if (ifp->if_flags & IFF_DELAYUP) {
			DDHCP(KPrintF("Executing delayed DHCP start for %s%u\n", ifp->if_name, ifp->if_unit);)
			ifp->if_flags &= ~IFF_DELAYUP;
			if (ifp->if_data.ifi_aros_usedhcp)
				need4 = 1;
#if INET6 && DHCP6
			if (ifp->if_data.ifi_aros_usedhcp6)
				need6 = 1;
#endif
		}
	}

	if (need4)
		_update_dhclient(NULL);
#if INET6 && DHCP6
	if (need6)
		_update_dhclient6(NULL);
#endif
}

