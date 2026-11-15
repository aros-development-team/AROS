/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    CUnit tests for the resolver as a module loaded on first use
    (resolver.atcp, module "resolver"; bsdsocket/api/
    resolver_ops.h in the stack):

     - numeric addresses and the localhost names asked of gethostbyname(),
       gethostbyname_r() and miami.library's gethostbyname2() are answered
       without it, and so are freeaddrinfo(), gai_strerror() and the
       resolver options;
     - the first call that needs it has it started, its library loaded,
       once; a name is then resolved (the DNS server of dnstest.h);
     - getaddrinfo() and getnameinfo() give the same through
       bsdsocket.library and through miami.library (they are one
       implementation now), and what they always gave;
     - without it (ENV:AROSTCP/Without_resolver, as on an installation
       without the file) every call answers as the design's table says:
       a name fails with h_errno NO_RECOVERY or EAI_FAIL and errno ENOENT,
       numeric addresses work, the services database works;
     - it is not unloaded while a task waits inside it for a name server,
       and is after; the next look-up loads it again;
     - a list from getaddrinfo() stays valid, and is freed, after the
       library is unloaded.

    Where the stack has the resolver as part of its program (a monolithic
    build) "unloaded" is "stopped" and the library counts are not asked.
    Some tests need a second task: this program, started again with an
    argument (--serve, --silent, --lookup).
    Usage:  cunit-net-resolvermod
*/

#define __CONFIG_ROADSHOW__     /* gethostbyname_r(), getaddrinfo(), AddDomainNameServer() */
#include "socktest.h"
#include "dnstest.h"

#include <proto/dos.h>
#include <aros/libcall.h>
#include <exec/ports.h>
#include <exec/execbase.h>
#include <exec/memory.h>
#include <dos/dostags.h>
#include <dos/var.h>
#include <bsdsocket/netcontrol.h>

#include <netdb.h>

struct Library *SocketBase = NULL;
struct Library *MiamiBase = NULL;

#define PROGRAM         "cunit-net-resolvermod"
#define RES_LIBRARY     "resolver.atcp"
#define RES_MODULE      "resolver"
#define RES_SWITCH      "AROSTCP/Without_resolver"

#define HELPER_PORT     "cunit-net-resolvermod.helper"
#define DRIFT_MAX       4096
#define CYCLES          10

/* ---- miami.library's calls, by vector (its header clashes with bsdsocket's) */

static struct hostent *m_gethostbyname2(const char *name, LONG family)
{
    return AROS_LC2(struct hostent *, gethostbyname2,
                    AROS_LCA(const char *, name, A0),
                    AROS_LCA(LONG, family, D0),
                    struct Library *, MiamiBase, 41, Miami);
}

static char *m_gai_strerror(LONG error)
{
    return AROS_LC1(char *, gai_strerror,
                    AROS_LCA(LONG, error, D0),
                    struct Library *, MiamiBase, 42, Miami);
}

static void m_freeaddrinfo(struct addrinfo *ai)
{
    AROS_LC1(void, freeaddrinfo,
             AROS_LCA(struct addrinfo *, ai, A0),
             struct Library *, MiamiBase, 43, Miami);
}

static LONG m_getaddrinfo(const char *host, const char *serv, struct addrinfo *hints,
                          struct addrinfo **res)
{
    return AROS_LC4(LONG, getaddrinfo,
                    AROS_LCA(const char *, host, A0),
                    AROS_LCA(const char *, serv, A1),
                    AROS_LCA(struct addrinfo *, hints, A2),
                    AROS_LCA(struct addrinfo **, res, A3),
                    struct Library *, MiamiBase, 44, Miami);
}

static LONG m_getnameinfo(struct sockaddr *sa, LONG salen, char *host, LONG hostlen,
                          char *serv, LONG servlen, LONG flags)
{
    return AROS_LC7(LONG, getnameinfo,
                    AROS_LCA(struct sockaddr *, sa, A0),
                    AROS_LCA(LONG, salen, D0),
                    AROS_LCA(char *, host, A1),
                    AROS_LCA(LONG, hostlen, D1),
                    AROS_LCA(char *, serv, A2),
                    AROS_LCA(LONG, servlen, D2),
                    AROS_LCA(LONG, flags, D3),
                    struct Library *, MiamiBase, 45, Miami);
}

static LONG m_getresoptions(void)
{
    return AROS_LC0(LONG, MiamiGetResOptions, struct Library *, MiamiBase, 51, Miami);
}

static void m_setresoptions(LONG options)
{
    AROS_LC1(void, MiamiSetResOptions,
             AROS_LCA(LONG, options, D0),
             struct Library *, MiamiBase, 52, Miami);
}

/* ---- the control port, the module's state ------------------------------- */

static int builtin;             /* the resolver is part of the stack program */
static int installed;           /* it can be had at all (file there, or built in) */
static int ready;               /* the DNS server of dnstest.h answers for the resolver */
static int server;              /* ... and was started by this program */
static const char *argv0;       /* how dns_start() runs this program again */
static LONG added = -1;         /* what AddDomainNameServer() said */
static const char *server_addr = "127.0.0.1";
static char progpath[256];
static struct MsgPort *hport = NULL;

