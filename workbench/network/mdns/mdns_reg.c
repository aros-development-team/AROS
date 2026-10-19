/*
 * SPDX-License-Identifier: BSD-2-Clause
 * Copyright 2026 amigazen project
 * Copyright 2026 Jaime Dias
 *
 * mdns_reg.c - the services clients registered
 *
 * A service is an instance name under a type, "Printer._ipp._tcp.local",
 * with four records (RFC 6763): a PTR from the type to the instance, the
 * instance's SRV with the host and port and its TXT, and the address of
 * the host. The instance's name is probed and announced as the host's is,
 * and changed to "Name (2)", "Name (3)" when another host has it.
 */

#include <exec/types.h>
#include <exec/memory.h>
#include <exec/lists.h>
#include <string.h>

#include <proto/exec.h>
#include <proto/utility.h>
#include <netinet/in.h>

#include "mdnsd.h"

/* a response stays within one Ethernet frame */
#define MDNS_RESPONSE_MAX   1400L

#define MDNS_TYPE_SERVICES  "_services._dns-sd._udp." MDNS_DOMAIN_LOCAL

#define FOR_SERVICES(md, n) \
    for ((n) = (md)->md_Services.lh_Head; (n)->ln_Succ != NULL; \
        (n) = (n)->ln_Succ)

/* Adds text to what buf holds from position at; returns where it ends. */
static LONG
text_add(char *buf, LONG size, LONG at, const char *text)
{
    mdnsTextCopy(buf + at, size - at, text);
    return at + mdnsTextLength(buf + at, size - at);
}

/* "Printer._ipp._tcp.local" */
static void
instance_fqdn(const struct MDNSRegisteredService *rs, char *buf, LONG size)
{
    LONG at = text_add(buf, size, 0, rs->rs_Instance);

    at = text_add(buf, size, at, ".");
    at = text_add(buf, size, at, rs->rs_Type);
    at = text_add(buf, size, at, ".");
    text_add(buf, size, at, rs->rs_Domain);
}

/* "_ipp._tcp.local" */
static void
type_fqdn(const struct MDNSRegisteredService *rs, char *buf, LONG size)
{
    LONG at = text_add(buf, size, 0, rs->rs_Type);

    at = text_add(buf, size, at, ".");
    text_add(buf, size, at, rs->rs_Domain);
}

/* The host an instance is on: this one, under the name it has now, or the
 * one its client named. */
static void
target_fqdn(const struct MDNSDaemon *md,
    const struct MDNSRegisteredService *rs, char *buf, LONG size)
{
    if (rs->rs_Proxy) {
        mdnsTextCopy(buf, size, rs->rs_HostTarget);
    } else {
        mdnsHostFqdn(md, buf, size);
    }
}

/* The data of an SRV record: priority and weight, which are 0, the port,
 * and the host's name. Returns its length, or -1. */
static LONG
srv_data(const struct MDNSDaemon *md, const struct MDNSRegisteredService *rs,
    UBYTE *data, LONG size)
{
    char target[256];
    LONG wirelen;

    target_fqdn(md, rs, target, (LONG)sizeof(target));
    if (size < 6 ||
        mdnsNameToWire(target, data + 6, size - 6, &wirelen) != 0) {
        return -1;
    }
    data[0] = 0;
    data[1] = 0;
    data[2] = 0;
    data[3] = 0;
    data[4] = (UBYTE)((rs->rs_Port >> 8) & 0xFF);
    data[5] = (UBYTE)(rs->rs_Port & 0xFF);
    return 6 + wirelen;
}

/*
 * The data of a TXT record. The client's text is "key=value" items with a
 * space between them; DNS-SD wants each as a string of its own (RFC 6763
 * section 6). An instance always has a TXT record: one empty string when
 * it has nothing to say.
 */
static LONG
txt_data(const struct MDNSRegisteredService *rs, UBYTE *data, LONG size)
{
    LONG length = 0;
    LONG pos = 0;

    for (;;) {
        UBYTE item[255];
        LONG len = mdnsTxtNext(rs->rs_TXT, &pos, item, (LONG)sizeof(item));

        if (len < 0) {
            break;
        }
        if (len > 0 && length + 1 + len <= size) {
            data[length++] = (UBYTE)len;
            memcpy(data + length, item, (size_t)len);
            length += len;
        }
    }
    if (length == 0) {
        data[length++] = 0;
    }
    return length;
}

/* A resolver that speaks plain DNS keeps no mDNS cache: ten seconds at
 * most, and no cache-flush bit, which it would read as another class. */
static BOOL
reply_legacy(const struct MDNSDaemon *md)
{
    return md->md_ReplyAddr != 0 && md->md_ReplyLegacy;
}

static ULONG
reply_ttl(const struct MDNSDaemon *md, ULONG ttl)
{
    return (reply_legacy(md) && ttl > 10) ? 10 : ttl;
}

/* The answer in hand goes to the one who asked, or to the group again. */
static void
reply_to_asker(struct MDNSDaemon *md, struct MDNSQuestion *q, BOOL legacy)
{
    md->md_ReplyAddr = md->md_QueryAddr;
    md->md_ReplyPort = legacy ? md->md_QueryPort : MDNS_DNS_PORT;
    md->md_ReplyLegacy = legacy;
    md->md_ReplyQuestion = q;
}

