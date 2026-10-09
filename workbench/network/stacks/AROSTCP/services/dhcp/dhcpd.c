/*
 * Copyright (C) 2026 The AROS Dev Team
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * AROSTCP DHCP service daemon.
 *
 * The dhclient-launching logic that used to live in the stack (kern/amiga_dhcp.c)
 * now lives here, out of the core.  The daemon iterates all interfaces, finds
 * the ones that want DHCP (SIOCGIFDHCP), and launches/manages the ISC dhclient
 * for them - one client for all DHCPv4 interfaces, one for all DHCPv6.  It
 * registers with netservices.library and reacts, in its own task, to the
 * reconfigure-begin/end and stop signals it asked for: on begin it stops the
 * clients and clears the dynamic DNS it published (miami.library
 * ClearDynNameServ); on end it relaunches for the (possibly changed) interface
 * set; on stop it stops the clients and exits.
 *
 * dhclient itself publishes (and, on exit, withdraws) the DNS servers it learns
 * from a lease via the miami/roadshow dynamic-nameserver API (AddDynNameServ),
 * which stays in the stack - so the DHCP service owns its DNS through dhclient.
 * The daemon therefore does ONLY local process management on a reconfigure
 * signal (stop/relaunch its clients); it must NOT call back into the stack
 * while the stack may be holding its reload fence, or the reload deadlocks.
 */

#include <exec/types.h>
#include <exec/libraries.h>
#include <utility/tagitem.h>
#include <libraries/netservice.h>
#include <dos/dos.h>
#include <dos/dostags.h>
#include <sys/param.h>
#include <sys/socket.h>
#include <sys/sockio.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/socket.h>
#include <proto/netservice.h>

#include <string.h>
#include <stdio.h>

/* SocketBase and MiamiBase are provided and auto-opened by the "net" and
 * "miami" link libraries (see uselibs in the mmakefile) - this IS the daemon's
 * own stack binding, released when it exits.  Only netservices.library, which
 * has no autoinit, is opened by hand. */
struct Library *NetServicesBase;

#define DHCLIENT_FALLBACK  "SYS:System/Network/AROSTCP/C/dhclient"
#define ARGS_LEN        256
#define IFBUF_SIZE      (64 * sizeof(struct ifreq))

/* Resolved once at startup from ENV:AROSTCP/dhclient (published by the stack). */
static char dhclient_path[256];

static const TEXT dhcpv4_name[] = "AROSTCP DHCP client";
static const TEXT dhcpv6_name[] = "AROSTCP DHCPv6 client";
static const TEXT dhclient_cmd[] = "dhclient";

static pid_t dhcpv4_pid;
static pid_t dhcpv6_pid;
static char  dhcpv4_args[ARGS_LEN];
static char  dhcpv6_args[ARGS_LEN];

static void logln(CONST_STRPTR s)
{
    BPTR f = Open("SYS:dhcpd.log", MODE_READWRITE);
    if(f) { Seek(f, 0, OFFSET_END); FPuts(f, (STRPTR)s); FPuts(f, (STRPTR)"\n"); Close(f); }
}

/*
 * Build the dhclient argument string for every interface whose SIOCGIFDHCP
 * reports the wanted family (v6 bit 1, else bit 0).  Returns the count.
 */
static int build_args(int sock, char *buf, size_t len, int v6)
{
    char ifbuf[IFBUF_SIZE];
    struct ifconf ifc;
    char *p, *pend;
    char seen[32][IFNAMSIZ];
    int nseen = 0, count = 0;

    strncpy(buf, v6 ? "-6 -q" : "-q", len);
    buf[len - 1] = '\0';

    ifc.ifc_len = sizeof(ifbuf);
    ifc.ifc_buf = ifbuf;
    if(IoctlSocket(sock, SIOCGIFCONF, (char *)&ifc) < 0)
        return 0;

    p    = ifbuf;
    pend = ifbuf + ifc.ifc_len;
    while(p + sizeof(struct ifreq) <= pend + 1 && p < pend) {
        struct ifreq *ifr = (struct ifreq *)p;
        struct sockaddr *sa = &ifr->ifr_addr;
        size_t salen = sa->sa_len;
        struct ifreq q;
        int wants, dup = 0, i;

        if(salen < sizeof(struct sockaddr))
            salen = sizeof(struct sockaddr);
        p += IFNAMSIZ + salen;

        for(i = 0; i < nseen; i++)
            if(strncmp(seen[i], ifr->ifr_name, IFNAMSIZ) == 0) { dup = 1; break; }
        if(dup)
            continue;
        if(nseen < 32) {
            strncpy(seen[nseen], ifr->ifr_name, IFNAMSIZ);
            nseen++;
        }

        memset(&q, 0, sizeof(q));
        strncpy(q.ifr_name, ifr->ifr_name, IFNAMSIZ - 1);
        if(IoctlSocket(sock, SIOCGIFDHCP, (char *)&q) < 0)
            continue;
        wants = v6 ? (q.ifr_metric & 2) : (q.ifr_metric & 1);
        if(!wants)
            continue;

        {
            char tmp[IFNAMSIZ + 2];
            snprintf(tmp, sizeof(tmp), " %s", ifr->ifr_name);
            strncat(buf, tmp, len - strlen(buf) - 1);
            count++;
        }
    }
    return count;
}

