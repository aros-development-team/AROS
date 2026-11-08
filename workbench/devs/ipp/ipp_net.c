/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    The connection of a job. For ipp:// a Print-Job request (RFC 8010 encoding,
    RFC 8011 operation) goes out as an HTTP/1.1 POST of application/ipp with a
    chunked body: the request's attributes, then the document as it is written.
    For socket:// the document's bytes go out as they are.
*/

#include <aros/debug.h>

#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/bsdsocket.h>

#include <sys/socket.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <netdb.h>

#include <string.h>

#include "ipp_intern.h"

#define IPP_TIMEOUT     30      /* seconds without progress before the printer is given up */

/* RFC 8010 section 3.5: delimiter and value tags. */
#define TAG_OPERATION_ATTRIBUTES    0x01
#define TAG_END_OF_ATTRIBUTES       0x03
#define TAG_NAME_WITHOUT_LANGUAGE   0x42
#define TAG_URI                     0x45
#define TAG_CHARSET                 0x47
#define TAG_NATURAL_LANGUAGE        0x48
#define TAG_MIME_MEDIA_TYPE         0x49

#define IPP_OP_PRINT_JOB            0x0002

#define USER_MAX    64

/* A Print-Job's attributes: the fixed ones, then the URI, the user and the format at their longest. */
#define REQ_MAX     (160 + IPP_URI_MAX + USER_MAX + IPP_TEXT_MAX)

#define SocketBase (iu->iu_SocketBase)

static void setError(struct IPPUnit *iu, const char *a, const char *b)
{
    strncpy(iu->iu_Error, a, sizeof(iu->iu_Error) - 1);
    iu->iu_Error[sizeof(iu->iu_Error) - 1] = 0;
    if (b)
        strncat(iu->iu_Error, b, sizeof(iu->iu_Error) - strlen(iu->iu_Error) - 1);
}

static BOOL sendAll(struct IPPUnit *iu, const UBYTE *data, LONG len)
{
    while (len > 0) {
        LONG n = send(iu->iu_Socket, (APTR)data, len, 0);
        if (n <= 0) {
            setError(iu, "the connection to the printer broke", NULL);
            return FALSE;
        }
        data += n;
        len -= n;
    }
    return TRUE;
}

/* One HTTP chunk: its size in hex, the data, CRLF. */
static BOOL sendChunk(struct IPPUnit *iu, const UBYTE *data, LONG len)
{
    const char *hex = "0123456789abcdef";
    char size[12], rev[8];
    ULONG n = len;
    int i = 0, j = 0;

    do {
        rev[i++] = hex[n & 15];
        n >>= 4;
    } while (n);
    while (i > 0)
        size[j++] = rev[--i];
    size[j++] = '\r';
    size[j++] = '\n';
    return sendAll(iu, (UBYTE *)size, j) && sendAll(iu, data, len) && sendAll(iu, (UBYTE *)"\r\n", 2);
}

/* One attribute: value-tag, name-length, name, value-length, value. NULL when it does not fit. */
static UBYTE *putAttr(UBYTE *p, const UBYTE *end, UBYTE tag, const char *name, const char *value)
{
    UWORD nl = strlen(name), vl = strlen(value);

    if (p == NULL || end - p < 5 + nl + vl)
        return NULL;
    *p++ = tag;
    *p++ = nl >> 8;
    *p++ = nl;
    memcpy(p, name, nl);
    p += nl;
    *p++ = vl >> 8;
    *p++ = vl;
    memcpy(p, value, vl);
    return p + vl;
}