static void
reply_to_group(struct MDNSDaemon *md)
{
    md->md_ReplyAddr = 0;
    md->md_ReplyPort = 0;
    md->md_ReplyLegacy = FALSE;
    md->md_ReplyQuestion = NULL;
}

/*
 * Appends a service's records to a response that is being built: PTR, SRV,
 * TXT and, with withAddr, the address record of its host. As a goodbye
 * their time to live is zero, which withdraws them from every cache on the
 * link (RFC 6762 section 10.1). Returns FALSE and leaves the packet as it
 * was when they do not all fit.
 */
static BOOL
service_append(struct MDNSDaemon *md, const struct MDNSRegisteredService *rs,
    BOOL goodbye, LONG ifindex, UBYTE *pkt, LONG *posp, UWORD *ancountp,
    BOOL withAddr)
{
    /* The address record names the interface it leaves on, unless the
     * service stands for another host. */
    ULONG addr = rs->rs_Proxy ? rs->rs_HostAddr : md->md_Ifs[ifindex].mi_Addr;
    /* The service's own three records live as long as its client said;
     * the address record is the host's, and other services' too. */
    ULONG ttl = reply_ttl(md, goodbye ? 0 :
        (rs->rs_TTL != 0 ? rs->rs_TTL : MDNS_TTL_OTHER));
    ULONG srvttl = reply_ttl(md, goodbye ? 0 :
        (rs->rs_TTL != 0 ? rs->rs_TTL : MDNS_TTL_HOST));
    UWORD unique = reply_legacy(md) ? DNS_CLASS_IN : DNS_CLASS_IN_FLUSH;
    LONG pos = *posp;
    UWORD ancount = *ancountp;
    char instance[256];
    char type[256];
    UBYTE data[512];
    LONG length;

    instance_fqdn(rs, instance, (LONG)sizeof(instance));
    type_fqdn(rs, type, (LONG)sizeof(type));

    /* A PTR record is shared by every instance of the type, so it does not
     * carry the cache-flush bit (RFC 6762 section 10.2). */
    if (mdnsNameToWire(instance, data, (LONG)sizeof(data), &length) != 0 ||
        mdnsAppendRecord(pkt, MDNS_RESPONSE_MAX, &pos, &ancount, type,
            DNS_TYPE_PTR, DNS_CLASS_IN, ttl, data, (UWORD)length) != 0) {
        return FALSE;
    }

    length = srv_data(md, rs, data, (LONG)sizeof(data));
    if (length < 0 ||
        mdnsAppendRecord(pkt, MDNS_RESPONSE_MAX, &pos, &ancount, instance,
            DNS_TYPE_SRV, unique, srvttl, data, (UWORD)length) != 0) {
        return FALSE;
    }

    length = txt_data(rs, data, (LONG)sizeof(data));
    if (mdnsAppendRecord(pkt, MDNS_RESPONSE_MAX, &pos, &ancount, instance,
            DNS_TYPE_TXT, unique, ttl, data, (UWORD)length) != 0) {
        return FALSE;
    }

    /* The address of the SRV record's host, so that whoever receives this
     * need not ask for it. */
    if (addr != 0 && withAddr && (!goodbye || rs->rs_Proxy)) {
        char target[256];

        target_fqdn(md, rs, target, (LONG)sizeof(target));
        mdnsAddrData(addr, data);
        if (mdnsAppendRecord(pkt, MDNS_RESPONSE_MAX, &pos, &ancount, target,
                DNS_TYPE_A, unique,
                reply_ttl(md, goodbye ? 0 : MDNS_TTL_HOST), data, 4) != 0) {
            return FALSE;
        }
    }

    *posp = pos;
    *ancountp = ancount;
    return TRUE;
}

/* One service's records as a response of their own, to the interface a
 * question came in on or else to every one. */
static void
service_send(struct MDNSDaemon *md, struct MDNSRegisteredService *rs,
    BOOL goodbye)
{
    UBYTE *pkt = md->md_TxBuf;
    struct MDNSHeader *hdr = (struct MDNSHeader *)pkt;
    LONG i;

    for (i = 0; i < md->md_IfCount; i++) {
        LONG pos;
        UWORD ancount = 0;

        if (md->md_SendIf >= 0 && i != md->md_SendIf) {
            continue;
        }
        pos = mdnsResponseBegin(md, pkt);
        if (service_append(md, rs, goodbye, i, pkt, &pos, &ancount, TRUE)) {
            hdr->ancount = htons(ancount);
            mdnsNetSendOn(md, i, pkt, pos);
            if (md->md_ReplyAddr == 0 && !goodbye) {
                rs->rs_Multicast = mdnsNow();
            }
        }
    }
}

/*
 * Answers to browse questions are not sent at once. A PTR record is shared:
 * every host with an instance of the type answers the same question, and if
 * all did so at once their answers would collide. Each answer waits between
 * 20 and 120 ms, picked at random (RFC 6762 section 6), and when the first
 * one of an interface is due every answer waiting for that interface goes
 * with it in one response, with the host's address record once.
 */
static ULONG
answer_delay(const struct MDNSDaemon *md)
{
    static ULONG seed;

    if (seed == 0) {
        seed = mdnsTicks() ^ md->md_HostAddr ^ 0x5bd1e995UL;
    }
    seed = seed * 1103515245UL + 12345UL;
    /* in fiftieths of a second, counted from the tick the question came
     * in: two to six of them is more than 20 ms and at most 120 */
    return 2 + ((seed >> 16) % 5);
}

