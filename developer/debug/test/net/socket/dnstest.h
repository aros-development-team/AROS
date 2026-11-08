/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    A small DNS server for the resolver's cunit tests: the test program
    runs itself again with --serve (dns_start()), and that copy answers
    queries to port 53 on every address, IPv4 and IPv6, from the table
    below - an A record for a name in it, "no such name" for any other,
    and "quit.dns.test" stops it. Include after socktest.h.
*/

#ifndef NET_DNSTEST_H
#define NET_DNSTEST_H

#include <proto/dos.h>
#include <dos/dostags.h>
#include <netdb.h>

#define DNS_PORT    53

/* 192.0.2.last, with a TTL of 0 - the resolver's cache keeps none - but
 * for the names that test it. */
static const struct { const char *name; unsigned char last; unsigned long ttl; } dns_records[] = {
    { "one.rdnss.test", 77 },
    { "two.rdnss.test", 78 },
    { "host.b.search.test", 79 },
    { "www.search.test", 80 },
    { "www.search.test.a.search.test", 81 },
    { "x.y.b.search.test", 82 },
    { "cached.rdnss.test", 83, 600 },
    { "short.rdnss.test", 84, 3 },
};

/* The question's name, dotted, into name; the offset after it, 0 if bad. */
static int dns_qname(const unsigned char *q, int len, char *name, int size)
{
    int p = 12, n = 0;

    while (p < len && q[p] != 0) {
        int l = q[p++];

        if (l > 63 || p + l > len || n + l + 2 > size)
            return 0;
        if (n > 0)
            name[n++] = '.';
        memcpy(name + n, q + p, l);
        n += l;
        p += l;
    }
    name[n] = '\0';
    return p < len ? p + 1 : 0;
}

/* Answer one query on s; 0 once asked to quit. */
static int dns_answer(int s)
{
    struct sockaddr_storage from;
    unsigned char q[512];
    char name[256];
    socklen_t fl = sizeof(from);
    int n, end, i;
    const int count = sizeof(dns_records) / sizeof(dns_records[0]);

    n = recvfrom(s, (char *)q, sizeof(q) - 16, 0, (struct sockaddr *)&from, &fl);
    if (n < 12 || (end = dns_qname(q, n, name, sizeof(name))) == 0 || end + 4 > n)
        return 1;
    if (strcasecmp(name, "quit.dns.test") == 0)
        return 0;
    n = end + 4;                                /* the question, QTYPE, QCLASS */
    q[2] = 0x84 | (q[2] & 0x01);                /* QR, AA, RD as asked */
    q[3] = 0x80 | 3;                            /* RA, "no such name" */
    q[4] = 0; q[5] = 1;                         /* QDCOUNT */
    q[6] = q[7] = q[8] = q[9] = q[10] = q[11] = 0;
    for (i = 0; i < count; i++)
        if (strcasecmp(name, dns_records[i].name) == 0)
            break;
    if (i < count) {
        q[3] = 0x80;                            /* no error */
        if (q[end] == 0 && q[end + 1] == 1) {   /* type A: 192.0.2.last */
            unsigned char rr[] = {
                0xc0, 12, 0, 1, 0, 1, 0, 0, 0, 0, 0, 4, 192, 0, 2, 0
            };
            unsigned long ttl = dns_records[i].ttl;

            rr[6] = ttl >> 24; rr[7] = ttl >> 16; rr[8] = ttl >> 8; rr[9] = ttl;
            rr[15] = dns_records[i].last;
            q[7] = 1;                           /* ANCOUNT */
            memcpy(q + n, rr, sizeof(rr));
            n += sizeof(rr);
        }
    }
    sendto(s, (char *)q, n, 0, (struct sockaddr *)&from, fl);
    return 1;
}

static int dns_socket(int family)
{
    struct sockaddr_storage a;
    int s = socket(family, SOCK_DGRAM, 0), on = 1;

    bzero(&a, sizeof(a));
    if (family == AF_INET6) {
        struct sockaddr_in6 *a6 = (struct sockaddr_in6 *)&a;

        a6->sin6_len = sizeof(*a6);
        a6->sin6_family = AF_INET6;
        a6->sin6_port = htons(DNS_PORT);
        if (s >= 0)
            setsockopt(s, IPPROTO_IPV6, IPV6_V6ONLY, &on, sizeof(on));
    } else {
        struct sockaddr_in *a4 = (struct sockaddr_in *)&a;

        a4->sin_len = sizeof(*a4);
        a4->sin_family = AF_INET;
        a4->sin_port = htons(DNS_PORT);
    }
    if (s >= 0 && bind(s, (struct sockaddr *)&a, a.ss_len) != 0) {
        CloseSocket(s);
        s = -1;
    }
    return s;
}

