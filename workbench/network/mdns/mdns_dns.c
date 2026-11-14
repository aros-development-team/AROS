/*
 * SPDX-License-Identifier: BSD-2-Clause
 * Copyright 2026 amigazen project
 * Copyright 2026 Jaime Dias
 *
 * mdns_dns.c - DNS wire encode/decode (C89)
 */

#include <exec/types.h>
#include <string.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "mdns_dns.h"

void
mdnsInitHeader(struct MDNSHeader *hdr, UWORD flags, UWORD qdcount)
{
    hdr->id = 0;
    hdr->flags = htons(flags);
    hdr->qdcount = htons(qdcount);
    hdr->ancount = 0;
    hdr->nscount = 0;
    hdr->arcount = 0;
}

/*
 * A name as text - labels between dots, "\." a dot and "\\" a backslash
 * inside a label (RFC 1035 section 5.1), so that an instance's name may
 * hold dots - as wire labels. A trailing dot is allowed.
 */
LONG
mdnsNameToWire(const char *name, UBYTE *buf, LONG buflen, LONG *used)
{
    const char *p = name;
    LONG pos = 0;

    if (name == NULL || buf == NULL || used == NULL || buflen < 2) {
        return -1;
    }
    while (*p != '\0') {
        LONG start = pos++;

        while (*p != '\0' && *p != '.') {
            if (*p == '\\' && p[1] != '\0') {
                p++;
            }
            if (pos - start > 63 || pos + 2 > buflen) {
                return -1;
            }
            buf[pos++] = (UBYTE)*p++;
        }
        if (pos - start == 1) {
            return -1;                  /* an empty label */
        }
        buf[start] = (UBYTE)(pos - start - 1);
        if (*p == '.') {
            p++;
        }
    }
    if (pos + 1 > buflen) {
        return -1;
    }
    buf[pos++] = 0;
    *used = pos;
    return 0;
}

LONG
mdnsSkipName(const UBYTE *pkt, LONG pktlen, LONG offset)
{
    LONG pos;

    if (pkt == NULL || offset < 0 || offset >= pktlen) {
        return -1;
    }

    pos = offset;
    while (pos < pktlen) {
        UBYTE label = pkt[pos];

        if (label == 0) {
            return pos + 1;
        }
        if ((label & 0xC0) == 0xC0) {
            if (pos + 1 >= pktlen) {
                return -1;
            }
            return pos + 2;
        }
        if (label > 63 || pos + 1 + label >= pktlen) {
            return -1;
        }
        pos += 1 + label;
    }
    return -1;
}

LONG
mdnsWireToName(const UBYTE *pkt, LONG pktlen, LONG offset,
    const UBYTE *pktbase, LONG baselen, char *name, LONG namelen)
{
    LONG pos;
    LONG out;
    LONG jumps;
    LONG nextpos;

    if (pkt == NULL || name == NULL || namelen <= 0) {
        return -1;
    }

    pos = offset;
    out = 0;
    jumps = 0;
    nextpos = -1;

    while (pos >= 0 && pos < pktlen && jumps < 16) {
        UBYTE label = pkt[pos];

        if (label == 0) {
            if (out > 0) {
                name[out - 1] = '\0';
            } else {
                name[0] = '\0';
            }
            if (nextpos >= 0) {
                return nextpos;
            }
            return pos + 1;
        }
        if ((label & 0xC0) == 0xC0) {
            if (pos + 1 >= pktlen) {
                return -1;
            }
            LONG jump = ((label & 0x3F) << 8) | pkt[pos + 1];

            if (jump < 0 || jump >= baselen) {
                return -1;
            }
            if (nextpos < 0) {
                nextpos = pos + 2;
            }
            pos = jump;
            jumps++;
            continue;
        }
        if (label > 63 || pos + 1 + label >= pktlen) {
            return -1;
        }
        {
            LONG k;

            /* a dot or backslash in a label is escaped */
            for (k = 0; k < label; k++) {
                UBYTE c = pkt[pos + 1 + k];

                if (out + 3 >= namelen) {
                    return -1;
                }
                if (c == '.' || c == '\\') {
                    name[out++] = '\\';
                }
                name[out++] = (char)c;
            }
        }
        name[out++] = '.';
        pos += 1 + label;
    }
    return -1;
}