static void
answers_flush(struct MDNSDaemon *md, ULONG now)
{
    UBYTE *pkt = md->md_TxBuf;
    struct MDNSHeader *hdr = (struct MDNSHeader *)pkt;
    struct Node *n;
    BOOL due = FALSE;
    LONG i;

    md->md_AnswerPending = FALSE;
    FOR_SERVICES(md, n) {
        struct MDNSRegisteredService *rs = (struct MDNSRegisteredService *)n;

        if (rs->rs_AnswerTick == 0) {
            continue;
        }
        if (rs->rs_State != MDNS_NAME_ANNOUNCED) {
            rs->rs_AnswerTick = 0;
        } else if ((LONG)(now - rs->rs_AnswerTick) >= 0) {
            due = TRUE;
        } else {
            md->md_AnswerPending = TRUE;
        }
    }
    if (!due) {
        return;
    }
    for (i = 0; i < md->md_IfCount; i++) {
        LONG pos = (LONG)sizeof(struct MDNSHeader);
        UWORD ancount = 0;
        BOOL addrDone = FALSE;

        mdnsInitHeader(hdr, DNS_FLAG_QR | DNS_FLAG_AA, 0);
        FOR_SERVICES(md, n) {
            struct MDNSRegisteredService *rs =
                (struct MDNSRegisteredService *)n;

            if (rs->rs_AnswerTick == 0 ||
                (rs->rs_AnswerIf >= 0 && rs->rs_AnswerIf != i)) {
                continue;
            }
            /* the host's address once; a stand-in's host has its own */
            if (!service_append(md, rs, FALSE, i, pkt, &pos, &ancount,
                    rs->rs_Proxy || !addrDone)) {
                /* full: send what there is and start another */
                if (ancount != 0) {
                    hdr->ancount = htons(ancount);
                    mdnsNetSendOn(md, i, pkt, pos);
                }
                mdnsInitHeader(hdr, DNS_FLAG_QR | DNS_FLAG_AA, 0);
                pos = (LONG)sizeof(struct MDNSHeader);
                ancount = 0;
                addrDone = FALSE;
                if (!service_append(md, rs, FALSE, i, pkt, &pos, &ancount,
                        TRUE)) {
                    continue;
                }
            }
            if (!rs->rs_Proxy) {
                addrDone = TRUE;
            }
        }
        if (ancount != 0) {
            hdr->ancount = htons(ancount);
            mdnsNetSendOn(md, i, pkt, pos);
        }
    }
    FOR_SERVICES(md, n) {
        struct MDNSRegisteredService *rs = (struct MDNSRegisteredService *)n;

        if (rs->rs_AnswerTick != 0) {
            rs->rs_Multicast = mdnsNow();
        }
        rs->rs_AnswerTick = 0;
    }
    md->md_AnswerPending = FALSE;
}

/* Before the sockets go away: nobody should keep what this host offered. */
void
mdnsRegGoodbyeAll(struct MDNSDaemon *md)
{
    struct Node *n;

    if (md->md_Socket < 0) {
        return;
    }
    FOR_SERVICES(md, n) {
        struct MDNSRegisteredService *rs = (struct MDNSRegisteredService *)n;

        if (rs->rs_State == MDNS_NAME_ANNOUNCED) {
            service_send(md, rs, TRUE);
        }
    }
}

/* The host's name is its own, perhaps a new one or with a new address:
 * what was announced before is announced again with it. */
void
mdnsRegAnnounceAll(struct MDNSDaemon *md)
{
    struct Node *n;

    FOR_SERVICES(md, n) {
        struct MDNSRegisteredService *rs = (struct MDNSRegisteredService *)n;

        if (rs->rs_State == MDNS_NAME_ANNOUNCED) {
            service_send(md, rs, FALSE);
        }
    }
}

/* An interface came up or changed: claim each service name again before
 * announcing it on the new link (RFC 6762 section 8.1). */
void
mdnsRegRestartAll(struct MDNSDaemon *md)
{
    struct Node *n;

    FOR_SERVICES(md, n) {
        struct MDNSRegisteredService *rs = (struct MDNSRegisteredService *)n;

        rs->rs_State = MDNS_NAME_PROBING;
        rs->rs_Step = 0;
        rs->rs_NextTick = mdnsTicks();
        rs->rs_AnswerTick = 0;
    }
    md->md_AnswerPending = FALSE;
}

/*
 * Asks whether anyone already has the instance's name, with the SRV record
 * this host would publish in the authority section (RFC 6762 section 8.1).
 * The first probe asks for a unicast answer.
 */
