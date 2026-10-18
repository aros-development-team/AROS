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
#define MDNS_PROPOSED_MAX   8

/* One packet at a time is taken apart, in mdnsd's only task. */
static struct MDNSQuestion questions[MDNS_QUESTIONS_MAX];
/* the instances the asker says it already knows */
static char known[MDNS_KNOWN_MAX][256];
static struct MDNSRecord record;
/* what the sender of a probe says it is about to publish */
static struct MDNSRecord proposed[MDNS_PROPOSED_MAX];

void
mdnsHandlePacket(struct MDNSDaemon *md, const UBYTE *pkt, LONG pktlen,
    ULONG srcaddr, UWORD srcport)
{
    const struct MDNSHeader *hdr;
    struct MDNSQuestion q;
    LONG offset, consumed, nq = 0;
    UWORD i, flags, answers, authorities, additionals;

    if (pktlen < (LONG)sizeof(struct MDNSHeader)) {
        return;
    }
    /* The group's traffic may be looped back to the sender. */
    if (mdnsNetIsOwn(md, srcaddr)) {
        return;
    }

    hdr = (const struct MDNSHeader *)pkt;
    flags = ntohs(hdr->flags);
    offset = (LONG)sizeof(struct MDNSHeader);

    for (i = 0; i < ntohs(hdr->qdcount); i++) {
        if (mdnsParseQuestion(pkt, pktlen, offset, &q, &consumed) != 0) {
            return;
        }
        if (nq < MDNS_QUESTIONS_MAX) {
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
                record.ttl >= MDNS_TTL_OTHER / 2 &&
                nknown < MDNS_KNOWN_MAX &&
                mdnsWireToName(pkt, pktlen, record.rdoffset, pkt, pktlen,
                    known[nknown], (LONG)sizeof(known[nknown])) >= 0) {
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

            for (i = 0; i < ntohs(hdr->nscount); i++) {
                if (mdnsParseRecord(pkt, pktlen, offset,
                        &proposed[nproposed], &consumed) != 0) {
                    break;
                }
                offset += consumed;
                if (nproposed < MDNS_PROPOSED_MAX - 1) {
                    nproposed++;
                }
            }
            if (nproposed > 0) {
                mdnsHostHandleProbe(md, proposed, nproposed);
                mdnsRegHandleProbe(md, proposed, nproposed, pkt, pktlen);
            }
        }
        md->md_PacketSerial++;
        /* who asked, for an answer that goes back to the asker alone */
        md->md_QueryAddr = srcaddr;
        md->md_QueryPort = srcport;
        md->md_ReplyId = ntohs(hdr->id);
        for (i = 0; i < nq; i++) {
            mdnsHostHandleQuery(md, &questions[i]);
            mdnsRegHandleQuery(md, &questions[i], known, nknown);
        }
        md->md_QueryAddr = 0;
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
        mdnsHostHandleRecord(md, &record);
        if (i < answers || i >= (UWORD)(answers + authorities)) {
            mdnsRegHandleRecord(md, &record, pkt, pktlen);
        }
        offset += consumed;
    }
}
