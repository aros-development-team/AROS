/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    CUnit tests for IPv6 fragmentation on output (RFC 8200 section 4.5): a
    datagram larger than the link MTU leaves as fragments that carry it
    whole - offsets that follow each other, the M flag on every fragment but
    the last, each within the MTU - and one that fits leaves unfragmented.

    The test sends UDP datagrams to a link-scope group on the interface and
    reads its own output with the bsdsocket BPF calls, so it needs no peer,
    only a multicast-capable interface (on hosted AROS, tap.device attached
    to a host tap). Usage:  cunit-net-frag6 [IFNAME]  (default eth0).
    Without such an interface the tests are skipped.
*/

#define __CONFIG_ROADSHOW__     /* inet_pton() and the bpf_*() calls */
#include "socktest.h"

#include <proto/dos.h>
#include <net/if.h>
#include <net/bpf.h>

struct Library *SocketBase = NULL;

static const char *ifname = "eth0";
static unsigned ifindex;
static int mtu;
static int ready;
static long bpf = -1;

#define GROUP   "ff02::4d54:66"
#define PORT    4567
#define BIG     3000    /* UDP payload larger than a 1500-byte link */

/* One fragment of the datagram with this identification, as captured. */
struct frag {
    unsigned off, len, more;
    unsigned char data[1500];
};

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

/* The IPv6 packets to the group seen within secs: the fragments of one
 * datagram (by the identification of the first one seen) into f[], or the
 * unfragmented datagram as a single entry. Returns how many, -1 for an
 * oversized packet (larger than the MTU). */
static int capture(struct frag *f, int max, int secs, int *whole)
{
    struct in6_addr g;
    char buf[BPF_DFLTBUFSIZE];
    unsigned long id = 0;
    int n = 0, ticks, have_id = 0;

    *whole = 0;
    inet_pton(AF_INET6, (char *)GROUP, &g);
    for (ticks = 0; ticks < secs * 10; ticks++) {
        long got;
        while ((got = bpf_read(bpf, buf, sizeof(buf))) > 0) {
            char *p = buf;
            while (p < buf + got) {
                struct bpf_hdr *h = (struct bpf_hdr *)p;
                const unsigned char *fr = (unsigned char *)p + h->bh_hdrlen, *ip = NULL;
                unsigned len = h->bh_caplen, iplen;

                if (len >= 14 + 40 && fr[12] == 0x86 && fr[13] == 0xdd) {
                    ip = fr + 14;
                    len -= 14;
                } else if (len >= 40 && (fr[0] >> 4) == 6)
                    ip = fr;
                if (ip && memcmp(ip + 24, &g, 16) == 0) {
                    iplen = 40 + ((ip[4] << 8) | ip[5]);
                    if ((int)iplen > mtu)
                        return -1;
                    if (ip[6] == 44 && len >= 48 && n < max) {      /* Fragment header */
                        unsigned long fid = ((unsigned long)ip[44] << 24) | (ip[45] << 16) | (ip[46] << 8) | ip[47];
                        unsigned offlg = (ip[42] << 8) | ip[43];
                        if (!have_id) {
                            id = fid;
                            have_id = 1;
                        }
                        if (fid == id) {
                            f[n].off = offlg & 0xfff8;
                            f[n].more = offlg & 1;
                            f[n].len = iplen - 48;
                            memcpy(f[n].data, ip + 48, f[n].len < sizeof(f[n].data) ? f[n].len : sizeof(f[n].data));
                            n++;
                        }
                    } else if (ip[6] == IPPROTO_UDP && !have_id) {
                        *whole = iplen - 40;
                        return 1;
                    }
                }
                p += BPF_WORDALIGN(h->bh_hdrlen + h->bh_caplen);
            }
        }
        if (n > 0 && !f[n - 1].more)
            return n;
        Delay(5);
    }
    return n;
}

static int send_group(int size, unsigned char fill)
{
    static unsigned char payload[BIG];
    struct sockaddr_in6 d;
    int s = socket(AF_INET6, SOCK_DGRAM, 0), r;

    if (s < 0)
        return -1;
    memset(payload, fill, sizeof(payload));
    payload[0] = 'F';
    payload[size - 1] = 'L';
    setsockopt(s, IPPROTO_IPV6, IPV6_MULTICAST_IF, &ifindex, sizeof(ifindex));
    bzero(&d, sizeof(d));
    d.sin6_len = sizeof(d);
    d.sin6_family = AF_INET6;
    d.sin6_port = htons(PORT);
    inet_pton(AF_INET6, (char *)GROUP, &d.sin6_addr);
    r = sendto(s, (char *)payload, size, 0, (struct sockaddr *)&d, sizeof(d));
    CloseSocket(s);
    return r;
}

#define NEED_IF() do { if (!ready) { CU_PASS("no suitable interface - skipping"); return; } } while (0)

/* ----------------------------------------------------------------- tests */

int init_suite(void) { return (SocketBase != NULL) ? 0 : -1; }
int clean_suite(void) { return 0; }

/* A datagram larger than the MTU leaves as every one of its fragments. */
void testFRAG6_ALL_FRAGMENTS(void)
{
    static struct frag f[8];
    int n, i, whole, total = 0;
    NEED_IF();
    CU_ASSERT_EQUAL(send_group(BIG, 'a'), BIG);
    n = capture(f, 8, 2, &whole);
    CU_ASSERT(n >= 0);          /* no packet larger than the MTU */
    CU_ASSERT(n >= 2);
    for (i = 0; i < n; i++)
        total += f[i].len;
    CU_ASSERT_EQUAL(total, 8 + BIG);    /* UDP header and payload */
    CU_ASSERT(n > 0 && !f[n - 1].more);
}

