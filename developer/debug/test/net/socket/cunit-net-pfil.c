/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    CUnit tests for the packet hook points of each family (bsdsocket
    net/pfil.h in the stack), as an application reaches them: the hooks of
    miami.library (MiamiPFAddHook()).

    - a Miami hook on an interface sees the IPv4 packets that leave and
      arrive there, as it always did (ptype MIAMIPFBPT_IP), and none after
      it is removed;
    - it sees IPv6 packets only when it asked for them with MIAMITAG_IPV6
      (ptype MIAMIPFBPT_IP6): a hook written for IPv4 and ARP is never
      shown a packet it would misread;
    - the ARP point: a hook on a SANA-II interface sees the ARP packets
      that arrive there (where a neighbour answers: on the loopback there
      is no ARP, and with no neighbour nothing arrives - then it is said,
      not failed);
    - hooks added and removed while traffic flows lose no packet: a TCP
      stream over the loopback, with a hook put on and taken off between
      its writes, arrives whole.
*/

#include "socktest.h"

#include <proto/dos.h>
#include <aros/asmcall.h>
#include <utility/hooks.h>
#include <utility/tagitem.h>
#include <proto/miami.h>
#include <libraries/miami.h>
#include <net/if.h>
#include <net/if_arp.h>
#include <netinet/in.h>

#include <stdlib.h>

struct Library *SocketBase = NULL;
struct Library *MiamiBase = NULL;

#define PROGRAM "cunit-net-pfil"

/* what a hook saw */
struct pfcount {
    volatile ULONG  pc_ip, pc_arp, pc_ip6, pc_other;
    volatile ULONG  pc_badname;     /* called with a buffer of another interface */
    char            pc_name[IFNAMSIZ];
};

AROS_UFH3(void, pf_hookfunc,
          AROS_UFHA(struct Hook *, hook, A0),
          AROS_UFHA(APTR, handle, A2),
          AROS_UFHA(struct MiamiPFBuffer *, pfb, A1))
{
    AROS_USERFUNC_INIT

    struct pfcount *pc = (struct pfcount *)hook->h_Data;

    switch (pfb->ptype) {
    case MIAMIPFBPT_IP:  pc->pc_ip++;  break;
    case MIAMIPFBPT_ARP: pc->pc_arp++; break;
    case MIAMIPFBPT_IP6: pc->pc_ip6++; break;
    default:             pc->pc_other++; break;
    }
    if (pfb->name == NULL || strcmp((char *)pfb->name, pc->pc_name) != 0)
        pc->pc_badname++;

    AROS_USERFUNC_EXIT
}

static struct Hook pf_hook;
static struct pfcount pf_count;

/* A hook on the interface; ip6: with MIAMITAG_IPV6. NULL if refused. */
static APTR hook_add(const char *ifname, int ip6)
{
    struct TagItem tags[] = {
        { MIAMITAG_HOOKTYPE, MIAMICPU_M68KREG },
        { MIAMITAG_IPV6, TRUE },
        { TAG_DONE, 0 }
    };

    if (!ip6)
        tags[1].ti_Tag = TAG_DONE;
    memset((void *)&pf_count, 0, sizeof(pf_count));
    strncpy(pf_count.pc_name, ifname, sizeof(pf_count.pc_name) - 1);
    memset(&pf_hook, 0, sizeof(pf_hook));
    pf_hook.h_Entry = (HOOKFUNC)pf_hookfunc;
    pf_hook.h_Data = &pf_count;
    return MiamiPFAddHook(&pf_hook, (UBYTE *)ifname, tags);
}

