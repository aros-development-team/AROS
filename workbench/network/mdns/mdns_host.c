/*
 * SPDX-License-Identifier: BSD-2-Clause
 * Copyright 2026 amigazen project
 * Copyright 2026 Jaime Dias
 *
 * mdns_host.c - the host's own name
 *
 * The host's name in the mDNS domain is a record of the daemon's own: it is
 * probed when the network comes up, announced, answered for, defended, and
 * changed to "name-2", "name-3" when another host has it (RFC 6762 sections
 * 8 and 9). Each interface gets the address record of that interface.
 */

#include <exec/types.h>
#include <string.h>

#include <proto/exec.h>
#include <proto/utility.h>
#include <netinet/in.h>

#include "mdnsd.h"

/* a record that went to the group this many seconds ago or less may be
 * answered to the asker alone when the question allows it: a quarter of
 * its life */
#define MDNS_UNICAST_SECONDS (MDNS_TTL_HOST / 4)

/*
 * "amiga1" becomes "amiga1.local". A host name with a unicast DNS domain
 * ("amiga1.example.org") keeps only its first label: a name answered over
 * mDNS is in the mDNS domain.
 */
static void
host_fqdn(struct MDNSDaemon *md, char *buf, LONG buflen)
{
    LONG n = 0;

    while (n < buflen - 1 && md->md_HostName[n] != '\0' &&
           md->md_HostName[n] != '.') {
        buf[n] = md->md_HostName[n];
        n++;
    }
    mdnsTextCopy(buf + n, buflen - n, "." MDNS_DOMAIN_LOCAL);
}

static void
host_addr_data(ULONG addr, UBYTE *data)
{
    data[0] = (UBYTE)((addr >> 24) & 0xFF);
    data[1] = (UBYTE)((addr >> 16) & 0xFF);
    data[2] = (UBYTE)((addr >> 8) & 0xFF);
    data[3] = (UBYTE)(addr & 0xFF);
}

/*
 * Starts a response in pkt and returns where its records begin. An answer
 * for a resolver that speaks plain DNS repeats the ID and the question it
 * sent (RFC 6762 section 6.7).
 */
static LONG
response_begin(struct MDNSDaemon *md, UBYTE *pkt)
{
    struct MDNSHeader *hdr = (struct MDNSHeader *)pkt;
    LONG pos;

    if (md->md_ReplyAddr != 0 && md->md_ReplyLegacy &&
        md->md_ReplyQuestion != NULL &&
        mdnsBuildQuery(pkt, (LONG)MDNS_MAX_PACKET, md->md_ReplyId,
            md->md_ReplyQuestion->qname, md->md_ReplyQuestion->qtype,
            (UWORD)(md->md_ReplyQuestion->qclass & 0x7fff), &pos) == 0) {
        hdr->flags = htons((unsigned short)(DNS_FLAG_QR | DNS_FLAG_AA));
        return pos;
    }
    mdnsInitHeader(hdr, DNS_FLAG_QR | DNS_FLAG_AA, 0);
    return (LONG)sizeof(struct MDNSHeader);
}

/* Asks every link whether anyone has the name, with the address record this
 * host would publish there in the authority section (RFC 6762 section
 * 8.1). The first probe asks for a unicast answer. */
static void
host_send_probe(struct MDNSDaemon *md)
{
    UBYTE *pkt = md->md_TxBuf;
    struct MDNSHeader *hdr = (struct MDNSHeader *)pkt;
    char fqdn[256];
    UBYTE addrdata[4];
    LONG pos, i;
    UWORD nscount;

    host_fqdn(md, fqdn, (LONG)sizeof(fqdn));
    for (i = 0; i < md->md_IfCount; i++) {
        nscount = 0;
        if (mdnsBuildQuery(pkt, (LONG)MDNS_MAX_PACKET, 0, fqdn, DNS_TYPE_ANY,
                (UWORD)(DNS_CLASS_IN |
                    (md->md_HostStep == 0 ? DNS_QU_BIT : 0)), &pos) != 0) {
            return;
        }
        host_addr_data(md->md_Ifs[i].mi_Addr, addrdata);
        if (mdnsAppendRecord(pkt, (LONG)MDNS_MAX_PACKET, &pos, &nscount, fqdn,
                DNS_TYPE_A, DNS_CLASS_IN, MDNS_TTL_HOST, addrdata, 4) != 0) {
            return;
        }
        hdr->nscount = htons(nscount);
        mdnsNetSendOn(md, i, pkt, pos);
    }
}