/* The server (--serve): until asked to quit, or two minutes of nothing. */
static int dns_serve(void)
{
    int s4, s6, idle = 0, on = 1;

    SocketBase = OpenLibrary("bsdsocket.library", 3);
    if (SocketBase == NULL)
        return 20;
    s4 = dns_socket(AF_INET);
    s6 = dns_socket(AF_INET6);
    while (on && s4 >= 0 && s6 >= 0 && idle < 120) {
        fd_set fs;
        struct timeval tv = { 1, 0 };

        FD_ZERO(&fs);
        FD_SET(s4, &fs);
        FD_SET(s6, &fs);
        if (WaitSelect((s4 > s6 ? s4 : s6) + 1, &fs, NULL, NULL, &tv, NULL) <= 0) {
            idle++;
            continue;
        }
        idle = 0;
        if (FD_ISSET(s4, &fs))
            on = dns_answer(s4);
        if (on && FD_ISSET(s6, &fs))
            on = dns_answer(s6);
    }
    if (s4 >= 0)
        CloseSocket(s4);
    if (s6 >= 0)
        CloseSocket(s6);
    CloseLibrary(SocketBase);
    return 0;
}

/* One query straight to the server at ::1, for name: whether it answers. */
static int dns_ask(const char *name)
{
    unsigned char q[96] = { 0x12, 0x34, 0x01, 0, 0, 1, 0, 0, 0, 0, 0, 0 }, r[512];
    struct sockaddr_storage a;
    int s, n = 12, ok = 0;
    const char *p = name;

    while (*p && n < (int)sizeof(q) - 70) {
        const char *dot = strchr(p, '.');
        int l = dot ? dot - p : (int)strnlen(p, 63);   /* a label is 63 octets at most */

        q[n++] = l;
        memcpy(q + n, p, l);
        n += l;
        p += l + (dot != NULL);
    }
    q[n++] = 0;
    q[n++] = 0; q[n++] = 1; q[n++] = 0; q[n++] = 1;
    s = socket(AF_INET6, SOCK_DGRAM, 0);
    if (s < 0)
        return 0;
    st_loopback(AF_INET6, &a, htons(DNS_PORT));
    if (sendto(s, (char *)q, n, 0, (struct sockaddr *)&a, a.ss_len) == n &&
            sock_wait(s, 0, 1) > 0 && recv(s, (char *)r, sizeof(r), 0) >= 12)
        ok = 1;
    CloseSocket(s);
    return ok;
}

/* Start the server, this program again with --serve: whether it answers. */
static int dns_start(const char *argv0)
{
    char cmd[300];
    BPTR in, out;
    int i;

    cmd[0] = '"';
    if (!NameFromLock(GetProgramDir(), cmd + 1, sizeof(cmd) - 20) ||
            !AddPart(cmd + 1, FilePart((STRPTR)argv0), sizeof(cmd) - 20))
        return 0;
    strcat(cmd, "\" --serve");
    in = Open("NIL:", MODE_OLDFILE);
    out = Open("NIL:", MODE_NEWFILE);
    if (SystemTags(cmd, SYS_Input, in, SYS_Output, out, SYS_Asynch, TRUE, TAG_DONE) != 0) {
        Close(in);
        Close(out);
        return 0;
    }
    for (i = 0; i < 5; i++)
        if (dns_ask("ping.dns.test"))
            return 1;
    return 0;
}

static void dns_stop(void)
{
    dns_ask("quit.dns.test");
}

/* What gethostbyname() gives for name: the last octet of its first IPv4
 * address, 0 if it finds none. */
static int dns_lookup(const char *name)
{
    struct hostent *h = gethostbyname((char *)name);

    if (h == NULL || h->h_addrtype != AF_INET || h->h_addr_list[0] == NULL)
        return 0;
    return (unsigned char)h->h_addr_list[0][3];
}

/* Whether a server the resolver already has answers before ours would:
 * "no such name" for a name only ours knows. */
static int dns_other_server(void)
{
    return dns_lookup("one.rdnss.test") == 0 && h_errno == HOST_NOT_FOUND;
}

#endif /* NET_DNSTEST_H */