/* n datagrams over the loopback of the family, each received; how many arrived */
static int udp_loop(int family, int n)
{
    unsigned short port;
    struct sockaddr_storage ss;
    socklen_t len;
    int r = st_bound(family, SOCK_DGRAM, &port), w = socket(family, SOCK_DGRAM, 0);
    int i, got = 0;
    char b = 'x';

    if (r < 0 || w < 0) {
        if (r >= 0) CloseSocket(r);
        if (w >= 0) CloseSocket(w);
        return -1;
    }
    len = st_loopback(family, &ss, port);
    for (i = 0; i < n; i++) {
        if (sendto(w, &b, 1, 0, (struct sockaddr *)&ss, len) != 1)
            continue;
        if (sock_wait(r, 0, 2) > 0 && recv(r, &b, 1, 0) == 1)
            got++;
    }
    CloseSocket(w);
    CloseSocket(r);
    return got;
}

static int init_suite(void) { return 0; }
static int clean_suite(void) { return 0; }

/* IPv4 as before: in and out on lo0, typed IP, nothing after removal. */
static void testPFIL_MIAMI_IPV4(void)
{
    APTR h = hook_add("lo0", 0);
    ULONG seen;

    CU_ASSERT_FATAL(h != NULL);
    CU_ASSERT_EQUAL(udp_loop(AF_INET, 4), 4);
    MiamiPFRemoveHook(h);
    printf("\n    lo0, 4 datagrams: %lu IP, %lu IPv6, %lu other",
           (unsigned long)pf_count.pc_ip, (unsigned long)pf_count.pc_ip6,
           (unsigned long)pf_count.pc_other);
    CU_ASSERT(pf_count.pc_ip >= 8);             /* each out and in */
    CU_ASSERT_EQUAL(pf_count.pc_ip6, 0);
    CU_ASSERT_EQUAL(pf_count.pc_other, 0);
    CU_ASSERT_EQUAL(pf_count.pc_badname, 0);

    seen = pf_count.pc_ip;
    CU_ASSERT_EQUAL(udp_loop(AF_INET, 2), 2);
    CU_ASSERT_EQUAL(pf_count.pc_ip, seen);      /* removed: called no more */
}

/* IPv6 only to a hook that asked for it. */
static void testPFIL_MIAMI_IPV6_TAG(void)
{
    APTR h;

    h = hook_add("lo0", 0);
    CU_ASSERT_FATAL(h != NULL);
    CU_ASSERT_EQUAL(udp_loop(AF_INET6, 4), 4);
    MiamiPFRemoveHook(h);
    printf("\n    without the tag: %lu IPv6", (unsigned long)pf_count.pc_ip6);
    CU_ASSERT_EQUAL(pf_count.pc_ip6, 0);

    h = hook_add("lo0", 1);
    CU_ASSERT_FATAL(h != NULL);
    CU_ASSERT_EQUAL(udp_loop(AF_INET6, 4), 4);
    MiamiPFRemoveHook(h);
    printf(", with it: %lu IPv6, %lu other", (unsigned long)pf_count.pc_ip6,
           (unsigned long)pf_count.pc_other);
    CU_ASSERT(pf_count.pc_ip6 >= 8);
    CU_ASSERT_EQUAL(pf_count.pc_other, 0);
    CU_ASSERT_EQUAL(pf_count.pc_badname, 0);
}

/*
 * The first interface other than the loopback with an IPv4 address: its
 * name, address and mask (network order). 0 if there is none.
 */
static int first_lan(char *name, ULONG *addr, ULONG *mask)
{
    char buf[1024];
    struct ifconf ifc;
    struct ifreq *ifr, ifm;
    int s = socket(AF_INET, SOCK_DGRAM, 0), found = 0;
    char *p;

    if (s < 0)
        return 0;
    ifc.ifc_len = sizeof(buf);
    ifc.ifc_buf = buf;
    if (IoctlSocket(s, SIOCGIFCONF, (char *)&ifc) == 0) {
        for (p = buf; !found && p < buf + ifc.ifc_len;
                p += sizeof(ifr->ifr_name) +
                     (((struct ifreq *)p)->ifr_addr.sa_len > sizeof(struct sockaddr) ?
                      ((struct ifreq *)p)->ifr_addr.sa_len : sizeof(struct sockaddr))) {
            ifr = (struct ifreq *)p;
            if (ifr->ifr_addr.sa_family != AF_INET || strncmp(ifr->ifr_name, "lo", 2) == 0)
                continue;
            memset(&ifm, 0, sizeof(ifm));
            strncpy(ifm.ifr_name, ifr->ifr_name, sizeof(ifm.ifr_name));
            if (IoctlSocket(s, SIOCGIFNETMASK, (char *)&ifm) != 0)
                continue;
            strncpy(name, ifr->ifr_name, IFNAMSIZ);
            name[IFNAMSIZ - 1] = '\0';
            *addr = ((struct sockaddr_in *)&ifr->ifr_addr)->sin_addr.s_addr;
            *mask = ((struct sockaddr_in *)&ifm.ifr_addr)->sin_addr.s_addr;
            found = 1;
        }
    }
    CloseSocket(s);
    return found;
}

