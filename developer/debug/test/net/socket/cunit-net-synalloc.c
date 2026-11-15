/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    CUnit tests for the SYN cache's table, which TCP makes at the first
    listen() (of either family) instead of when it starts, so that a
    machine that only connects out does not pay for it, and keeps until
    the transports stop.

    The table itself cannot be seen from here: the stack has listened
    before this program runs (other tests, servers), and its 256 bytes to
    8 KB are lost in what other tasks do to free memory. The tests hold to
    what an application sees:

    - a SYN to a port that is bound but not listening is refused and puts
      nothing into the cache (net.inet.tcp.stats: the tcps_sc_* counters
      do not move);
    - listen() and accept() work, over IPv4 and over IPv6, and the
      connection went through the cache (the counters moved);
    - after every listener this program had was closed, listen() works
      again and accepts; listen() twice on one socket is no error.

    The counters are read with MiamiSysCtl(); without miami.library or
    without the variable, the counter checks are left out and the rest is
    still tested.
*/

#include "socktest.h"

#include <proto/miami.h>
#include <sys/sysctl.h>
#include <stddef.h>
#include <netinet/in_systm.h>
#include <netinet/ip.h>
#include <netinet/tcp_timer.h>
#include <netinet/tcp_var.h>

struct Library *SocketBase = NULL;
struct Library *MiamiBase = NULL;

#define PROGRAM     "cunit-net-synalloc"
#define LISTENERS   4           /* listeners open at once in the relisten test */

int init_suite(void) { return (SocketBase != NULL) ? 0 : -1; }
int clean_suite(void) { return 0; }

/* The SYN cache's counters of net.inet.tcp.stats that a handshake moves. */
struct sc_counts {
    u_long added, sendcookie, completed, dupsyn, reset, recvcookie;
};

/* Read them; 0 if they could not be read (no miami.library, no variable). */
static int sc_read(struct sc_counts *c)
{
    LONG name[4] = { CTL_NET, PF_INET, IPPROTO_TCP, TCPCTL_STATS };
    struct tcpstat st;
    LONG len = sizeof(st);

    if (MiamiBase == NULL)
        return 0;
    if (MiamiSysCtl(name, 4, &st, &len, NULL, 0, 0) != 0)
        return 0;
    c->added = st.tcps_sc_added;
    c->sendcookie = st.tcps_sc_sendcookie;
    c->completed = st.tcps_sc_completed;
    c->dupsyn = st.tcps_sc_dupsyn;
    c->reset = st.tcps_sc_reset;
    c->recvcookie = st.tcps_sc_recvcookie;
    return 1;
}

/*
 * Connect to the loopback port nport (network order) of the family and,
 * if lfd is a listener, accept the connection and pass a byte each way.
 * 0 when that worked; with lfd < 0 the connection is expected to be
 * refused, and 0 means it was.
 */
static int handshake(int family, int lfd, unsigned short nport)
{
    struct sockaddr_storage ss;
    socklen_t len;
    int c, a = -1, on = 1, err = 0, ok = -1;
    socklen_t elen = sizeof(err);
    char b = 'x';

    c = socket(family, SOCK_STREAM, 0);
    if (c < 0)
        return -1;
    IoctlSocket(c, FIONBIO, (char *)&on);
    len = st_loopback(family, &ss, nport);
    if (connect(c, (struct sockaddr *)&ss, len) != 0 && Errno() != EINPROGRESS) {
        /* refused at once: right only when nothing listens */
        ok = (lfd < 0 && Errno() == ECONNREFUSED) ? 0 : -1;
        CloseSocket(c);
        return ok;
    }
    if (lfd < 0) {
        /* the answer is a reset: the connection ends with ECONNREFUSED */
        if (sock_wait(c, 1, 3) > 0 &&
                getsockopt(c, SOL_SOCKET, SO_ERROR, &err, &elen) == 0 &&
                err == ECONNREFUSED)
            ok = 0;
        CloseSocket(c);
        return ok;
    }
    if (sock_wait(lfd, 0, 3) <= 0)
        goto out;
    len = sizeof(ss);
    a = accept(lfd, (struct sockaddr *)&ss, &len);
    if (a < 0 || ss.ss_family != family)
        goto out;
    if (sock_wait(c, 1, 3) <= 0 ||
            getsockopt(c, SOL_SOCKET, SO_ERROR, &err, &elen) != 0 || err != 0)
        goto out;
    if (send(c, &b, 1, 0) != 1 || sock_wait(a, 0, 3) <= 0 || recv(a, &b, 1, 0) != 1)
        goto out;
    if (send(a, &b, 1, 0) != 1 || sock_wait(c, 0, 3) <= 0 || recv(c, &b, 1, 0) != 1)
        goto out;
    ok = (b == 'x') ? 0 : -1;
out:
    if (a >= 0)
        CloseSocket(a);
    CloseSocket(c);
    return ok;
}

/* A loopback TCP listener on a port of its own, its port in *nport. */
static int listener(int family, unsigned short *nport)
{
    int s = st_bound(family, SOCK_STREAM, nport);

    if (s >= 0 && listen(s, 5) != 0) {
        CloseSocket(s);
        s = -1;
    }
    return s;
}

/* A SYN to a port that is bound but does not listen is refused, and the
 * SYN cache is not entered: its counters stay as they were. */