/* One module command (or RELOAD with name NULL); the stack's answer, -1 without a control port. */
static LONG module_cmd(ULONG cmd, const char *name, ULONG *flags)
{
    struct NetControlModuleMsg mm;
    struct MsgPort *reply, *port;

    if ((reply = CreateMsgPort()) == NULL)
        return -1;
    memset(&mm, 0, sizeof(mm));
    mm.ncmm_Msg.ncm_Msg.mn_ReplyPort = reply;
    mm.ncmm_Msg.ncm_Msg.mn_Length = name ? sizeof(mm) : sizeof(mm.ncmm_Msg);
    mm.ncmm_Msg.ncm_Command = cmd;
    mm.ncmm_Msg.ncm_Result = -1;
    if (name)
        strncpy(mm.ncmm_Name, name, sizeof(mm.ncmm_Name) - 1);

    Forbid();
    if ((port = FindPort(AROSTCP_CTRLPORT_NAME)) != NULL)
        PutMsg(port, &mm.ncmm_Msg.ncm_Msg);
    Permit();
    if (port != NULL) {
        WaitPort(reply);
        GetMsg(reply);
    }
    DeleteMsgPort(reply);
    if (flags) *flags = mm.ncmm_Flags;
    return port ? mm.ncmm_Msg.ncm_Result : -1;
}

/* exec's open count of a library in memory, -1 when it is not there */
static LONG lib_opencount(const char *name)
{
    struct Library *lib;
    LONG count = -1;

    Forbid();
    if ((lib = (struct Library *)FindName(&SysBase->LibList, (STRPTR)name)) != NULL)
        count = lib->lib_OpenCnt;
    Permit();
    return count;
}

/* whether the resolver runs */
static int res_running(void)
{
    ULONG flags = 0;

    return module_cmd(NCMD_MODULE_QUERY, RES_MODULE, &flags) == 0 && (flags & NCMMF_STARTED);
}

/* Out of the way: unloaded (stopped, when it is part of the program). The stack's answer. */
static LONG res_out(void)
{
    if (builtin)
        return module_cmd(NCMD_MODULE_STOP, RES_MODULE, NULL);
    return module_cmd(NCMD_MODULE_UNLOAD, RES_MODULE, NULL);
}

/* Not running, not loaded: whatever the program and the file. */
static int res_gone(void)
{
    return !res_running() && (builtin || lib_opencount(RES_LIBRARY) < 0);
}

/* Not running, the test switch not set, nothing remembered against it. */
static void res_reset(void)
{
    DeleteVar(RES_SWITCH, GVF_GLOBAL_ONLY);
    if (!builtin && installed && lib_opencount(RES_LIBRARY) < 0)
        module_cmd(NCMD_MODULE_LOAD, RES_LIBRARY, NULL);  /* forgets that it could not be had */
    res_out();
}

/* A call that always needs the resolver and no name server: getnameinfo() of a number. */
static LONG numeric_nameinfo(char *host, char *serv)
{
    struct sockaddr_in sin;

    bzero(&sin, sizeof(sin));
    sin.sin_len = sizeof(sin);
    sin.sin_family = AF_INET;
    sin.sin_port = htons(80);
    sin.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    return getnameinfo((struct sockaddr *)&sin, sizeof(sin), host, 64, serv, 16,
                       NI_NUMERICHOST | NI_NUMERICSERV);
}

static int is_addr4(struct hostent *h, unsigned long addr)
{
    return h != NULL && h->h_addrtype == AF_INET && h->h_length == 4 &&
           h->h_addr_list[0] != NULL &&
           memcmp(h->h_addr_list[0], &addr, 4) == 0 && h->h_addr_list[1] == NULL;
}

/* ---- the second task ----------------------------------------------------- */

#define HK_READY    1   /* the silent server is listening */
#define HK_ASKED    2   /* ... and has a question it does not answer yet */
#define HK_DONE     3   /* the look-up returned */

struct hmsg {
    struct Message  hm_Msg;
    LONG            hm_Kind;
    LONG            hm_Result;  /* the look-up: 1 found, 0 not */
};

static BOOL to_parent(struct hmsg *m, struct MsgPort *reply, LONG kind)
{
    struct MsgPort *port;

    m->hm_Msg.mn_ReplyPort = reply;
    m->hm_Msg.mn_Length = sizeof(*m);
    m->hm_Kind = kind;
    Forbid();
    if ((port = FindPort(HELPER_PORT)) != NULL)
        PutMsg(port, &m->hm_Msg);
    Permit();
    if (port == NULL)
        return FALSE;
    WaitPort(reply);
    GetMsg(reply);
    return TRUE;
}

/*
 * --silent: a name server that takes one question, says so, waits for the
 * parent's reply - the asking task is inside the resolver all that time -
 * then answers it "no such name" and leaves.
 */
static int silent_server(void)
{
    struct MsgPort *reply;
    struct sockaddr_storage a, from;
    struct hmsg m;
    unsigned char q[512];
    socklen_t fl = sizeof(from);
    int s = -1, n = 0, family;

    if ((reply = CreateMsgPort()) == NULL)
        return 20;
    if ((SocketBase = OpenLibrary("bsdsocket.library", 4)) != NULL) {
        family = strchr(server_addr, ':') ? AF_INET6 : AF_INET;
        if (!st_have_family(AF_INET))
            family = AF_INET6;
        s = socket(family, SOCK_DGRAM, 0);
        if (s >= 0 && bind(s, (struct sockaddr *)&a, st_wildcard(family, &a, htons(DNS_PORT))) != 0) {
            CloseSocket(s);
            s = -1;
        }
    }
    memset(&m, 0, sizeof(m));
    if (s >= 0 && to_parent(&m, reply, HK_READY) && sock_wait(s, 0, 30) > 0 &&
            (n = recvfrom(s, (char *)q, sizeof(q), 0, (struct sockaddr *)&from, &fl)) >= 12) {
        to_parent(&m, reply, HK_ASKED);
        q[2] = 0x84 | (q[2] & 0x01);            /* QR, AA, RD as asked */
        q[3] = 0x80 | 3;                        /* RA, "no such name" */
        q[6] = q[7] = q[8] = q[9] = q[10] = q[11] = 0;
        sendto(s, (char *)q, n, 0, (struct sockaddr *)&from, fl);
    }
    if (s >= 0)
        CloseSocket(s);
    if (SocketBase)
        CloseLibrary(SocketBase);
    DeleteMsgPort(reply);
    return 0;
}