/*
 * The ARP point: a hook on a SANA-II interface sees ARP packets. The
 * neighbour asked is the first address of the subnet (the router, in the
 * test harness the host), its cache entry deleted first so that it is
 * asked again; the answer arrives through the ARP point.
 */
static void testPFIL_ARP(void)
{
    char ifname[IFNAMSIZ];
    ULONG addr, mask, peer;
    struct arpreq ar;
    struct sockaddr_in to;
    APTR h;
    int s, i;
    char b = 'x';

    if (!first_lan(ifname, &addr, &mask)) {
        printf("\n    no interface but the loopback has an IPv4 address: ARP not tested");
        return;
    }
    peer = (addr & mask) | htonl(1);
    if (peer == addr)
        peer = (addr & mask) | htonl(2);

    s = socket(AF_INET, SOCK_DGRAM, 0);
    CU_ASSERT_FATAL(s >= 0);
    memset(&ar, 0, sizeof(ar));
    ((struct sockaddr_in *)&ar.arp_pa)->sin_family = AF_INET;
    ((struct sockaddr_in *)&ar.arp_pa)->sin_len = sizeof(struct sockaddr_in);
    ((struct sockaddr_in *)&ar.arp_pa)->sin_addr.s_addr = peer;
    (void)IoctlSocket(s, SIOCDARP, (char *)&ar);   /* none there is no error here */

    h = hook_add(ifname, 0);
    CU_ASSERT_FATAL(h != NULL);
    memset(&to, 0, sizeof(to));
    to.sin_len = sizeof(to);
    to.sin_family = AF_INET;
    to.sin_port = htons(9);                         /* discard */
    to.sin_addr.s_addr = peer;
    for (i = 0; i < 6 && pf_count.pc_arp == 0; i++) {
        sendto(s, &b, 1, 0, (struct sockaddr *)&to, sizeof(to));
        Delay(25);
    }
    MiamiPFRemoveHook(h);
    CloseSocket(s);
    printf("\n    %s, neighbour %lu.%lu.%lu.%lu: %lu ARP, %lu IP, %lu IPv6", ifname,
           (unsigned long)(ntohl(peer) >> 24), (unsigned long)(ntohl(peer) >> 16) & 255,
           (unsigned long)(ntohl(peer) >> 8) & 255, (unsigned long)ntohl(peer) & 255,
           (unsigned long)pf_count.pc_arp, (unsigned long)pf_count.pc_ip,
           (unsigned long)pf_count.pc_ip6);
    if (pf_count.pc_arp == 0)
        printf(" - no neighbour answered: the ARP point is not seen here");
    CU_ASSERT_EQUAL(pf_count.pc_ip6, 0);
    CU_ASSERT_EQUAL(pf_count.pc_other, 0);
    CU_ASSERT_EQUAL(pf_count.pc_badname, 0);
}

/*
 * A TCP stream over the loopback while a hook is put on and taken off
 * between its writes (the stack's task runs the hooks while this task
 * changes them): every byte arrives, in order.
 */
#define STREAM_CHUNK    512
#define STREAM_CHUNKS   128

