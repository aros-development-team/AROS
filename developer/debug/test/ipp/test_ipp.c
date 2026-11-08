/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    ipp.device against a printer of its own: a child process listens on the
    loopback address and takes the job ipp.device unit 9 sends it, then the
    request line, the chunked encoding, the IPP header, the attributes and
    the document are checked against what was written; then once more with
    the printer URI, the FORMAT and the USER as long as the device takes
    them. A unit whose printer does not listen must fail to open, as must one
    whose URI has a port beyond 65535. With URI, a page is also printed to
    that printer and ENV:IPP/Status shown.
*/

#include <exec/io.h>
#include <exec/memory.h>
#include <devices/newstyle.h>
#include <dos/dos.h>
#include <dos/dostags.h>
#include <dos/var.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/bsdsocket.h>

#include <sys/socket.h>
#include <sys/time.h>
#include <netinet/in.h>

#include <string.h>

#define TEMPLATE "URI/K,SIZE/K/N"

#define UNIT        9
#define DEADUNIT    8
#define URIUNIT     7

#define DOCUMENT    "Hello from ipp.device\n"

static const char *unit_file[] = { "ENV:IPP/Unit7", "ENV:IPP/Unit8", "ENV:IPP/Unit9" };
static const char *saved_file[] = { "T:test_ipp-Unit7", "T:test_ipp-Unit8", "T:test_ipp-Unit9" };
static BOOL had_file[3];

static struct Task *parent;
static volatile UWORD server_port;
static volatile BOOL server_done;
static char server_fault[128];          /* the first thing the server did not like */
static char server_uri[128];
static char want_uri[256], want_format[128], want_user[64], want_path[256];   /* what the job must carry */
static char nsd_fault[64];
static UBYTE request[4096];
static LONG request_len;

struct Library *SocketBase;

static void append_number(char *s, ULONG v)
{
    char d[12];
    LONG i = 0;

    do {
        d[i++] = '0' + v % 10;
        v /= 10;
    } while (v);
    s += strlen(s);
    while (i)
        *s++ = d[--i];
    *s = 0;
}

static void fault(const char *what)
{
    if (!server_fault[0])
        strncpy(server_fault, what, sizeof(server_fault) - 1);
}

/* The request as sent: the chunks put back together, the headers checked. */
static LONG dechunk(const UBYTE *in, LONG len, UBYTE *out)
{
    const char *end = strstr((const char *)in, "\r\n\r\n");
    LONG off, n = 0;

    if (!end) {
        fault("no end of the HTTP headers");
        return -1;
    }
    if (strncmp((const char *)in, "POST ", 5) != 0 || strncmp((const char *)in + 5, want_path, strlen(want_path)) != 0
        || strncmp((const char *)in + 5 + strlen(want_path), " HTTP/1.1\r\n", 11) != 0)
        fault("request line");
    if (!strstr((const char *)in, "Transfer-Encoding: chunked\r\n"))
        fault("not chunked");
    if (!strstr((const char *)in, "Content-Type: application/ipp\r\n"))
        fault("content type");
    off = end + 4 - (const char *)in;
    for (;;) {
        LONG size = 0;
        while (off < len && in[off] != '\r') {
            UBYTE c = in[off++];
            size = size * 16 + (c >= 'a' ? c - 'a' + 10 : c >= 'A' ? c - 'A' + 10 : c - '0');
        }
        off += 2;
        if (size == 0)
            return n;
        if (off + size + 2 > len) {
            fault("a chunk runs past the data");
            return -1;
        }
        memcpy(out + n, in + off, size);
        n += size;
        off += size;
        if (in[off] != '\r' || in[off + 1] != '\n') {
            fault("no CRLF after a chunk");
            return -1;
        }
        off += 2;
    }
}