static pid_t launch(const char *args, const char *procname)
{
    BPTR seg = LoadSeg((STRPTR)dhclient_path);
    pid_t pid;

    if(seg == BNULL) {
        logln("dhcpd: cannot load dhclient");
        return 0;
    }
    pid = (pid_t)CreateNewProcTags(NP_Seglist,    (IPTR)seg,
                                   NP_Arguments,  (IPTR)args,
                                   NP_Cli,        TRUE,
                                   NP_Name,       (IPTR)procname,
                                   NP_CommandName,(IPTR)dhclient_cmd,
                                   NP_ConsoleTask,(IPTR)NULL,
                                   TAG_DONE);
    if(!pid) {
        UnLoadSeg(seg);
        logln("dhcpd: cannot start dhclient");
    }
    return pid;
}

/* (Re)launch dhclient for the current DHCP interface set. */
static void dhcp_start(int sock)
{
    int n4;
#if INET6
    int n6;
#endif

    if(dhcpv4_pid) { Signal((struct Task *)dhcpv4_pid, SIGBREAKF_CTRL_C); dhcpv4_pid = 0; }
    n4 = build_args(sock, dhcpv4_args, sizeof(dhcpv4_args), 0);
    if(n4 > 0) {
        dhcpv4_pid = launch(dhcpv4_args, dhcpv4_name);
        if(dhcpv4_pid) logln("dhcpd: started DHCPv4 client");
    }

#if INET6
    if(dhcpv6_pid) { Signal((struct Task *)dhcpv6_pid, SIGBREAKF_CTRL_C); dhcpv6_pid = 0; }
    n6 = build_args(sock, dhcpv6_args, sizeof(dhcpv6_args), 1);
    if(n6 > 0) {
        dhcpv6_pid = launch(dhcpv6_args, dhcpv6_name);
        if(dhcpv6_pid) logln("dhcpd: started DHCPv6 client");
    }
#endif
}

/*
 * Stop every dhclient.  dhclient withdraws the dynamic DNS it published as it
 * exits, so there is nothing for us to clean up here - and we must not call a
 * stack API from this reconfigure-signal context anyway.
 */
static void dhcp_stop(void)
{
    if(dhcpv4_pid) { Signal((struct Task *)dhcpv4_pid, SIGBREAKF_CTRL_C); dhcpv4_pid = 0; }
    if(dhcpv6_pid) { Signal((struct Task *)dhcpv6_pid, SIGBREAKF_CTRL_C); dhcpv6_pid = 0; }
    dhcpv4_args[0] = dhcpv6_args[0] = '\0';
}

int main(void)
{
    APTR handle;
    BYTE sStop, sBegin, sEnd;
    ULONG mStop, mBegin, mEnd, got;
    int sock, running = 1;

    NetServicesBase = OpenLibrary(NETSERVICESNAME, 0);
    if(!NetServicesBase) { logln("dhcpd: no netservices.library"); return 20; }

    /* Which DHCP client to run is THIS service's concern, not the stack's: use
     * our own optional override (ENV:AROSTCP/dhcp/client), else the install
     * default. */
    if(GetVar("AROSTCP/dhcp/client", dhclient_path, sizeof(dhclient_path),
              GVF_GLOBAL_ONLY) <= 0)
        strcpy(dhclient_path, DHCLIENT_FALLBACK);

    sock = socket(AF_INET, SOCK_DGRAM, 0);
    if(sock < 0) { logln("dhcpd: no socket"); goto out; }

    sStop  = AllocSignal(-1);
    sBegin = AllocSignal(-1);
    sEnd   = AllocSignal(-1);
    mStop  = (sStop  >= 0) ? (1UL << sStop)  : 0;
    mBegin = (sBegin >= 0) ? (1UL << sBegin) : 0;
    mEnd   = (sEnd   >= 0) ? (1UL << sEnd)   : 0;

    {
        struct TagItem t[] = {
            { NETSERVICE_Name,             (IPTR)"dhcp" },
            { NETSERVICE_Order,            (IPTR)NSPRI_DHCP },
            { NETSERVICE_ReconfigBeginSig, (IPTR)sBegin },
            { NETSERVICE_ReconfigEndSig,   (IPTR)sEnd },
            { NETSERVICE_StopSig,          (IPTR)sStop },
            { TAG_DONE, 0 }
        };
        handle = RegisterNetService(t);
    }
    if(!handle) { logln("dhcpd: RegisterNetService failed"); goto out2; }

    logln("dhcpd: started, registered");
    dhcp_start(sock);       /* initial launch for the boot config */

    while(running) {
        got = Wait(mStop | mBegin | mEnd | SIGBREAKF_CTRL_C);
        if(got & mBegin) { logln("dhcpd: reconfigure-begin (stopping clients)"); dhcp_stop(); }
        if(got & mEnd)   { logln("dhcpd: reconfigure-end (relaunching)");        dhcp_start(sock); }
        if(got & (mStop | SIGBREAKF_CTRL_C)) { logln("dhcpd: stop"); running = 0; }
    }

    dhcp_stop();
    UnregisterNetService(handle);
out2:
    if(sStop  >= 0) FreeSignal(sStop);
    if(sBegin >= 0) FreeSignal(sBegin);
    if(sEnd   >= 0) FreeSignal(sEnd);
    CloseSocket(sock);
out:
    CloseLibrary(NetServicesBase);
    logln("dhcpd: exited");
    return 0;
}
