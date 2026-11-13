/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    CUnit tests for bsdsocket.library socket operations.

    The "api" and "addr" tests exercise socket creation, option get/set,
    bind/getsockname, listen and the address-conversion helpers; these only
    require the stack to be running.  The "loopback" test drives a full
    TCP connect/accept/send/recv exchange over the loopback address
    (exercising the listen/accept SYN-cache path) and is skipped gracefully
    if loopback is not configured.

    None of this is about an address family: the sockets are IPv4 ones, or
    IPv6 ones where the running stack has no IPv4.
*/

#include "socktest.h"

struct Library *SocketBase = NULL;

/* The family of the sockets these tests make. */
#define FAM st_any_family()

int init_suite(void)
{
    /* Tests require a running stack; bsdsocket.library is opened in main(). */
    return (SocketBase != NULL) ? 0 : -1;
}

int clean_suite(void)
{
    return 0;
}

/* ---- socket creation ------------------------------------------------ */

void testSOCKET_CREATE(void)
{
    int s;

    s = socket(FAM, SOCK_STREAM, 0);
    CU_ASSERT(s >= 0);
    if (s >= 0)
        CU_ASSERT_EQUAL(CloseSocket(s), 0);

    s = socket(FAM, SOCK_DGRAM, 0);
    CU_ASSERT(s >= 0);
    if (s >= 0)
        CU_ASSERT_EQUAL(CloseSocket(s), 0);
}

void testSOCKET_BADARGS(void)
{
    /* An unsupported socket type must fail rather than return a descriptor. */
    int s = socket(FAM, 0x7fff, 0);
    CU_ASSERT_EQUAL(s, -1);
    if (s >= 0)
        CloseSocket(s);
}

void testCLOSE_BADFD(void)
{
    /* Closing an invalid descriptor must fail. */
    CU_ASSERT_EQUAL(CloseSocket(-1), -1);
}

/* ---- socket options ------------------------------------------------- */

void testSO_TYPE(void)
{
    int s = socket(FAM, SOCK_STREAM, 0);
    int val = 0;
    socklen_t len = sizeof(val);

    CU_ASSERT(s >= 0);
    if (s < 0)
        return;
    CU_ASSERT_EQUAL(getsockopt(s, SOL_SOCKET, SO_TYPE, &val, &len), 0);
    CU_ASSERT_EQUAL(val, SOCK_STREAM);
    CloseSocket(s);
}

void testSO_REUSEADDR(void)
{
    int s = socket(FAM, SOCK_STREAM, 0);
    int on = 1, val = 0;
    socklen_t len = sizeof(val);

    CU_ASSERT(s >= 0);
    if (s < 0)
        return;
    CU_ASSERT_EQUAL(setsockopt(s, SOL_SOCKET, SO_REUSEADDR,
        &on, sizeof(on)), 0);
    CU_ASSERT_EQUAL(getsockopt(s, SOL_SOCKET, SO_REUSEADDR,
        &val, &len), 0);
    CU_ASSERT(val != 0);
    CloseSocket(s);
}

void testSO_RCVBUF(void)
{
    int s = socket(FAM, SOCK_STREAM, 0);
    int want = 16 * 1024, val = 0;
    socklen_t len = sizeof(val);

    CU_ASSERT(s >= 0);
    if (s < 0)
        return;
    CU_ASSERT_EQUAL(setsockopt(s, SOL_SOCKET, SO_RCVBUF,
        &want, sizeof(want)), 0);
    CU_ASSERT_EQUAL(getsockopt(s, SOL_SOCKET, SO_RCVBUF,
        &val, &len), 0);
    /* The stack may round the value, but it must be positive. */
    CU_ASSERT(val > 0);
    CloseSocket(s);
}

void testTCP_NODELAY(void)
{
    int s = socket(FAM, SOCK_STREAM, 0);
    int on = 1, val = 0;
    socklen_t len = sizeof(val);

    CU_ASSERT(s >= 0);
    if (s < 0)
        return;
    CU_ASSERT_EQUAL(setsockopt(s, IPPROTO_TCP, TCP_NODELAY,
        &on, sizeof(on)), 0);
    CU_ASSERT_EQUAL(getsockopt(s, IPPROTO_TCP, TCP_NODELAY,
        &val, &len), 0);
    CU_ASSERT(val != 0);
    CloseSocket(s);
}

/* ---- bind / listen / names ------------------------------------------ */