static void
service_probe(struct MDNSDaemon *md, const struct MDNSRegisteredService *rs)
{
    UBYTE *pkt = md->md_TxBuf;
    struct MDNSHeader *hdr = (struct MDNSHeader *)pkt;
    char instance[256];
    UBYTE data[6 + 256];
    LONG length, pos;
    UWORD nscount = 0;

    instance_fqdn(rs, instance, (LONG)sizeof(instance));
    length = srv_data(md, rs, data, (LONG)sizeof(data));
    if (length < 0 ||
        mdnsBuildQuery(pkt, (LONG)MDNS_MAX_PACKET, 0, instance, DNS_TYPE_ANY,
            (UWORD)(DNS_CLASS_IN | (rs->rs_Step == 0 ? DNS_QU_BIT : 0)),
            &pos) != 0 ||
        mdnsAppendRecord(pkt, (LONG)MDNS_MAX_PACKET, &pos, &nscount,
            instance, DNS_TYPE_SRV, DNS_CLASS_IN, MDNS_TTL_HOST, data,
            (UWORD)length) != 0) {
        return;
    }
    hdr->nscount = htons(nscount);
    length = txt_data(rs, data, (LONG)sizeof(data));
    if (mdnsAppendRecord(pkt, (LONG)MDNS_MAX_PACKET, &pos, &nscount,
            instance, DNS_TYPE_TXT, DNS_CLASS_IN,
            rs->rs_TTL != 0 ? rs->rs_TTL : MDNS_TTL_OTHER, data,
            (UWORD)length) != 0) {
        return;
    }
    hdr->nscount = htons(nscount);
    mdnsNetSendAll(md, pkt, pos);
}

/* "Name" becomes "Name (2)", "Name (2)" becomes "Name (3)". */
static void
service_rename_next(struct MDNSRegisteredService *rs)
{
    char *name = rs->rs_Instance;
    LONG len = mdnsTextLength(name, MDNS_MAX_NAME);
    LONG open = len;
    ULONG number = 1;
    char digits[12];
    LONG n = 0;

    if (len >= 4 && name[len - 1] == ')') {
        LONG i = len - 2;

        while (i > 0 && name[i] >= '0' && name[i] <= '9') {
            i--;
        }
        if (i < len - 2 && i > 0 && name[i] == '(' && name[i - 1] == ' ') {
            LONG d;

            number = 0;
            for (d = i + 1; d < len - 1; d++) {
                number = number * 10 + (ULONG)(name[d] - '0');
            }
            open = i - 1;
        }
    }
    number++;
    /* room for " (4294967295)" */
    if (open > 63 - 13) {
        open = 63 - 13;
    }
    do {
        digits[n++] = (char)('0' + number % 10);
        number /= 10;
    } while (number != 0);
    name[open++] = ' ';
    name[open++] = '(';
    while (n > 0) {
        name[open++] = digits[--n];
    }
    name[open++] = ')';
    name[open] = '\0';
}

/* Do not rename into a name another local service already owns. */
static void
service_rename(const struct MDNSDaemon *md, struct MDNSRegisteredService *rs)
{
    BOOL used;

    do {
        struct Node *n;

        service_rename_next(rs);
        used = FALSE;
        FOR_SERVICES(md, n) {
            const struct MDNSRegisteredService *other =
                (const struct MDNSRegisteredService *)n;

            if (other != rs && Stricmp(other->rs_Instance, rs->rs_Instance) == 0 &&
                Stricmp(other->rs_Type, rs->rs_Type) == 0 &&
                Stricmp(other->rs_Domain, rs->rs_Domain) == 0) {
                used = TRUE;
                break;
            }
        }
    } while (used);
}

/*
 * A record in a response from another host is owned by "name". If one of
 * this host's services has that name, the name is contested: the service
 * takes the next name and is probed from the start. For a service that was
 * already announced only its own kinds of record count. No goodbye is sent
 * for it: the records under that name now are the other host's.
 */
void
mdnsRegHandleRecord(const struct MDNSDaemon *md, const struct MDNSRecord *r,
    const UBYTE *pkt, LONG pktlen)
{
    struct Node *n;

    if (r->ttl == 0 || (r->rclass & 0x7fff) != DNS_CLASS_IN ||
        (r->type != DNS_TYPE_SRV && r->type != DNS_TYPE_TXT)) {
        return;
    }
    FOR_SERVICES(md, n) {
        struct MDNSRegisteredService *rs = (struct MDNSRegisteredService *)n;
        char instance[256];
        instance_fqdn(rs, instance, (LONG)sizeof(instance));
        if (Stricmp(instance, r->name) != 0) {
            continue;
        }
        if (r->type == DNS_TYPE_SRV) {
            char target[256], theirs[256];

            if (r->rdlength < 7 ||
                mdnsSkipName(pkt, pktlen, r->rdoffset + 6) !=
                    r->rdoffset + r->rdlength ||
                mdnsWireToName(pkt, pktlen, r->rdoffset + 6, pkt, pktlen,
                    theirs, (LONG)sizeof(theirs)) < 0) {
                continue;
            }
            target_fqdn(md, rs, target, (LONG)sizeof(target));
            if (r->rdata[0] == 0 && r->rdata[1] == 0 &&
                r->rdata[2] == 0 && r->rdata[3] == 0 &&
                r->rdata[4] == (UBYTE)(rs->rs_Port >> 8) &&
                r->rdata[5] == (UBYTE)rs->rs_Port &&
                Stricmp(target, theirs) == 0) {
                continue;
            }
        } else {
            UBYTE expected[512];
            LONG length = txt_data(rs, expected, (LONG)sizeof(expected));

            if (length == r->rdlength &&
                memcmp(expected, r->rdata, (size_t)length) == 0) {
                continue;
            }
        }
        service_rename(md, rs);
        rs->rs_State = MDNS_NAME_PROBING;
        rs->rs_Step = 0;
        rs->rs_NextTick = mdnsTicks() + MDNS_PROBE_TICKS;
    }
}