/* --lookup: one name, as an application would; says when it is back. */
static int lookup_task(void)
{
    struct MsgPort *reply;
    struct hmsg m;

    if ((reply = CreateMsgPort()) == NULL)
        return 20;
    memset(&m, 0, sizeof(m));
    if ((SocketBase = OpenLibrary("bsdsocket.library", 4)) != NULL) {
        m.hm_Result = gethostbyname("wait.dns.test") != NULL;
        CloseLibrary(SocketBase);
    }
    to_parent(&m, reply, HK_DONE);
    DeleteMsgPort(reply);
    return 0;
}

static int spawn(const char *args)
{
    char cmd[400];
    BPTR in = Open("NIL:", MODE_OLDFILE), out = Open("NIL:", MODE_NEWFILE);

    snprintf(cmd, sizeof(cmd), "\"%s\" %s", progpath, args);
    if (SystemTags(cmd, SYS_Input, in, SYS_Output, out, SYS_Asynch, TRUE, TAG_DONE) != 0) {
        Close(in);
        Close(out);
        return -1;
    }
    return 0;
}

static struct MsgPort *helper_port(void)
{
    struct MsgPort *port = CreateMsgPort();

    if (port != NULL) {
        port->mp_Node.ln_Name = HELPER_PORT;
        port->mp_Node.ln_Pri = 0;
        AddPort(port);
    }
    return port;
}

static void helper_port_end(struct MsgPort *port)
{
    struct Message *m;

    if (port == NULL)
        return;
    RemPort(port);
    while ((m = GetMsg(port)) != NULL)
        ReplyMsg(m);
    DeleteMsgPort(port);
}

/* A helper's next message of the kind (others are answered and dropped), or NULL after secs. */
static struct hmsg *helper_msg(LONG kind, int secs, struct hmsg **other)
{
    struct hmsg *m;
    int i;

    for (i = 0; i < secs * 10; i++) {
        while ((m = (struct hmsg *)GetMsg(hport)) != NULL) {
            if (m->hm_Kind == kind)
                return m;
            if (other != NULL && *other == NULL)
                *other = m;     /* kept for the caller, who replies to it */
            else
                ReplyMsg(&m->hm_Msg);
        }
        Delay(5);
    }
    return NULL;
}

/* ---- comparing the two front-ends ---------------------------------------- */

/* Whether two lists from getaddrinfo() are the same, entry by entry. */
static int same_list(struct addrinfo *a, struct addrinfo *b)
{
    for (; a != NULL && b != NULL; a = a->ai_next, b = b->ai_next) {
        if (a->ai_flags != b->ai_flags || a->ai_family != b->ai_family ||
                a->ai_socktype != b->ai_socktype || a->ai_protocol != b->ai_protocol ||
                a->ai_addrlen != b->ai_addrlen ||
                memcmp(a->ai_addr, b->ai_addr, a->ai_addrlen) != 0)
            return 0;
        if ((a->ai_canonname == NULL) != (b->ai_canonname == NULL) ||
                (a->ai_canonname != NULL && strcmp(a->ai_canonname, b->ai_canonname) != 0))
            return 0;
    }
    return a == NULL && b == NULL;
}

/* getaddrinfo() through both libraries: whether they agree. The first's list, if any, in *res. */
static int both_getaddrinfo(const char *host, const char *serv, int family, int socktype,
                            int flags, LONG *rc, struct addrinfo **res)
{
    struct addrinfo hints, *r1 = NULL, *r2 = NULL;
    LONG e1, e2;
    int same;

    bzero(&hints, sizeof(hints));
    hints.ai_family = family;
    hints.ai_socktype = socktype;
    hints.ai_flags = flags;
    e1 = getaddrinfo(host, serv, &hints, &r1);
    e2 = m_getaddrinfo(host, serv, &hints, &r2);
    same = e1 == e2 && same_list(r1, r2);
    if (!same)
        printf("\n    getaddrinfo(%s, %s, family %d, type %d, flags 0x%x): bsdsocket %ld, miami %ld",
               host ? host : "NULL", serv ? serv : "NULL", family, socktype, flags,
               (long)e1, (long)e2);
    if (r2 != NULL)
        m_freeaddrinfo(r2);
    *rc = e1;
    if (res != NULL)
        *res = r1;
    else if (r1 != NULL)
        freeaddrinfo(r1);
    return same;
}