void testSYNALLOC_NOT_LISTENING(void)
{
    struct sc_counts c0, c1;
    unsigned short nport;
    int family = st_any_family(), have, s;

    s = st_bound(family, SOCK_STREAM, &nport);
    CU_ASSERT_FATAL(s >= 0);
    have = sc_read(&c0);
    CU_ASSERT_EQUAL(handshake(family, -1, nport), 0);
    if (have && sc_read(&c1)) {
        CU_ASSERT_EQUAL(c1.added, c0.added);
        CU_ASSERT_EQUAL(c1.sendcookie, c0.sendcookie);
        CU_ASSERT_EQUAL(c1.completed, c0.completed);
        CU_ASSERT_EQUAL(c1.recvcookie, c0.recvcookie);
        CU_ASSERT_EQUAL(c1.dupsyn, c0.dupsyn);
        CU_ASSERT_EQUAL(c1.reset, c0.reset);
    }
    CloseSocket(s);
}

/* listen() then accept(): the connection is made, through the cache (an
 * entry, or a cookie if the cache answers with cookies only), and
 * completed from it. */
static void accept_family(int family)
{
    struct sc_counts c0, c1;
    unsigned short nport;
    int have, l;

    l = listener(family, &nport);
    CU_ASSERT_FATAL(l >= 0);
    have = sc_read(&c0);
    CU_ASSERT_EQUAL(handshake(family, l, nport), 0);
    if (have && sc_read(&c1)) {
        CU_ASSERT(c1.added + c1.sendcookie >= c0.added + c0.sendcookie + 1);
        CU_ASSERT(c1.completed >= c0.completed + 1);
    }
    CloseSocket(l);
}
void testSYNALLOC_ACCEPT4(void) { accept_family(AF_INET); }
void testSYNALLOC_ACCEPT6(void) { accept_family(AF_INET6); }

/* Several listeners at once, of every family the stack has, all closed;
 * then a new one listens and accepts, and so does each family's. */
void testSYNALLOC_RELISTEN(void)
{
    int fams[2], nfam = 0, i, f, l[LISTENERS];
    unsigned short nport[LISTENERS];

    if (st_have_family(AF_INET))
        fams[nfam++] = AF_INET;
    if (st_have_family(AF_INET6))
        fams[nfam++] = AF_INET6;
    CU_ASSERT_FATAL(nfam > 0);

    for (i = 0; i < LISTENERS; i++) {
        l[i] = listener(fams[i % nfam], &nport[i]);
        CU_ASSERT(l[i] >= 0);
    }
    for (i = 0; i < LISTENERS; i++)
        if (l[i] >= 0)
            CU_ASSERT_EQUAL(handshake(fams[i % nfam], l[i], nport[i]), 0);
    for (i = 0; i < LISTENERS; i++)
        if (l[i] >= 0)
            CloseSocket(l[i]);

    for (f = 0; f < nfam; f++) {
        l[0] = listener(fams[f], &nport[0]);
        CU_ASSERT_FATAL(l[0] >= 0);
        CU_ASSERT_EQUAL(handshake(fams[f], l[0], nport[0]), 0);
        CloseSocket(l[0]);
    }
}

/* listen() a second time on a listening socket is no error, and the
 * socket goes on accepting. */
void testSYNALLOC_LISTEN_TWICE(void)
{
    unsigned short nport;
    int family = st_any_family(), l;

    l = listener(family, &nport);
    CU_ASSERT_FATAL(l >= 0);
    CU_ASSERT_EQUAL(listen(l, 5), 0);
    CU_ASSERT_EQUAL(handshake(family, l, nport), 0);
    CloseSocket(l);
}

int main(int argc, char **argv)
{
    CU_pSuite p;

    SocketBase = OpenLibrary("bsdsocket.library", 4);
    if (SocketBase == NULL) {
        printf("no bsdsocket.library\n");
        return 20;
    }
    if (st_skip_program(PROGRAM, ST_ANY)) {
        CloseLibrary(SocketBase);
        return 0;
    }
    MiamiBase = OpenLibrary("miami.library", 0);
    if (MiamiBase == NULL)
        printf("%s: no miami.library - the SYN cache's counters are not checked\n", PROGRAM);
    if (CU_initialize_registry() != CUE_SUCCESS) {
        if (MiamiBase)
            CloseLibrary(MiamiBase);
        CloseLibrary(SocketBase);
        return CU_get_error();
    }
    p = CU_add_suite("SYNALLOC_Suite", init_suite, clean_suite);
    if (p == NULL ||
        !st_add_test(p, "a port that does not listen leaves the SYN cache alone", testSYNALLOC_NOT_LISTENING, ST_ANY) ||
        !st_add_test(p, "listen and accept (IPv4)", testSYNALLOC_ACCEPT4, ST_V4) ||
        !st_add_test(p, "listen and accept (IPv6)", testSYNALLOC_ACCEPT6, ST_V6) ||
        !st_add_test(p, "listen after every listener was closed", testSYNALLOC_RELISTEN, ST_ANY) ||
        !st_add_test(p, "listen twice on one socket", testSYNALLOC_LISTEN_TWICE, ST_ANY))
    {
        CU_cleanup_registry();
        if (MiamiBase)
            CloseLibrary(MiamiBase);
        CloseLibrary(SocketBase);
        return CU_get_error();
    }
    CU_basic_set_mode(CU_BRM_VERBOSE);
    CU_basic_run_tests();
    CU_basic_set_mode(CU_BRM_SILENT);
    CU_automated_package_name_set("NetUnitTests");
    CU_set_output_filename("Net-SYNALLOC");
    CU_automated_enable_junit_xml(CU_TRUE);
    CU_automated_run_tests();
    CU_cleanup_registry();
    if (MiamiBase)
        CloseLibrary(MiamiBase);
    CloseLibrary(SocketBase);
    return CU_get_error();
}