/*
 * A record as it is compared when two hosts probe for one name: the data
 * with no name compressed (RFC 6762 section 8.2). Returns its length, or
 * -1.
 */
static LONG
probe_rdata(const struct MDNSRecord *r, const UBYTE *pkt, LONG pktlen,
    UBYTE *out, LONG outmax)
{
    char target[256];
    LONG wirelen, i;

    if (r->type != DNS_TYPE_SRV) {
        if ((LONG)r->rdlength > outmax) {
            return -1;
        }
        memcpy(out, r->rdata, r->rdlength);
        return (LONG)r->rdlength;
    }
    /* priority, weight and port, then a name that may be compressed */
    if (r->rdlength < 7 || outmax < 6 ||
        mdnsSkipName(pkt, pktlen, r->rdoffset + 6) != r->rdoffset + r->rdlength ||
        mdnsWireToName(pkt, pktlen, r->rdoffset + 6, pkt, pktlen, target,
            (LONG)sizeof(target)) < 0) {
        return -1;
    }
    for (i = 0; target[i] != '\0'; i++) {
        if (target[i] >= 'A' && target[i] <= 'Z') {
            target[i] = (char)(target[i] + ('a' - 'A'));
        }
    }
    if (mdnsNameToWire(target, out + 6, outmax - 6, &wirelen) != 0) {
        return -1;
    }
    memcpy(out, r->rdata, 6);
    return 6 + wirelen;
}

/* <0 when (class, type, data) a sorts before b, 0 when equal, >0 after;
 * the class without its cache-flush bit. */
static LONG
probe_compare(UWORD classA, UWORD typeA, const UBYTE *a, LONG lenA,
    UWORD classB, UWORD typeB, const UBYTE *b, LONG lenB)
{
    LONG d;

    classA &= 0x7fff;
    classB &= 0x7fff;
    if (classA != classB) {
        return (LONG)classA - (LONG)classB;
    }
    if (typeA != typeB) {
        return (LONG)typeA - (LONG)typeB;
    }
    d = (LONG)memcmp(a, b, (size_t)(lenA < lenB ? lenA : lenB));
    if (d != 0) {
        return d;
    }
    return lenA - lenB;
}

/*
 * Another host is probing, and these are the records it proposes. For a
 * name a service of this host is probing for too, the two sets of records
 * are compared and the host whose set is lexicographically later keeps
 * going; the other waits a second and probes again, by which time the
 * winner has announced and the usual conflict gives the loser another name
 * (RFC 6762 section 8.2). Both the TXT and SRV are proposed and compared in sorted order; when
 * all common records match, the longer set is the later one.
 */
void
mdnsRegHandleProbe(const struct MDNSDaemon *md,
    const struct MDNSRecord *theirs, LONG count, const UBYTE *pkt,
    LONG pktlen)
{
    /* Canonical RDATA stays in static storage: mdnsd has one task. */
    static UBYTE mine[2][512], data[MDNS_PROPOSED_MAX][512], other[512];
    struct Node *n;

    FOR_SERVICES(md, n) {
        struct MDNSRegisteredService *rs = (struct MDNSRegisteredService *)n;
        char instance[256];
        LONG lengths[MDNS_PROPOSED_MAX], order[MDNS_PROPOSED_MAX];
        UWORD classes[MDNS_PROPOSED_MAX], types[MDNS_PROPOSED_MAX];
        LONG minelen[2], matches = 0, i, j, d = 0;

        if (rs->rs_State != MDNS_NAME_PROBING || rs->rs_Step == 0) {
            continue;
        }
        instance_fqdn(rs, instance, (LONG)sizeof(instance));
        minelen[0] = txt_data(rs, mine[0], (LONG)sizeof(mine[0]));
        minelen[1] = srv_data(md, rs, mine[1], (LONG)sizeof(mine[1]));
        if (minelen[1] < 0) {
            continue;
        }
        for (i = 6; i < minelen[1]; i++) {
            if (mine[1][i] >= 'A' && mine[1][i] <= 'Z') {
                mine[1][i] += 'a' - 'A';
            }
        }
        for (i = 0; i < count && matches < MDNS_PROPOSED_MAX; i++) {
            LONG length;

            if (Stricmp(theirs[i].name, instance) != 0) {
                continue;
            }
            length = probe_rdata(&theirs[i], pkt, pktlen, other,
                (LONG)sizeof(other));
            if (length < 0) {
                continue;
            }
            for (j = 0; j < matches; j++) {
                if (probe_compare(theirs[i].rclass, theirs[i].type, other,
                        length, classes[j], types[j], data[j], lengths[j]) == 0) {
                    break;
                }
            }
            if (j < matches) {
                continue;       /* duplicates do not add to a record set */
            }
            memcpy(data[matches], other, (size_t)length);
            lengths[matches] = length;
            classes[matches] = theirs[i].rclass;
            types[matches] = theirs[i].type;
            j = matches;
            while (j > 0 && probe_compare(classes[matches], types[matches],
                    data[matches], lengths[matches], classes[order[j - 1]],
                    types[order[j - 1]], data[order[j - 1]],
                    lengths[order[j - 1]]) < 0) {
                order[j] = order[j - 1];
                j--;
            }
            order[j] = matches++;
        }
        if (matches == 0) {
            continue;
        }
        for (i = 0; i < 2 && i < matches; i++) {
            j = order[i];
            d = probe_compare(DNS_CLASS_IN, i == 0 ? DNS_TYPE_TXT : DNS_TYPE_SRV,
                mine[i], minelen[i], classes[j], types[j], data[j], lengths[j]);
            if (d != 0) {
                break;
            }
        }
        if (d == 0) {
            d = 2 - matches;
        }
        if (d < 0) {
            rs->rs_Step = 0;
            rs->rs_NextTick = mdnsTicks() + 50;
        }
    }
}