/* getnameinfo() through both libraries: whether they agree. */
static int both_getnameinfo(int family, const char *addr, int port, int flags, LONG *rc,
                            char *host)
{
    struct sockaddr_storage ss;
    char h1[256], h2[256], s1[32], s2[32];
    LONG e1, e2;

    st_loopback(family, &ss, htons(port));
    if (addr != NULL) {
        if (family == AF_INET6)
            inet_pton(AF_INET6, addr, &((struct sockaddr_in6 *)&ss)->sin6_addr);
        else
            inet_pton(AF_INET, addr, &((struct sockaddr_in *)&ss)->sin_addr);
    }
    h1[0] = h2[0] = s1[0] = s2[0] = '\0';
    e1 = getnameinfo((struct sockaddr *)&ss, ss.ss_len, h1, sizeof(h1), s1, sizeof(s1), flags);
    e2 = m_getnameinfo((struct sockaddr *)&ss, ss.ss_len, h2, sizeof(h2), s2, sizeof(s2), flags);
    *rc = e1;
    if (host != NULL)
        strcpy(host, h1);
    if (e1 != e2 || strcmp(h1, h2) != 0 || strcmp(s1, s2) != 0) {
        printf("\n    getnameinfo(%s port %d flags 0x%x): bsdsocket %ld \"%s\" \"%s\", miami %ld \"%s\" \"%s\"",
               addr ? addr : "loopback", port, flags, (long)e1, h1, s1, (long)e2, h2, s2);
        return 0;
    }
    return 1;
}

/* ---- the suite ----------------------------------------------------------- */

/*
 * Every run of the suite (the console run, then the XML run) starts with the
 * test's DNS server answering: testRESMOD_BUSY_REFUSED puts a silent one in
 * its place, at the same address, and the run after it needs it back.
 */
static int init_suite(void)
{
    if (SocketBase == NULL || MiamiBase == NULL)
        return -1;
    if (ready && !server && argv0 != NULL) {
        if ((server = dns_start(argv0)) == 0) {
            printf("\n    the test's DNS server did not start again - the tests of names are skipped");
            ready = 0;
        }
    }
    return 0;
}

static int clean_suite(void)
{
    res_reset();
    return 0;
}

/*
 * Numeric addresses, the localhost names, gai_strerror(), freeaddrinfo(),
 * the resolver options: answered with the resolver neither running nor
 * loaded, and it stays so.
 */
static void testRESMOD_LITERALS_WITHOUT_LOADING(void)
{
    struct hostent h, *hp;
    char buf[256];
    LONG herr = -5, opts;
    unsigned char lo6[16] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1 };

    res_reset();
    CU_ASSERT_FATAL(res_gone());

    CU_ASSERT(is_addr4(gethostbyname("127.0.0.1"), htonl(INADDR_LOOPBACK)));
    CU_ASSERT(is_addr4(gethostbyname("192.0.2.10"), htonl(0xc000020a)));
    CU_ASSERT(is_addr4(gethostbyname("localhost"), htonl(INADDR_LOOPBACK)));
    CU_ASSERT(is_addr4(m_gethostbyname2("192.0.2.11", AF_INET), htonl(0xc000020b)));
    hp = m_gethostbyname2("::1", AF_INET6);
    CU_ASSERT(hp != NULL && hp->h_addrtype == AF_INET6 && hp->h_length == 16 &&
              memcmp(hp->h_addr_list[0], lo6, 16) == 0);
    hp = m_gethostbyname2("localhost", AF_INET6);
    CU_ASSERT(hp != NULL && hp->h_addrtype == AF_INET6 && memcmp(hp->h_addr_list[0], lo6, 16) == 0);
    /* a family gethostbyname2() does not know: refused without a look-up */
    CU_ASSERT(m_gethostbyname2("192.0.2.12", 12345) == NULL);
    CU_ASSERT(is_addr4(gethostbyname_r("192.0.2.13", &h, buf, sizeof(buf), &herr), htonl(0xc000020d)));
    CU_ASSERT_EQUAL(herr, 0);
    CU_ASSERT_PTR_NULL(gethostbyname_r("192.0.2.13", &h, buf, 4, &herr));
    CU_ASSERT_EQUAL(herr, NETDB_INTERNAL);

    CU_ASSERT_STRING_EQUAL(gai_strerror(EAI_FAIL), "Non-recoverable failure in name resolution");
    CU_ASSERT_STRING_EQUAL(m_gai_strerror(EAI_NONAME), gai_strerror(EAI_NONAME));
    CU_ASSERT_STRING_EQUAL(gai_strerror(-3), "Unknown error");
    freeaddrinfo(NULL);
    m_freeaddrinfo(NULL);
    opts = m_getresoptions();
    m_setresoptions(opts | 0x0040);
    CU_ASSERT_EQUAL(m_getresoptions(), opts | 0x0040);
    m_setresoptions(opts);

    CU_ASSERT(res_gone());
}

/* The first call that needs it starts it (loads its library), once. */
static void testRESMOD_FIRST_CALL_LOADS(void)
{
    char host[64], serv[16];

    res_reset();
    CU_ASSERT_FATAL(res_gone());
    CU_ASSERT_EQUAL(numeric_nameinfo(host, serv), 0);
    CU_ASSERT_STRING_EQUAL(host, "127.0.0.1");
    CU_ASSERT_STRING_EQUAL(serv, "80");
    CU_ASSERT(res_running());
    if (!builtin)
        CU_ASSERT_EQUAL(lib_opencount(RES_LIBRARY), 1);
    /* not loaded again, not started again */
    CU_ASSERT_EQUAL(numeric_nameinfo(host, serv), 0);
    if (!builtin)
        CU_ASSERT_EQUAL(lib_opencount(RES_LIBRARY), 1);
    /* out, and the next call has it again */
    CU_ASSERT_EQUAL(res_out(), 0);
    CU_ASSERT(res_gone());
    CU_ASSERT_EQUAL(numeric_nameinfo(host, serv), 0);
    CU_ASSERT(res_running());
}