/* The IPP message: version 1.1, Print-Job, the attributes, then the document. */
static void check_ipp(const UBYTE *p, LONG len)
{
    LONG off = 9;
    char doc_format[128] = "", printer_uri[256] = "", user[64] = "";

    if (len < 10 || p[0] != 1 || p[1] != 1)
        fault("IPP version");
    if (p[2] != 0 || p[3] != 2)
        fault("not a Print-Job");
    if (p[8] != 0x01)
        fault("no operation attributes");
    while (off < len && p[off] != 0x03) {
        UBYTE tag = p[off];
        UWORD nl, vl;
        if (tag < 0x10) {
            off++;
            continue;
        }
        nl = (p[off + 1] << 8) | p[off + 2];
        vl = (p[off + 3 + nl] << 8) | p[off + 4 + nl];
        if (off + 5 + nl + vl > len) {
            fault("an attribute runs past the message");
            return;
        }
        if (nl == 15 && !strncmp((const char *)p + off + 3, "document-format", 15) && vl < sizeof(doc_format)) {
            memcpy(doc_format, p + off + 5 + nl, vl);
            doc_format[vl] = 0;
        } else if (nl == 11 && !strncmp((const char *)p + off + 3, "printer-uri", 11) && vl < sizeof(printer_uri)) {
            memcpy(printer_uri, p + off + 5 + nl, vl);
            printer_uri[vl] = 0;
        } else if (nl == 20 && !strncmp((const char *)p + off + 3, "requesting-user-name", 20) && vl < sizeof(user)) {
            memcpy(user, p + off + 5 + nl, vl);
            user[vl] = 0;
        }
        off += 5 + nl + vl;
    }
    if (off >= len) {
        fault("no end of attributes");
        return;
    }
    off++;
    if (strcmp(doc_format, want_format))
        fault("document-format");
    if (strcmp(printer_uri, want_uri))
        fault("printer-uri");
    if (want_user[0] ? strcmp(user, want_user) != 0 : !user[0])
        fault("requesting-user-name");
    if (len - off != (LONG)strlen(DOCUMENT) || memcmp(p + off, DOCUMENT, len - off))
        fault("the document is not what was written");
}

