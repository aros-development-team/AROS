/*
 * SPDX-License-Identifier: BSD-2-Clause
 * Copyright 2026 amigazen project
 * Copyright 2026 Jaime Dias
 *
 * mdns_host.c - the host's own name
 *
 * The host's name in the mDNS domain is a record of the daemon's own: it is
 * probed for, announced, answered for, defended, and changed to "name-2",
 * "name-3" when another host has it (RFC 6762 sections 8 and 9). The name is
 * one; it is published on each link apart, and on each link by each address
 * family apart (struct MDNSPub): a link or family that comes is probed for
 * alone, addresses that change are announced again where they changed
 * (section 8.4), and the others go on answering. Each link gets the address
 * records of that link.
 */

#include <exec/types.h>
#include <string.h>

#include <proto/exec.h>
#include <proto/utility.h>
#include <netinet/in.h>

#include "mdnsd.h"

/*
 * "amiga1" becomes "amiga1.local". A host name with a unicast DNS domain
 * ("amiga1.example.org") keeps only its first label: a name answered over
 * mDNS is in the mDNS domain.
 */
void
mdnsHostFqdn(const struct MDNSDaemon *md, char *buf, LONG buflen)
{
    LONG n = 0;

    while (n < buflen - 1 && md->md_HostName[n] != '\0' &&
           md->md_HostName[n] != '.') {
        buf[n] = md->md_HostName[n];
        n++;
    }
    mdnsTextCopy(buf + n, buflen - n, ".local");
}

void
mdnsAddrData(ULONG addr, UBYTE *data)
{
    data[0] = (UBYTE)((addr >> 24) & 0xFF);
    data[1] = (UBYTE)((addr >> 16) & 0xFF);
    data[2] = (UBYTE)((addr >> 8) & 0xFF);
    data[3] = (UBYTE)(addr & 0xFF);
}

void
mdnsAddr6Data(const struct in6_addr *addr, UBYTE *data)
{
    memcpy(data, addr, 16);
}

BOOL
mdnsReplyIsUnicast(const struct MDNSDaemon *md)
{
    /* md_ReplyAddr fallback keeps host fixtures for the IPv4-only daemon
     * source-compatible while the family field is private. */
    return md->md_ReplyFamily != 0 || md->md_ReplyAddr != 0;
}

/*
 * Starts a response in pkt and returns where its records begin. An answer
 * for a resolver that speaks plain DNS repeats the ID and the question it
 * sent (RFC 6762 section 6.7).
 */
