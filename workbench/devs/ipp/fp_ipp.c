/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    FindPrinters: asks an IPP printer about itself with Get-Printer-Attributes
    (RFC 8011 section 4.2.5), encoded as RFC 8010 describes and sent as an
    HTTP/1.1 POST to the printer's ipp:// address (http on port 631).
*/

#include <exec/types.h>
#include <exec/memory.h>
#include <proto/exec.h>
#include <proto/bsdsocket.h>

#include <sys/socket.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <netdb.h>

#include <string.h>

#include "fp_ipp.h"

extern struct Library *SocketBase;

#define HTTP_TIMEOUT                10      /* seconds */
#define REPLY_MAX                   65536

#define TAG_OPERATION_ATTRIBUTES    0x01
#define TAG_END_OF_ATTRIBUTES       0x03
#define TAG_KEYWORD                 0x44
#define TAG_URI                     0x45
#define TAG_CHARSET                 0x47
#define TAG_NATURAL_LANGUAGE        0x48

#define IPP_OP_GET_PRINTER_ATTRIBUTES 0x000b

static const char *wanted[] = {
    "printer-make-and-model", "document-format-supported", "printer-state", "printer-state-reasons",
    "printer-state-message", "media-ready",
    "marker-names", "marker-levels", "marker-colors", NULL
};

/* ------------------------------------------------------------------ HTTP */

static void appendNum(char *s, ULONG v)
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

/* ipp://host[:port]/path */
static BOOL splitURI(const char *uri, char *host, LONG hostmax, UWORD *port, const char **path)
{
    const char *p, *h;
    ULONG n, v = 0;

    if (strncmp(uri, "ipp://", 6) != 0)
        return FALSE;
    *port = 631;
    h = p = uri + 6;
    while (*p && *p != ':' && *p != '/')
        p++;
    n = p - h;
    if (n == 0 || n >= (ULONG)hostmax)
        return FALSE;
    memcpy(host, h, n);
    host[n] = 0;
    if (*p == ':') {
        for (p++; *p >= '0' && *p <= '9'; p++)
            v = v * 10 + (*p - '0');
        if (v == 0 || v > 65535)
            return FALSE;
        *port = v;
    }
    *path = *p == '/' ? p : "/";
    return TRUE;
}

static LONG hexValue(const char *s)
{
    LONG v = 0;

    for (;; s++) {
        if (*s >= '0' && *s <= '9') v = v * 16 + *s - '0';
        else if (*s >= 'a' && *s <= 'f') v = v * 16 + *s - 'a' + 10;
        else if (*s >= 'A' && *s <= 'F') v = v * 16 + *s - 'A' + 10;
        else return v;
    }
}

/* Removes chunked transfer coding in place; returns the body length. */
static LONG unchunk(UBYTE *body, LONG len)
{
    LONG in = 0, out = 0;

    while (in < len) {
        LONG size = hexValue((const char *)body + in);
        while (in < len && body[in] != '\n')
            in++;
        in++;
        if (size <= 0 || in + size > len)
            break;
        memmove(body + out, body + in, size);
        out += size;
        in += size + 2;
    }
    return out;
}

/* POSTs an application/ipp request; the answer goes to buf, *body to its de-chunked body.
   Returns the HTTP status, -1 when there was no answer. */