/*
 * Moves each service along: three probes, then two announcements. Returns
 * TRUE while a service is being probed, so that the caller comes back in a
 * quarter of a second.
 */
BOOL
mdnsRegTick(struct MDNSDaemon *md)
{
    struct Node *n;
    ULONG now = mdnsTicks();
    BOOL probing = FALSE;
    /* A service's records point at the host's name and carry its address,
     * so none is announced before the name is this host's. */
    BOOL hostReady = md->md_HostState == MDNS_NAME_ANNOUNCED;

    if (md->md_IfCount == 0 || md->md_Socket < 0) {
        return FALSE;
    }
    if (md->md_AnswerPending) {
        answers_flush(md, now);
    }

    FOR_SERVICES(md, n) {
        struct MDNSRegisteredService *rs = (struct MDNSRegisteredService *)n;

        if (rs->rs_State == MDNS_NAME_PROBING) {
            probing = TRUE;
            if (!hostReady || (LONG)(now - rs->rs_NextTick) < 0) {
                continue;
            }
            if (rs->rs_Step < MDNS_PROBE_COUNT) {
                service_probe(md, rs);
                rs->rs_Step++;
                rs->rs_NextTick = now + MDNS_PROBE_TICKS;
                continue;
            }
            /* Nobody objected: the name is this host's. */
            rs->rs_State = MDNS_NAME_ANNOUNCED;
            rs->rs_Step = 1;
            rs->rs_NextTick = now + 50;
            service_send(md, rs, FALSE);
        } else if (rs->rs_Step < MDNS_ANNOUNCE_COUNT &&
                   (LONG)(now - rs->rs_NextTick) >= 0 &&
                   (hostReady || rs->rs_Proxy)) {
            rs->rs_Step++;
            service_send(md, rs, FALSE);
        }
    }
    return probing;
}

/* Answers "which service types are there" (RFC 6763 section 9) with one
 * PTR record per type. */
static void
types_send(struct MDNSDaemon *md)
{
    UBYTE *pkt = md->md_TxBuf;
    struct MDNSHeader *hdr = (struct MDNSHeader *)pkt;
    struct Node *n;
    LONG pos = mdnsResponseBegin(md, pkt);
    UWORD ancount = 0;

    FOR_SERVICES(md, n) {
        const struct MDNSRegisteredService *rs =
            (const struct MDNSRegisteredService *)n;
        const struct Node *m;
        char type[256];
        UBYTE target[256];
        LONG targetlen;

        if (rs->rs_State != MDNS_NAME_ANNOUNCED) {
            continue;
        }
        /* once per type */
        for (m = md->md_Services.lh_Head; m != n; m = m->ln_Succ) {
            const struct MDNSRegisteredService *earlier =
                (const struct MDNSRegisteredService *)m;

            if (earlier->rs_State == MDNS_NAME_ANNOUNCED &&
                Stricmp(earlier->rs_Type, rs->rs_Type) == 0) {
                break;
            }
        }
        if (m != n) {
            continue;
        }
        type_fqdn(rs, type, (LONG)sizeof(type));
        if (mdnsNameToWire(type, target, (LONG)sizeof(target),
                &targetlen) == 0) {
            mdnsAppendRecord(pkt, (LONG)MDNS_MAX_PACKET, &pos, &ancount,
                MDNS_TYPE_SERVICES, DNS_TYPE_PTR, DNS_CLASS_IN,
                reply_ttl(md, MDNS_TTL_OTHER), target, (UWORD)targetlen);
        }
    }
    if (ancount != 0) {
        hdr->ancount = htons(ancount);
        mdnsNetSendAll(md, pkt, pos);
    }
}

/*
 * One question of a received query. "known" holds the PTR targets the asker
 * says it already has (RFC 6762 section 7.1). A service answers at most
 * once for one query packet, always with all of its records.
 *
 * A question from another port than 5353 comes from a resolver that speaks
 * plain DNS: it gets its answer back alone, at once (section 6.7). A
 * question with the unicast-response bit is answered to the asker alone
 * when the group has had the records not long ago, and goes to the group
 * otherwise (section 5.4).
 */