LONG
mdnsBuildQuery(UBYTE *pkt, LONG pktmax, UWORD id, const char *qname,
    UWORD qtype, UWORD qclass, LONG *pktlen)
{
    struct MDNSHeader *hdr;
    LONG pos;
    LONG nused;
    UWORD ntype;
    UWORD nclass;

    if (pkt == NULL || qname == NULL || pktlen == NULL || pktmax < 512) {
        return -1;
    }

    hdr = (struct MDNSHeader *)pkt;
    mdnsInitHeader(hdr, 0, 1);
    hdr->id = htons(id);

    pos = (LONG)sizeof(struct MDNSHeader);
    if (mdnsNameToWire(qname, pkt + pos, pktmax - pos, &nused) != 0) {
        return -1;
    }
    pos += nused;

    if (pos + 4 > pktmax) {
        return -1;
    }

    ntype = qtype;
    nclass = qclass;
    pkt[pos++] = (UBYTE)((ntype >> 8) & 0xFF);
    pkt[pos++] = (UBYTE)(ntype & 0xFF);
    pkt[pos++] = (UBYTE)((nclass >> 8) & 0xFF);
    pkt[pos++] = (UBYTE)(nclass & 0xFF);

    *pktlen = pos;
    return 0;
}

/* ------------------------------------------------------------------
 * Wire names: labels as they are on the wire, uncompressed, ended by the
 * root label. What mdnsd keeps and compares (dnssd_reg.h); text is for
 * presentation.
 * ------------------------------------------------------------------ */
LONG
mdnsWireNameLength(const UBYTE *name)
{
    LONG at = 0;

    while (at < 255 && name[at] != 0) {
        at += name[at] + 1;
    }
    return at < 255 ? at + 1 : -1;
}

static UBYTE
wire_lower(UBYTE c)
{
    return c >= 'A' && c <= 'Z' ? (UBYTE)(c + 32) : c;
}

BOOL
mdnsWireNameEqual(const UBYTE *a, const UBYTE *b)
{
    LONG la = mdnsWireNameLength(a), lb = mdnsWireNameLength(b), i;

    if (la < 0 || la != lb) {
        return FALSE;
    }
    for (i = 0; i < la; i++) {
        if (wire_lower(a[i]) != wire_lower(b[i])) {
            return FALSE;
        }
    }
    return TRUE;
}

/* A name at offset of pkt, compressed or not, into name as wire labels;
 * the bytes it takes at offset, or -1. */
LONG
mdnsWireNameRead(const UBYTE *pkt, LONG length, LONG offset, UBYTE *name)
{
    LONG at = 0, consumed = 0, jumps = 0;
    BOOL jumped = FALSE;

    while (offset >= 0 && offset < length && jumps++ < 128) {
        UBYTE n = pkt[offset++];

        if (!jumped) {
            consumed++;
        }
        if ((n & 0xc0) == 0xc0) {
            if (offset >= length) {
                return -1;
            }
            if (!jumped) {
                consumed++;
            }
            offset = ((n & 0x3f) << 8) | pkt[offset];
            jumped = TRUE;
        } else {
            if (n > 63 || at + n + 1 > 255 || offset + n > length) {
                return -1;
            }
            name[at++] = n;
            if (n == 0) {
                return consumed;
            }
            memcpy(name + at, pkt + offset, n);
            at += n;
            offset += n;
            if (!jumped) {
                consumed += n;
            }
        }
    }
    return -1;
}

LONG
mdnsParseQuestion(const UBYTE *pkt, LONG pktlen, LONG offset,
    struct MDNSQuestion *q, LONG *consumed)
{
    LONG end;

    if (pkt == NULL || q == NULL || consumed == NULL) {
        return -1;
    }

    end = mdnsWireToName(pkt, pktlen, offset, pkt, pktlen, q->qname,
        (LONG)sizeof(q->qname));
    if (end < 0 || end + 4 > pktlen ||
        mdnsWireNameRead(pkt, pktlen, offset, q->qwire) < 0) {
        return -1;
    }

    q->qtype = (UWORD)((pkt[end] << 8) | pkt[end + 1]);
    q->qclass = (UWORD)((pkt[end + 2] << 8) | pkt[end + 3]);
    *consumed = (end + 4) - offset;
    return 0;
}

LONG
mdnsParseRecord(const UBYTE *pkt, LONG pktlen, LONG offset,
    struct MDNSRecord *r, LONG *consumed)
{
    LONG end;
    LONG rdlen;

    if (pkt == NULL || r == NULL || consumed == NULL) {
        return -1;
    }

    end = mdnsWireToName(pkt, pktlen, offset, pkt, pktlen, r->name,
        (LONG)sizeof(r->name));
    if (end < 0 || end + 10 > pktlen ||
        mdnsWireNameRead(pkt, pktlen, offset, r->wire) < 0) {
        return -1;
    }

