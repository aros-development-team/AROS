/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    CUnit tests for a socket shared between tasks whose receive buffer is
    held by another call: a receive with MSG_WAITALL sleeps holding it until
    all it asked for has come. A call that may not block - MSG_DONTWAIT, or
    a non-blocking socket (FIONBIO) - gets EWOULDBLOCK at once instead of
    waiting for it, as FreeBSD's SBLOCKWAIT(flags); a blocking call still
    waits, and gets its data once the other one is done.

    The test shares one end of a TCP connection over the loopback with a
    copy of itself (ReleaseCopyOfSocket(), ObtainSocket()) that receives
    with MSG_WAITALL, and the other end with a second copy that sends 50
    bytes at once and the rest 3 s later: the first copy takes the 50 and
    sleeps holding the buffer. (Were it not holding it, the test's own calls
    would find those 50 bytes there and fail.) The receive asks for more
    than the receive buffer holds (SO_RCVBUF, as TCP left it once
    connected), so it takes the data in sections and holds the buffer
    between them; one that fits waits for all of it first, without holding
    it.
    Usage:  cunit-net-sblock
*/

#include "socktest.h"

#include <proto/dos.h>
#include <dos/dostags.h>
#include <libraries/bsdsocket.h>
#include <sys/ioctl.h>
#include <stdlib.h>

struct Library *SocketBase = NULL;

static char progpath[256];

#define FIRST_LEN       50      /* what comes at once */
#define SEND_DELAY      3       /* s before the data comes */

/* ---- the helpers (this program again) --------------------------------- */

/* --waitall ID LEN: receive LEN bytes with MSG_WAITALL. */
static int helper_waitall(LONG id, int len)
{
    char *buf = malloc(len > 0 ? len : 1);
    int s;

    if (!buf || !(SocketBase = OpenLibrary("bsdsocket.library", 3))) {
        free(buf);
        return 20;
    }
    if ((s = ObtainSocket(id, AF_INET, SOCK_STREAM, 0)) >= 0) {
        recv(s, buf, len, MSG_WAITALL);
        CloseSocket(s);
    }
    CloseLibrary(SocketBase);
    free(buf);
    return 0;
}