void
mdnsRegHandleQuery(struct MDNSDaemon *md, struct MDNSQuestion *q,
    const struct MDNSKnownAnswer *known, LONG knownCount)
{
    struct Node *n;
    BOOL any = q->qtype == DNS_TYPE_ANY;
    BOOL legacy = md->md_QueryPort != MDNS_DNS_PORT;
    BOOL direct = !legacy && (q->qclass & DNS_QU_BIT) != 0;
    ULONG now = mdnsNow();

    if ((any || q->qtype == DNS_TYPE_PTR) &&
        Stricmp(q->qname, MDNS_TYPE_SERVICES) == 0) {
        if (legacy) {
            reply_to_asker(md, q, TRUE);
        }
        types_send(md);
        reply_to_group(md);
        return;
    }

    FOR_SERVICES(md, n) {
        struct MDNSRegisteredService *rs = (struct MDNSRegisteredService *)n;
        char fqdn[256];
        BOOL answer = FALSE, shared = FALSE;

        if (rs->rs_State != MDNS_NAME_ANNOUNCED ||
            rs->rs_Answered == md->md_PacketSerial) {
            continue;
        }
        /* a service of this host waits while the host's name is in doubt */
        if (!rs->rs_Proxy && md->md_HostState != MDNS_NAME_ANNOUNCED) {
            continue;
        }

        instance_fqdn(rs, fqdn, (LONG)sizeof(fqdn));
        if (Stricmp(q->qname, fqdn) == 0) {
            answer = any || q->qtype == DNS_TYPE_SRV ||
                q->qtype == DNS_TYPE_TXT;
        } else if (rs->rs_Proxy &&
                   Stricmp(q->qname, rs->rs_HostTarget) == 0) {
            /* the host a service stands in for; this host's own name is
             * answered in mdns_host.c */
            answer = any || q->qtype == DNS_TYPE_A;
        } else if (any || q->qtype == DNS_TYPE_PTR) {
            char type[256];

            type_fqdn(rs, type, (LONG)sizeof(type));
            if (Stricmp(q->qname, type) == 0) {
                LONG k;

                answer = TRUE;
                shared = TRUE;
                for (k = 0; k < knownCount; k++) {
                    ULONG ttl = rs->rs_TTL != 0 ? rs->rs_TTL : MDNS_TTL_OTHER;

                    if (Stricmp(known[k].owner, type) == 0 &&
                        Stricmp(known[k].target, fqdn) == 0 &&
                        known[k].ttl >= (ttl + 1) / 2) {
                        answer = FALSE;
                        break;
                    }
                }
            }
        }
        if (!answer) {
            continue;
        }
        rs->rs_Answered = md->md_PacketSerial;
        if (legacy || (direct && rs->rs_Multicast != 0 &&
                now - rs->rs_Multicast <= MDNS_UNICAST_SECONDS)) {
            /* to the asker alone, and at once: nobody else is waiting to
             * answer on a channel of two */
            reply_to_asker(md, q, legacy);
            service_send(md, rs, FALSE);
            reply_to_group(md);
        } else if (shared) {
            /* a browse question: answered a little later, with the others */
            if (rs->rs_AnswerTick == 0) {
                rs->rs_AnswerTick = mdnsTicks() + answer_delay(md);
                if (rs->rs_AnswerTick == 0) {
                    rs->rs_AnswerTick = 1;
                }
                rs->rs_AnswerIf = md->md_SendIf;
            } else if (rs->rs_AnswerIf != md->md_SendIf) {
                rs->rs_AnswerIf = -1;
            }
            md->md_AnswerPending = TRUE;
        } else {
            /* a question for this one name: nobody else answers it */
            service_send(md, rs, FALSE);
        }
    }
}

static struct MDNSRegisteredService *
service_find(const struct MDNSDaemon *md, CONST_APTR ref)
{
    struct Node *n;

    FOR_SERVICES(md, n) {
        if (((struct MDNSRegisteredService *)n)->rs_ClientRef == ref) {
            return (struct MDNSRegisteredService *)n;
        }
    }
    return NULL;
}

/*
 * A client registers a service. The name is probed first; mdnsRegTick()
 * announces it about a second from now. If another host has the name, the
 * service is announced under the next free one.
 */
