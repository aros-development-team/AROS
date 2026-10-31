/*
 * SPDX-License-Identifier: BSD-2-Clause
 * Copyright 2026 amigazen project
 * Copyright 2026 Jaime Dias
 *
 * mdns_packet.c - takes a received packet apart
 */

#include <exec/types.h>
#include <string.h>

#include <proto/exec.h>
#include <netinet/in.h>

#include "mdnsd.h"

#define MDNS_QUESTIONS_MAX  8
#define MDNS_KNOWN_MAX      16

/* One packet at a time is taken apart, in mdnsd's only task. */
static struct MDNSQuestion questions[MDNS_QUESTIONS_MAX];
/* the instances the asker says it already knows */
static struct MDNSKnownAnswer known[MDNS_KNOWN_MAX];
static struct MDNSRecord record;
/* what the sender of a probe says it is about to publish */
static struct MDNSRecord proposed[MDNS_PROPOSED_MAX];

static void
handle_packet(struct MDNSDaemon *md, const UBYTE *pkt, LONG pktlen,
    UWORD family, ULONG srcaddr, const struct in6_addr *srcaddr6,
    UWORD srcport)
{
    const struct MDNSHeader *hdr;
    struct MDNSQuestion q;
    LONG offset, consumed, nq = 0;
    UWORD i, flags, answers, authorities, additionals;

    if (pktlen < (LONG)sizeof(struct MDNSHeader)) {
        return;
    }
    /* The group's traffic may be looped back to the sender. */
    if ((family == AF_INET && mdnsNetIsOwn(md, srcaddr)) ||
        (family == AF_INET6 && mdnsNetIsOwn6(md, srcaddr6))) {
        return;
    }

    hdr = (const struct MDNSHeader *)pkt;
    flags = ntohs(hdr->flags);
    /* mDNS uses standard queries/responses with no DNS error code. */
    if ((flags & 0x780f) != 0) {
        return;
    }
    offset = (LONG)sizeof(struct MDNSHeader);

    for (i = 0; i < ntohs(hdr->qdcount); i++) {
        if (mdnsParseQuestion(pkt, pktlen, offset, &q, &consumed) != 0) {
            return;
        }
        if (nq < MDNS_QUESTIONS_MAX &&
            ((q.qclass & 0x7fff) == DNS_CLASS_IN ||
             (q.qclass & 0x7fff) == 255)) {
            questions[nq++] = q;
        }
        offset += consumed;
    }

    if ((flags & DNS_FLAG_QR) == 0) {
        LONG nknown = 0;

        /*
         * The answers of a query are what the asker already knows (RFC
         * 6762 section 7.1): a PTR record it holds, with more than half
         * its life left, need not be sent again.
         */
        for (i = 0; i < ntohs(hdr->ancount); i++) {
            if (mdnsParseRecord(pkt, pktlen, offset, &record,
                    &consumed) != 0) {
                break;
            }
            if (record.type == DNS_TYPE_PTR &&
                (record.rclass & 0x7fff) == DNS_CLASS_IN &&
                nknown < MDNS_KNOWN_MAX &&
                mdnsWireToName(pkt, pktlen, record.rdoffset, pkt, pktlen,
                    known[nknown].target,
                    (LONG)sizeof(known[nknown].target)) >= 0) {
                mdnsTextCopy(known[nknown].owner,
                    (LONG)sizeof(known[nknown].owner), record.name);
                known[nknown].ttl = record.ttl;
                nknown++;
            }
            offset += consumed;
        }
        /*
         * The authority section of a query holds the records its sender is
         * probing for (RFC 6762 section 8.1). If this host is probing for
         * the same name at this moment, one of the two has to give way.
         */
        if (i == ntohs(hdr->ancount)) {
            LONG nproposed = 0;
            BOOL overflow = FALSE;

            for (i = 0; i < ntohs(hdr->nscount); i++) {
                if (mdnsParseRecord(pkt, pktlen, offset,
                        &record, &consumed) != 0) {
                    break;
                }
                offset += consumed;
                if (nproposed < MDNS_PROPOSED_MAX) {
                    proposed[nproposed++] = record;
                } else {
                    overflow = TRUE;
                }
            }
            if (nproposed > 0 && !overflow && i == ntohs(hdr->nscount)) {
                mdnsHostHandleProbe(md, proposed, nproposed);
                mdnsRegHandleProbe(md, proposed, nproposed, pkt, pktlen);
            }
        }
        md->md_PacketSerial++;
        /* who asked, for an answer that goes back to the asker alone */
        md->md_QueryAddr = srcaddr;
        md->md_QueryFamily = family;
        if (family == AF_INET6) {
            md->md_QueryAddr6 = *srcaddr6;
        }
        md->md_QueryPort = srcport;
        md->md_ReplyId = ntohs(hdr->id);
        for (i = 0; i < nq; i++) {
            mdnsHostHandleQuery(md, &questions[i]);
            mdnsRegHandleQuery(md, &questions[i], known, nknown);
        }
        md->md_QueryAddr = 0;
        md->md_QueryFamily = 0;
        memset(&md->md_QueryAddr6, 0, sizeof(md->md_QueryAddr6));
        md->md_QueryPort = 0;
        return;
    }

    /*
     * A response: every record in it may be about this host's name. For a
     * service's name the answers count, and the additional records, where
     * responders put the SRV, TXT and address records that go with a PTR
     * answer.
     */
    answers = ntohs(hdr->ancount);
    authorities = ntohs(hdr->nscount);
    additionals = ntohs(hdr->arcount);
    for (i = 0; i < (UWORD)(answers + authorities + additionals); i++) {
        if (mdnsParseRecord(pkt, pktlen, offset, &record, &consumed) != 0) {
            return;
        }
        if (i < answers) {
            md->md_RxAnswers++;
        }
        mdnsHostHandleRecord(md, &record);
        if (i < answers || i >= (UWORD)(answers + authorities)) {
            mdnsRegHandleRecord(md, &record, pkt, pktlen);
            if (srcport == MDNS_DNS_PORT && ntohs(hdr->id) == 0) {
                mdnsDiscoveryRecord(md, &record, pkt, pktlen, offset);
                mdnsQueryRecord(md, &record, pkt, pktlen);
            }
        }
        offset += consumed;
    }
}