static void testPFIL_ADD_REMOVE_UNDER_TRAFFIC(void)
{
    int family = st_any_family(), c, a, i, got = 0, bad = 0, n, off = 0;
    static unsigned char out[STREAM_CHUNK], in[STREAM_CHUNK * 2];
    APTR h = NULL;

    CU_ASSERT_FATAL(st_tcp_pair(family, &c, &a) == 0);
    for (i = 0; i < STREAM_CHUNKS; i++) {
        int k;

        if (i & 1) {
            h = hook_add("lo0", 1);
            CU_ASSERT(h != NULL);
        }
        for (k = 0; k < STREAM_CHUNK; k++)
            out[k] = (unsigned char)(i + k);
        CU_ASSERT_EQUAL(send(c, out, STREAM_CHUNK, 0), STREAM_CHUNK);
        if (h != NULL) {
            MiamiPFRemoveHook(h);
            h = NULL;
        }
        /* read it back, checking the pattern */
        while (got < (i + 1) * STREAM_CHUNK && sock_wait(a, 0, 2) > 0 &&
                (n = recv(a, in, sizeof(in), 0)) > 0) {
            for (k = 0; k < n; k++, off++)
                if (in[k] != (unsigned char)(off / STREAM_CHUNK + off % STREAM_CHUNK))
                    bad++;
            got += n;
        }
    }
    while (got < STREAM_CHUNKS * STREAM_CHUNK && sock_wait(a, 0, 3) > 0 &&
            (n = recv(a, in, sizeof(in), 0)) > 0) {
        int k;

        for (k = 0; k < n; k++, off++)
            if (in[k] != (unsigned char)(off / STREAM_CHUNK + off % STREAM_CHUNK))
                bad++;
        got += n;
    }
    printf("\n    %d of %d bytes arrived, %d out of order", got, STREAM_CHUNKS * STREAM_CHUNK, bad);
    CU_ASSERT_EQUAL(got, STREAM_CHUNKS * STREAM_CHUNK);
    CU_ASSERT_EQUAL(bad, 0);
    CloseSocket(c);
    CloseSocket(a);
}

int main(int argc, char **argv)
{
    CU_pSuite p;

    SocketBase = OpenLibrary("bsdsocket.library", 4);
    if (SocketBase == NULL) {
        printf("no bsdsocket.library\n");
        return 20;
    }
    MiamiBase = OpenLibrary("miami.library", 0);
    if (MiamiBase == NULL) {
        printf("SKIP %s: no miami.library\n", PROGRAM);
        CloseLibrary(SocketBase);
        return 0;
    }
    if (CU_initialize_registry() != CUE_SUCCESS) {
        CloseLibrary(MiamiBase);
        CloseLibrary(SocketBase);
        return CU_get_error();
    }
    p = CU_add_suite("PFIL_Suite", init_suite, clean_suite);
    if (p == NULL ||
        !st_add_test(p, "a Miami hook sees IPv4 in and out as before", testPFIL_MIAMI_IPV4, ST_V4) ||
        !st_add_test(p, "a Miami hook sees IPv6 only when it asks", testPFIL_MIAMI_IPV6_TAG, ST_V6) ||
        !st_add_test(p, "a Miami hook on a SANA-II interface sees ARP", testPFIL_ARP, ST_V4) ||
        !st_add_test(p, "hooks added and removed under traffic lose nothing",
                     testPFIL_ADD_REMOVE_UNDER_TRAFFIC, ST_ANY))
    {
        CU_cleanup_registry();
        CloseLibrary(MiamiBase);
        CloseLibrary(SocketBase);
        return CU_get_error();
    }
    CU_basic_set_mode(CU_BRM_VERBOSE);
    CU_basic_run_tests();
    CU_basic_set_mode(CU_BRM_SILENT);
    CU_automated_package_name_set("NetUnitTests");
    CU_set_output_filename("Net-Pfil");
    CU_automated_enable_junit_xml(CU_TRUE);
    CU_automated_run_tests();
    CU_cleanup_registry();
    CloseLibrary(MiamiBase);
    CloseLibrary(SocketBase);
    return CU_get_error();
}
