/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    CUnit tests for the packets queued while an IPv4 address is resolved
    with ARP: a datagram fragmented while its neighbour is resolved leaves
    as all of its fragments, not just the last (RFC 1122 section 2.3.2.2
    allows one packet; FreeBSD keeps 16); and the requests for an address
    go out at most once a second, not once for every packet queued for it
    (section 2.3.2.1: a guard against ARP floods MUST be there).

    The first test needs a neighbour that answers ARP - by default the
    address of the interface with 1 as its last octet (on hosted AROS, the
    host side of the tap) - whose entry it deletes (SIOCDARP) first. The
    second sends to an address nobody has. What goes out is read with the
    bsdsocket BPF calls. Usage:  cunit-net-arpqueue [IFNAME [NEIGHBOUR]]
    (default eth0). Without an IPv4 interface the tests are skipped.
*/

#define __CONFIG_ROADSHOW__     /* inet_pton() and the bpf_*() calls */
#include "socktest.h"

#include <proto/dos.h>
#include <net/if.h>
#include <net/if_arp.h>
#include <net/bpf.h>

struct Library *SocketBase = NULL;

static const char *ifname = "eth0";
static const char *peer_arg;
static struct in_addr ours, peer, nobody;
static int ready;
static long bpf = -1;

#define BIG     3000            /* three fragments on a 1500-byte link */

static int bpf_start(void)
{
    u_int one = 1;
    char buf[BPF_DFLTBUFSIZE];

    bpf = bpf_open(0);
    if (bpf < 0)
        return -1;
    if (bpf_ioctl(bpf, BIOCSETIF, (char *)ifname) != 0 ||
            bpf_ioctl(bpf, BIOCIMMEDIATE, (char *)&one) != 0) {
        bpf_close(bpf);
        bpf = -1;
        return -1;
    }
    while (bpf_read(bpf, buf, sizeof(buf)) > 0)
        ;
    return 0;
}

/* Delete the ARP entry for addr. */
static void forget(struct in_addr addr)
{
    struct arpreq ar;
    struct sockaddr_in *sin = (struct sockaddr_in *)&ar.arp_pa;
    int s = socket(AF_INET, SOCK_DGRAM, 0);

    if (s < 0)
        return;
    bzero(&ar, sizeof(ar));
    sin->sin_len = sizeof(*sin);
    sin->sin_family = AF_INET;
    sin->sin_addr = addr;
    IoctlSocket(s, SIOCDARP, (char *)&ar);
    CloseSocket(s);
}

static int send_to(struct in_addr addr, int size)
{
    static char payload[BIG];
    struct sockaddr_in d;
    int s = socket(AF_INET, SOCK_DGRAM, 0), r;

    if (s < 0)
        return -1;
    memset(payload, 'a', sizeof(payload));
    bzero(&d, sizeof(d));
    d.sin_len = sizeof(d);
    d.sin_family = AF_INET;
    d.sin_port = htons(9);
    d.sin_addr = addr;
    r = sendto(s, payload, size, 0, (struct sockaddr *)&d, sizeof(d));
    CloseSocket(s);
    return r;
}

/*
 * What goes out on the link for ticks tenths of a second: the ARP requests
 * for addr and whether a reply came, and the fragments of one datagram to
 * addr - how many, how many bytes they carry together, the last with MF
 * clear.
 */
static void watch(struct in_addr addr, int ticks, int *requests, int *replied,
                  int *frags, int *bytes, int *last)
{
    char buf[BPF_DFLTBUFSIZE];
    unsigned id = 0;
    int t, have_id = 0;

    *requests = *replied = *frags = *bytes = *last = 0;
    for (t = 0; t < ticks; t++) {
        long got;

        while ((got = bpf_read(bpf, buf, sizeof(buf))) > 0) {
            char *p = buf;

            while (p < buf + got) {
                const struct bpf_hdr *bh = (const struct bpf_hdr *)p;
                const unsigned char *fr = (unsigned char *)p + bh->bh_hdrlen;
                unsigned len = bh->bh_caplen;

                if (len >= 14 + 28 && fr[12] == 0x08 && fr[13] == 0x06) {
                    const unsigned char *arp = fr + 14;    /* Ethernet/IPv4 ARP */

                    if (arp[7] == 1 && memcmp(arp + 24, &addr, 4) == 0)
                        (*requests)++;
                    if (arp[7] == 2 && memcmp(arp + 14, &addr, 4) == 0)
                        *replied = 1;
                }
                if (len >= 14 + 20 && fr[12] == 0x08 && fr[13] == 0x00 &&
                        memcmp(fr + 14 + 16, &addr, 4) == 0) {
                    const unsigned char *ip = fr + 14;
                    unsigned off = ((ip[6] & 0x1f) << 8) | ip[7];

                    if ((ip[6] & 0x20) || off != 0) {       /* a fragment */
                        unsigned fid = (ip[4] << 8) | ip[5];

                        if (!have_id) {
                            id = fid;
                            have_id = 1;
                        }
                        if (fid == id) {
                            (*frags)++;
                            *bytes += ((ip[2] << 8) | ip[3]) - (ip[0] & 0x0f) * 4;
                            if (!(ip[6] & 0x20))
                                *last = 1;
                        }
                    }
                }
                p += BPF_WORDALIGN(bh->bh_hdrlen + bh->bh_caplen);
            }
        }
        Delay(5);
    }
}