/* --send ID FIRST LATER: send FIRST bytes now, LATER more SEND_DELAY s on. */
static int helper_send(LONG id, int first, int later)
{
    char *buf = malloc(first > later ? first + 1 : later + 1);
    int s;

    if (!buf || !(SocketBase = OpenLibrary("bsdsocket.library", 3))) {
        free(buf);
        return 20;
    }
    if ((s = ObtainSocket(id, AF_INET, SOCK_STREAM, 0)) >= 0) {
        memset(buf, 'x', first > later ? first : later);
        send(s, buf, first, 0);
        Delay(SEND_DELAY * TICKS_PER_SECOND);
        send(s, buf, later, 0);
        Delay(2 * TICKS_PER_SECOND);
        CloseSocket(s);
    }
    CloseLibrary(SocketBase);
    free(buf);
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

/* ---- the tests -------------------------------------------------------- */

/* A connected TCP pair over the loopback: *a the client, *b the server. */
static int tcp_pair(int *a, int *b)
{
    struct sockaddr_in sa;
    socklen_t len = sizeof(sa);
    int l = socket(AF_INET, SOCK_STREAM, 0);

    bzero(&sa, sizeof(sa));
    sa.sin_len = sizeof(sa);
    sa.sin_family = AF_INET;
    sa.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    if (l < 0 || bind(l, (struct sockaddr *)&sa, sizeof(sa)) != 0 || listen(l, 1) != 0 ||
            getsockname(l, (struct sockaddr *)&sa, &len) != 0)
        return -1;
    *a = socket(AF_INET, SOCK_STREAM, 0);
    if (*a < 0 || connect(*a, (struct sockaddr *)&sa, sizeof(sa)) != 0)
        return -1;
    len = sizeof(sa);
    *b = accept(l, (struct sockaddr *)&sa, &len);
    CloseSocket(l);
    return *b >= 0 ? 0 : -1;
}

/* DOS ticks since t0. */
static long ticks_since(const struct DateStamp *t0)
{
    struct DateStamp now;

    DateStamp(&now);
    return ((now.ds_Days - t0->ds_Days) * 1440L + (now.ds_Minute - t0->ds_Minute)) * 3000L
           + (now.ds_Tick - t0->ds_Tick);
}

/*
 * Set up: a pair whose client end a helper receives more than its receive
 * buffer holds on with MSG_WAITALL, and whose server end another helper
 * sends FIRST_LEN bytes on at once - the first helper takes them and sleeps
 * holding its receive buffer - and SEND_DELAY s from now the rest, and
 * extra bytes more. *a: our copy of the client end.
 */
static int held(int *a, int extra)
{
    char args[64];
    LONG ida, idb;
    socklen_t len = sizeof(int);
    int b, rcvbuf, want;

    if (tcp_pair(a, &b) != 0 ||
            getsockopt(*a, SOL_SOCKET, SO_RCVBUF, &rcvbuf, &len) != 0)
        return -1;
    want = rcvbuf + 1000;
    ida = ReleaseCopyOfSocket(*a, UNIQUE_ID);
    idb = ReleaseCopyOfSocket(b, UNIQUE_ID);
    CloseSocket(b);
    if (ida < 0 || idb < 0)
        return -1;
    snprintf(args, sizeof(args), "--waitall %ld %d", (long)ida, want);
    if (spawn(args) != 0)
        return -1;
    Delay(TICKS_PER_SECOND / 2);
    snprintf(args, sizeof(args), "--send %ld %d %d", (long)idb, FIRST_LEN,
             want - FIRST_LEN + extra);
    if (spawn(args) != 0)
        return -1;
    Delay(TICKS_PER_SECOND);           /* the helper is asleep in its receive */
    return 0;
}

int init_suite(void) { return (SocketBase != NULL) ? 0 : -1; }
int clean_suite(void) { return 0; }

/* MSG_DONTWAIT: EWOULDBLOCK at once. */
void testSBLOCK_DONTWAIT(void)
{
    struct DateStamp t0;
    char c;
    int a, r;

    CU_ASSERT_EQUAL_FATAL(held(&a, 0), 0);
    DateStamp(&t0);
    r = recv(a, &c, 1, MSG_DONTWAIT);
    CU_ASSERT_EQUAL(r, -1);
    CU_ASSERT_EQUAL(Errno(), EWOULDBLOCK);
    CU_ASSERT(ticks_since(&t0) < TICKS_PER_SECOND / 2);
    Delay((SEND_DELAY + 2) * TICKS_PER_SECOND);
    CloseSocket(a);
}

/* A non-blocking socket: the same. */
void testSBLOCK_NBIO(void)
{
    struct DateStamp t0;
    long on = 1;
    char c;
    int a, r;

    CU_ASSERT_EQUAL_FATAL(held(&a, 0), 0);
    CU_ASSERT_EQUAL(IoctlSocket(a, FIONBIO, (char *)&on), 0);
    DateStamp(&t0);
    r = recv(a, &c, 1, 0);
    CU_ASSERT_EQUAL(r, -1);
    CU_ASSERT_EQUAL(Errno(), EWOULDBLOCK);
    CU_ASSERT(ticks_since(&t0) < TICKS_PER_SECOND / 2);
    Delay((SEND_DELAY + 2) * TICKS_PER_SECOND);
    CloseSocket(a);
}

/* A blocking call waits for the other one, then gets its data. */
void testSBLOCK_BLOCKING_WAITS(void)
{
    struct DateStamp t0;
    char c = 0;
    int a, r;

    CU_ASSERT_EQUAL_FATAL(held(&a, 1), 0);
    DateStamp(&t0);
    r = recv(a, &c, 1, 0);
    CU_ASSERT_EQUAL(r, 1);
    CU_ASSERT_EQUAL(c, 'x');
    CU_ASSERT(ticks_since(&t0) >= (SEND_DELAY - 1) * TICKS_PER_SECOND);
    Delay(2 * TICKS_PER_SECOND);
    CloseSocket(a);
}

int main(int argc, char **argv)
{
    CU_pSuite p;

    if (argc > 3 && strcmp(argv[1], "--waitall") == 0)
        return helper_waitall(strtol(argv[2], NULL, 10), atoi(argv[3]));
    if (argc > 4 && strcmp(argv[1], "--send") == 0)
        return helper_send(strtol(argv[2], NULL, 10), atoi(argv[3]), atoi(argv[4]));

    SocketBase = OpenLibrary("bsdsocket.library", 3);
    if (SocketBase == NULL) {
        printf("bsdsocket.library not available - skipping SBLOCK tests\n");
        return 0;
    }
    if (!NameFromLock(GetProgramDir(), progpath, sizeof(progpath)) ||
            !AddPart(progpath, FilePart((STRPTR)argv[0]), sizeof(progpath))) {
        printf("cannot find this program - skipping SBLOCK tests\n");
        CloseLibrary(SocketBase);
        return 0;
    }

    if (CU_initialize_registry() != CUE_SUCCESS) {
        CloseLibrary(SocketBase);
        return CU_get_error();
    }
    p = CU_add_suite("SBLOCK_Suite", init_suite, clean_suite);
    if (p == NULL ||
        !CU_add_test(p, "MSG_DONTWAIT does not wait for a held buffer", testSBLOCK_DONTWAIT) ||
        !CU_add_test(p, "nor does a non-blocking socket", testSBLOCK_NBIO) ||
        !CU_add_test(p, "a blocking call waits, then gets its data", testSBLOCK_BLOCKING_WAITS))
    {
        CU_cleanup_registry();
        CloseLibrary(SocketBase);
        return CU_get_error();
    }
    CU_basic_set_mode(CU_BRM_VERBOSE);
    CU_basic_run_tests();
    CU_basic_set_mode(CU_BRM_SILENT);
    CU_automated_package_name_set("NetUnitTests");
    CU_set_output_filename("Net-SBLOCK");
    CU_automated_enable_junit_xml(CU_TRUE);
    CU_automated_run_tests();
    CU_cleanup_registry();
    CloseLibrary(SocketBase);
    return CU_get_error();
}