LONG
mdnsRegAdd(struct MDNSDaemon *md, struct MDNSIPCMsg *msg)
{
    struct MDNSRegisteredService *rs;
    struct Node *n;

    if (msg->mi_Name[0] == '\0' || msg->mi_ServiceType[0] == '\0' ||
        msg->mi_Port == 0 || msg->mi_TTL > MDNS_MAX_TTL ||
        mdnsTextLength(msg->mi_Name, MDNS_MAX_NAME) > 63 ||
        strchr(msg->mi_Name, '.') != NULL ||
        ((msg->mi_HostName[0] != '\0') != (msg->mi_HostAddr != 0))) {
        return MDNSERR_BADPARAM;
    }
    /* One instance name per type on this host. */
    FOR_SERVICES(md, n) {
        rs = (struct MDNSRegisteredService *)n;
        if (Stricmp(rs->rs_Instance, msg->mi_Name) == 0 &&
            Stricmp(rs->rs_Type, msg->mi_ServiceType) == 0) {
            return MDNSERR_DUPLICATE;
        }
    }

    rs = (struct MDNSRegisteredService *)AllocMem(sizeof(*rs), MEMF_CLEAR);
    if (rs == NULL) {
        return MDNSERR_NOMEM;
    }
    mdnsTextCopy(rs->rs_Instance, (LONG)sizeof(rs->rs_Instance),
        msg->mi_Name);
    mdnsTextCopy(rs->rs_Type, (LONG)sizeof(rs->rs_Type),
        msg->mi_ServiceType);
    mdnsTextCopy(rs->rs_Domain, (LONG)sizeof(rs->rs_Domain),
        msg->mi_Domain[0] != '\0' ? msg->mi_Domain : MDNS_DOMAIN_LOCAL);
    mdnsTextCopy(rs->rs_TXT, (LONG)sizeof(rs->rs_TXT), msg->mi_TXT);
    rs->rs_Port = msg->mi_Port;
    rs->rs_TTL = msg->mi_TTL;
    rs->rs_TaskID = msg->mi_TaskID;
    if (msg->mi_HostName[0] != '\0' && msg->mi_HostAddr != 0) {
        LONG at = 0;
        char own[256];

        /* Announced on behalf of a host that does not speak mDNS itself:
         * its first label, in the mDNS domain. */
        rs->rs_Proxy = TRUE;
        rs->rs_HostAddr = msg->mi_HostAddr;
        while (at < MDNS_MAX_HOST && msg->mi_HostName[at] != '\0' &&
               msg->mi_HostName[at] != '.') {
            rs->rs_HostTarget[at] = msg->mi_HostName[at];
            at++;
        }
        if (at == 0 || at > 63 ||
            (msg->mi_HostName[at] != '\0' &&
             Stricmp(msg->mi_HostName + at, ".local") != 0 &&
             Stricmp(msg->mi_HostName + at, ".local.") != 0)) {
            FreeMem(rs, sizeof(*rs));
            return MDNSERR_BADPARAM;
        }
        mdnsTextCopy(rs->rs_HostTarget + at,
            (LONG)sizeof(rs->rs_HostTarget) - at, "." MDNS_DOMAIN_LOCAL);
        mdnsHostFqdn(md, own, (LONG)sizeof(own));
        if (Stricmp(rs->rs_HostTarget, own) == 0 ||
            (md->md_HostBase[0] != '\0' &&
             Strnicmp(rs->rs_HostTarget, md->md_HostBase, at) == 0 &&
             mdnsTextLength(md->md_HostBase, MDNS_MAX_HOST) == at)) {
            FreeMem(rs, sizeof(*rs));
            return MDNSERR_BADPARAM;
        }
    }
    rs->rs_ClientRef = (APTR)(IPTR)md->md_NextHandle++;
    rs->rs_State = MDNS_NAME_PROBING;
    rs->rs_Step = 0;
    rs->rs_NextTick = mdnsTicks();
    AddTail(&md->md_Services, &rs->rs_Node);
    msg->mi_Handle = rs->rs_ClientRef;
    return MDNS_OK;
}

/* Changes a registered service's text or port and tells the link. Only
 * the client that registered it may. */
LONG
mdnsRegUpdate(struct MDNSDaemon *md, const struct MDNSIPCMsg *msg)
{
    struct MDNSRegisteredService *rs = service_find(md, msg->mi_Handle);

    if (rs == NULL) {
        return MDNSERR_NOTFOUND;
    }
    if (rs->rs_TaskID != msg->mi_TaskID) {
        return MDNSERR_BADPARAM;
    }
    if (msg->mi_TTL > MDNS_MAX_TTL) {
        return MDNSERR_BADPARAM;
    }
    if (msg->mi_Flags & MDNS_IPC_UPDF_TXT) {
        mdnsTextCopy(rs->rs_TXT, (LONG)sizeof(rs->rs_TXT), msg->mi_TXT);
    }
    if (msg->mi_Port != 0) {
        rs->rs_Port = msg->mi_Port;
    }
    if (msg->mi_TTL != 0) {
        rs->rs_TTL = msg->mi_TTL;
    }
    if (rs->rs_State == MDNS_NAME_ANNOUNCED) {
        service_send(md, rs, FALSE);
        rs->rs_Step = 1;
        rs->rs_NextTick = mdnsTicks() + 50;
    }
    return MDNS_OK;
}

static void
service_remove(struct MDNSDaemon *md, struct MDNSRegisteredService *rs)
{
    Remove(&rs->rs_Node);
    if (rs->rs_State == MDNS_NAME_ANNOUNCED) {
        service_send(md, rs, TRUE);
    }
    FreeMem(rs, sizeof(*rs));
}

/* Withdraws a service, for the client that registered it. */
void
mdnsRegRemove(struct MDNSDaemon *md, const struct MDNSIPCMsg *msg)
{
    struct MDNSRegisteredService *rs = service_find(md, msg->mi_Handle);

    if (rs != NULL && rs->rs_TaskID == msg->mi_TaskID) {
        service_remove(md, rs);
    }
}

/* Withdraws every service a client registered. */
void
mdnsRegRelease(struct MDNSDaemon *md, ULONG taskID)
{
    struct Node *n, *next;

    for (n = md->md_Services.lh_Head; n->ln_Succ != NULL; n = next) {
        next = n->ln_Succ;
        if (((struct MDNSRegisteredService *)n)->rs_TaskID == taskID) {
            service_remove(md, (struct MDNSRegisteredService *)n);
        }
    }
}

/* At the end: nothing is told to the link, which has had its goodbyes. */
void
mdnsRegFree(struct MDNSDaemon *md)
{
    struct Node *n;

    while ((n = RemHead(&md->md_Services)) != NULL) {
        FreeMem(n, sizeof(struct MDNSRegisteredService));
    }
}