LONG
mdnsResponseBegin(struct MDNSDaemon *md, UBYTE *pkt)
{
    struct MDNSHeader *hdr = (struct MDNSHeader *)pkt;
    LONG pos;

    if (mdnsReplyIsUnicast(md) && md->md_ReplyLegacy &&
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

static BOOL
append_addresses(struct MDNSDaemon *md, LONG index, UBYTE *pkt, LONG max,
    LONG *pos, UWORD *count, const char *owner, UWORD qtype, UWORD rclass,
    ULONG ttl)
{
    static const UWORD families[2] = {
        DNSSD_ADDRESS_IPV4, DNSSD_ADDRESS_IPV6
    };
    static const UWORD types[2] = { DNS_TYPE_A, DNS_TYPE_AAAA };
    size_t family;

    for (family = 0; family < 2u; ++family) {
        size_t ordinal;

        if (qtype != DNS_TYPE_ANY && qtype != types[family])
            continue;
        for (ordinal = 0u; ; ++ordinal) {
            const struct dnssd_address_entry *address =
                mdnsNetAddressAt(md, index, families[family], ordinal);

            if (address == NULL)
                break;
            if (mdnsAppendRecord(pkt, max, pos, count, owner, types[family],
                    rclass, ttl, address->value.bytes,
                    address->value.length) != 0)
                return FALSE;
        }
    }
    return TRUE;
}

static void host_forget(struct MDNSDaemon *md);

/* The host's publication of a link (md_Ifs[] slot) in a family, or NULL. */
static struct MDNSFamilyPub *
host_family(struct MDNSDaemon *md, LONG index, UWORD fam)
{
    struct MDNSPub *pub;

    if (index < 0 || index >= md->md_IfCount) {
        return NULL;
    }
    pub = mdnsPubFind(md->md_HostPubs, md->md_HostPubCount,
        md->md_Ifs[index].mi_Index);
    return pub != NULL ? &pub->p_Family[fam] : NULL;
}

/* The records went to the group on this link by af (AF_INET/AF_INET6; 0:
 * both): the second they may not go again in is counted from now (RFC 6762
 * section 6), and an asker may be answered alone a while (section 5.4). */
static void
host_multicast(struct MDNSDaemon *md, LONG index, UWORD af)
{
    UWORD fam;

    for (fam = 0; fam < MDNS_FAMS; fam++) {
        struct MDNSFamilyPub *fp = host_family(md, index, fam);

        if (fp != NULL && (af == 0 || mdnsFamOf(af) == fam)) {
            fp->fp_Multicast = mdnsTicks() | 1;
        }
    }
}

/* Asks the link, by one family, whether anyone has the name, with the
 * address records this host would publish there in the authority section
 * (RFC 6762 section 8.1); the first probe asks for a unicast answer. */
static void
host_probe_on(struct MDNSDaemon *md, LONG index, UWORD fam, BOOL first)
{
    UBYTE *pkt = md->md_TxBuf;
    struct MDNSHeader *hdr = (struct MDNSHeader *)pkt;
    char fqdn[256];
    LONG pos;
    UWORD nscount = 0;

    mdnsHostFqdn(md, fqdn, (LONG)sizeof(fqdn));
    if (mdnsBuildQuery(pkt, (LONG)MDNS_MAX_PACKET, 0, fqdn, DNS_TYPE_ANY,
            (UWORD)(DNS_CLASS_IN | (first ? DNS_QU_BIT : 0)), &pos) != 0 ||
        !append_addresses(md, index, pkt, (LONG)MDNS_MAX_PACKET, &pos,
            &nscount, fqdn, DNS_TYPE_ANY, DNS_CLASS_IN, MDNS_TTL_HOST)) {
        return;
    }
    hdr->nscount = htons(nscount);
    mdnsNetSendOnFamily(md, index, fam, pkt, pos);
}

/* The link's address records to the group by one family, with the
 * cache-flush bit; as a goodbye with no time to live (section 10.1). The
 * host's own cache gets what the group gets. */
static void
host_announce_on(struct MDNSDaemon *md, LONG index, UWORD fam, BOOL goodbye)
{
    UBYTE *pkt = md->md_TxBuf;
    struct MDNSHeader *hdr = (struct MDNSHeader *)pkt;
    char fqdn[256];
    LONG pos;
    UWORD ancount = 0;

    mdnsHostFqdn(md, fqdn, (LONG)sizeof(fqdn));
    pos = mdnsResponseBegin(md, pkt);
    if (!append_addresses(md, index, pkt, (LONG)MDNS_MAX_PACKET, &pos,
            &ancount, fqdn, DNS_TYPE_ANY, DNS_CLASS_IN_FLUSH,
            goodbye ? 0 : MDNS_TTL_HOST) || ancount == 0) {
        return;
    }
    hdr->ancount = htons(ancount);
    if (mdnsNetSendOnFamily(md, index, fam, pkt, pos) && !goodbye) {
        host_multicast(md, index, fam == MDNS_FAM_IPV6 ? AF_INET6 : AF_INET);
    }
    mdnsReflectPacket(md, pkt, pos, index);
}

/* Addresses a link lost with a family (dnssd_link.h, FAMILY_DOWN): their
 * records cannot go by that family any more; a goodbye for each by the
 * family the link keeps reaches whoever listens there (RFC 6762 section
 * 10.1). Without the cache-flush bit: the link's other records stay. */
static void
host_goodbye_lost(struct MDNSDaemon *md, LONG index, UWORD by,
    const struct dnssd_link_change *c)
{
    UBYTE *pkt = md->md_TxBuf;
    struct MDNSHeader *hdr = (struct MDNSHeader *)pkt;
    char fqdn[256];
    LONG pos;
    UWORD ancount = 0;

    mdnsHostFqdn(md, fqdn, (LONG)sizeof(fqdn));
    pos = mdnsResponseBegin(md, pkt);
    if (mdnsAppendRecord(pkt, (LONG)MDNS_MAX_PACKET, &pos, &ancount, fqdn,
            c->family == DNSSD_ADDRESS_IPV4 ? DNS_TYPE_A : DNS_TYPE_AAAA,
            DNS_CLASS_IN, 0, c->bytes,
            c->family == DNSSD_ADDRESS_IPV4 ? 4 : 16) != 0) {
        return;
    }
    hdr->ancount = htons(ancount);
    mdnsNetSendOnFamily(md, index, by, pkt, pos);
}

/* The host's records in its own cache as they are now: those of the old
 * addresses go, those of the links and families that answer come back. */
static void
host_cache_refresh(struct MDNSDaemon *md)
{
    UBYTE *pkt = md->md_TxBuf;
    struct MDNSHeader *hdr = (struct MDNSHeader *)pkt;
    char fqdn[256];
    UWORD i;

    host_forget(md);
    mdnsHostFqdn(md, fqdn, (LONG)sizeof(fqdn));
    for (i = 0; i < md->md_HostPubCount; i++) {
        const struct MDNSPub *pub = &md->md_HostPubs[i];
        LONG index = mdnsNetSlot(md, pub->p_Link), pos;
        UWORD ancount = 0;

        if (index < 0 || (!mdnsFamilyAnswers(&pub->p_Family[MDNS_FAM_IPV4]) &&
                          !mdnsFamilyAnswers(&pub->p_Family[MDNS_FAM_IPV6]))) {
            continue;
        }
        pos = mdnsResponseBegin(md, pkt);
        if (append_addresses(md, index, pkt, (LONG)MDNS_MAX_PACKET, &pos,
                &ancount, fqdn, DNS_TYPE_ANY, DNS_CLASS_IN_FLUSH,
                MDNS_TTL_HOST) && ancount != 0) {
            hdr->ancount = htons(ancount);
            mdnsReflectPacket(md, pkt, pos, index);
        }
    }
}

/*
 * The answer to a question for the host's address records, on the link it
 * came by (md_SendIf): to the group, or to the asker alone (md_Reply*). A
 * resolver that speaks plain DNS keeps no mDNS cache: ten seconds at most,
 * and no cache-flush bit, which it would read as another class.
 */
static void
host_answer(struct MDNSDaemon *md, UWORD qtype)
{
    UBYTE *pkt = md->md_TxBuf;
    struct MDNSHeader *hdr = (struct MDNSHeader *)pkt;
    BOOL legacy = mdnsReplyIsUnicast(md) && md->md_ReplyLegacy;
    LONG index = md->md_SendIf;
    char fqdn[256];
    LONG pos;
    UWORD ancount = 0;

    mdnsHostFqdn(md, fqdn, (LONG)sizeof(fqdn));
    pos = mdnsResponseBegin(md, pkt);
    if (!append_addresses(md, index, pkt, (LONG)MDNS_MAX_PACKET, &pos,
            &ancount, fqdn, qtype,
            (UWORD)(legacy ? DNS_CLASS_IN : DNS_CLASS_IN_FLUSH),
            legacy ? 10 : MDNS_TTL_HOST) || ancount == 0) {
        return;
    }
    hdr->ancount = htons(ancount);
    mdnsNetSendOn(md, index, pkt, pos);
    if (!mdnsReplyIsUnicast(md)) {
        host_multicast(md, index, md->md_SendFamily);
        /* what the group has, the host's own cache has too: a lookup of
         * its own name is answered as any other's */
        mdnsReflectPacket(md, pkt, pos, index);
    }
}

/*
 * The host's records under the name it has now leave its cache. They are a
 * copy of what it announced (host_announce_on()), there for its own programs,
 * and end only with a goodbye; so whatever makes them untrue drops them, and
 * only two things do - both here: the name changes (mdnsHostSetName()), or
 * the links or their addresses do (mdnsHostLinksChanged(), which puts back
 * what is true now). A link that goes takes its records with it in the
 * cache's turn.
 */
static void
host_forget(struct MDNSDaemon *md)
{
    char fqdn[256];
    UBYTE wire[256];
    LONG used;

    if (md->md_HostName[0] == '\0') {
        return;                         /* no name yet */
    }
    mdnsHostFqdn(md, fqdn, (LONG)sizeof(fqdn));
    if (mdnsNameToWire(fqdn, wire, (LONG)sizeof(wire), &used) == 0) {
        mdnsCacheDropLocal(md, wire, NULL);
    }
}

/*
 * The name the host goes by: what the system calls itself (base; NULL keeps
 * it) and the number it took after other hosts had the name (suffix, 0 for
 * none: "aros", "aros-2"). The only place md_HostName changes; what the host
 * said under the name it had goes from its cache first - another host has
 * that name now, or the host gives it up.
 */
void
mdnsHostSetName(struct MDNSDaemon *md, const char *base, ULONG suffix)
{
    char digits[12];
    ULONG number = suffix;
    LONG n = 0, len;

    host_forget(md);
    if (base != NULL) {
        mdnsTextCopy(md->md_HostBase, (LONG)sizeof(md->md_HostBase), base);
    }
    md->md_HostSuffix = suffix;
    mdnsTextCopy(md->md_HostName, (LONG)sizeof(md->md_HostName),
        md->md_HostBase);
    if (number == 0) {
        return;
    }
    do {
        digits[n++] = (char)('0' + number % 10);
        number /= 10;
    } while (number != 0);
    /* a label has at most 63 characters */
    len = mdnsTextLength(md->md_HostName, MDNSD_HOST_MAX);
    if (len > 63 - 1 - n) {
        len = 63 - 1 - n;
    }
    md->md_HostName[len++] = '-';
    while (n > 0) {
        md->md_HostName[len++] = digits[--n];
    }
    md->md_HostName[len] = '\0';
}

/* Every link's every usable family probes for the name from the start,
 * due at next: the name is new, or the links are not known any more. */
static void
host_restart_all(struct MDNSDaemon *md, ULONG next)
{
    LONG i;
    UWORD fam;

    md->md_HostPubCount = 0;
    for (i = 0; i < md->md_IfCount; i++) {
        struct MDNSPub *pub = mdnsPubAdd(&md->md_HostPubs,
            &md->md_HostPubCount, &md->md_HostPubMax, md->md_Ifs[i].mi_Index);

        md->md_HostPubsShort |= pub == NULL;
        for (fam = 0; pub != NULL && fam < MDNS_FAMS; fam++) {
            if (mdnsNetHasFamily(md, i, fam)) {
                mdnsFamilyStart(&pub->p_Family[fam], next);
            }
        }
    }
}

/*
 * The links changed (mdnsNetRescan()), as the inventory tells per link and
 * family:
 * - a family that came on a link is probed for and announced there alone
 *   (RFC 6762 section 8); the name answers on as before everywhere else;
 * - an address added or removed where its family stays: the link's address
 *   records are announced again, by each family that answers there, with
 *   the cache-flush bit, which takes the old ones from the caches (section
 *   8.4: no probing, no goodbye for a unique record);
 * - a family a link lost: a goodbye for each of its addresses by the family
 *   the link keeps, if it answers there; by the lost one nothing can go any
 *   more, and a listener that has only it keeps them until they expire;
 * - a link that went: its publication goes.
 * When more changed than the list holds, the name is probed for on every
 * link again, as on a start.
 */
void
mdnsHostLinksChanged(struct MDNSDaemon *md,
    const struct dnssd_link_changes *changes)
{
    ULONG now = mdnsTicks();
    size_t k;
    UWORD i;

    if (changes == NULL || changes->overflow) {
        host_restart_all(md, now);
        host_cache_refresh(md);
        return;
    }
    for (k = 0; k < changes->count; k++) {
        const struct dnssd_link_change *c = &changes->items[k];
        LONG index = mdnsNetSlot(md, c->interface_index);
        UWORD fam = mdnsFamOfDnssd(c->family);
        struct MDNSPub *pub = mdnsPubFind(md->md_HostPubs,
            md->md_HostPubCount, c->interface_index);

        switch (c->kind) {
        case DNSSD_LINK_FAMILY_UP:
            if (index >= 0 && mdnsNetHasFamily(md, index, fam)) {
                pub = mdnsPubAdd(&md->md_HostPubs, &md->md_HostPubCount,
                    &md->md_HostPubMax, c->interface_index);
                md->md_HostPubsShort |= pub == NULL;
            }
            if (pub != NULL && index >= 0 && mdnsNetHasFamily(md, index, fam) &&
                pub->p_Family[fam].fp_State == MDNS_PUB_OFF) {
                mdnsFamilyStart(&pub->p_Family[fam], now);
            }
            break;
        case DNSSD_LINK_FAMILY_DOWN:
            if (pub != NULL) {
                pub->p_Family[fam].fp_State = MDNS_PUB_OFF;
            }
            break;
        case DNSSD_LINK_ADDRESS_LOST:
            /* of a family the link lost: its goodbye, by the other family
             * if it answers here */
            if (pub != NULL && index >= 0) {
                UWORD by = fam == MDNS_FAM_IPV4 ? MDNS_FAM_IPV6 : MDNS_FAM_IPV4;

                if (mdnsFamilyAnswers(&pub->p_Family[by])) {
                    host_goodbye_lost(md, index, by, c);
                }
            }
            break;
        case DNSSD_LINK_ADDRESSES:
            /* changed data, the name already the host's: announced again
             * by each family that answers on this link */
            if (pub != NULL && pub->p_Family[fam].fp_State != MDNS_PUB_OFF) {
                mdnsPubUpdate(pub, now);
            }
            break;
        }
    }
    /* a link with no family left, or gone, has no publication */
    for (i = 0; i < md->md_HostPubCount; ) {
        struct MDNSPub *pub = &md->md_HostPubs[i];

        if (mdnsNetSlot(md, pub->p_Link) < 0 ||
            (pub->p_Family[MDNS_FAM_IPV4].fp_State == MDNS_PUB_OFF &&
             pub->p_Family[MDNS_FAM_IPV6].fp_State == MDNS_PUB_OFF)) {
            mdnsPubRemove(md->md_HostPubs, &md->md_HostPubCount, pub->p_Link);
        } else {
            i++;
        }
    }
    host_cache_refresh(md);
}

/* The sockets are gone (mdnsNetClose()): nothing is published anywhere. */
void
mdnsHostLinksClear(struct MDNSDaemon *md)
{
    if (md->md_HostPubs != NULL) {
        FreeVec(md->md_HostPubs);
        md->md_HostPubs = NULL;
    }
    md->md_HostPubCount = md->md_HostPubMax = 0;
}

/* Is the name the host's on every link it publishes on, by every family
 * there: none still probes, and at least one answers? */
BOOL
mdnsHostReady(const struct MDNSDaemon *md)
{
    BOOL answers = FALSE;
    UWORD i, fam;

    for (i = 0; i < md->md_HostPubCount; i++) {
        for (fam = 0; fam < MDNS_FAMS; fam++) {
            const struct MDNSFamilyPub *fp = &md->md_HostPubs[i].p_Family[fam];

            if (fp->fp_State == MDNS_PUB_PROBING) {
                return FALSE;
            }
            answers |= mdnsFamilyAnswers(fp);
        }
    }
    return answers;
}

/* Does the name answer on this link (md_Ifs[] slot) by af (0: either)? */
BOOL
mdnsHostAnswersOn(const struct MDNSDaemon *md, LONG index, UWORD af)
{
    const struct MDNSFamilyPub *fp;

    if (af == 0) {
        const struct MDNSPub *pub = index >= 0 && index < md->md_IfCount ?
            mdnsPubFind((struct MDNSPub *)md->md_HostPubs, md->md_HostPubCount,
                md->md_Ifs[index].mi_Index) : NULL;

        return pub != NULL && mdnsPubAnswers(pub);
    }
    fp = host_family((struct MDNSDaemon *)md, index, mdnsFamOf(af));
    return fp != NULL && mdnsFamilyAnswers(fp);
}

/*
 * The system's name changed while the host runs (the stack's host name
 * signal): what was announced under the old name goes with a goodbye, and
 * the new one is asked for from the first probe on every link; its services
 * follow it once it is the host's (mdnsHostTick()). The same name changes
 * nothing.
 */
void
mdnsHostRename(struct MDNSDaemon *md, const char *base)
{
    if (strcmp(base, md->md_HostBase) == 0) {
        return;
    }
    mdnsHostGoodbye(md);
    mdnsHostSetName(md, base, 0);
    host_restart_all(md, mdnsTicks());
    md->md_HostRenamed = TRUE;
}

/* Nobody should keep this host's address under its name: before the sockets
 * go away, or the name changes - wherever, by whichever family, it was
 * announced. */
void
mdnsHostGoodbye(struct MDNSDaemon *md)
{
    UWORD i, fam;

    if (md->md_Socket < 0) {
        return;
    }
    for (i = 0; i < md->md_HostPubCount; i++) {
        struct MDNSPub *pub = &md->md_HostPubs[i];
        LONG index = mdnsNetSlot(md, pub->p_Link);

        for (fam = 0; index >= 0 && fam < MDNS_FAMS; fam++) {
            if (mdnsFamilyAnswers(&pub->p_Family[fam])) {
                host_announce_on(md, index, fam, TRUE);
                pub->p_Family[fam].fp_State = MDNS_PUB_OFF;
            }
        }
    }
}

/*
 * A publication that found no memory (mdnsPubAdd()) is made up for: every
 * family usable on a link, and not published there, is probed for from
 * now. TRUE when nothing is missing any more.
 */
static BOOL
host_complete(struct MDNSDaemon *md, ULONG now)
{
    LONG i;
    UWORD fam;

    for (i = 0; i < md->md_IfCount; i++) {
        for (fam = 0; fam < MDNS_FAMS; fam++) {
            struct MDNSPub *pub;

            if (!mdnsNetHasFamily(md, i, fam)) {
                continue;
            }
            pub = mdnsPubAdd(&md->md_HostPubs, &md->md_HostPubCount,
                &md->md_HostPubMax, md->md_Ifs[i].mi_Index);
            if (pub == NULL) {
                return FALSE;
            }
            if (pub->p_Family[fam].fp_State == MDNS_PUB_OFF) {
                mdnsFamilyStart(&pub->p_Family[fam], now);
            }
        }
    }
    return TRUE;
}

/*
 * Moves the name along on each link, by each family: three probes, then two
 * announcements; and the announcements of changed data. Returns when the
 * next of them is due (mdnsTicks(), 0 for none), for the main loop to wake
 * then. Once a new name is the host's everywhere, its services are
 * announced again with it.
 */
ULONG
mdnsHostTick(struct MDNSDaemon *md)
{
    ULONG now = mdnsTicks(), soonest = 0;
    UWORD i, fam;

    /* what found no memory, once a second: the publications that are
     * there go on meanwhile */
    if (md->md_HostPubsShort && (LONG)(now - md->md_HostPubsRetry) >= 0) {
        md->md_HostPubsShort = !host_complete(md, now);
        md->md_HostPubsRetry = now + MDNS_SECOND_TICKS;
    }
    if (md->md_HostPubsShort) {
        soonest = md->md_HostPubsRetry;
    }
    for (i = 0; i < md->md_HostPubCount; i++) {
        struct MDNSPub *pub = &md->md_HostPubs[i];
        LONG index = mdnsNetSlot(md, pub->p_Link);

        for (fam = 0; index >= 0 && fam < MDNS_FAMS; fam++) {
            switch (mdnsFamilyStep(&pub->p_Family[fam], now, &soonest)) {
            case MDNS_STEP_PROBE_FIRST:
                host_probe_on(md, index, fam, TRUE);
                break;
            case MDNS_STEP_PROBE:
                host_probe_on(md, index, fam, FALSE);
                break;
            case MDNS_STEP_ANNOUNCE:
                host_announce_on(md, index, fam, FALSE);
                break;
            }
        }
    }
    if (md->md_HostRenamed && mdnsHostReady(md)) {
        md->md_HostRenamed = FALSE;
        mdnsRegAnnounceAll(md);
    }
    return soonest;
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
    const struct MDNSFamilyPub *fp;
    UWORD saved;

    /* the name answers on the link by either family: a question by the
     * other, still probing there, is answered too - a probe of another
     * host's among them, in the name's defence (section 8.1, 9) */
    if (md->md_IfCount == 0 || !mdnsHostAnswersOn(md, md->md_SendIf, 0) ||
        (q->qtype != DNS_TYPE_ANY && q->qtype != DNS_TYPE_A &&
         q->qtype != DNS_TYPE_AAAA)) {
        return;
    }
    mdnsHostFqdn(md, fqdn, (LONG)sizeof(fqdn));
    if (Stricmp(q->qname, fqdn) != 0) {
        return;
    }
    legacy = md->md_QueryPort != MDNS_DNS_PORT;
    fp = host_family(md, md->md_SendIf, mdnsFamOf(md->md_QueryFamily));
    direct = !legacy && (q->qclass & DNS_QU_BIT) != 0 && fp != NULL &&
        fp->fp_Multicast != 0 &&
        mdnsTicks() - fp->fp_Multicast <= MDNS_UNICAST_SECONDS * 50UL;
    if (legacy || direct) {
        md->md_ReplyFamily = md->md_QueryFamily != 0 ?
            md->md_QueryFamily : AF_INET;
        md->md_ReplyAddr = md->md_QueryAddr;
        md->md_ReplyAddr6 = md->md_QueryAddr6;
        md->md_ReplyPort = legacy ? md->md_QueryPort : MDNS_DNS_PORT;
        md->md_ReplyLegacy = legacy;
        md->md_ReplyQuestion = q;
    }
    /* to the group, by the family the question came by: whoever asked
     * listens there */
    saved = md->md_SendFamily;
    md->md_SendFamily = md->md_QueryFamily;
    host_answer(md, q->qtype);
    md->md_SendFamily = saved;
    md->md_ReplyFamily = 0;
    md->md_ReplyAddr = 0;
    memset(&md->md_ReplyAddr6, 0, sizeof(md->md_ReplyAddr6));
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
 *
 * The claim is the link's, whichever family the probe came by: the other
 * host is on that segment, whatever its listeners speak. So when the name
 * already answers on the link by either family it is the host's there, and
 * there is no tie to break - the probe is a question, answered in its
 * defence (mdnsHostHandleQuery()); and a tie lost puts every family still
 * probing on the link back by a second, not the one the probe came by
 * alone. Other links are other segments, and go on.
 */
void
mdnsHostHandleProbe(struct MDNSDaemon *md, const struct MDNSRecord *theirs,
    LONG count)
{
    struct AddressRecord {
        UWORD type;
        UWORD length;
        UBYTE data[16];
    } other[MDNS_PROPOSED_MAX];
    /* every address of the link, 5 KB: not on the stack of a 68000's task.
     * Static: only mdnsd's own task calls this (handle_packet()), one call
     * at a time, and nothing of it outlives the call */
    static struct AddressRecord mine[MDNS_ADDRESS_MAX];
    char fqdn[256];
    LONG i, j, index, mineCount = 0, otherCount = 0, difference = 0;
    struct MDNSPub *pub = NULL;

    index = md->md_SendIf;
    if (index >= 0 && index < md->md_IfCount) {
        pub = mdnsPubFind(md->md_HostPubs, md->md_HostPubCount,
            md->md_Ifs[index].mi_Index);
    }
    if (pub == NULL || !mdnsPubAsked(pub)) {
        return;         /* the host's here already, or not asked yet */
    }
    mdnsHostFqdn(md, fqdn, (LONG)sizeof(fqdn));
    for (i = 0; i < 2; ++i) {
        UWORD family = i == 0 ? DNSSD_ADDRESS_IPV4 : DNSSD_ADDRESS_IPV6;
        UWORD type = i == 0 ? DNS_TYPE_A : DNS_TYPE_AAAA;
        size_t ordinal;

        for (ordinal = 0u; mineCount < MDNS_ADDRESS_MAX; ++ordinal) {
            const struct dnssd_address_entry *address =
                mdnsNetAddressAt(md, index, family, ordinal);

            if (address == NULL)
                break;
            mine[mineCount].type = type;
            mine[mineCount].length = address->value.length;
            memcpy(mine[mineCount].data, address->value.bytes,
                address->value.length);
            ++mineCount;
        }
    }
    for (i = 0; i < count; i++) {
        if (Stricmp(theirs[i].name, fqdn) == 0 &&
            (theirs[i].rclass & 0x7fff) == DNS_CLASS_IN &&
            ((theirs[i].type == DNS_TYPE_A && theirs[i].rdlength == 4) ||
             (theirs[i].type == DNS_TYPE_AAAA &&
              theirs[i].rdlength == 16)) &&
            otherCount < MDNS_PROPOSED_MAX) {
            other[otherCount].type = theirs[i].type;
            other[otherCount].length = theirs[i].rdlength;
            memcpy(other[otherCount].data, theirs[i].rdata,
                theirs[i].rdlength);
            otherCount++;
        }
    }
    /* Canonical RR ordering is class, type, then RDATA. The class is IN
     * for every record retained above. */
    for (i = 0; i < otherCount; i++) {
        for (j = i + 1; j < otherCount; j++) {
            if (other[j].type < other[i].type ||
                (other[j].type == other[i].type &&
                 memcmp(other[j].data, other[i].data,
                    other[i].length) < 0)) {
                struct AddressRecord swap;

                swap = other[i];
                other[i] = other[j];
                other[j] = swap;
            }
        }
    }
    for (i = 0; i < mineCount && i < otherCount; i++) {
        difference = (LONG)mine[i].type - (LONG)other[i].type;
        if (difference == 0) {
            difference = (LONG)memcmp(mine[i].data, other[i].data,
                mine[i].length);
        }
        if (difference != 0) {
            break;
        }
    }
    if (difference == 0) {
        difference = mineCount - otherCount;
    }
    if (otherCount > 0 && difference < 0) {
        /* every family probing on this link waits a second and probes
         * again (section 8.2) */
        mdnsPubDefer(pub, mdnsTicks() + MDNS_SECOND_TICKS);
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

    if (md->md_HostPubCount == 0 || r->ttl == 0 ||
        !((r->type == DNS_TYPE_A && r->rdlength == 4) ||
          (r->type == DNS_TYPE_AAAA && r->rdlength == 16))) {
        return;
    }
    mdnsHostFqdn(md, fqdn, (LONG)sizeof(fqdn));
    if (Stricmp(fqdn, r->name) != 0) {
        return;
    }
    if (r->type == DNS_TYPE_A) {
        addr = ((ULONG)r->rdata[0] << 24) | ((ULONG)r->rdata[1] << 16) |
               ((ULONG)r->rdata[2] << 8) | (ULONG)r->rdata[3];
        if (mdnsNetIsOwn(md, addr)) {
            return;
        }
    } else {
        struct in6_addr addr6;

        memcpy(&addr6, r->rdata, sizeof(addr6));
        if (mdnsNetIsOwn6(md, &addr6)) {
            return;
        }
    }
    /* another host has the name, on whichever link: the next one, on
     * every link (RFC 6762 sections 9 and 14); no goodbye - the other host's
     * records of the name would go with it (B68) */
    mdnsHostSetName(md, NULL,
        md->md_HostSuffix == 0 ? 2 : md->md_HostSuffix + 1);
    host_restart_all(md, mdnsTicks() + MDNS_PROBE_TICKS);
    md->md_HostRenamed = TRUE;
}