    r->type = (UWORD)((pkt[end] << 8) | pkt[end + 1]);
    r->rclass = (UWORD)((pkt[end + 2] << 8) | pkt[end + 3]);
    r->ttl = (ULONG)pkt[end + 4] << 24;
    r->ttl |= (ULONG)pkt[end + 5] << 16;
    r->ttl |= (ULONG)pkt[end + 6] << 8;
    r->ttl |= (ULONG)pkt[end + 7];
    r->rdlength = (UWORD)((pkt[end + 8] << 8) | pkt[end + 9]);
    rdlen = (LONG)r->rdlength;

    if (end + 10 + rdlen > pktlen) {
        return -1;
    }

    /* a record longer than rdata (a TXT may be 8900 bytes, RFC 6763
     * section 6.2) is taken all the same: rdata holds its start, the
     * whole is at rdoffset in the packet */
    r->rdoffset = end + 10;
    memcpy(r->rdata, pkt + end + 10,
        (size_t)(rdlen < (LONG)sizeof(r->rdata) ? rdlen : (LONG)sizeof(r->rdata)));
    *consumed = (end + 10 + rdlen) - offset;
    return 0;
}

/* A record whose owner is a wire name (name, mdnsWireNameLength() bytes),
 * at *pos of pkt (pktmax bytes); -1 if it does not fit. */
LONG
mdnsAppendRecordWire(UBYTE *pkt, LONG pktmax, LONG *pos, UWORD *ancount,
    const UBYTE *name, UWORD type, UWORD rclass, ULONG ttl,
    const UBYTE *rdata, UWORD rdlength)
{
    LONG nlen, p;

    if (pkt == NULL || pos == NULL || ancount == NULL || name == NULL ||
        (nlen = mdnsWireNameLength(name)) < 0) {
        return -1;
    }
    p = *pos;
    if (p + nlen + 10 + rdlength > pktmax) {
        return -1;
    }
    memcpy(pkt + p, name, (size_t)nlen);
    p += nlen;
    pkt[p++] = (UBYTE)((type >> 8) & 0xFF);
    pkt[p++] = (UBYTE)(type & 0xFF);
    pkt[p++] = (UBYTE)((rclass >> 8) & 0xFF);
    pkt[p++] = (UBYTE)(rclass & 0xFF);
    pkt[p++] = (UBYTE)((ttl >> 24) & 0xFF);
    pkt[p++] = (UBYTE)((ttl >> 16) & 0xFF);
    pkt[p++] = (UBYTE)((ttl >> 8) & 0xFF);
    pkt[p++] = (UBYTE)(ttl & 0xFF);
    pkt[p++] = (UBYTE)((rdlength >> 8) & 0xFF);
    pkt[p++] = (UBYTE)(rdlength & 0xFF);
    if (rdlength > 0 && rdata != NULL) {
        memcpy(pkt + p, rdata, (size_t)rdlength);
    }
    p += rdlength;
    *pos = p;
    (*ancount)++;
    return 0;
}

/* The same, with the owner as text (the host's name, mdns_host.c). */
LONG
mdnsAppendRecord(UBYTE *pkt, LONG pktmax, LONG *pos, UWORD *ancount,
    const char *name, UWORD type, UWORD rclass, ULONG ttl,
    const UBYTE *rdata, UWORD rdlength)
{
    UBYTE wire[256];
    LONG nused;

    if (name == NULL || mdnsNameToWire(name, wire, (LONG)sizeof(wire), &nused) != 0) {
        return -1;
    }
    return mdnsAppendRecordWire(pkt, pktmax, pos, ancount, wire, type, rclass,
        ttl, rdata, rdlength);
}

/* A query with one question, its name a wire name; its length at *pktlen. */
LONG
mdnsBuildQueryWire(UBYTE *pkt, LONG pktmax, UWORD id, const UBYTE *qname,
    UWORD qtype, UWORD qclass, LONG *pktlen)
{
    LONG pos = (LONG)sizeof(struct MDNSHeader), nlen;

    if (pkt == NULL || qname == NULL || pktlen == NULL ||
        (nlen = mdnsWireNameLength(qname)) < 0 || pos + nlen + 4 > pktmax) {
        return -1;
    }
    mdnsInitHeader((struct MDNSHeader *)pkt, 0, 1);
    ((struct MDNSHeader *)pkt)->id = htons(id);
    memcpy(pkt + pos, qname, (size_t)nlen);
    pos += nlen;
    pkt[pos++] = (UBYTE)(qtype >> 8);
    pkt[pos++] = (UBYTE)qtype;
    pkt[pos++] = (UBYTE)(qclass >> 8);
    pkt[pos++] = (UBYTE)qclass;
    *pktlen = pos;
    return 0;
}

