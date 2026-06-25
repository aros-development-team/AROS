/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    CUnit tests for bsdsocket.library UDP (IPv4) over the loopback interface:
    sendto/recvfrom, connected UDP, multiple datagrams and bound names.
*/

#include "socktest.h"

struct Library *SocketBase = NULL;

int init_suite(void) { return (SocketBase != NULL) ? 0 : -1; }
int clean_suite(void) { return 0; }

void testUDP_SENDTO(void)
{
    unsigned short nport;
    int rfd, cfd, r;
    struct sockaddr_storage dst, src;
    socklen_t len;
    char buf[32];

    rfd = st_bound(AF_INET, SOCK_DGRAM, &nport);
    CU_ASSERT(rfd >= 0);
    if (rfd < 0)
        return;
    cfd = socket(AF_INET, SOCK_DGRAM, 0);
    CU_ASSERT(cfd >= 0);
    if (cfd < 0) {
        CloseSocket(rfd);
        return;
    }
    len = st_loopback(AF_INET, &dst, nport);
    CU_ASSERT_EQUAL(sendto(cfd, "ping", 4, 0, (struct sockaddr *)&dst, len), 4);

    CU_ASSERT(sock_wait(rfd, 0, 3) > 0);
    len = sizeof(src);
    r = recvfrom(rfd, buf, sizeof(buf), 0, (struct sockaddr *)&src, &len);
    CU_ASSERT_EQUAL(r, 4);
    CU_ASSERT(r == 4 && memcmp(buf, "ping", 4) == 0);
    CU_ASSERT_EQUAL(src.ss_family, AF_INET);
    CloseSocket(cfd);
    CloseSocket(rfd);
}

void testUDP_CONNECTED(void)
{
    unsigned short nport;
    int rfd, cfd, r;
    struct sockaddr_storage dst;
    socklen_t len;
    char buf[32];

    rfd = st_bound(AF_INET, SOCK_DGRAM, &nport);
    CU_ASSERT(rfd >= 0);
    if (rfd < 0)
        return;
    cfd = socket(AF_INET, SOCK_DGRAM, 0);
    CU_ASSERT(cfd >= 0);
    if (cfd < 0) {
        CloseSocket(rfd);
        return;
    }
    len = st_loopback(AF_INET, &dst, nport);
    /* connect() on a datagram socket fixes the default destination. */
    CU_ASSERT_EQUAL(connect(cfd, (struct sockaddr *)&dst, len), 0);
    CU_ASSERT_EQUAL(send(cfd, "data", 4, 0), 4);
    CU_ASSERT(sock_wait(rfd, 0, 3) > 0);
    r = recv(rfd, buf, sizeof(buf), 0);
    CU_ASSERT_EQUAL(r, 4);
    CU_ASSERT(r == 4 && memcmp(buf, "data", 4) == 0);
    CloseSocket(cfd);
    CloseSocket(rfd);
}

void testUDP_MULTI(void)
{
    unsigned short nport;
    int rfd, cfd, i, got = 0;
    struct sockaddr_storage dst;
    socklen_t len;
    char buf[32];

    rfd = st_bound(AF_INET, SOCK_DGRAM, &nport);
    CU_ASSERT(rfd >= 0);
    if (rfd < 0)
        return;
    cfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (cfd < 0) {
        CloseSocket(rfd);
        return;
    }
    len = st_loopback(AF_INET, &dst, nport);
    for (i = 0; i < 4; i++)
        CU_ASSERT_EQUAL(sendto(cfd, "x", 1, 0,
            (struct sockaddr *)&dst, len), 1);
    for (i = 0; i < 4; i++) {
        if (sock_wait(rfd, 0, 3) <= 0)
            break;
        if (recv(rfd, buf, sizeof(buf), 0) == 1)
            got++;
    }
    CU_ASSERT_EQUAL(got, 4);
    CloseSocket(cfd);
    CloseSocket(rfd);
}

void testUDP_GETSOCKNAME(void)
{
    unsigned short nport = 0;
    int rfd = st_bound(AF_INET, SOCK_DGRAM, &nport);

    CU_ASSERT(rfd >= 0);
    if (rfd < 0)
        return;
    CU_ASSERT(nport != 0);		/* an ephemeral port was assigned */
    CloseSocket(rfd);
}

int main(void)
{
    CU_pSuite p;

    SocketBase = OpenLibrary("bsdsocket.library", 3);
    if (SocketBase == NULL) {
        printf("bsdsocket.library not available - skipping UDP tests\n");
        return 0;
    }
    if (CU_initialize_registry() != CUE_SUCCESS) {
        CloseLibrary(SocketBase);
        return CU_get_error();
    }
    p = CU_add_suite("UDP_Suite", init_suite, clean_suite);
    if (p == NULL ||
        !CU_add_test(p, "sendto/recvfrom", testUDP_SENDTO) ||
        !CU_add_test(p, "connected UDP", testUDP_CONNECTED) ||
        !CU_add_test(p, "multiple datagrams", testUDP_MULTI) ||
        !CU_add_test(p, "bind/getsockname", testUDP_GETSOCKNAME))
    {
        CU_cleanup_registry();
        CloseLibrary(SocketBase);
        return CU_get_error();
    }
    CU_basic_set_mode(CU_BRM_VERBOSE);
    CU_basic_run_tests();
    CU_basic_set_mode(CU_BRM_SILENT);
    CU_automated_package_name_set("NetUnitTests");
    CU_set_output_filename("Net-UDP");
    CU_automated_enable_junit_xml(CU_TRUE);
    CU_automated_run_tests();
    CU_cleanup_registry();
    CloseLibrary(SocketBase);
    return CU_get_error();
}
