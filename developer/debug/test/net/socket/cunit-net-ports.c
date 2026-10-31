/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    CUnit tests for ephemeral port selection (RFC 6056), over IPv4 and
    IPv6: a port the stack picks - binding to port 0, or the implicit bind
    of connect() and sendto() - is in the IANA dynamic range 49152-65535
    (RFC 6335 section 6), and successive ones do not follow each other, so
    the next one cannot be guessed. Runs over the loopback interface.
*/

#include "socktest.h"

struct Library *SocketBase = NULL;

#define FIRST   49152
#define LAST    65535
#define N       16

static int in_range(unsigned port)
{
    return port >= FIRST && port <= LAST;
}

/* The local port of s, in host byte order (0 on error). */
static unsigned local_port(int s)
{
    struct sockaddr_storage ss;
    socklen_t len = sizeof(ss);

    if (getsockname(s, (struct sockaddr *)&ss, &len) != 0)
        return 0;
    return ntohs(st_port(&ss));
}

/* Bind N sockets of type to port 0 on the loopback address (or the
 * unspecified one); ports[] gets their ports. Returns how many were
 * bound. */
static int bind_many(int family, int type, int loopback, unsigned *ports)
{
    int s[N], i, n = 0;

    for (i = 0; i < N; i++) {
        struct sockaddr_storage ss;
        socklen_t len = st_loopback(family, &ss, 0);

        s[i] = socket(family, type, 0);
        if (s[i] < 0)
            break;
        if (!loopback) {
            if (family == AF_INET6)
                bzero(&((struct sockaddr_in6 *)&ss)->sin6_addr, sizeof(struct in6_addr));
            else
                ((struct sockaddr_in *)&ss)->sin_addr.s_addr = htonl(INADDR_ANY);
        }
        if (bind(s[i], (struct sockaddr *)&ss, len) != 0) {
            CloseSocket(s[i]);
            break;
        }
        ports[n++] = local_port(s[i]);
    }
    for (i = 0; i < n; i++)
        CloseSocket(s[i]);
    return n;
}

/* All in range, and not a run: fewer than half the steps are +1. */
static void check_ports(const unsigned *ports, int n)
{
    int i, ok = 1, steps = 0;

    for (i = 0; i < n; i++)
        if (!in_range(ports[i]))
            ok = 0;
    CU_ASSERT(ok);
    for (i = 1; i < n; i++)
        if (ports[i] == ports[i - 1] + 1)
            steps++;
    CU_ASSERT(steps < (n - 1) / 2);
}

static void bind_test(int family, int type, int loopback)
{
    unsigned ports[N];

    CU_ASSERT_EQUAL_FATAL(bind_many(family, type, loopback, ports), N);
    check_ports(ports, N);
}

/* ----------------------------------------------------------------- tests */

int init_suite(void) { return (SocketBase != NULL) ? 0 : -1; }
int clean_suite(void) { return 0; }

void testPORTS_TCP_BIND(void)     { bind_test(AF_INET, SOCK_STREAM, 0); }
void testPORTS_UDP_BIND(void)     { bind_test(AF_INET, SOCK_DGRAM, 0); }
void testPORTS_TCP6_BIND(void)    { bind_test(AF_INET6, SOCK_STREAM, 0); }
void testPORTS_UDP6_BIND(void)    { bind_test(AF_INET6, SOCK_DGRAM, 0); }

/* Port 0 with an explicit address picks a port too. */
void testPORTS_ADDR_BIND(void)    { bind_test(AF_INET, SOCK_DGRAM, 1); }
void testPORTS_ADDR6_BIND(void)   { bind_test(AF_INET6, SOCK_DGRAM, 1); }

/* The implicit bind of connect(): the client ends of N connections. */
static void connect_test(int family)
{
    unsigned ports[N];
    int i, n = 0;

    for (i = 0; i < N; i++) {
        int c, s;

        if (st_tcp_pair(family, &c, &s) != 0)
            break;
        ports[n++] = local_port(c);
        CloseSocket(c);
        CloseSocket(s);
    }
    CU_ASSERT_EQUAL_FATAL(n, N);
    check_ports(ports, N);
}

void testPORTS_CONNECT(void)      { connect_test(AF_INET); }
void testPORTS_CONNECT6(void)     { connect_test(AF_INET6); }

/* The implicit bind of sendto() on an unbound UDP socket. */
static void sendto_test(int family)
{
    unsigned short nport;
    unsigned ports[N];
    int r, i, n = 0;

    r = st_bound(family, SOCK_DGRAM, &nport);
    CU_ASSERT_FATAL(r >= 0);
    for (i = 0; i < N; i++) {
        struct sockaddr_storage ss;
        socklen_t len = st_loopback(family, &ss, nport);
        int s = socket(family, SOCK_DGRAM, 0);

        if (s < 0)
            break;
        if (sendto(s, "x", 1, 0, (struct sockaddr *)&ss, len) == 1)
            ports[n++] = local_port(s);
        CloseSocket(s);
    }
    CloseSocket(r);
    CU_ASSERT_EQUAL_FATAL(n, N);
    check_ports(ports, N);
}

void testPORTS_SENDTO(void)       { sendto_test(AF_INET); }
void testPORTS_SENDTO6(void)      { sendto_test(AF_INET6); }

int main(void)
{
    CU_pSuite p;

    SocketBase = OpenLibrary("bsdsocket.library", 3);
    if (SocketBase == NULL) {
        printf("bsdsocket.library not available - skipping PORTS tests\n");
        return 0;
    }
    if (CU_initialize_registry() != CUE_SUCCESS) {
        CloseLibrary(SocketBase);
        return CU_get_error();
    }
    p = CU_add_suite("PORTS_Suite", init_suite, clean_suite);
    if (p == NULL ||
        !CU_add_test(p, "TCP bind to port 0", testPORTS_TCP_BIND) ||
        !CU_add_test(p, "UDP bind to port 0", testPORTS_UDP_BIND) ||
        !CU_add_test(p, "TCP6 bind to port 0", testPORTS_TCP6_BIND) ||
        !CU_add_test(p, "UDP6 bind to port 0", testPORTS_UDP6_BIND) ||
        !CU_add_test(p, "bind to an address and port 0", testPORTS_ADDR_BIND) ||
        !CU_add_test(p, "bind to an IPv6 address and port 0", testPORTS_ADDR6_BIND) ||
        !CU_add_test(p, "connect() without bind", testPORTS_CONNECT) ||
        !CU_add_test(p, "connect() without bind (IPv6)", testPORTS_CONNECT6) ||
        !CU_add_test(p, "sendto() without bind", testPORTS_SENDTO) ||
        !CU_add_test(p, "sendto() without bind (IPv6)", testPORTS_SENDTO6))
    {
        CU_cleanup_registry();
        CloseLibrary(SocketBase);
        return CU_get_error();
    }
    CU_basic_set_mode(CU_BRM_VERBOSE);
    CU_basic_run_tests();
    CU_basic_set_mode(CU_BRM_SILENT);
    CU_automated_package_name_set("NetUnitTests");
    CU_set_output_filename("Net-PORTS");
    CU_automated_enable_junit_xml(CU_TRUE);
    CU_automated_run_tests();
    CU_cleanup_registry();
    CloseLibrary(SocketBase);
    return CU_get_error();
}