/*
 * The host's address record, to the interface a question came in on or to
 * every one; as a goodbye with no time to live, which withdraws it from
 * every cache on the link (RFC 6762 section 10.1). A resolver that speaks
 * plain DNS keeps no mDNS cache: ten seconds at most, and no cache-flush
 * bit, which it would read as another class.
 */
static void
host_send_addr(struct MDNSDaemon *md, BOOL goodbye)
{
    UBYTE *pkt = md->md_TxBuf;
    struct MDNSHeader *hdr = (struct MDNSHeader *)pkt;
    BOOL legacy = md->md_ReplyAddr != 0 && md->md_ReplyLegacy;
    char fqdn[256];
    UBYTE addrdata[4];
    LONG pos, i;
    UWORD ancount;

    host_fqdn(md, fqdn, (LONG)sizeof(fqdn));
    for (i = 0; i < md->md_IfCount; i++) {
        if (md->md_SendIf >= 0 && i != md->md_SendIf) {
            continue;
        }
        pos = response_begin(md, pkt);
        ancount = 0;
        host_addr_data(md->md_Ifs[i].mi_Addr, addrdata);
        if (mdnsAppendRecord(pkt, (LONG)MDNS_MAX_PACKET, &pos, &ancount, fqdn,
                DNS_TYPE_A,
                (UWORD)(legacy ? DNS_CLASS_IN : DNS_CLASS_IN_FLUSH),
                goodbye ? 0 : (legacy ? 10 : MDNS_TTL_HOST), addrdata,
                4) != 0) {
            continue;
        }
        hdr->ancount = htons(ancount);
        mdnsNetSendOn(md, i, pkt, pos);
        if (md->md_ReplyAddr == 0 && !goodbye) {
            md->md_HostMulticast = mdnsNow();
        }
    }
}

/* Starts asking for the name, from the first probe. */
static void
host_probe_from(struct MDNSDaemon *md, ULONG ticks)
{
    md->md_HostState = MDNS_NAME_PROBING;
    md->md_HostStep = 0;
    md->md_HostNextTick = mdnsTicks() + ticks;
}

/* The network came up, or changed: the name is asked for again on what is
 * there now. */
void
mdnsHostRestart(struct MDNSDaemon *md)
{
    host_probe_from(md, 0);
}

/* Before the sockets go away: nobody should keep this host's address. */
void
mdnsHostGoodbye(struct MDNSDaemon *md)
{
    if (md->md_Socket >= 0 && md->md_HostState == MDNS_NAME_ANNOUNCED) {
        host_send_addr(md, TRUE);
    }
}

/*
 * Moves the name along: three probes, then two announcements. Returns TRUE
 * while it is being probed, so that the caller comes back in a quarter of
 * a second.
 */
BOOL
mdnsHostTick(struct MDNSDaemon *md)
{
    ULONG now = mdnsTicks();

    if (md->md_IfCount == 0) {
        return FALSE;
    }
    if (md->md_HostState == MDNS_NAME_PROBING) {
        if ((LONG)(now - md->md_HostNextTick) >= 0) {
            if (md->md_HostStep < MDNS_PROBE_COUNT) {
                host_send_probe(md);
                md->md_HostStep++;
                md->md_HostNextTick = now + MDNS_PROBE_TICKS;
            } else {
                /* Nobody objected: the name is this host's. */
                md->md_HostState = MDNS_NAME_ANNOUNCED;
                md->md_HostStep = 1;
                md->md_HostNextTick = now + 50;
                host_send_addr(md, FALSE);
            }
        }
        return md->md_HostState == MDNS_NAME_PROBING;
    }
    if (md->md_HostStep < MDNS_ANNOUNCE_COUNT &&
        (LONG)(now - md->md_HostNextTick) >= 0) {
        md->md_HostStep++;
        host_send_addr(md, FALSE);
    }
    return FALSE;
}