static LONG buildPrintJob(UBYTE *p, LONG size, ULONG id, const char *uri, const char *user, const char *format)
{
    UBYTE *start = p, *end = p + size;

    if (size < 10)
        return -1;
    *p++ = 1;                           /* version 1.1 */
    *p++ = 1;
    *p++ = IPP_OP_PRINT_JOB >> 8;
    *p++ = IPP_OP_PRINT_JOB & 0xff;
    *p++ = id >> 24;
    *p++ = id >> 16;
    *p++ = id >> 8;
    *p++ = id;
    *p++ = TAG_OPERATION_ATTRIBUTES;
    p = putAttr(p, end, TAG_CHARSET, "attributes-charset", "utf-8");
    p = putAttr(p, end, TAG_NATURAL_LANGUAGE, "attributes-natural-language", "en");
    p = putAttr(p, end, TAG_URI, "printer-uri", uri);
    p = putAttr(p, end, TAG_NAME_WITHOUT_LANGUAGE, "requesting-user-name", user);
    p = putAttr(p, end, TAG_NAME_WITHOUT_LANGUAGE, "job-name", "AROS print job");
    p = putAttr(p, end, TAG_MIME_MEDIA_TYPE, "document-format", format);
    if (p == NULL || p == end)
        return -1;
    *p++ = TAG_END_OF_ATTRIBUTES;
    return p - start;
}