void
mdnsHandlePacket(struct MDNSDaemon *md, const UBYTE *pkt, LONG pktlen,
    ULONG srcaddr, UWORD srcport)
{
    handle_packet(md, pkt, pktlen, AF_INET, srcaddr, NULL, srcport);
}

void
mdnsHandlePacket6(struct MDNSDaemon *md, const UBYTE *pkt, LONG pktlen,
    const struct in6_addr *srcaddr, UWORD srcport)
{
    if (srcaddr != NULL) {
        handle_packet(md, pkt, pktlen, AF_INET6, 0, srcaddr, srcport);
    }
}

/* Feed records produced by this daemon to its discovery/query clients.
 * Multicast loopback is deliberately ignored by mdnsHandlePacket(), so local
 * registrations otherwise exist on the wire but not in the local API view. */
void
mdnsReflectPacket(struct MDNSDaemon *md, const UBYTE *pkt, LONG pktlen,
    LONG ifindex, UWORD consumers, APTR queryHandle)
{
    const struct MDNSHeader *hdr;
    struct MDNSQuestion q;
    LONG offset, consumed, previous;
    UWORD i, flags, answers, authorities, additionals;

    if (pktlen < (LONG)sizeof(struct MDNSHeader) || ifindex < 0 ||
        ifindex >= md->md_IfCount || (consumers &
            (MDNS_REFLECT_DISCOVERY | MDNS_REFLECT_QUERY)) == 0) {
        return;
    }
    hdr = (const struct MDNSHeader *)pkt;
    flags = ntohs(hdr->flags);
    if ((flags & DNS_FLAG_QR) == 0 || (flags & 0x780f) != 0) {
        return;
    }
    offset = (LONG)sizeof(struct MDNSHeader);
    for (i = 0; i < ntohs(hdr->qdcount); i++) {
        if (mdnsParseQuestion(pkt, pktlen, offset, &q, &consumed) != 0) {
            return;
        }
        offset += consumed;
    }
    answers = ntohs(hdr->ancount);
    authorities = ntohs(hdr->nscount);
    additionals = ntohs(hdr->arcount);
    previous = md->md_SendIf;
    md->md_SendIf = ifindex;
    for (i = 0; i < (UWORD)(answers + authorities + additionals); i++) {
        if (mdnsParseRecord(pkt, pktlen, offset, &record, &consumed) != 0) {
            break;
        }
        if (i < answers || i >= (UWORD)(answers + authorities)) {
            if (consumers & MDNS_REFLECT_DISCOVERY) {
                mdnsDiscoveryRecord(md, &record, pkt, pktlen, offset);
            }
            if (consumers & MDNS_REFLECT_QUERY) {
                mdnsQueryLocalRecord(md, &record, pkt, pktlen, queryHandle);
            }
        }
        offset += consumed;
    }
    md->md_SendIf = previous;
}