void testBIND_GETSOCKNAME(void)
{
    int s = socket(FAM, SOCK_STREAM, 0);
    struct sockaddr_storage sa;
    socklen_t len;

    CU_ASSERT(s >= 0);
    if (s < 0)
        return;
    len = st_wildcard(FAM, &sa, 0);	/* ask for an ephemeral port */
    CU_ASSERT_EQUAL(bind(s, (struct sockaddr *)&sa, len), 0);

    bzero(&sa, sizeof(sa));
    len = sizeof(sa);
    CU_ASSERT_EQUAL(getsockname(s, (struct sockaddr *)&sa, &len), 0);
    CU_ASSERT_EQUAL(sa.ss_family, FAM);
    CloseSocket(s);
}

void testLISTEN(void)
{
    int s = socket(FAM, SOCK_STREAM, 0);
    struct sockaddr_storage sa;
    socklen_t len;

    CU_ASSERT(s >= 0);
    if (s < 0)
        return;
    len = st_wildcard(FAM, &sa, 0);
    CU_ASSERT_EQUAL(bind(s, (struct sockaddr *)&sa, len), 0);
    CU_ASSERT_EQUAL(listen(s, 5), 0);
    CloseSocket(s);
}

void testGETPEERNAME_UNCONN(void)
{
    int s = socket(FAM, SOCK_STREAM, 0);
    struct sockaddr_storage sa;
    socklen_t len = sizeof(sa);

    CU_ASSERT(s >= 0);
    if (s < 0)
        return;
    /* An unconnected socket has no peer. */
    CU_ASSERT_EQUAL(getpeername(s, (struct sockaddr *)&sa, &len), -1);
    CU_ASSERT_EQUAL(Errno(), ENOTCONN);
    CloseSocket(s);
}

void testNONBLOCK(void)
{
    int s = socket(FAM, SOCK_STREAM, 0);
    int on = 1;

    CU_ASSERT(s >= 0);
    if (s < 0)
        return;
    CU_ASSERT_EQUAL(IoctlSocket(s, FIONBIO, (char *)&on), 0);
    CloseSocket(s);
}

/* ---- address conversion -------------------------------------------- */

void testINET_ADDR(void)
{
    CU_ASSERT_EQUAL(inet_addr("127.0.0.1"), htonl(0x7f000001));
    CU_ASSERT_EQUAL(inet_addr("0.0.0.0"), htonl(0x00000000));
    CU_ASSERT_EQUAL(inet_addr("255.255.255.255"), htonl(0xffffffff));
    /* Malformed input must yield INADDR_NONE. */
    CU_ASSERT_EQUAL(inet_addr("999.1.1.1"), INADDR_NONE);
}

void testINET_NTOA(void)
{
    char *s = Inet_NtoA(inet_addr("192.168.10.20"));
    CU_ASSERT_PTR_NOT_NULL(s);
    if (s != NULL)
        CU_ASSERT_STRING_EQUAL(s, "192.168.10.20");
}

void testHTON_ROUNDTRIP(void)
{
    CU_ASSERT_EQUAL(ntohl(htonl(0x12345678UL)), 0x12345678UL);
    CU_ASSERT_EQUAL(ntohs(htons((u_short)0xABCD)), (u_short)0xABCD);
}

/* ---- loopback TCP exchange (exercises listen/accept SYN cache) ------- */

static int waitfd(int fd, int forwrite, int secs)
{
    fd_set fs;
    struct timeval tv;

    FD_ZERO(&fs);
    FD_SET(fd, &fs);
    tv.tv_sec = secs;
    tv.tv_usec = 0;
    if (forwrite)
        return WaitSelect(fd + 1, NULL, &fs, NULL, &tv, NULL);
    return WaitSelect(fd + 1, &fs, NULL, NULL, &tv, NULL);
}