/* A name: the resolver is had, and the DNS asked. */
static void testRESMOD_NAME_RESOLVES(void)
{
    if (!ready) {
        CU_PASS("no DNS server of our own - skipping");
        return;
    }
    res_reset();
    CU_ASSERT_FATAL(res_gone());
    CU_ASSERT_EQUAL(dns_lookup("one.rdnss.test"), 77);
    CU_ASSERT(res_running());
    CU_ASSERT_EQUAL(dns_lookup("two.rdnss.test"), 78);
}

/* getaddrinfo() and getnameinfo(): both libraries give the same, and what they always gave. */
static void testRESMOD_FRONTENDS_AGREE(void)
{
    int have4 = st_have_family(AF_INET), have6 = st_have_family(AF_INET6);
    struct addrinfo *res, *ai;
    char host[256];
    LONG rc;
    int n;

    res_reset();

    /* a number of a family: one entry per socket type, the port as asked */
    CU_ASSERT(both_getaddrinfo("192.0.2.1", "80", AF_UNSPEC, 0, 0, &rc, &res));
    if (have4) {
        CU_ASSERT_EQUAL(rc, 0);
        for (n = 0, ai = res; ai != NULL; ai = ai->ai_next, n++)
            CU_ASSERT(ai->ai_family == AF_INET &&
                      ((struct sockaddr_in *)ai->ai_addr)->sin_port == htons(80) &&
                      ((struct sockaddr_in *)ai->ai_addr)->sin_addr.s_addr == htonl(0xc0000201));
        CU_ASSERT_EQUAL(n, 2);
        CU_ASSERT(res != NULL && res->ai_socktype == SOCK_STREAM && res->ai_protocol == IPPROTO_TCP);
        CU_ASSERT(res != NULL && res->ai_next != NULL && res->ai_next->ai_socktype == SOCK_DGRAM);
    } else
        CU_ASSERT_EQUAL(rc, EAI_ADDRFAMILY);
    if (res != NULL)
        freeaddrinfo(res);

    /* the port as a number (as strtol() reads it), as a service name, as an unknown one */
    CU_ASSERT(both_getaddrinfo(NULL, "80", AF_UNSPEC, SOCK_STREAM, AI_PASSIVE, &rc, NULL));
    CU_ASSERT(both_getaddrinfo(NULL, " +80", AF_UNSPEC, SOCK_STREAM, 0, &rc, NULL));
    CU_ASSERT_EQUAL(rc, 0);
    CU_ASSERT(both_getaddrinfo(NULL, "echo", AF_UNSPEC, 0, 0, &rc, &res));
    CU_ASSERT_EQUAL(rc, 0);
    CU_ASSERT(res != NULL && st_port((struct sockaddr_storage *)res->ai_addr) == htons(7));
    if (res != NULL)
        freeaddrinfo(res);
    CU_ASSERT(both_getaddrinfo(NULL, "ftp", AF_UNSPEC, SOCK_DGRAM, 0, &rc, NULL));
    CU_ASSERT_EQUAL(rc, EAI_SERVICE);
    CU_ASSERT(both_getaddrinfo(NULL, "no-such-service", AF_UNSPEC, 0, 0, &rc, NULL));
    CU_ASSERT_EQUAL(rc, EAI_SERVICE);
    CU_ASSERT(both_getaddrinfo(NULL, "echo", AF_UNSPEC, 0, AI_NUMERICSERV, &rc, NULL));
    CU_ASSERT_EQUAL(rc, EAI_NONAME);
    CU_ASSERT(both_getaddrinfo(NULL, NULL, AF_UNSPEC, 0, 0, &rc, NULL));
    CU_ASSERT_EQUAL(rc, EAI_NONAME);
    CU_ASSERT(both_getaddrinfo(NULL, "80", 12345, 0, 0, &rc, NULL));
    CU_ASSERT_EQUAL(rc, EAI_FAMILY);

    /* localhost, with the canonical name; IPv6 numbers */
    CU_ASSERT(both_getaddrinfo("localhost", "7", AF_UNSPEC, SOCK_STREAM, AI_CANONNAME, &rc, NULL));
    CU_ASSERT_EQUAL(rc, 0);
    CU_ASSERT(both_getaddrinfo("2001:db8::5", "443", AF_INET6, 0, AI_NUMERICHOST, &rc, NULL));
    CU_ASSERT_EQUAL(rc, have6 ? 0 : EAI_FAMILY);
    CU_ASSERT(both_getaddrinfo("not-a-number", "443", AF_UNSPEC, 0, AI_NUMERICHOST, &rc, NULL));
    CU_ASSERT_EQUAL(rc, EAI_NONAME);

    /* names, where the DNS asked is ours */
    if (ready) {
        CU_ASSERT(both_getaddrinfo("one.rdnss.test", "53", AF_UNSPEC, 0, AI_CANONNAME, &rc, NULL));
        CU_ASSERT_EQUAL(rc, 0);
        CU_ASSERT(both_getaddrinfo("no-such-name.rdnss.test", NULL, AF_UNSPEC, 0, 0, &rc, NULL));
        CU_ASSERT_EQUAL(rc, EAI_NONAME);
    }

    /* getnameinfo(): numbers, a service name, the numeric fallbacks */
    if (have4) {
        CU_ASSERT(both_getnameinfo(AF_INET, "192.0.2.3", 80, NI_NUMERICHOST | NI_NUMERICSERV, &rc, host));
        CU_ASSERT_EQUAL(rc, 0);
        CU_ASSERT_STRING_EQUAL(host, "192.0.2.3");
        CU_ASSERT(both_getnameinfo(AF_INET, NULL, 7, NI_NUMERICHOST, &rc, NULL));
        CU_ASSERT(both_getnameinfo(AF_INET, NULL, 7, NI_NUMERICHOST | NI_DGRAM, &rc, NULL));
        CU_ASSERT(both_getnameinfo(AF_INET, NULL, 65000, NI_NUMERICHOST, &rc, NULL));
    }
    if (have6) {
        CU_ASSERT(both_getnameinfo(AF_INET6, "2001:db8::7", 443, NI_NUMERICHOST | NI_NUMERICSERV, &rc, host));
        CU_ASSERT_EQUAL(rc, 0);
        CU_ASSERT_STRING_EQUAL(host, "2001:db8::7");
    }
    if (ready) {
        /* a reverse look-up our server answers "no such name": the number, or EAI_NONAME */
        CU_ASSERT(both_getnameinfo(st_any_family(), NULL, 7, 0, &rc, NULL));
        CU_ASSERT(both_getnameinfo(st_any_family(), NULL, 7, NI_NAMEREQD, &rc, NULL));
    }
}