static LONG server(void)
{
    struct Library *SocketBase = OpenLibrary((CONST_STRPTR)"bsdsocket.library", 4);
    struct sockaddr_in sin;
    struct timeval tv = { 10, 0 };
    socklen_t sl = sizeof(sin);
    LONG ls = -1, s = -1, got = 0, n;
    UBYTE *raw;

    if (!SocketBase) {
        fault("the server has no bsdsocket.library");
        goto done;
    }
    memset(&sin, 0, sizeof(sin));
    sin.sin_family = AF_INET;
    sin.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    ls = socket(AF_INET, SOCK_STREAM, 0);
    if (ls < 0 || bind(ls, (struct sockaddr *)&sin, sizeof(sin)) < 0 || listen(ls, 1) < 0
        || getsockname(ls, (struct sockaddr *)&sin, &sl) < 0) {
        fault("the server cannot listen on the loopback address");
        goto done;
    }
    server_port = ntohs(sin.sin_port);
    strcpy(server_uri, "ipp://127.0.0.1:");
    append_number(server_uri, server_port);
    strcat(server_uri, "/ipp/print");
    Signal(parent, SIGBREAKF_CTRL_E);

    setsockopt(ls, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
    raw = AllocMem(sizeof(request) * 2, MEMF_CLEAR);
    if (!raw) {
        fault("no memory");
        goto done;
    }
    s = accept(ls, (struct sockaddr *)&sin, &sl);
    if (s < 0) {
        fault("nothing connected to the server");
        goto freeraw;
    }
    setsockopt(s, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
    while (got < (LONG)sizeof(request) * 2 - 1 && (n = recv(s, raw + got, sizeof(request) * 2 - 1 - got, 0)) > 0) {
        got += n;
        if (got >= 5 && !memcmp(raw + got - 5, "0\r\n\r\n", 5))
            break;
    }
    raw[got] = 0;
    request_len = dechunk(raw, got, request);
    if (request_len > 0)
        check_ipp(request, request_len);
    {
        static const UBYTE ipp[] = {
            1, 1, 0, 0, 0, 0, 0, 0, 0x01,
            0x47, 0, 18, 'a','t','t','r','i','b','u','t','e','s','-','c','h','a','r','s','e','t', 0, 5, 'u','t','f','-','8',
            0x48, 0, 27, 'a','t','t','r','i','b','u','t','e','s','-','n','a','t','u','r','a','l','-','l','a','n','g','u','a','g','e', 0, 2, 'e','n',
            0x03
        };
        UBYTE answer[256];
        LONG len;
        strcpy((char *)answer, "HTTP/1.1 200 OK\r\nContent-Type: application/ipp\r\nContent-Length: ");
        append_number((char *)answer, sizeof(ipp));
        strcat((char *)answer, "\r\nConnection: close\r\n\r\n");
        len = strlen((char *)answer);
        memcpy(answer + len, ipp, sizeof(ipp));
        if (request_len >= 8)
            memcpy(answer + len + 4, request + 4, 4);       /* the request-id */
        send(s, answer, len + sizeof(ipp), 0);
    }
    CloseSocket(s);
freeraw:
    FreeMem(raw, sizeof(request) * 2);
done:
    if (ls >= 0)
        CloseSocket(ls);
    if (SocketBase)
        CloseLibrary(SocketBase);
    server_done = TRUE;
    Signal(parent, SIGBREAKF_CTRL_F);
    return 0;
}

static BOOL write_unit(const char *file, const char *uri, const char *format)
{
    BPTR fh = Open((CONST_STRPTR)file, MODE_NEWFILE);

    if (!fh)
        return FALSE;
    FPuts(fh, (CONST_STRPTR)"URI=");
    FPuts(fh, (CONST_STRPTR)uri);
    FPuts(fh, (CONST_STRPTR)"\nFORMAT=");
    FPuts(fh, (CONST_STRPTR)format);
    FPuts(fh, (CONST_STRPTR)"\n");
    Close(fh);
    return TRUE;
}

static BOOL copy_file(const char *from, const char *to)
{
    UBYTE buf[512];
    BPTR in = Open((CONST_STRPTR)from, MODE_OLDFILE), out;
    LONG n;

    if (!in)
        return FALSE;
    out = Open((CONST_STRPTR)to, MODE_NEWFILE);
    if (out) {
        while ((n = Read(in, buf, sizeof(buf))) > 0)
            Write(out, buf, n);
        Close(out);
    }
    Close(in);
    return out != BNULL;
}

static void status(char *out, LONG size)
{
    out[0] = 0;
    if (GetVar((CONST_STRPTR)"IPP/Status", (STRPTR)out, size, GVF_GLOBAL_ONLY) < 0)
        strcpy(out, "(not set)");
}

static LONG print_to(LONG unit, const char *text)
{
    struct MsgPort *port = CreateMsgPort();
    struct IOStdReq *io = port ? (struct IOStdReq *)CreateIORequest(port, sizeof(*io)) : NULL;
    LONG err = -1;

    if (io) {
        err = OpenDevice((CONST_STRPTR)"ipp.device", unit, (struct IORequest *)io, 0);
        if (err == 0) {
            const char *nl = strchr(text, '\n');
            struct NSDeviceQueryResult q;
            io->io_Command = NSCMD_DEVICEQUERY;
            io->io_Data = &q;
            io->io_Length = sizeof(q);
            DoIO((struct IORequest *)io);
            if (io->io_Error || io->io_Actual < sizeof(q))
                strcpy(nsd_fault, "NSCMD_DEVICEQUERY not answered");
            else if (q.DeviceType != NSDEVTYPE_PRINTER || !q.SupportedCommands || q.SupportedCommands[0] != CMD_WRITE)
                strcpy(nsd_fault, "NSCMD_DEVICEQUERY: not a printer that takes CMD_WRITE");
            io->io_Command = CMD_WRITE;
            io->io_Data = (APTR)text;
            io->io_Length = nl ? nl - text : -1;    /* the first line by length */
            DoIO((struct IORequest *)io);
            err = io->io_Error;
            if (!err && nl) {
                io->io_Command = CMD_WRITE;
                io->io_Data = (APTR)nl;
                io->io_Length = -1;                 /* the rest as a NUL-terminated string */
                DoIO((struct IORequest *)io);
                err = io->io_Error;
            }
            CloseDevice((struct IORequest *)io);
        }
        DeleteIORequest((struct IORequest *)io);
    }
    if (port)
        DeleteMsgPort(port);
    return err;
}

/* The server for one job; FALSE when it could not listen. */
static BOOL start_server(void)
{
    struct Process *child;

    server_port = 0;
    server_done = FALSE;
    server_fault[0] = 0;
    SetSignal(0, SIGBREAKF_CTRL_E | SIGBREAKF_CTRL_F);
    child = CreateNewProcTags(NP_Entry, (IPTR)server, NP_Name, (IPTR)"test_ipp server",
                              NP_StackSize, 65536, TAG_DONE);
    if (!child) {
        strcpy(server_fault, "no server process");
        return FALSE;
    }
    Wait(SIGBREAKF_CTRL_E | SIGBREAKF_CTRL_F);
    return server_port != 0;
}

/* A job to the test server; TRUE when the server got it as it must be. */
static BOOL job_ok(const char *what)
{
    char st[200];
    LONG err = print_to(UNIT, DOCUMENT);
    BOOL ok = TRUE;

    while (!server_done)
        Wait(SIGBREAKF_CTRL_F);
    status(st, sizeof(st));
    Printf((CONST_STRPTR)"  unit %ld, %s: error %ld, status \"%s\"\n", UNIT, (IPTR)what, err, (IPTR)st);
    if (err || strcmp(st, "Unit9: printed, 22 bytes")) {
        PutStr((CONST_STRPTR)"  expected error 0 and \"Unit9: printed, 22 bytes\"\n");
        ok = FALSE;
    }
    if (server_fault[0]) {
        Printf((CONST_STRPTR)"  the server did not get a proper job: %s\n", (IPTR)server_fault);
        ok = FALSE;
    } else
        PutStr((CONST_STRPTR)"  the server got the Print-Job in chunks, with the document\n");
    return ok;
}

int main(void)
{
    IPTR args[2] = { 0 };
    struct RDArgs *rda = ReadArgs((CONST_STRPTR)TEMPLATE, args, NULL);
    char st[200], old_user[64], long_uri[256], long_format[128], long_user[64];
    LONG i, err, had_user;
    BOOL ok = TRUE;

    if (!rda) {
        PrintFault(IoErr(), (CONST_STRPTR)"test_ipp");
        return RETURN_FAIL;
    }
    {
        BPTR lock = Lock((CONST_STRPTR)"ENV:IPP", SHARED_LOCK);
        if (!lock)
            lock = CreateDir((CONST_STRPTR)"ENV:IPP");
        if (lock)
            UnLock(lock);
    }
    for (i = 0; i < 3; i++)
        had_file[i] = copy_file(unit_file[i], saved_file[i]);

    parent = FindTask(NULL);
    if (!start_server()) {
        Printf((CONST_STRPTR)"FAIL: %s\n", (IPTR)server_fault);
        FreeArgs(rda);
        return RETURN_FAIL;
    }
    strcpy(want_uri, server_uri);
    strcpy(want_format, "text/plain");
    strcpy(want_path, "/ipp/print");
    want_user[0] = 0;
    write_unit(unit_file[2], server_uri, "text/plain");
    Printf((CONST_STRPTR)"  to %s\n", (IPTR)server_uri);
    if (!job_ok("the test server"))
        ok = FALSE;
    if (nsd_fault[0]) {
        Printf((CONST_STRPTR)"  %s\n", (IPTR)nsd_fault);
        ok = FALSE;
    }

    /* The same job with the URI, FORMAT and USER at their longest: 255, 127 and 63 characters. */
    if (!start_server()) {
        Printf((CONST_STRPTR)"FAIL: %s\n", (IPTR)server_fault);
        FreeArgs(rda);
        return RETURN_FAIL;
    }
    strcpy(long_uri, server_uri);
    strcat(long_uri, "/");
    for (i = strlen(long_uri); i < 255; i++)
        long_uri[i] = 'a' + i % 26;
    long_uri[255] = 0;
    strcpy(long_format, "application/");
    for (i = strlen(long_format); i < 127; i++)
        long_format[i] = 'a' + i % 26;
    long_format[127] = 0;
    for (i = 0; i < 63; i++)
        long_user[i] = 'A' + i % 26;
    long_user[63] = 0;
    had_user = GetVar((CONST_STRPTR)"USER", (STRPTR)old_user, sizeof(old_user), GVF_LOCAL_ONLY);
    SetVar((CONST_STRPTR)"USER", (CONST_STRPTR)long_user, -1, GVF_LOCAL_ONLY);
    strcpy(want_uri, long_uri);
    strcpy(want_format, long_format);
    strcpy(want_user, long_user);
    strcpy(want_path, long_uri + strlen(server_uri) - strlen("/ipp/print"));
    /* A port beyond 65535 must not wrap around to the server's. */
    {
        char wrap[64] = "ipp://127.0.0.1:", d[24];
        UQUAD v = 4294967296ULL + server_port;
        LONG n = 0;
        do {
            d[n++] = '0' + v % 10;
            v /= 10;
        } while (v);
        i = strlen(wrap);
        while (n)
            wrap[i++] = d[--n];
        wrap[i] = 0;
        strcat(wrap, "/ipp/print");
        write_unit(unit_file[1], wrap, "text/plain");
        err = print_to(DEADUNIT, DOCUMENT);
        status(st, sizeof(st));
        Printf((CONST_STRPTR)"  unit %ld to %s: error %ld, status \"%s\"\n", DEADUNIT, (IPTR)wrap, err, (IPTR)st);
        if (err == 0) {
            PutStr((CONST_STRPTR)"  expected the open to fail\n");
            ok = FALSE;
        }
    }
    write_unit(unit_file[2], long_uri, long_format);
    if (!job_ok("the longest URI, FORMAT and USER"))
        ok = FALSE;
    if (had_user >= 0)
        SetVar((CONST_STRPTR)"USER", (CONST_STRPTR)old_user, -1, GVF_LOCAL_ONLY);
    else
        DeleteVar((CONST_STRPTR)"USER", GVF_LOCAL_ONLY);

    write_unit(unit_file[1], "ipp://127.0.0.1:1/ipp/print", "text/plain");
    err = print_to(DEADUNIT, DOCUMENT);
    status(st, sizeof(st));
    Printf((CONST_STRPTR)"  unit %ld to a port nobody listens on: error %ld, status \"%s\"\n", DEADUNIT, err, (IPTR)st);
    if (err == 0) {
        PutStr((CONST_STRPTR)"  expected the open to fail\n");
        ok = FALSE;
    }

    if (args[0]) {
        LONG kb = args[1] ? *(LONG *)args[1] : 0;
        char *big = kb > 0 ? AllocMem(kb * 1024 + 1, MEMF_ANY) : NULL;

        write_unit(unit_file[0], (const char *)args[0], "text/plain");
        if (big) {
            /* SIZE: that many KB of text lines, as a large job */
            LONG i;
            for (i = 0; i < kb * 1024; i++)
                big[i] = (i % 64) == 63 ? '\n' : 'A' + (i % 26);
            big[kb * 1024] = 0;
        }
        err = print_to(URIUNIT, big ? big : "AROS ipp.device test page\n\f");
        status(st, sizeof(st));
        Printf((CONST_STRPTR)"  unit %ld to %s: error %ld, status \"%s\"\n", URIUNIT, args[0], err, (IPTR)st);
        if (err)
            ok = FALSE;
        if (big)
            FreeMem(big, kb * 1024 + 1);
    }

    for (i = 0; i < 3; i++) {
        if (had_file[i]) {
            copy_file(saved_file[i], unit_file[i]);
            DeleteFile((CONST_STRPTR)saved_file[i]);
        } else
            DeleteFile((CONST_STRPTR)unit_file[i]);
    }
    FreeArgs(rda);
    PutStr(ok ? (CONST_STRPTR)"PASS\n" : (CONST_STRPTR)"FAIL\n");
    return ok ? RETURN_OK : RETURN_FAIL;
}