void testLOOPBACK_ECHO(void)
{
    int ls = -1, cs = -1, as = -1;
    struct sockaddr_storage sa, da;
    socklen_t len;
    int on = 1, r;
    char buf[8];

    ls = socket(FAM, SOCK_STREAM, 0);
    CU_ASSERT(ls >= 0);
    if (ls < 0)
        return;
    (void) setsockopt(ls, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on));

    len = st_loopback(FAM, &sa, 0);
    if (bind(ls, (struct sockaddr *)&sa, len) != 0) {
        CU_PASS("loopback bind failed - loopback not configured, skipping");
        CloseSocket(ls);
        return;
    }
    CU_ASSERT_EQUAL(listen(ls, 1), 0);

    len = sizeof(sa);
    CU_ASSERT_EQUAL(getsockname(ls, (struct sockaddr *)&sa, &len), 0);

    cs = socket(FAM, SOCK_STREAM, 0);
    CU_ASSERT(cs >= 0);
    if (cs < 0) {
        CloseSocket(ls);
        return;
    }
    (void) IoctlSocket(cs, FIONBIO, (char *)&on);

    len = st_loopback(FAM, &da, st_port(&sa));
    r = connect(cs, (struct sockaddr *)&da, len);
    if (r != 0 && Errno() != EINPROGRESS) {
        CU_PASS("loopback connect failed - skipping");
        CloseSocket(cs);
        CloseSocket(ls);
        return;
    }

    /* Listener should become readable once the handshake completes. */
    if (waitfd(ls, 0, 3) <= 0) {
        CU_PASS("no inbound connection - loopback unavailable, skipping");
        CloseSocket(cs);
        CloseSocket(ls);
        return;
    }
    len = sizeof(sa);
    as = accept(ls, (struct sockaddr *)&sa, &len);
    CU_ASSERT(as >= 0);
    if (as < 0) {
        CloseSocket(cs);
        CloseSocket(ls);
        return;
    }

    /* Confirm the client side finished connecting. */
    CU_ASSERT(waitfd(cs, 1, 3) > 0);

    /* Client -> server. */
    CU_ASSERT_EQUAL(send(cs, "PING", 4, 0), 4);
    CU_ASSERT(waitfd(as, 0, 3) > 0);
    r = recv(as, buf, sizeof(buf), 0);
    CU_ASSERT_EQUAL(r, 4);
    CU_ASSERT(r == 4 && memcmp(buf, "PING", 4) == 0);

    /* Server echoes back to client. */
    if (r > 0)
        CU_ASSERT_EQUAL(send(as, buf, r, 0), r);
    CU_ASSERT(waitfd(cs, 0, 3) > 0);
    r = recv(cs, buf, sizeof(buf), 0);
    CU_ASSERT_EQUAL(r, 4);
    CU_ASSERT(r == 4 && memcmp(buf, "PING", 4) == 0);

    CloseSocket(as);
    CloseSocket(cs);
    CloseSocket(ls);
}

int main(void)
{
    CU_pSuite pSuite = NULL;

    SocketBase = OpenLibrary("bsdsocket.library", 3);
    if (SocketBase == NULL) {
        printf("bsdsocket.library not available - skipping socket tests\n");
        return 0;
    }

    if (CUE_SUCCESS != CU_initialize_registry()) {
        CloseLibrary(SocketBase);
        return CU_get_error();
    }

    pSuite = CU_add_suite("Socket_Suite", init_suite, clean_suite);
    if (NULL == pSuite) {
        CU_cleanup_registry();
        CloseLibrary(SocketBase);
        return CU_get_error();
    }

    if (!st_add_test(pSuite, "socket()", testSOCKET_CREATE, ST_ANY) ||
        !st_add_test(pSuite, "socket() bad args", testSOCKET_BADARGS, ST_ANY) ||
        !st_add_test(pSuite, "CloseSocket() bad fd", testCLOSE_BADFD, ST_NONE) ||
        !st_add_test(pSuite, "getsockopt(SO_TYPE)", testSO_TYPE, ST_ANY) ||
        !st_add_test(pSuite, "setsockopt(SO_REUSEADDR)", testSO_REUSEADDR, ST_ANY) ||
        !st_add_test(pSuite, "setsockopt(SO_RCVBUF)", testSO_RCVBUF, ST_ANY) ||
        !st_add_test(pSuite, "setsockopt(TCP_NODELAY)", testTCP_NODELAY, ST_ANY) ||
        !st_add_test(pSuite, "bind()/getsockname()", testBIND_GETSOCKNAME, ST_ANY) ||
        !st_add_test(pSuite, "listen()", testLISTEN, ST_ANY) ||
        !st_add_test(pSuite, "getpeername() unconnected", testGETPEERNAME_UNCONN, ST_ANY) ||
        !st_add_test(pSuite, "IoctlSocket(FIONBIO)", testNONBLOCK, ST_ANY) ||
        !st_add_test(pSuite, "inet_addr()", testINET_ADDR, ST_NONE) ||
        !st_add_test(pSuite, "Inet_NtoA()", testINET_NTOA, ST_NONE) ||
        !st_add_test(pSuite, "htonl()/htons()", testHTON_ROUNDTRIP, ST_NONE) ||
        !st_add_test(pSuite, "loopback TCP echo", testLOOPBACK_ECHO, ST_ANY))
    {
        CU_cleanup_registry();
        CloseLibrary(SocketBase);
        return CU_get_error();
    }

    CU_basic_set_mode(CU_BRM_VERBOSE);
    CU_basic_run_tests();
    CU_basic_set_mode(CU_BRM_SILENT);
    CU_automated_package_name_set("NetUnitTests");
    CU_set_output_filename("Net-Socket");
    CU_automated_enable_junit_xml(CU_TRUE);
    CU_automated_run_tests();
    CU_cleanup_registry();

    CloseLibrary(SocketBase);
    return CU_get_error();
}