/*
 * Without the resolver (ENV:AROSTCP/Without_resolver - what an installation
 * without the file sees): the design's table, value by value.
 */
static void testRESMOD_WITHOUT(void)
{
    struct sockaddr_in sin;
    struct addrinfo hints, *res;
    struct hostent h;
    struct servent *sp;
    char buf[256], host[64], serv[16];
    LONG herr, rc;
    ULONG addr = htonl(INADDR_LOOPBACK);

    res_reset();
    CU_ASSERT_FATAL(SetVar(RES_SWITCH, "1", 1, GVF_GLOBAL_ONLY) != 0);

    /* a name: NO_RECOVERY, ENOENT - nothing is loaded */
    CU_ASSERT_PTR_NULL(gethostbyname("one.rdnss.test"));
    CU_ASSERT_EQUAL(h_errno, NO_RECOVERY);
    CU_ASSERT_EQUAL(Errno(), ENOENT);
    /* (miami.library has a bsdsocket.library base of its own: its errno is not ours) */
    CU_ASSERT_PTR_NULL(m_gethostbyname2("one.rdnss.test", AF_INET6));
    CU_ASSERT_PTR_NULL(gethostbyaddr((char *)&addr, 4, AF_INET));
    CU_ASSERT_EQUAL(h_errno, NO_RECOVERY);
    CU_ASSERT_EQUAL(Errno(), ENOENT);
    herr = -5;
    CU_ASSERT_PTR_NULL(gethostbyname_r("one.rdnss.test", &h, buf, sizeof(buf), &herr));
    CU_ASSERT_EQUAL(herr, NO_RECOVERY);
    CU_ASSERT_EQUAL(Errno(), ENOENT);
    herr = -5;
    CU_ASSERT_PTR_NULL(gethostbyaddr_r((char *)&addr, 4, AF_INET, &h, buf, sizeof(buf), &herr));
    CU_ASSERT_EQUAL(herr, NO_RECOVERY);
    CU_ASSERT_EQUAL(Errno(), ENOENT);

    /* getaddrinfo(), getnameinfo(), even of numbers: EAI_FAIL, ENOENT */
    bzero(&hints, sizeof(hints));
    res = (struct addrinfo *)1;
    CU_ASSERT_EQUAL(getaddrinfo("192.0.2.1", "80", &hints, &res), EAI_FAIL);
    CU_ASSERT_EQUAL(Errno(), ENOENT);
    CU_ASSERT_PTR_NULL(res);
    res = (struct addrinfo *)1;
    CU_ASSERT_EQUAL(m_getaddrinfo("192.0.2.1", "80", &hints, &res), EAI_FAIL);
    CU_ASSERT_PTR_NULL(res);
    CU_ASSERT_EQUAL(numeric_nameinfo(host, serv), EAI_FAIL);
    CU_ASSERT_EQUAL(Errno(), ENOENT);
    bzero(&sin, sizeof(sin));
    sin.sin_len = sizeof(sin);
    sin.sin_family = AF_INET;
    sin.sin_addr.s_addr = addr;
    CU_ASSERT_EQUAL(m_getnameinfo((struct sockaddr *)&sin, sizeof(sin), host, sizeof(host),
                                  serv, sizeof(serv), NI_NUMERICHOST), EAI_FAIL);

    /* what needs no resolver works as before */
    CU_ASSERT(is_addr4(gethostbyname("127.0.0.1"), htonl(INADDR_LOOPBACK)));
    CU_ASSERT(is_addr4(gethostbyname("localhost"), htonl(INADDR_LOOPBACK)));
    CU_ASSERT(m_gethostbyname2("::1", AF_INET6) != NULL);
    CU_ASSERT(is_addr4(gethostbyname_r("127.0.0.1", &h, buf, sizeof(buf), &herr), htonl(INADDR_LOOPBACK)));
    CU_ASSERT_STRING_EQUAL(gai_strerror(EAI_FAIL), "Non-recoverable failure in name resolution");
    CU_ASSERT_STRING_EQUAL(m_gai_strerror(EAI_FAIL), "Non-recoverable failure in name resolution");
    freeaddrinfo(NULL);
    m_freeaddrinfo(NULL);
    rc = m_getresoptions();
    m_setresoptions(rc ^ 0x0040);
    CU_ASSERT_EQUAL(m_getresoptions(), rc ^ 0x0040);
    m_setresoptions(rc);
    sp = getservbyname("ftp", "tcp");
    CU_ASSERT(sp != NULL && sp->s_port == htons(21));
    sp = getservbyport(htons(23), "tcp");
    CU_ASSERT(sp != NULL && strcmp(sp->s_name, "telnet") == 0);
    CU_ASSERT(res_gone());

    /* the switch gone, the stack does not look again until told */
    DeleteVar(RES_SWITCH, GVF_GLOBAL_ONLY);
    CU_ASSERT_PTR_NULL(gethostbyaddr((char *)&addr, 4, AF_INET));
    CU_ASSERT_EQUAL(Errno(), ENOENT);
    CU_ASSERT(res_gone());
    if (!installed)
        return;
    if (builtin) {
        rc = module_cmd(NCMD_RELOAD, NULL, NULL);
        printf("\n    configuration read again (result %ld)", (long)rc);
        Delay(4 * 50);          /* duplicate address detection, for the programs after this one */
    } else {
        CU_ASSERT_EQUAL(module_cmd(NCMD_MODULE_LOAD, RES_LIBRARY, NULL), 0);
        CU_ASSERT_EQUAL(res_out(), 0);
    }
    CU_ASSERT_EQUAL(numeric_nameinfo(host, serv), 0);
    CU_ASSERT(res_running());
}