static LONG post(const char *uri, const UBYTE *data, LONG datalen, UBYTE *buf, LONG bufmax,
    UBYTE **body, LONG *bodylen)
{
    struct sockaddr_in sin;
    struct hostent *he;
    struct timeval tv = { HTTP_TIMEOUT, 0 };
    char host[128], head[512];
    const char *path, *h;
    UWORD port;
    LONG fd, got = 0, n, status = -1, hdrend = -1, i;
    BOOL chunked = FALSE;

    *body = NULL;
    *bodylen = 0;
    if (!splitURI(uri, host, sizeof(host), &port, &path) || strlen(path) > 256)
        return -1;
    memset(&sin, 0, sizeof(sin));
    sin.sin_family = AF_INET;
    sin.sin_port = htons(port);
    sin.sin_addr.s_addr = inet_addr((STRPTR)host);
    if (sin.sin_addr.s_addr == INADDR_NONE) {
        if (!(he = gethostbyname((STRPTR)host)) || he->h_addrtype != AF_INET)
            return -1;
        memcpy(&sin.sin_addr, he->h_addr, sizeof(sin.sin_addr));
    }
    if ((fd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
        return -1;
    setsockopt(fd, SOL_SOCKET, SO_SNDTIMEO, &tv, sizeof(tv));
    setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
    if (connect(fd, (struct sockaddr *)&sin, sizeof(sin)) < 0) {
        CloseSocket(fd);
        return -1;
    }

    strcpy(head, "POST ");
    strcat(head, path);
    strcat(head, " HTTP/1.1\r\nHost: ");
    strcat(head, host);
    strcat(head, "\r\nUser-Agent: FindPrinters (AROS)\r\nConnection: close\r\n"
        "Content-Type: application/ipp\r\nContent-Length: ");
    appendNum(head, datalen);
    strcat(head, "\r\n\r\n");
    if (send(fd, head, strlen(head), 0) != (LONG)strlen(head) || send(fd, (APTR)data, datalen, 0) != datalen) {
        CloseSocket(fd);
        return -1;
    }
    while (got < bufmax - 1 && (n = recv(fd, buf + got, bufmax - 1 - got, 0)) > 0)
        got += n;
    CloseSocket(fd);
    buf[got] = 0;

    if (got < 12 || strncmp((const char *)buf, "HTTP/1.", 7) != 0)
        return -1;
    status = (buf[9] - '0') * 100 + (buf[10] - '0') * 10 + (buf[11] - '0');
    for (i = 0; i + 3 < got; i++)
        if (buf[i] == '\r' && buf[i + 1] == '\n' && buf[i + 2] == '\r' && buf[i + 3] == '\n') {
            hdrend = i;
            break;
        }
    if (hdrend < 0)
        return status;
    buf[hdrend] = 0;
    for (h = (const char *)buf; (h = strchr(h, '\n')); h++)
        if (strncasecmp(h + 1, "Transfer-Encoding:", 18) == 0) {
            const char *c = strstr(h + 19, "chunked"), *e = strchr(h + 1, '\r');
            chunked = c && (!e || c < e);
            break;
        }
    *body = buf + hdrend + 4;
    *bodylen = got - hdrend - 4;
    if (chunked)
        *bodylen = unchunk(*body, *bodylen);
    return status;
}

/* ------------------------------------------------------------------ IPP */

static UBYTE *put(UBYTE *p, UBYTE tag, const char *name, const char *value)
{
    UWORD nl = strlen(name), vl = strlen(value);

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

static void text(char *to, LONG max, const UBYTE *v, LONG len)
{
    if (len >= max)
        len = max - 1;
    memcpy(to, v, len);
    to[len] = 0;
}

/* Appends one value of a 1setOf attribute, comma separated. */
static void addValue(char *to, LONG max, const UBYTE *v, LONG len)
{
    LONG have = strlen(to);

    if (have && have + 1 < max)
        strcat(to, ",");
    have = strlen(to);
    text(to + have, max - have, v, len);
}

BOOL IPP_GetInfo(const char *uri, struct PrinterInfo *pi)
{
    UBYTE req[1024], *p = req, *buf, *body;
    LONG status, bodylen, i, names = 0, levels = 0, colours = 0;
    char attr[48] = "";

    memset(pi, 0, sizeof(*pi));
    for (i = 0; i < FP_SUPPLIES; i++)
        pi->pi_Level[i] = -1;
    if (strlen(uri) > 256)
        return FALSE;

    *p++ = 1;
    *p++ = 1;
    *p++ = IPP_OP_GET_PRINTER_ATTRIBUTES >> 8;
    *p++ = IPP_OP_GET_PRINTER_ATTRIBUTES & 0xff;
    *p++ = 0; *p++ = 0; *p++ = 0; *p++ = 1;
    *p++ = TAG_OPERATION_ATTRIBUTES;
    p = put(p, TAG_CHARSET, "attributes-charset", "utf-8");
    p = put(p, TAG_NATURAL_LANGUAGE, "attributes-natural-language", "en");
    p = put(p, TAG_URI, "printer-uri", uri);
    for (i = 0; wanted[i]; i++)
        p = put(p, TAG_KEYWORD, i == 0 ? "requested-attributes" : "", wanted[i]);
    *p++ = TAG_END_OF_ATTRIBUTES;

    if (!(buf = AllocVec(REPLY_MAX, MEMF_ANY)))
        return FALSE;
    status = post(uri, req, p - req, buf, REPLY_MAX, &body, &bodylen);
    if (status != 200 || bodylen < 9 || ((body[2] << 8) | body[3]) >= 0x0100) {
        FreeVec(buf);
        return FALSE;
    }

    /* Attribute groups: delimiter tags below 0x10; an empty name continues the last attribute. */
    for (i = 8; i < bodylen;) {
        UBYTE tag = body[i++];
        LONG nl, vl;
        const UBYTE *name, *v;

        if (tag == TAG_END_OF_ATTRIBUTES)
            break;
        if (tag < 0x10)
            continue;
        if (i + 2 > bodylen)
            break;
        nl = (body[i] << 8) | body[i + 1];
        name = body + i + 2;
        i += 2 + nl;
        if (i + 2 > bodylen)
            break;
        vl = (body[i] << 8) | body[i + 1];
        v = body + i + 2;
        i += 2 + vl;
        if (i > bodylen)
            break;
        if (nl > 0)
            text(attr, sizeof(attr), name, nl);
        if (strcmp(attr, "printer-make-and-model") == 0)
            text(pi->pi_Model, sizeof(pi->pi_Model), v, vl);
        else if (strcmp(attr, "document-format-supported") == 0)
            addValue(pi->pi_Formats, sizeof(pi->pi_Formats), v, vl);
        else if (strcmp(attr, "printer-state") == 0 && vl == 4) {
            LONG s = (v[0] << 24) | (v[1] << 16) | (v[2] << 8) | v[3];
            strcpy(pi->pi_State, s == 3 ? "idle" : s == 4 ? "processing" : s == 5 ? "stopped" : "unknown");
        } else if (strcmp(attr, "media-ready") == 0)
            addValue(pi->pi_Paper, sizeof(pi->pi_Paper), v, vl);
        else if (strcmp(attr, "printer-state-reasons") == 0)
            addValue(pi->pi_Reasons, sizeof(pi->pi_Reasons), v, vl);
        else if (strcmp(attr, "printer-state-message") == 0)
            text(pi->pi_Message, sizeof(pi->pi_Message), v, vl);
        else if (strcmp(attr, "marker-names") == 0 && names < FP_SUPPLIES)
            text(pi->pi_Supply[names++], sizeof(pi->pi_Supply[0]), v, vl);
        else if (strcmp(attr, "marker-levels") == 0 && levels < FP_SUPPLIES && vl == 4)
            pi->pi_Level[levels++] = (v[0] << 24) | (v[1] << 16) | (v[2] << 8) | v[3];
        else if (strcmp(attr, "marker-colors") == 0 && colours < FP_SUPPLIES)
            text(pi->pi_Colour[colours++], sizeof(pi->pi_Colour[0]), v, vl);
    }
    pi->pi_NumSupplies = names;
    FreeVec(buf);
    return TRUE;
}