#define NEED_IF() do { if (!ready) { CU_PASS("no suitable interface - skipping"); return; } } while (0)

/* ----------------------------------------------------------------- tests */

int init_suite(void) { return (SocketBase != NULL) ? 0 : -1; }
int clean_suite(void) { return 0; }

/* A datagram fragmented while its neighbour is resolved leaves whole. */
void testARPQ_FRAGMENTS(void)
{
    char drain[BPF_DFLTBUFSIZE];
    int requests, replied, frags, bytes, last;
    NEED_IF();
    forget(peer);
    while (bpf_read(bpf, drain, sizeof(drain)) > 0)
        ;
    CU_ASSERT_EQUAL(send_to(peer, BIG), BIG);
    watch(peer, 20, &requests, &replied, &frags, &bytes, &last);
    if (!requests || !replied) {
        CU_PASS("the neighbour did not answer ARP - skipping");
        return;
    }
    CU_ASSERT_EQUAL(frags, 3);
    CU_ASSERT_EQUAL(bytes, 8 + BIG);            /* UDP header and payload */
    CU_ASSERT(last);
}

/* Ten packets for an address nobody has: at most two requests a second. */
void testARPQ_RATE(void)
{
    char drain[BPF_DFLTBUFSIZE];
    int i, requests, replied, frags, bytes, last;
    NEED_IF();
    forget(nobody);
    while (bpf_read(bpf, drain, sizeof(drain)) > 0)
        ;
    for (i = 0; i < 10; i++)
        send_to(nobody, 10);
    watch(nobody, 8, &requests, &replied, &frags, &bytes, &last);
    CU_ASSERT(requests >= 1);
    CU_ASSERT(requests <= 2);
}

static int setup(void)
{
    struct ifreq ifr;
    int s = socket(AF_INET, SOCK_DGRAM, 0), ok;

    if (s < 0)
        return 0;
    bzero(&ifr, sizeof(ifr));
    strncpy(ifr.ifr_name, ifname, sizeof(ifr.ifr_name) - 1);
    ok = IoctlSocket(s, SIOCGIFADDR, (char *)&ifr) == 0 &&
         ((struct sockaddr_in *)&ifr.ifr_addr)->sin_family == AF_INET;
    if (ok)
        ours = ((struct sockaddr_in *)&ifr.ifr_addr)->sin_addr;
    if (ok && (IoctlSocket(s, SIOCGIFMTU, (char *)&ifr) != 0 || ifr.ifr_mtu >= 20 + 8 + BIG))
        ok = 0;
    CloseSocket(s);
    if (!ok || ours.s_addr == 0)
        return 0;
    if (peer_arg != NULL)
        inet_pton(AF_INET, (char *)peer_arg, &peer);
    else {
        peer = ours;
        ((unsigned char *)&peer)[3] = 1;
    }
    nobody = ours;
    ((unsigned char *)&nobody)[3] = 254;
    if (peer.s_addr == ours.s_addr || nobody.s_addr == ours.s_addr)
        return 0;
    return bpf_start() == 0;
}

int main(int argc, char **argv)
{
    CU_pSuite p;

    if (argc > 1)
        ifname = argv[1];
    if (argc > 2)
        peer_arg = argv[2];
    SocketBase = OpenLibrary("bsdsocket.library", 3);
    if (SocketBase == NULL) {
        printf("bsdsocket.library not available - skipping ARPQUEUE tests\n");
        return 0;
    }
    ready = setup();
    if (!ready)
        printf("%s: no IPv4 interface - skipping ARPQUEUE tests\n", ifname);

    if (CU_initialize_registry() != CUE_SUCCESS) {
        CloseLibrary(SocketBase);
        return CU_get_error();
    }
    p = CU_add_suite("ARPQUEUE_Suite", init_suite, clean_suite);
    if (p == NULL ||
        !CU_add_test(p, "a datagram fragmented while resolving leaves whole", testARPQ_FRAGMENTS) ||
        !CU_add_test(p, "ARP requests go out at most once a second", testARPQ_RATE))
    {
        CU_cleanup_registry();
        CloseLibrary(SocketBase);
        return CU_get_error();
    }
    CU_basic_set_mode(CU_BRM_VERBOSE);
    CU_basic_run_tests();
    CU_basic_set_mode(CU_BRM_SILENT);
    CU_automated_package_name_set("NetUnitTests");
    CU_set_output_filename("Net-ARPQUEUE");
    CU_automated_enable_junit_xml(CU_TRUE);
    CU_automated_run_tests();
    CU_cleanup_registry();
    if (bpf >= 0)
        bpf_close(bpf);
    CloseLibrary(SocketBase);
    return CU_get_error();
}