/*
 * While a task waits inside the resolver for a name server's answer, the
 * resolver is neither unloaded nor stopped; once the answer came, it is;
 * and the next look-up has it again.
 */
static void testRESMOD_BUSY_REFUSED(void)
{
    struct hmsg *m, *asked, *done = NULL, *other = NULL;
    ULONG flags = 0;
    char host[64], serv[16];

    if (!ready || progpath[0] == '\0') {
        CU_PASS("no DNS server of our own, or this program cannot be found - skipping");
        return;
    }
    /* the silent server takes the place of dnstest.h's, at the same address */
    dns_stop();
    server = 0;
    Delay(25);

    res_reset();
    CU_ASSERT_EQUAL_FATAL(numeric_nameinfo(host, serv), 0);    /* running */
    CU_ASSERT_FATAL(spawn("--silent") == 0);
    m = helper_msg(HK_READY, 10, NULL);
    CU_ASSERT_FATAL(m != NULL);
    ReplyMsg(&m->hm_Msg);
    CU_ASSERT_FATAL(spawn("--lookup") == 0);

    /* the look-up is inside the resolver, waiting for the answer */
    asked = helper_msg(HK_ASKED, 30, &other);
    if (asked == NULL) {
        if (other != NULL)
            ReplyMsg(&other->hm_Msg);
        CU_PASS("the question never reached our server (another name server answered) - skipping");
        return;
    }
    CU_ASSERT_EQUAL(res_out(), EBUSY);
    CU_ASSERT_EQUAL(module_cmd(NCMD_MODULE_STOP, RES_MODULE, &flags), EBUSY);
    CU_ASSERT(flags & NCMMF_STARTED);
    if (!builtin)
        CU_ASSERT_EQUAL(lib_opencount(RES_LIBRARY), 1);

    /* answered: the look-up comes back, and the resolver can go */
    ReplyMsg(&asked->hm_Msg);
    done = other != NULL && other->hm_Kind == HK_DONE ? other : helper_msg(HK_DONE, 60, NULL);
    CU_ASSERT(done != NULL);
    if (done != NULL)
        ReplyMsg(&done->hm_Msg);
    Delay(25);
    CU_ASSERT_EQUAL(res_out(), 0);
    CU_ASSERT(res_gone());

    /* the next look-up has it again */
    CU_ASSERT_EQUAL(numeric_nameinfo(host, serv), 0);
    CU_ASSERT(res_running());
}

/*
 * A list from getaddrinfo() is the application's, not the library's: it
 * holds nothing of the resolver, survives an unload whole, and is freed
 * after it. Loading and unloading leave memory as it was.
 */
static void testRESMOD_LIST_AFTER_UNLOAD(void)
{
    struct addrinfo hints, *r1 = NULL, *r2 = NULL;
    char host[64], serv[16];
    ULONG before, after;
    LONG drift;
    int i;

    res_reset();
    bzero(&hints, sizeof(hints));
    hints.ai_family = st_any_family();
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_CANONNAME;
    CU_ASSERT_EQUAL_FATAL(getaddrinfo("localhost", "7", &hints, &r1), 0);
    CU_ASSERT_EQUAL_FATAL(m_getaddrinfo("localhost", "7", &hints, &r2), 0);
    CU_ASSERT_EQUAL(res_out(), 0);
    CU_ASSERT(res_gone());
    CU_ASSERT(r1 != NULL && st_port((struct sockaddr_storage *)r1->ai_addr) == htons(7));
    CU_ASSERT(r1 != NULL && r1->ai_canonname != NULL && strcmp(r1->ai_canonname, "localhost") == 0);
    CU_ASSERT(same_list(r1, r2));
    /* each library frees the other's list: one freeaddrinfo() for both */
    m_freeaddrinfo(r1);
    freeaddrinfo(r2);

    if (builtin)
        return;
    for (i = 0; i < 2; i++) {
        CU_ASSERT_EQUAL_FATAL(numeric_nameinfo(host, serv), 0);
        CU_ASSERT_EQUAL_FATAL(res_out(), 0);
    }
    before = AvailMem(MEMF_ANY);
    for (i = 0; i < CYCLES; i++)
        if (numeric_nameinfo(host, serv) != 0 || res_out() != 0)
            break;
    after = AvailMem(MEMF_ANY);
    drift = (LONG)before - (LONG)after;
    printf("\n    free memory after %d load/unload cycles: %ld bytes less (limit %d)",
           i, (long)drift, DRIFT_MAX);
    CU_ASSERT_EQUAL(i, CYCLES);
    CU_ASSERT(drift < DRIFT_MAX && drift > -DRIFT_MAX);
}

