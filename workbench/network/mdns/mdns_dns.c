/*
 * SPDX-License-Identifier: BSD-2-Clause
 * Copyright 2026 amigazen project
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

LONG
mdnsNameToWire(const char *name, UBYTE *buf, LONG buflen, LONG *used)
{
    const char *start;
    const char *p;
    LONG pos;
    LONG label;

    if (name == NULL || buf == NULL || used == NULL || buflen < 2) {
        return -1;
    }

    pos = 0;
    start = name;
    p = name;

    while (*p != '\0') {
        if (*p == '.') {
            label = p - start;
            if (label <= 0 || label > 63 || pos + 1 + label > buflen) {
                return -1;
            }
            buf[pos++] = (UBYTE)label;
            memcpy(buf + pos, start, (size_t)label);
            pos += label;
            start = p + 1;
        }
        p++;
    }

    label = p - start;
    if (label > 63 || pos + 1 + label + 1 > buflen) {
        return -1;
    }
    if (label > 0) {
        buf[pos++] = (UBYTE)label;
        memcpy(buf + pos, start, (size_t)label);
        pos += label;
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
    UBYTE label;
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
        label = pkt[pos];
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
        if (out + label + 1 >= namelen) {
            return -1;
        }
        memcpy(name + out, pkt + pos + 1, (size_t)label);
        out += label;
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
    if (end < 0 || end + 4 > pktlen) {
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
    if (end < 0 || end + 10 > pktlen) {
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

    if (end + 10 + rdlen > pktlen || rdlen > (LONG)sizeof(r->rdata)) {
        return -1;
    }

    r->rdoffset = end + 10;
    memcpy(r->rdata, pkt + end + 10, (size_t)rdlen);
    *consumed = (end + 10 + rdlen) - offset;
    return 0;
}

LONG
mdnsAppendRecord(UBYTE *pkt, LONG pktmax, LONG *pos, UWORD *ancount,
    const char *name, UWORD type, UWORD rclass, ULONG ttl,
    const UBYTE *rdata, UWORD rdlength)
{
    LONG nused;
    LONG p;
    ULONG nttl;

    if (pkt == NULL || pos == NULL || ancount == NULL || name == NULL) {
        return -1;
    }

    p = *pos;
    if (mdnsNameToWire(name, pkt + p, pktmax - p, &nused) != 0) {
        return -1;
    }
    p += nused;

    if (p + 10 + rdlength > pktmax) {
        return -1;
    }

    pkt[p++] = (UBYTE)((type >> 8) & 0xFF);
    pkt[p++] = (UBYTE)(type & 0xFF);
    pkt[p++] = (UBYTE)((rclass >> 8) & 0xFF);
    pkt[p++] = (UBYTE)(rclass & 0xFF);
    nttl = ttl;
    pkt[p++] = (UBYTE)((nttl >> 24) & 0xFF);
    pkt[p++] = (UBYTE)((nttl >> 16) & 0xFF);
    pkt[p++] = (UBYTE)((nttl >> 8) & 0xFF);
    pkt[p++] = (UBYTE)(nttl & 0xFF);
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