/*
 * One question of a received query.
 *
 * A question from another port than 5353 comes from a resolver that speaks
 * plain DNS: it gets its answer back alone (RFC 6762 section 6.7). A
 * question with the unicast-response bit is answered to the asker alone
 * when the group has had the record not long ago, and to the group
 * otherwise (section 5.4). An address record is unique to this host, so
 * the answer is not delayed.
 */
void
mdnsHostHandleQuery(struct MDNSDaemon *md, struct MDNSQuestion *q)
{
    char fqdn[256];
    BOOL legacy, direct;

    if (md->md_IfCount == 0 || md->md_HostState != MDNS_NAME_ANNOUNCED ||
        (q->qtype != DNS_TYPE_ANY && q->qtype != DNS_TYPE_A)) {
        return;
    }
    host_fqdn(md, fqdn, (LONG)sizeof(fqdn));
    if (Stricmp(q->qname, fqdn) != 0) {
        return;
    }
    legacy = md->md_QueryPort != MDNS_DNS_PORT;
    direct = !legacy && (q->qclass & DNS_QU_BIT) != 0 &&
        md->md_HostMulticast != 0 &&
        mdnsNow() - md->md_HostMulticast <= MDNS_UNICAST_SECONDS;
    if (legacy || direct) {
        md->md_ReplyAddr = md->md_QueryAddr;
        md->md_ReplyPort = legacy ? md->md_QueryPort : MDNS_DNS_PORT;
        md->md_ReplyLegacy = legacy;
        md->md_ReplyQuestion = q;
    }
    host_send_addr(md, FALSE);
    md->md_ReplyAddr = 0;
    md->md_ReplyQuestion = NULL;
}

/*
 * Another host is probing, and these are the records it proposes. If it
 * proposes an address for the name this host is probing for at this
 * moment, one of the two has to give way: the records are compared as
 * bytes, and the host with the later one keeps going. The other waits a
 * second and probes again, by which time the winner has announced and the
 * usual conflict gives the loser another name (RFC 6762 section 8.2).
 *
 * The record of this host is the address of the interface the probe came
 * in on. An equal record is this same host seen from somewhere else.
 */
void
mdnsHostHandleProbe(struct MDNSDaemon *md, const struct MDNSRecord *theirs,
    LONG count)
{
    char fqdn[256];
    UBYTE mine[4];
    LONG i, index;

    if (md->md_HostState != MDNS_NAME_PROBING || md->md_HostStep == 0 ||
        md->md_IfCount == 0) {
        return;         /* not probing, or has not asked yet */
    }
    index = md->md_SendIf >= 0 ? md->md_SendIf : 0;
    host_fqdn(md, fqdn, (LONG)sizeof(fqdn));
    host_addr_data(md->md_Ifs[index].mi_Addr, mine);
    for (i = 0; i < count; i++) {
        /* same class and type, so the data decides */
        if (Stricmp(theirs[i].name, fqdn) == 0 &&
            theirs[i].type == DNS_TYPE_A &&
            (theirs[i].rclass & 0x7fff) == DNS_CLASS_IN &&
            theirs[i].rdlength == 4 &&
            memcmp(mine, theirs[i].rdata, 4) < 0) {
            host_probe_from(md, 50);
            return;
        }
    }
}

/*
 * A record in a response from another host. An address record under this
 * host's name whose address is not one of this host's says somebody else
 * has the name, whether this host is still asking for it or has announced
 * it already: this host takes the next name and asks for that.
 */
void
mdnsHostHandleRecord(struct MDNSDaemon *md, const struct MDNSRecord *r)
{
    char fqdn[256];
    ULONG addr;

    if (md->md_IfCount == 0 || r->type != DNS_TYPE_A || r->rdlength != 4 ||
        r->ttl == 0) {
        return;
    }
    host_fqdn(md, fqdn, (LONG)sizeof(fqdn));
    if (Stricmp(fqdn, r->name) != 0) {
        return;
    }
    addr = ((ULONG)r->rdata[0] << 24) | ((ULONG)r->rdata[1] << 16) |
           ((ULONG)r->rdata[2] << 8) | (ULONG)r->rdata[3];
    if (mdnsNetIsOwn(md, addr)) {
        return;
    }
    md->md_HostSuffix = md->md_HostSuffix == 0 ? 2 : md->md_HostSuffix + 1;
    mdnsNetNameHost(md);
    host_probe_from(md, MDNS_PROBE_TICKS);
}