BOOL IPP_BeginJob(struct IPPBase *IPPBase, struct IPPUnit *iu, ULONG id)
{
    struct IPPTarget *t = &iu->iu_Target;
    struct sockaddr_in sin;
    struct hostent *he;
    struct timeval tv = { IPP_TIMEOUT, 0 };
    LONG one = 1;

    iu->iu_Socket = -1;
    iu->iu_JobId = id;
    iu->iu_Started = FALSE;
    iu->iu_SocketBase = OpenLibrary("bsdsocket.library", 4);
    if (iu->iu_SocketBase == NULL) {
        setError(iu, "the network is not running", NULL);
        return FALSE;
    }

    memset(&sin, 0, sizeof(sin));
    sin.sin_family = AF_INET;
    sin.sin_port = htons(t->it_Port);
    sin.sin_addr.s_addr = inet_addr(t->it_Host);
    if (sin.sin_addr.s_addr == INADDR_NONE) {
        he = gethostbyname(t->it_Host);
        if (he == NULL || he->h_addrtype != AF_INET) {
            setError(iu, "cannot find the printer ", t->it_Host);
            IPP_Disconnect(IPPBase, iu);
            return FALSE;
        }
        memcpy(&sin.sin_addr, he->h_addr, sizeof(sin.sin_addr));
    }
    iu->iu_Socket = socket(AF_INET, SOCK_STREAM, 0);
    if (iu->iu_Socket < 0) {
        setError(iu, "no socket", NULL);
        IPP_Disconnect(IPPBase, iu);
        return FALSE;
    }
    /* A chunk is its size line, the data and CRLF: three sends that must not wait for each other's ACK */
    setsockopt(iu->iu_Socket, IPPROTO_TCP, TCP_NODELAY, &one, sizeof(one));
    setsockopt(iu->iu_Socket, SOL_SOCKET, SO_SNDTIMEO, &tv, sizeof(tv));
    setsockopt(iu->iu_Socket, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
    if (connect(iu->iu_Socket, (struct sockaddr *)&sin, sizeof(sin)) < 0) {
        setError(iu, "the printer does not answer at ", t->it_Host);
        IPP_Disconnect(IPPBase, iu);
        return FALSE;
    }
    return TRUE;
}

/* The request's head, before the first chunk of the document. */
static BOOL startJob(struct IPPBase *IPPBase, struct IPPUnit *iu)
{
    struct IPPTarget *t = &iu->iu_Target;
    char head[IPP_URI_MAX * 2 + 160], user[USER_MAX] = "AROS";
    UBYTE req[REQ_MAX];
    LONG reqlen;

    iu->iu_Started = TRUE;
    if (!t->it_IPP)
        return TRUE;
    GetVar("USER", user, sizeof(user), 0);
    reqlen = buildPrintJob(req, sizeof(req), iu->iu_JobId, iu->iu_Config.ic_URI, user, iu->iu_Config.ic_Format);
    if (reqlen < 0) {
        setError(iu, "the job's attributes do not fit the request", NULL);
        return FALSE;
    }
    strcpy(head, "POST ");
    strcat(head, t->it_Path);
    strcat(head, " HTTP/1.1\r\nHost: ");
    strcat(head, t->it_Host);
    strcat(head, ":");
    IPP_AppendNumber(head, t->it_Port);
    strcat(head, "\r\nContent-Type: application/ipp\r\nTransfer-Encoding: chunked\r\nConnection: close\r\n\r\n");
    return sendAll(iu, (UBYTE *)head, strlen(head)) && sendChunk(iu, req, reqlen);
}

BOOL IPP_Send(struct IPPBase *IPPBase, struct IPPUnit *iu, const UBYTE *data, LONG len)
{
    BOOL ok = (iu->iu_Started || startJob(IPPBase, iu)) &&
              (iu->iu_Target.it_IPP ? sendChunk(iu, data, len) : sendAll(iu, data, len));

    if (ok)
        iu->iu_Bytes += len;
    return ok;
}

/* The last chunk, then the printer's answer: HTTP 200 and an IPP status below 0x0100. */
BOOL IPP_EndJob(struct IPPBase *IPPBase, struct IPPUnit *iu)
{
    UBYTE buf[2048];
    LONG got = 0, n, body = -1, i, http = 0;
    UWORD status;

    if (!iu->iu_Target.it_IPP || !iu->iu_Started)
        return TRUE;
    if (!sendAll(iu, (UBYTE *)"0\r\n\r\n", 5))
        return FALSE;

    while (got < (LONG)sizeof(buf) - 1 && (n = recv(iu->iu_Socket, buf + got, sizeof(buf) - 1 - got, 0)) > 0) {
        got += n;
        for (i = 0; body < 0 && i + 3 < got; i++)
            if (buf[i] == '\r' && buf[i + 1] == '\n' && buf[i + 2] == '\r' && buf[i + 3] == '\n')
                body = i + 4;
        if (body >= 0 && got >= body + 4)
            break;
    }
    buf[got] = 0;
    if (got < 12 || strncmp((char *)buf, "HTTP/1.", 7) != 0) {
        setError(iu, "the printer gave no answer to the job", NULL);
        return FALSE;
    }
    for (i = 9; i < 12; i++)
        http = http * 10 + (buf[i] - '0');
    if (http != 200) {
        for (i = 0; i < got && buf[i] != '\r' && buf[i] != '\n'; i++)
            ;
        buf[i] = 0;
        setError(iu, "the printer refused the job: ", (char *)buf);
        return FALSE;
    }
    /* A chunked answer puts a size line before the IPP message. */
    if (body >= 0 && body + 4 <= got && strstr((char *)buf, "chunked")) {
        for (i = body; i + 1 < got && !(buf[i] == '\r' && buf[i + 1] == '\n'); i++)
            ;
        body = i + 2;
    }
    if (body < 0 || body + 4 > got) {
        setError(iu, "the printer's answer was cut short", NULL);
        return FALSE;
    }
    status = (buf[body + 2] << 8) | buf[body + 3];
    D(bug("[ipp] unit %lu: IPP status 0x%04x\n", (unsigned long)iu->iu_Number, status));
    if (status >= 0x0100) {
        char code[8] = "0x";
        const char *hex = "0123456789abcdef";
        code[2] = hex[(status >> 12) & 15];
        code[3] = hex[(status >> 8) & 15];
        code[4] = hex[(status >> 4) & 15];
        code[5] = hex[status & 15];
        code[6] = 0;
        setError(iu, "the printer did not take the job, IPP status ", code);
        return FALSE;
    }
    return TRUE;
}

void IPP_Disconnect(struct IPPBase *IPPBase, struct IPPUnit *iu)
{
    if (iu->iu_SocketBase == NULL)
        return;
    if (iu->iu_Socket >= 0) {
        shutdown(iu->iu_Socket, 2);
        CloseSocket(iu->iu_Socket);
        iu->iu_Socket = -1;
    }
    CloseLibrary(iu->iu_SocketBase);
    iu->iu_SocketBase = NULL;
}