/* The fragments' offsets follow each other and M is set on all but the
 * last (RFC 8200 section 4.5). */
void testFRAG6_OFFSETS_AND_M(void)
{
    static struct frag f[8];
    int n, i, whole, ok = 1, next = 0;
    NEED_IF();
    CU_ASSERT_EQUAL(send_group(BIG, 'b'), BIG);
    n = capture(f, 8, 2, &whole);
    CU_ASSERT_FATAL(n >= 2);
    for (i = 0; i < n; i++) {
        if (f[i].off != (unsigned)next || f[i].more != (unsigned)(i < n - 1) ||
                (i < n - 1 && f[i].len % 8))
            ok = 0;
        next += f[i].len;
    }
    CU_ASSERT(ok);
}

/* Put back together, the fragments carry the datagram that was sent. */
void testFRAG6_REASSEMBLES(void)
{
    static struct frag f[8];
    static unsigned char d[8 + BIG];
    int n, i, whole, k;
    NEED_IF();
    CU_ASSERT_EQUAL(send_group(BIG, 'c'), BIG);
    n = capture(f, 8, 2, &whole);
    CU_ASSERT_FATAL(n >= 2);
    for (i = 0; i < n; i++)
        if (f[i].off + f[i].len <= sizeof(d))
            memcpy(d + f[i].off, f[i].data, f[i].len);
    CU_ASSERT_EQUAL((d[2] << 8) | d[3], PORT);              /* UDP destination port */
    CU_ASSERT_EQUAL((d[4] << 8) | d[5], 8 + BIG);           /* UDP length */
    CU_ASSERT_EQUAL(d[8], 'F');
    CU_ASSERT_EQUAL(d[8 + BIG - 1], 'L');
    for (k = 9; k < 8 + BIG - 1 && d[k] == 'c'; k++)
        ;
    CU_ASSERT_EQUAL(k, 8 + BIG - 1);
}

/* A datagram that fits the MTU is not fragmented. */
void testFRAG6_FITS_NOT_FRAGMENTED(void)
{
    static struct frag f[8];
    int n, whole, size = mtu - 40 - 8;
    NEED_IF();
    CU_ASSERT_EQUAL(send_group(size, 'd'), size);
    n = capture(f, 8, 2, &whole);
    CU_ASSERT_EQUAL(n, 1);
    CU_ASSERT_EQUAL(whole, 8 + size);
}

static int setup(void)
{
    struct ifreq ifr;
    int s, ok;

    s = socket(AF_INET6, SOCK_DGRAM, 0);
    if (s < 0)
        return 0;
    bzero(&ifr, sizeof(ifr));
    strncpy(ifr.ifr_name, ifname, sizeof(ifr.ifr_name) - 1);
    ok = IoctlSocket(s, SIOCGIFFLAGS, (char *)&ifr) == 0 &&
         (ifr.ifr_flags & IFF_MULTICAST) && (ifr.ifr_flags & IFF_UP) &&
         IoctlSocket(s, SIOCGIFINDEX, (char *)&ifr) == 0;
    if (ok)
        ifindex = ifr.ifr_index;
    if (ok && IoctlSocket(s, SIOCGIFMTU, (char *)&ifr) == 0)
        mtu = ifr.ifr_mtu;
    CloseSocket(s);
    return ok && mtu >= 1280 && mtu < 40 + 8 + BIG && bpf_start() == 0;
}

int main(int argc, char **argv)
{
    CU_pSuite p;

    if (argc > 1)
        ifname = argv[1];
    SocketBase = OpenLibrary("bsdsocket.library", 3);
    if (SocketBase == NULL) {
        printf("bsdsocket.library not available - skipping FRAG6 tests\n");
        return 0;
    }
    ready = setup();
    if (!ready)
        printf("%s: no multicast interface with an MTU below %d - skipping FRAG6 tests\n", ifname, 40 + 8 + BIG);

    if (CU_initialize_registry() != CUE_SUCCESS) {
        CloseLibrary(SocketBase);
        return CU_get_error();
    }
    p = CU_add_suite("FRAG6_Suite", init_suite, clean_suite);
    if (p == NULL ||
        !CU_add_test(p, "a large datagram leaves as all its fragments", testFRAG6_ALL_FRAGMENTS) ||
        !CU_add_test(p, "fragment offsets follow each other, M on all but the last", testFRAG6_OFFSETS_AND_M) ||
        !CU_add_test(p, "the fragments reassemble to the datagram sent", testFRAG6_REASSEMBLES) ||
        !CU_add_test(p, "a datagram that fits is not fragmented", testFRAG6_FITS_NOT_FRAGMENTED))
    {
        CU_cleanup_registry();
        CloseLibrary(SocketBase);
        return CU_get_error();
    }
    CU_basic_set_mode(CU_BRM_VERBOSE);
    CU_basic_run_tests();
    CU_basic_set_mode(CU_BRM_SILENT);
    CU_automated_package_name_set("NetUnitTests");
    CU_set_output_filename("Net-FRAG6");
    CU_automated_enable_junit_xml(CU_TRUE);
    CU_automated_run_tests();
    CU_cleanup_registry();
    if (bpf >= 0)
        bpf_close(bpf);
    CloseLibrary(SocketBase);
    return CU_get_error();
}