/* ---- main ---------------------------------------------------------------- */

/* What the running stack knows of the resolver: 0 usable, else said in a SKIP line. */
static int resolver_usable(void)
{
    ULONG flags = 0;
    LONG r;

    DeleteVar(RES_SWITCH, GVF_GLOBAL_ONLY);
    r = module_cmd(NCMD_MODULE_DEMAND, RES_MODULE, NULL);
    if (r == -1) {
        printf("SKIP %s: the stack has no control port\n", PROGRAM);
        return -1;
    }
    /* perhaps only remembered as not to be had (an earlier run): a load by name forgets that */
    if (r != 0 && module_cmd(NCMD_MODULE_LOAD, RES_LIBRARY, NULL) == 0)
        r = 0;
    if (module_cmd(NCMD_MODULE_QUERY, RES_MODULE, &flags) != 0 || !(flags & NCMMF_STARTED)) {
        /* not installed (or a stack without the module): only the answers without it are tested */
        printf("SKIP %s (loading): %s is not installed here (error %ld)\n", PROGRAM, RES_LIBRARY, (long)r);
        return 1;
    }
    builtin = (flags & NCMMF_BUILTIN) != 0;
    return 0;
}

int main(int argc, char **argv)
{
    CU_pSuite p;
    int usable, loaded;

    if (argc > 1 && strcmp(argv[1], "--serve") == 0)
        return dns_serve();
    if (argc > 1 && strcmp(argv[1], "--silent") == 0)
        return silent_server();
    if (argc > 1 && strcmp(argv[1], "--lookup") == 0)
        return lookup_task();

    SocketBase = OpenLibrary("bsdsocket.library", 4);
    MiamiBase = OpenLibrary("miami.library", 0);
    if (SocketBase == NULL || MiamiBase == NULL) {
        printf("no bsdsocket.library or no miami.library\n");
        if (MiamiBase)
            CloseLibrary(MiamiBase);
        if (SocketBase)
            CloseLibrary(SocketBase);
        return 20;
    }
    if (!st_have_family(AF_INET))
        server_addr = "::1";
    if (!NameFromLock(GetProgramDir(), progpath, sizeof(progpath)) ||
            !AddPart(progpath, FilePart((STRPTR)argv[0]), sizeof(progpath)) ||
            (hport = helper_port()) == NULL)
        progpath[0] = '\0';

    loaded = lib_opencount(RES_LIBRARY) >= 0;
    if ((usable = resolver_usable()) < 0) {
        helper_port_end(hport);
        CloseLibrary(MiamiBase);
        CloseLibrary(SocketBase);
        return 0;
    }
    installed = usable == 0;
    printf("%s: the resolver is %s; its library was %sin memory when this program started\n",
           PROGRAM, !installed ? "not to be had" : builtin ? "part of the stack program" : "a library",
           loaded ? "" : "not ");
    if (installed) {
        if (dns_other_server())
            printf("another DNS server answers first - the tests of names are skipped\n");
        else if (!(server = dns_start(argv0 = argv[0])))
            printf("the test's DNS server did not start - the tests of names are skipped\n");
        else {
            added = AddDomainNameServer((STRPTR)server_addr);
            ready = added == 0;
        }
    }

    if (CU_initialize_registry() != CUE_SUCCESS)
        goto out;
    p = CU_add_suite("RESMOD_Suite", init_suite, clean_suite);
    if (p == NULL ||
        !st_add_test(p, "numbers and localhost are answered without the resolver", testRESMOD_LITERALS_WITHOUT_LOADING, ST_NONE) ||
        !st_add_test(p, "without the resolver every call answers as designed", testRESMOD_WITHOUT, ST_NONE) ||
        (installed && (
        !st_add_test(p, "the first call that needs it loads it, once", testRESMOD_FIRST_CALL_LOADS, ST_ANY) ||
        !st_add_test(p, "a name loads it and is resolved", testRESMOD_NAME_RESOLVES, ST_ANY) ||
        !st_add_test(p, "both libraries' getaddrinfo() and getnameinfo() agree", testRESMOD_FRONTENDS_AGREE, ST_ANY) ||
        !st_add_test(p, "a list from getaddrinfo() outlives an unload", testRESMOD_LIST_AFTER_UNLOAD, ST_ANY) ||
        !st_add_test(p, "it is not unloaded while a look-up waits", testRESMOD_BUSY_REFUSED, ST_ANY))))
    {
        CU_cleanup_registry();
        goto out;
    }
    CU_basic_set_mode(CU_BRM_VERBOSE);
    CU_basic_run_tests();
    CU_basic_set_mode(CU_BRM_SILENT);
    CU_automated_package_name_set("NetUnitTests");
    CU_set_output_filename("Net-ResolverMod");
    CU_automated_enable_junit_xml(CU_TRUE);
    CU_automated_run_tests();
    CU_cleanup_registry();
out:
    if (added == 0)
        RemoveDomainNameServer((STRPTR)server_addr);
    if (server)
        dns_stop();
    DeleteVar(RES_SWITCH, GVF_GLOBAL_ONLY);
    helper_port_end(hport);
    CloseLibrary(MiamiBase);
    CloseLibrary(SocketBase);
    return CU_get_error();
}
