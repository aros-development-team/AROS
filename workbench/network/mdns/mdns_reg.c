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
 *
 * What a service is is its registration (dnssd_reg.h): counted labels and
 * wire names, its TXT data as it goes on the wire, the addresses of a host
 * it stands in for. Its names are kept built, as wire names, and compared
 * with the wire names of what comes in (struct MDNSQuestion's qwire,
 * struct MDNSRecord's wire): no name goes through text here. The front ends
 * (dnssd_server.c, and the bonami.library port in mdns_client.c until it
 * goes) make the registration and call mdnsRegAddCanonical().
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

/* "_services._dns-sd._udp.local" (RFC 6763 section 9) */
static const UBYTE services_name[] = {
    9, '_', 's', 'e', 'r', 'v', 'i', 'c', 'e', 's',
    7, '_', 'd', 'n', 's', '-', 's', 'd', 4, '_', 'u', 'd', 'p',
    5, 'l', 'o', 'c', 'a', 'l', 0
};

#define FOR_SERVICES(md, n) \
    for ((n) = (md)->md_Services.lh_Head; (n)->ln_Succ != NULL; \
        (n) = (n)->ln_Succ)

/* Its names, built again from its registration (after a rename). */
static void
names_build(struct MDNSRegisteredService *rs)
{
    dnssd_reg_service_name(&rs->rs_Reg, rs->rs_Name);
    dnssd_reg_type_name(&rs->rs_Reg, rs->rs_TypeName);
    if (rs->rs_Reg.host_len != 0) {
        dnssd_reg_host_name(&rs->rs_Reg, rs->rs_HostName);
    }
}

#define PROXY(rs)   ((rs)->rs_Reg.host_len != 0)

/* Wire names, equal but for ASCII case. */
static BOOL
names_equal(const UBYTE *a, const UBYTE *b)
{
    return mdnsWireNameEqual(a, b);
}

/* This host's name as it is now, "<host>.local", as a wire name; its
 * length, or -1. */
static LONG
own_host_name(const struct MDNSDaemon *md, UBYTE *out)
{
    LONG n = 0;

    while (n < 63 && md->md_HostName[n] != '\0' && md->md_HostName[n] != '.') {
        out[1 + n] = (UBYTE)md->md_HostName[n];
        n++;
    }
    if (n == 0) {
        return -1;
    }
    out[0] = (UBYTE)n;
    memcpy(out + 1 + n, "\5local", 7);  /* with the root label */
    return 1 + n + 7;
}

/* The host an instance is on: this one, under the name it has now, or the
 * one its client named. */
static LONG
target_name(const struct MDNSDaemon *md, const struct MDNSRegisteredService *rs,
    UBYTE *out)
{
    if (PROXY(rs)) {
        LONG n = mdnsWireNameLength(rs->rs_HostName);

        memcpy(out, rs->rs_HostName, (size_t)n);
        return n;
    }
    return own_host_name(md, out);
}

/* The index of the subtype whose name is name, or -1. */
static LONG
subtype_index(const struct MDNSRegisteredService *rs, const UBYTE *name)
{
    UBYTE sub[DNSSD_REG_NAME_MAX];
    LONG i;

    for (i = 0; i < rs->rs_Reg.subtype_count; i++) {
        if (dnssd_reg_subtype_name(&rs->rs_Reg, (unsigned)i, sub) != 0 &&
            names_equal(name, sub)) {
            return i;
        }
    }
    return -1;
}

/* The data of an SRV record: priority and weight, which are 0, the port,
 * and the host's name. Returns its length, or -1. */
static LONG
srv_data(const struct MDNSDaemon *md, const struct MDNSRegisteredService *rs,
    UBYTE *data, LONG size)
{
    UBYTE target[256];
    LONG n = target_name(md, rs, target);

    if (n < 0 || 6 + n > size) {
        return -1;
    }
    data[0] = 0;
    data[1] = 0;
    data[2] = 0;
    data[3] = 0;
    data[4] = (UBYTE)((rs->rs_Reg.port >> 8) & 0xFF);
    data[5] = (UBYTE)(rs->rs_Reg.port & 0xFF);
    memcpy(data + 6, target, (size_t)n);
    return 6 + n;
}

/* The data of the TXT record, where it is. An instance always has one:
 * one empty string when it has nothing to say. */
static const UBYTE empty_txt[1] = { 0 };

static LONG
txt_ref(const struct MDNSRegisteredService *rs, const UBYTE **data)
{
    if (rs->rs_TXTLength == 0 || rs->rs_TXT == NULL) {
        *data = empty_txt;
        return 1;
    }
    *data = rs->rs_TXT;
    return rs->rs_TXTLength;
}

/* A service's TXT data becomes a copy of data (length bytes, 0 for none);
 * FALSE when there is no memory for it, the old data kept. */
static BOOL
txt_set(struct MDNSRegisteredService *rs, const UBYTE *data, UWORD length)
{
    UBYTE *copy = NULL;

    if (length != 0) {
        copy = AllocVec(length, MEMF_ANY);
        if (copy == NULL) {
            return FALSE;
        }
        memcpy(copy, data, length);
    }
    if (rs->rs_TXT != NULL) {
        FreeVec(rs->rs_TXT);
    }
    rs->rs_TXT = copy;
    rs->rs_TXTLength = length;
    return TRUE;
}

/* A proxied host's address record type, A or AAAA. */
static UWORD
address_type(const struct dnssd_model_wire_address *a)
{
    return a->family == DNSSD_MODEL_ADDR_IPV6 ? DNS_TYPE_AAAA : DNS_TYPE_A;
}

/* Is a proxied host's address published on the link of slot ifindex? One
 * with a zone (an IPv6 link-local one) only on that link. */
static BOOL
address_on_link(const struct MDNSDaemon *md,
    const struct dnssd_model_wire_address *a, LONG ifindex)
{
    return a->interface_index == 0 || ifindex < 0 || ifindex >= md->md_IfCount ||
        md->md_Ifs[ifindex].mi_Index == a->interface_index;
}

/* A resolver that speaks plain DNS keeps no mDNS cache: ten seconds at
 * most, and no cache-flush bit, which it would read as another class. */
static BOOL
reply_legacy(const struct MDNSDaemon *md)
{
    return mdnsReplyIsUnicast(md) && md->md_ReplyLegacy;
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
    md->md_ReplyFamily = md->md_QueryFamily != 0 ?
        md->md_QueryFamily : AF_INET;
    md->md_ReplyAddr = md->md_QueryAddr;
    md->md_ReplyAddr6 = md->md_QueryAddr6;
    md->md_ReplyPort = legacy ? md->md_QueryPort : MDNS_DNS_PORT;
    md->md_ReplyLegacy = legacy;
    md->md_ReplyQuestion = q;
}

static void
reply_to_group(struct MDNSDaemon *md)
{
    md->md_ReplyFamily = 0;
    md->md_ReplyAddr = 0;
    memset(&md->md_ReplyAddr6, 0, sizeof(md->md_ReplyAddr6));
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
    /* The service's own three records live as long as its client said;
     * the address record is the host's, and other services' too. */
    ULONG ttl = reply_ttl(md, goodbye ? 0 :
        (rs->rs_TTL != 0 ? rs->rs_TTL : MDNS_TTL_OTHER));
    ULONG srvttl = reply_ttl(md, goodbye ? 0 :
        (rs->rs_TTL != 0 ? rs->rs_TTL : MDNS_TTL_HOST));
    UWORD unique = reply_legacy(md) ? DNS_CLASS_IN : DNS_CLASS_IN_FLUSH;
    LONG pos = *posp;
    UWORD ancount = *ancountp;
    /* responses are packed to an Ethernet MTU; a service that does not
     * fit one - a large TXT - goes in a packet of its own, up to the
     * largest an mDNS packet may be (RFC 6762 section 17) */
    LONG max = ancount == 0 ? (LONG)MDNS_MAX_PACKET : MDNS_RESPONSE_MAX;
    UBYTE data[6 + 256], target[256];
    LONG length;

    /* A PTR record is shared by every instance of the type, so it does not
     * carry the cache-flush bit (RFC 6762 section 10.2). */
    if (mdnsAppendRecordWire(pkt, max, &pos, &ancount, rs->rs_TypeName,
            DNS_TYPE_PTR, DNS_CLASS_IN, ttl, rs->rs_Name,
            (UWORD)mdnsWireNameLength(rs->rs_Name)) != 0) {
        return FALSE;
    }

    length = srv_data(md, rs, data, (LONG)sizeof(data));
    if (length < 0 ||
        mdnsAppendRecordWire(pkt, max, &pos, &ancount, rs->rs_Name,
            DNS_TYPE_SRV, unique, srvttl, data, (UWORD)length) != 0) {
        return FALSE;
    }

    {
        const UBYTE *txt;

        length = txt_ref(rs, &txt);
        if (mdnsAppendRecordWire(pkt, max, &pos, &ancount, rs->rs_Name,
                DNS_TYPE_TXT, unique, ttl, txt, (UWORD)length) != 0) {
            return FALSE;
        }
    }

    /* The address of the SRV record's host, so that whoever receives this
     * need not ask for it. */
    if (PROXY(rs) && withAddr) {
        UWORD i;

        for (i = 0; i < rs->rs_Reg.address_count; i++) {
            const struct dnssd_model_wire_address *a = &rs->rs_Reg.addresses[i];

            if (!address_on_link(md, a, ifindex)) {
                continue;
            }
            if (mdnsAppendRecordWire(pkt, max, &pos, &ancount,
                    rs->rs_HostName, address_type(a), unique,
                    reply_ttl(md, goodbye ? 0 : MDNS_TTL_HOST), a->bytes,
                    a->length) != 0) {
                return FALSE;
            }
        }
    }
    if (!PROXY(rs) && withAddr && !goodbye && own_host_name(md, target) > 0) {
        static const UWORD families[2] = {
            DNSSD_ADDRESS_IPV4, DNSSD_ADDRESS_IPV6
        };
        static const UWORD types[2] = { DNS_TYPE_A, DNS_TYPE_AAAA };
        size_t family;

        for (family = 0u; family < 2u; ++family) {
            size_t ordinal;

            for (ordinal = 0u; ; ++ordinal) {
                const struct dnssd_address_entry *address =
                    mdnsNetAddressAt(md, ifindex, families[family], ordinal);

                if (address == NULL)
                    break;
                if (mdnsAppendRecordWire(pkt, max, &pos, &ancount,
                        target, types[family], unique,
                        reply_ttl(md, MDNS_TTL_HOST), address->value.bytes,
                        address->value.length) != 0)
                    return FALSE;
            }
        }
    }

    *posp = pos;
    *ancountp = ancount;
    return TRUE;
}

static UWORD
all_subtypes(const struct MDNSRegisteredService *rs)
{
    return rs->rs_Reg.subtype_count == 0 ? 0 :
        (UWORD)((1UL << rs->rs_Reg.subtype_count) - 1);
}

/* Append one subtype's shared PTR. Its target is the ordinary instance
 * name; SRV, TXT and address records remain owned by that base instance. */
static BOOL
subtype_append(const struct MDNSDaemon *md,
    const struct MDNSRegisteredService *rs, LONG subtype, BOOL goodbye,
    UBYTE *pkt, LONG *posp, UWORD *ancountp)
{
    UBYTE owner[DNSSD_REG_NAME_MAX];
    LONG pos = *posp;
    UWORD ancount = *ancountp;
    ULONG ttl = reply_ttl(md, goodbye ? 0 :
        (rs->rs_TTL != 0 ? rs->rs_TTL : MDNS_TTL_OTHER));

    if (dnssd_reg_subtype_name(&rs->rs_Reg, (unsigned)subtype, owner) == 0 ||
        mdnsAppendRecordWire(pkt, MDNS_RESPONSE_MAX, &pos, &ancount, owner,
            DNS_TYPE_PTR, DNS_CLASS_IN, ttl, rs->rs_Name,
            (UWORD)mdnsWireNameLength(rs->rs_Name)) != 0) {
        return FALSE;
    }
    *posp = pos;
    *ancountp = ancount;
    return TRUE;
}

/* Send selected subtype PTRs as one or more bounded packets. Keeping them
 * separate lets the registration retain the existing 1400-byte service
 * response bound even when it has several maximum-length subtype labels. */
static void
subtypes_send(struct MDNSDaemon *md, struct MDNSRegisteredService *rs,
    BOOL goodbye, UWORD mask)
{
    UBYTE *pkt = md->md_TxBuf;
    struct MDNSHeader *hdr = (struct MDNSHeader *)pkt;
    LONG i, subtype;

    if (mask == 0) {
        return;
    }
    for (i = 0; i < md->md_IfCount; i++) {
        LONG pos = mdnsResponseBegin(md, pkt);
        UWORD ancount = 0;

        if (md->md_SendIf >= 0 && i != md->md_SendIf) {
            continue;
        }
        for (subtype = 0; subtype < rs->rs_Reg.subtype_count; subtype++) {
            if ((mask & (1U << subtype)) == 0) {
                continue;
            }
            if (!subtype_append(md, rs, subtype, goodbye, pkt, &pos,
                    &ancount)) {
                if (ancount != 0) {
                    hdr->ancount = htons(ancount);
                    mdnsNetSendOn(md, i, pkt, pos);
                    if (!mdnsReplyIsUnicast(md)) {
                        mdnsReflectPacket(md, pkt, pos, i,
                            MDNS_REFLECT_DISCOVERY | MDNS_REFLECT_QUERY,
                            NULL);
                    }
                }
                pos = mdnsResponseBegin(md, pkt);
                ancount = 0;
                if (!subtype_append(md, rs, subtype, goodbye, pkt, &pos,
                        &ancount)) {
                    continue;
                }
            }
        }
        if (ancount != 0) {
            hdr->ancount = htons(ancount);
            mdnsNetSendOn(md, i, pkt, pos);
            if (!mdnsReplyIsUnicast(md)) {
                mdnsReflectPacket(md, pkt, pos, i,
                    MDNS_REFLECT_DISCOVERY | MDNS_REFLECT_QUERY, NULL);
            }
        }
    }
}

static void
subtypes_reflect(struct MDNSDaemon *md,
    const struct MDNSRegisteredService *rs, BOOL goodbye, LONG ifindex,
    UWORD consumers, APTR queryHandle)
{
    UBYTE *pkt = md->md_TxBuf;
    struct MDNSHeader *hdr = (struct MDNSHeader *)pkt;
    LONG subtype, pos = mdnsResponseBegin(md, pkt);
    UWORD ancount = 0;

    for (subtype = 0; subtype < rs->rs_Reg.subtype_count; subtype++) {
        if (!subtype_append(md, rs, subtype, goodbye, pkt, &pos, &ancount)) {
            if (ancount != 0) {
                hdr->ancount = htons(ancount);
                mdnsReflectPacket(md, pkt, pos, ifindex, consumers,
                    queryHandle);
            }
            pos = mdnsResponseBegin(md, pkt);
            ancount = 0;
            if (!subtype_append(md, rs, subtype, goodbye, pkt, &pos,
                    &ancount)) {
                continue;
            }
        }
    }
    if (ancount != 0) {
        hdr->ancount = htons(ancount);
        mdnsReflectPacket(md, pkt, pos, ifindex, consumers, queryHandle);
    }
}

/* One local type-enumeration record. A type is withdrawn only after its last
 * announced registration is gone. */
static void
type_reflect(struct MDNSDaemon *md, const struct MDNSRegisteredService *rs,
    BOOL goodbye, LONG ifindex, UWORD consumers, APTR queryHandle)
{
    UBYTE *pkt = md->md_TxBuf;
    struct MDNSHeader *hdr = (struct MDNSHeader *)pkt;
    LONG pos;
    UWORD ancount = 0;

    if (goodbye) {
        struct Node *n;

        FOR_SERVICES(md, n) {
            const struct MDNSRegisteredService *other =
                (const struct MDNSRegisteredService *)n;

            if (other != rs && other->rs_State == MDNS_NAME_ANNOUNCED &&
                names_equal(other->rs_TypeName, rs->rs_TypeName)) {
                return;
            }
        }
    }
    pos = mdnsResponseBegin(md, pkt);
    if (mdnsAppendRecordWire(pkt, MDNS_RESPONSE_MAX, &pos, &ancount,
            services_name, DNS_TYPE_PTR, DNS_CLASS_IN,
            reply_ttl(md, goodbye ? 0 : MDNS_TTL_OTHER), rs->rs_TypeName,
            (UWORD)mdnsWireNameLength(rs->rs_TypeName)) != 0) {
        return;
    }
    hdr->ancount = htons(ancount);
    mdnsReflectPacket(md, pkt, pos, ifindex, consumers, queryHandle);
}

static void
service_reflect(struct MDNSDaemon *md, struct MDNSRegisteredService *rs,
    BOOL goodbye, LONG ifindex, UWORD consumers, APTR queryHandle)
{
    UBYTE *pkt = md->md_TxBuf;
    struct MDNSHeader *hdr = (struct MDNSHeader *)pkt;
    LONG pos;
    UWORD ancount = 0;

    subtypes_reflect(md, rs, goodbye, ifindex, consumers, queryHandle);
    pos = mdnsResponseBegin(md, pkt);
    if (service_append(md, rs, goodbye, ifindex, pkt, &pos, &ancount,
            TRUE)) {
        hdr->ancount = htons(ancount);
        mdnsReflectPacket(md, pkt, pos, ifindex, consumers, queryHandle);
        type_reflect(md, rs, goodbye, ifindex, consumers, queryHandle);
    }
}

void
mdnsRegAnswerLocal(struct MDNSDaemon *md, UWORD consumers, APTR queryHandle)
{
    struct Node *n;
    LONG i;

    FOR_SERVICES(md, n) {
        struct MDNSRegisteredService *rs =
            (struct MDNSRegisteredService *)n;

        if (rs->rs_State != MDNS_NAME_ANNOUNCED) {
            continue;
        }
        for (i = 0; i < md->md_IfCount; i++) {
            service_reflect(md, rs, FALSE, i, consumers, queryHandle);
        }
    }
}

/* One service's records as a response of their own, to the interface a
 * question came in on or else to every one. */
static void
service_send(struct MDNSDaemon *md, struct MDNSRegisteredService *rs,
    BOOL goodbye, UWORD subtypeMask)
{
    UBYTE *pkt = md->md_TxBuf;
    struct MDNSHeader *hdr = (struct MDNSHeader *)pkt;
    LONG i;

    subtypes_send(md, rs, goodbye, subtypeMask);
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
            if (!mdnsReplyIsUnicast(md) && !goodbye) {
                rs->rs_Multicast = mdnsNow();
                rs->rs_MulticastTick[i] = mdnsTicks();
            }
            if (!mdnsReplyIsUnicast(md)) {
                mdnsReflectPacket(md, pkt, pos, i,
                    MDNS_REFLECT_DISCOVERY | MDNS_REFLECT_QUERY, NULL);
                type_reflect(md, rs, goodbye, i,
                    MDNS_REFLECT_DISCOVERY | MDNS_REFLECT_QUERY, NULL);
            }
        }
    }
}

static BOOL
sent_recently(ULONG sent, ULONG now)
{
    return sent != 0 && (LONG)(now - sent) < 50;
}

static void types_send(struct MDNSDaemon *md, LONG ifindex, BOOL multicast);

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

#define ANSWER_FAMILY4 1
#define ANSWER_FAMILY6 2

static UWORD
answer_family(const struct MDNSDaemon *md)
{
    return md->md_SendFamily == AF_INET6 ? ANSWER_FAMILY6 : ANSWER_FAMILY4;
}

static void
send_delayed(struct MDNSDaemon *md, LONG ifindex, const UBYTE *pkt, LONG length,
    UWORD families)
{
    UWORD saved = md->md_SendFamily;

    if (families & ANSWER_FAMILY4) {
        md->md_SendFamily = AF_INET;
        mdnsNetSendOn(md, ifindex, pkt, length);
    }
    if (families & ANSWER_FAMILY6) {
        md->md_SendFamily = AF_INET6;
        mdnsNetSendOn(md, ifindex, pkt, length);
    }
    md->md_SendFamily = saved;
}

static void
answers_flush(struct MDNSDaemon *md, ULONG now)
{
    UBYTE *pkt = md->md_TxBuf;
    struct MDNSHeader *hdr = (struct MDNSHeader *)pkt;
    struct Node *n;
    BOOL due = FALSE;
    BOOL types = md->md_TypesAnswerTick != 0;
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
    if (types && (LONG)(now - md->md_TypesAnswerTick) >= 0) {
        due = TRUE;
    } else if (types) {
        md->md_AnswerPending = TRUE;
    }
    if (!due) {
        return;
    }
    for (i = 0; i < md->md_IfCount; i++) {
        LONG pos = (LONG)sizeof(struct MDNSHeader);
        UWORD ancount = 0;
        UWORD families = 0;
        BOOL addrDone = FALSE;

        mdnsInitHeader(hdr, DNS_FLAG_QR | DNS_FLAG_AA, 0);
        FOR_SERVICES(md, n) {
            struct MDNSRegisteredService *rs =
                (struct MDNSRegisteredService *)n;

            if (rs->rs_AnswerTick == 0 ||
                (rs->rs_AnswerIf >= 0 && rs->rs_AnswerIf != i) ||
                sent_recently(rs->rs_MulticastTick[i], now)) {
                continue;
            }
            families |= rs->rs_AnswerFamilies;
            /* the host's address once; a stand-in's host has its own */
            if (!service_append(md, rs, FALSE, i, pkt, &pos, &ancount,
                    PROXY(rs) || !addrDone)) {
                /* full: send what there is and start another */
                if (ancount != 0) {
                    hdr->ancount = htons(ancount);
                    send_delayed(md, i, pkt, pos, families);
                }
                mdnsInitHeader(hdr, DNS_FLAG_QR | DNS_FLAG_AA, 0);
                pos = (LONG)sizeof(struct MDNSHeader);
                ancount = 0;
                families = rs->rs_AnswerFamilies;
                addrDone = FALSE;
                if (!service_append(md, rs, FALSE, i, pkt, &pos, &ancount,
                        TRUE)) {
                    continue;
                }
            }
            if (!PROXY(rs)) {
                addrDone = TRUE;
            }
            rs->rs_Multicast = mdnsNow();
            rs->rs_MulticastTick[i] = now;
        }
        if (ancount != 0) {
            hdr->ancount = htons(ancount);
            send_delayed(md, i, pkt, pos, families);
        }
        FOR_SERVICES(md, n) {
            struct MDNSRegisteredService *rs =
                (struct MDNSRegisteredService *)n;

            if (rs->rs_State == MDNS_NAME_ANNOUNCED &&
                rs->rs_AnswerTick != 0 &&
                (LONG)(now - rs->rs_AnswerTick) >= 0 &&
                (rs->rs_AnswerIf < 0 || rs->rs_AnswerIf == i) &&
                rs->rs_AnswerSubtypes != 0) {
                LONG savedIf = md->md_SendIf;
                UWORD savedFamily = md->md_SendFamily;

                md->md_SendIf = i;
                if (rs->rs_AnswerFamilies ==
                        (ANSWER_FAMILY4 | ANSWER_FAMILY6)) {
                    md->md_SendFamily = 0;
                } else if (rs->rs_AnswerFamilies & ANSWER_FAMILY6) {
                    md->md_SendFamily = AF_INET6;
                } else {
                    md->md_SendFamily = AF_INET;
                }
                subtypes_send(md, rs, FALSE, rs->rs_AnswerSubtypes);
                md->md_SendFamily = savedFamily;
                md->md_SendIf = savedIf;
            }
        }
        if (types && (md->md_TypesAnswerIf < 0 ||
                md->md_TypesAnswerIf == i)) {
            UWORD saved = md->md_SendFamily;

            if (md->md_TypesAnswerFamilies ==
                    (ANSWER_FAMILY4 | ANSWER_FAMILY6)) {
                md->md_SendFamily = 0;
                types_send(md, i, TRUE);
            } else if (md->md_TypesAnswerFamilies & ANSWER_FAMILY4) {
                md->md_SendFamily = AF_INET;
                types_send(md, i, TRUE);
            } else if (md->md_TypesAnswerFamilies & ANSWER_FAMILY6) {
                md->md_SendFamily = AF_INET6;
                types_send(md, i, TRUE);
            }
            md->md_SendFamily = saved;
        }
    }
    FOR_SERVICES(md, n) {
        struct MDNSRegisteredService *rs = (struct MDNSRegisteredService *)n;

        rs->rs_AnswerTick = 0;
        rs->rs_AnswerFamilies = 0;
        rs->rs_AnswerSubtypes = 0;
    }
    md->md_TypesAnswerTick = 0;
    md->md_TypesAnswerFamilies = 0;
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
            service_send(md, rs, TRUE, all_subtypes(rs));
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
            service_send(md, rs, FALSE, all_subtypes(rs));
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
        rs->rs_Reg.state = DNSSD_REG_PENDING;
        rs->rs_Step = 0;
        rs->rs_NextTick = mdnsTicks();
        rs->rs_AnswerTick = 0;
        rs->rs_AnswerFamilies = 0;
        rs->rs_AnswerSubtypes = 0;
        memset(rs->rs_MulticastTick, 0, sizeof(rs->rs_MulticastTick));
        memset(rs->rs_TypeTick, 0, sizeof(rs->rs_TypeTick));
    }
    md->md_TypesAnswerTick = 0;
    md->md_TypesAnswerFamilies = 0;
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
    UBYTE data[6 + 256];
    const UBYTE *txt;
    LONG pos, slot;
    UWORD qclass = (UWORD)(DNS_CLASS_IN | (rs->rs_Step == 0 ? DNS_QU_BIT : 0));
    UWORD saved = md->md_SendFamily;

    /* every record that is to be unique is probed for (RFC 6762 section
     * 8.1): the instance's SRV and TXT, and the addresses of a host it
     * stands in for - those of a link only on that link */
    for (slot = 0; slot < md->md_IfCount; slot++) {
        UWORD nscount = 0, i;
        LONG length = srv_data(md, rs, data, (LONG)sizeof(data));

        if (length < 0 ||
            mdnsBuildQueryWire(pkt, (LONG)MDNS_MAX_PACKET, 0, rs->rs_Name,
                DNS_TYPE_ANY, qclass, &pos) != 0) {
            return;
        }
        if (PROXY(rs)) {
            LONG used = mdnsWireNameLength(rs->rs_HostName);

            if (pos + used + 4 > (LONG)MDNS_MAX_PACKET) {
                return;
            }
            memcpy(pkt + pos, rs->rs_HostName, (size_t)used);
            pos += used;
            pkt[pos++] = 0;
            pkt[pos++] = DNS_TYPE_ANY;
            pkt[pos++] = (UBYTE)(qclass >> 8);
            pkt[pos++] = (UBYTE)qclass;
            hdr->qdcount = htons(2);
        }
        if (mdnsAppendRecordWire(pkt, (LONG)MDNS_MAX_PACKET, &pos, &nscount,
                rs->rs_Name, DNS_TYPE_SRV, DNS_CLASS_IN, MDNS_TTL_HOST, data,
                (UWORD)length) != 0) {
            return;
        }
        length = txt_ref(rs, &txt);
        if (mdnsAppendRecordWire(pkt, (LONG)MDNS_MAX_PACKET, &pos, &nscount,
                rs->rs_Name, DNS_TYPE_TXT, DNS_CLASS_IN,
                rs->rs_TTL != 0 ? rs->rs_TTL : MDNS_TTL_OTHER, txt,
                (UWORD)length) != 0) {
            return;
        }
        for (i = 0; PROXY(rs) && i < rs->rs_Reg.address_count; i++) {
            const struct dnssd_model_wire_address *a = &rs->rs_Reg.addresses[i];

            if (address_on_link(md, a, slot) &&
                mdnsAppendRecordWire(pkt, (LONG)MDNS_MAX_PACKET, &pos, &nscount,
                    rs->rs_HostName, address_type(a), DNS_CLASS_IN, MDNS_TTL_HOST,
                    a->bytes, a->length) != 0) {
                return;
            }
        }
        hdr->nscount = htons(nscount);
        md->md_SendFamily = 0;
        mdnsNetSendOn(md, slot, pkt, pos);
        md->md_SendFamily = saved;
    }
}

/* The next name after a conflict ("Name (2)", dnssd_reg_rename()), not
 * one another local service of the type already owns. */
static void
service_rename(const struct MDNSDaemon *md, struct MDNSRegisteredService *rs)
{
    BOOL used;

    do {
        struct Node *n;

        dnssd_reg_rename(&rs->rs_Reg);
        used = FALSE;
        FOR_SERVICES(md, n) {
            const struct MDNSRegisteredService *other =
                (const struct MDNSRegisteredService *)n;

            if (other != rs && dnssd_reg_same_service(&other->rs_Reg, &rs->rs_Reg)) {
                used = TRUE;
                break;
            }
        }
    } while (used);
    names_build(rs);
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

    /* The name of a host a service stands in for, with addresses that are
     * not the ones given: another host has the name. It is not this
     * host's to rename - the client named it - so the registration ends
     * (mdnsRegTick() tells its client). */
    if (r->ttl != 0 && (r->rclass & 0x7fff) == DNS_CLASS_IN &&
        (r->type == DNS_TYPE_A || r->type == DNS_TYPE_AAAA)) {
        FOR_SERVICES(md, n) {
            struct MDNSRegisteredService *rs = (struct MDNSRegisteredService *)n;
            UWORD i;

            if (!PROXY(rs) || rs->rs_Notify == NULL ||
                rs->rs_State == MDNS_NAME_CONFLICT ||
                !names_equal(rs->rs_HostName, r->wire)) {
                continue;
            }
            for (i = 0; i < rs->rs_Reg.address_count; i++) {
                const struct dnssd_model_wire_address *a = &rs->rs_Reg.addresses[i];

                if (address_type(a) == r->type && a->length == r->rdlength &&
                    memcmp(a->bytes, r->rdata, a->length) == 0) {
                    break;
                }
            }
            if (i == rs->rs_Reg.address_count) {
                rs->rs_State = MDNS_NAME_CONFLICT;
                rs->rs_Reg.state = DNSSD_REG_CONFLICT;
                rs->rs_NextTick = mdnsTicks();
            }
        }
        return;
    }
    if (r->ttl == 0 || (r->rclass & 0x7fff) != DNS_CLASS_IN ||
        (r->type != DNS_TYPE_SRV && r->type != DNS_TYPE_TXT)) {
        return;
    }
    FOR_SERVICES(md, n) {
        struct MDNSRegisteredService *rs = (struct MDNSRegisteredService *)n;

        if (!names_equal(rs->rs_Name, r->wire)) {
            continue;
        }
        if (r->type == DNS_TYPE_SRV) {
            UBYTE target[256], theirs[256];

            if (r->rdlength < 7 ||
                mdnsSkipName(pkt, pktlen, r->rdoffset + 6) !=
                    r->rdoffset + r->rdlength ||
                mdnsWireNameRead(pkt, pktlen, r->rdoffset + 6, theirs) < 0) {
                continue;
            }
            if (target_name(md, rs, target) > 0 &&
                r->rdata[0] == 0 && r->rdata[1] == 0 &&
                r->rdata[2] == 0 && r->rdata[3] == 0 &&
                r->rdata[4] == (UBYTE)(rs->rs_Reg.port >> 8) &&
                r->rdata[5] == (UBYTE)rs->rs_Reg.port &&
                names_equal(target, theirs)) {
                continue;
            }
        } else {
            const UBYTE *expected;
            LONG length = txt_ref(rs, &expected);

            /* a TXT longer than the parsed copy is compared as far as it
             * goes: its length must match all the same */
            if (length == r->rdlength &&
                memcmp(expected, r->rdata, (size_t)(length < (LONG)sizeof(r->rdata) ?
                    length : (LONG)sizeof(r->rdata))) == 0) {
                continue;
            }
        }
        service_rename(md, rs);
        rs->rs_State = MDNS_NAME_PROBING;
        rs->rs_Reg.state = DNSSD_REG_PENDING;
        rs->rs_Step = 0;
        rs->rs_NextTick = mdnsTicks() + MDNS_PROBE_TICKS;
    }
}

/* ASCII letters of a wire name in lower case (its length bytes, below 64,
 * are no letters). */
static void
name_lower(UBYTE *name, LONG length)
{
    LONG i;

    for (i = 0; i < length; i++) {
        if (name[i] >= 'A' && name[i] <= 'Z') {
            name[i] = (UBYTE)(name[i] + ('a' - 'A'));
        }
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
    UBYTE target[256];
    LONG wirelen;

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
        mdnsWireNameRead(pkt, pktlen, r->rdoffset + 6, target) < 0 ||
        (wirelen = mdnsWireNameLength(target)) < 0 || 6 + wirelen > outmax) {
        return -1;
    }
    name_lower(target, wirelen);
    memcpy(out, r->rdata, 6);
    memcpy(out + 6, target, (size_t)wirelen);
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
 * Two hosts probing for one name at once (RFC 6762 section 8.2): each
 * set of records, sorted by class, type and data, is compared record by
 * record; the first difference decides, and when one set runs out the
 * longer one is later. <0 when this host's set (n records) is the
 * earlier - it gives way - 0 when they are the same, >0 when it is later.
 */
#define PROBE_SET_MAX   (DNSSD_REG_ADDRESSES + 2)

struct ProbeRecord
{
    UWORD           pr_Class, pr_Type;
    LONG            pr_Length;
    const UBYTE    *pr_Data;
};

static void
probe_sort(struct ProbeRecord *set, LONG n)
{
    LONG i, j;

    for (i = 1; i < n; i++) {
        struct ProbeRecord r = set[i];

        for (j = i; j > 0 && probe_compare(r.pr_Class, r.pr_Type, r.pr_Data,
                r.pr_Length, set[j - 1].pr_Class, set[j - 1].pr_Type,
                set[j - 1].pr_Data, set[j - 1].pr_Length) < 0; j--) {
            set[j] = set[j - 1];
        }
        set[j] = r;
    }
}

static LONG
probe_tiebreak(struct ProbeRecord *mine, LONG n, const UBYTE *name,
    const struct MDNSRecord *theirs, LONG count, const UBYTE *pkt, LONG pktlen)
{
    /* their data, uncompressed, in storage of its own: mdnsd has one task */
    static UBYTE data[MDNS_PROPOSED_MAX][512];
    struct ProbeRecord other[MDNS_PROPOSED_MAX];
    LONG m = 0, i, j;

    for (i = 0; i < count && m < MDNS_PROPOSED_MAX; i++) {
        LONG length;

        if (!names_equal(theirs[i].wire, name) ||
            (length = probe_rdata(&theirs[i], pkt, pktlen, data[m],
                (LONG)sizeof(data[m]))) < 0) {
            continue;
        }
        other[m].pr_Class = theirs[i].rclass;
        other[m].pr_Type = theirs[i].type;
        other[m].pr_Length = length;
        other[m].pr_Data = data[m];
        for (j = 0; j < m; j++) {
            if (probe_compare(other[m].pr_Class, other[m].pr_Type, data[m],
                    length, other[j].pr_Class, other[j].pr_Type,
                    other[j].pr_Data, other[j].pr_Length) == 0) {
                break;
            }
        }
        if (j == m) {
            m++;                /* duplicates do not add to a set */
        }
    }
    if (m == 0) {
        return 1;               /* nobody else probes for the name */
    }
    probe_sort(mine, n);
    probe_sort(other, m);
    for (i = 0; i < n && i < m; i++) {
        LONG d = probe_compare(mine[i].pr_Class, mine[i].pr_Type, mine[i].pr_Data,
            mine[i].pr_Length, other[i].pr_Class, other[i].pr_Type,
            other[i].pr_Data, other[i].pr_Length);
        if (d != 0) {
            return d;
        }
    }
    return n - m;
}

/*
 * Another host is probing, and these are the records it proposes. For a
 * name a service of this host is probing for too - its instance's, or the
 * name of the host it stands in for - the two sets are compared and the
 * host whose set is later keeps going; this one, if it loses, waits a
 * second and probes again, by which time the winner has announced and the
 * usual conflict decides (RFC 6762 section 8.2).
 */
void
mdnsRegHandleProbe(const struct MDNSDaemon *md,
    const struct MDNSRecord *theirs, LONG count, const UBYTE *pkt,
    LONG pktlen)
{
    static UBYTE srv[6 + 256];
    struct Node *n;

    FOR_SERVICES(md, n) {
        struct MDNSRegisteredService *rs = (struct MDNSRegisteredService *)n;
        struct ProbeRecord mine[PROBE_SET_MAX];
        LONG k;

        if (rs->rs_State != MDNS_NAME_PROBING || rs->rs_Step == 0) {
            continue;
        }
        /* the instance: its TXT and SRV, the SRV's name in lower case */
        mine[1].pr_Length = srv_data(md, rs, srv, (LONG)sizeof(srv));
        if (mine[1].pr_Length < 0) {
            continue;
        }
        name_lower(srv + 6, mine[1].pr_Length - 6);
        mine[0].pr_Class = mine[1].pr_Class = DNS_CLASS_IN;
        mine[0].pr_Type = DNS_TYPE_TXT;
        mine[0].pr_Length = txt_ref(rs, &mine[0].pr_Data);
        mine[1].pr_Type = DNS_TYPE_SRV;
        mine[1].pr_Data = srv;
        if (probe_tiebreak(mine, 2, rs->rs_Name, theirs, count, pkt, pktlen) < 0) {
            rs->rs_Step = 0;
            rs->rs_NextTick = mdnsTicks() + 50;
            continue;
        }
        /* the host it stands in for: its addresses */
        if (!PROXY(rs)) {
            continue;
        }
        for (k = 0; k < rs->rs_Reg.address_count; k++) {
            mine[k].pr_Class = DNS_CLASS_IN;
            mine[k].pr_Type = address_type(&rs->rs_Reg.addresses[k]);
            mine[k].pr_Length = rs->rs_Reg.addresses[k].length;
            mine[k].pr_Data = rs->rs_Reg.addresses[k].bytes;
        }
        if (probe_tiebreak(mine, k, rs->rs_HostName, theirs, count, pkt, pktlen) < 0) {
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
    struct Node *n, *next;
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

    for (n = md->md_Services.lh_Head; n->ln_Succ != NULL; n = next) {
        struct MDNSRegisteredService *rs = (struct MDNSRegisteredService *)n;

        /* its client may withdraw it while it is told */
        next = n->ln_Succ;
        if (rs->rs_State == MDNS_NAME_CONFLICT) {
            MDNSRegNotify notify = rs->rs_Notify;

            if (notify != NULL) {
                rs->rs_Notify = NULL;           /* told once */
                notify(rs->rs_NotifyData, rs, MDNSD_ERR_DUPLICATE);
            }
            continue;
        }
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
            rs->rs_Reg.state = DNSSD_REG_ESTABLISHED;
            rs->rs_Step = 1;
            rs->rs_NextTick = now + 50;
            service_send(md, rs, FALSE, all_subtypes(rs));
            if (rs->rs_Notify != NULL) {
                rs->rs_Notify(rs->rs_NotifyData, rs, MDNSD_OK);
            }
        } else if (rs->rs_Step < MDNS_ANNOUNCE_COUNT &&
                   (LONG)(now - rs->rs_NextTick) >= 0 &&
                   (hostReady || PROXY(rs))) {
            rs->rs_Step++;
            service_send(md, rs, FALSE, all_subtypes(rs));
        }
    }
    return probing;
}

/* Marks every registration of one type. If its first registration goes away,
 * another one must still remember when their shared type PTR last went out. */
static void
type_sent(const struct MDNSDaemon *md, const UBYTE *typeName, LONG ifindex, ULONG now)
{
    struct Node *n;

    FOR_SERVICES(md, n) {
        struct MDNSRegisteredService *rs = (struct MDNSRegisteredService *)n;

        if (names_equal(rs->rs_TypeName, typeName)) {
            rs->rs_TypeTick[ifindex] = now;
        }
    }
}

/* Answers "which service types are there" (RFC 6763 section 9) with one
 * PTR record per type. A multicast answer is confined to ifindex and leaves
 * out records that were multicast there less than a second ago. */
static void
types_send(struct MDNSDaemon *md, LONG ifindex, BOOL multicast)
{
    UBYTE *pkt = md->md_TxBuf;
    struct MDNSHeader *hdr = (struct MDNSHeader *)pkt;
    struct Node *n;
    LONG pos = mdnsResponseBegin(md, pkt);
    UWORD ancount = 0;
    ULONG now = mdnsTicks();

    FOR_SERVICES(md, n) {
        const struct MDNSRegisteredService *rs =
            (const struct MDNSRegisteredService *)n;
        const struct Node *m;

        if (rs->rs_State != MDNS_NAME_ANNOUNCED) {
            continue;
        }
        if (multicast && sent_recently(rs->rs_TypeTick[ifindex], now)) {
            continue;
        }
        /* once per type */
        for (m = md->md_Services.lh_Head; m != n; m = m->ln_Succ) {
            const struct MDNSRegisteredService *earlier =
                (const struct MDNSRegisteredService *)m;

            if (earlier->rs_State == MDNS_NAME_ANNOUNCED &&
                names_equal(earlier->rs_TypeName, rs->rs_TypeName)) {
                break;
            }
        }
        if (m != n) {
            continue;
        }
        if (mdnsAppendRecordWire(pkt, (LONG)MDNS_MAX_PACKET, &pos, &ancount,
                services_name, DNS_TYPE_PTR, DNS_CLASS_IN,
                reply_ttl(md, MDNS_TTL_OTHER), rs->rs_TypeName,
                (UWORD)mdnsWireNameLength(rs->rs_TypeName)) == 0 && multicast) {
            type_sent(md, rs->rs_TypeName, ifindex, now);
        }
    }
    if (ancount != 0) {
        hdr->ancount = htons(ancount);
        if (multicast) {
            mdnsNetSendOn(md, ifindex, pkt, pos);
        } else {
            mdnsNetSendAll(md, pkt, pos);
        }
    }
}

static void
types_schedule(struct MDNSDaemon *md)
{
    if (md->md_TypesAnswerTick == 0) {
        md->md_TypesAnswerTick = mdnsTicks() + answer_delay(md);
        if (md->md_TypesAnswerTick == 0) {
            md->md_TypesAnswerTick = 1;
        }
        md->md_TypesAnswerIf = md->md_SendIf;
    } else if (md->md_TypesAnswerIf != md->md_SendIf) {
        md->md_TypesAnswerIf = -1;
    }
    md->md_TypesAnswerFamilies |= answer_family(md);
    md->md_AnswerPending = TRUE;
}

/*
 * One question of a received query. "known" holds the PTR targets the asker
 * says it already has (RFC 6762 section 7.1). A service answers at most
 * once for one query packet, always with all of its records.
 *
 * A question from another port than 5353 comes from a resolver that speaks
 * plain DNS: it gets the matching services together in an answer back to
 * it alone, at once (section 6.7). A question with the unicast-response bit
 * is answered to the asker alone when the group has had the records not
 * long ago, and goes to the group otherwise (section 5.4).
 */
void
mdnsRegHandleQuery(struct MDNSDaemon *md, struct MDNSQuestion *q,
    const struct MDNSKnownAnswer *known, LONG knownCount)
{
    UBYTE *pkt = md->md_TxBuf;
    struct MDNSHeader *hdr = (struct MDNSHeader *)pkt;
    struct Node *n;
    BOOL any = q->qtype == DNS_TYPE_ANY;
    BOOL legacy = md->md_QueryPort != MDNS_DNS_PORT;
    BOOL direct = !legacy && (q->qclass & DNS_QU_BIT) != 0;
    BOOL legacyAddrDone = FALSE;
    LONG legacyPos = 0;
    UWORD legacyAnswers = 0;
    ULONG now = mdnsNow();

    if ((any || q->qtype == DNS_TYPE_PTR) && names_equal(q->qwire, services_name)) {
        if (legacy) {
            reply_to_asker(md, q, TRUE);
            types_send(md, md->md_SendIf, FALSE);
            reply_to_group(md);
        } else {
            types_schedule(md);
        }
        return;
    }

    if (legacy) {
        reply_to_asker(md, q, TRUE);
        legacyPos = mdnsResponseBegin(md, pkt);
    }

    FOR_SERVICES(md, n) {
        struct MDNSRegisteredService *rs = (struct MDNSRegisteredService *)n;
        BOOL answer = FALSE, shared = FALSE;
        UWORD subtypeMask = 0;

        if (rs->rs_State != MDNS_NAME_ANNOUNCED ||
            rs->rs_Answered == md->md_PacketSerial) {
            continue;
        }
        /* a service of this host waits while the host's name is in doubt */
        if (!PROXY(rs) && md->md_HostState != MDNS_NAME_ANNOUNCED) {
            continue;
        }

        if (names_equal(q->qwire, rs->rs_Name)) {
            answer = any || q->qtype == DNS_TYPE_SRV ||
                q->qtype == DNS_TYPE_TXT;
        } else if (PROXY(rs) && names_equal(q->qwire, rs->rs_HostName)) {
            /* the host a service stands in for; this host's own name is
             * answered in mdns_host.c */
            answer = any || q->qtype == DNS_TYPE_A || q->qtype == DNS_TYPE_AAAA;
        } else if (any || q->qtype == DNS_TYPE_PTR) {
            LONG subtype = subtype_index(rs, q->qwire);

            if (names_equal(q->qwire, rs->rs_TypeName) || subtype >= 0) {
                LONG k;
                const UBYTE *owner = rs->rs_TypeName;

                if (subtype >= 0) {
                    subtypeMask = (UWORD)(1U << subtype);
                    owner = q->qwire;
                }

                answer = TRUE;
                shared = TRUE;
                for (k = 0; k < knownCount; k++) {
                    ULONG ttl = rs->rs_TTL != 0 ? rs->rs_TTL : MDNS_TTL_OTHER;

                    if (names_equal(known[k].owner, owner) &&
                        names_equal(known[k].target, rs->rs_Name) &&
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
        if (legacy) {
            LONG ifindex = md->md_SendIf >= 0 ? md->md_SendIf : 0;

            if (subtypeMask != 0) {
                if (legacyAnswers != 0) {
                    hdr->ancount = htons(legacyAnswers);
                    mdnsNetSendOn(md, ifindex, pkt, legacyPos);
                }
                subtypes_send(md, rs, FALSE, subtypeMask);
                legacyPos = mdnsResponseBegin(md, pkt);
                legacyAnswers = 0;
                legacyAddrDone = FALSE;
            }

            if (!service_append(md, rs, FALSE, ifindex, pkt, &legacyPos,
                    &legacyAnswers, PROXY(rs) || !legacyAddrDone)) {
                if (legacyAnswers != 0) {
                    hdr->ancount = htons(legacyAnswers);
                    mdnsNetSendOn(md, ifindex, pkt, legacyPos);
                }
                legacyPos = mdnsResponseBegin(md, pkt);
                legacyAnswers = 0;
                legacyAddrDone = FALSE;
                if (!service_append(md, rs, FALSE, ifindex, pkt, &legacyPos,
                        &legacyAnswers, TRUE)) {
                    continue;
                }
            }
            if (!PROXY(rs)) {
                legacyAddrDone = TRUE;
            }
        } else if (direct && rs->rs_Multicast != 0 &&
                now - rs->rs_Multicast <= MDNS_UNICAST_SECONDS) {
            /* to the asker alone, and at once: nobody else is waiting to
             * answer on a channel of two */
            reply_to_asker(md, q, FALSE);
            service_send(md, rs, FALSE, subtypeMask);
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
            rs->rs_AnswerFamilies |= answer_family(md);
            rs->rs_AnswerSubtypes |= subtypeMask;
            md->md_AnswerPending = TRUE;
        } else {
            /* a question for this one name: nobody else answers it */
            service_send(md, rs, FALSE, subtypeMask);
        }
    }
    if (legacy) {
        if (legacyAnswers != 0) {
            hdr->ancount = htons(legacyAnswers);
            mdnsNetSendOn(md, md->md_SendIf >= 0 ? md->md_SendIf : 0,
                pkt, legacyPos);
        }
        reply_to_group(md);
    }
}

/* The service whose reference a client has, or NULL. */
struct MDNSRegisteredService *
mdnsRegFind(const struct MDNSDaemon *md, CONST_APTR ref)
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
 * A service from its registration (dnssd_reg_init() has checked it): its
 * TXT data as it goes on the wire (txtLength bytes, 0 for one empty
 * string), the time to live of its own records (0 for the defaults), and
 * whose it is (owner: what the front end names its clients by). The name
 * is probed first; mdnsRegTick() announces it about a second from now, and
 * tells notify each time it is announced - under the next free name if
 * another host had it - or that the host it stands in for is another's.
 * *ref names it to mdnsRegUpdateRef()/mdnsRegRemoveRef().
 */
LONG
mdnsRegAddCanonical(struct MDNSDaemon *md, const struct dnssd_reg *reg,
    const UBYTE *txt, UWORD txtLength, ULONG ttl, ULONG owner,
    MDNSRegNotify notify, APTR data, APTR *ref)
{
    struct MDNSRegisteredService *rs;
    struct Node *n;

    if (reg == NULL || ref == NULL || txtLength > MDNS_TXT_DATA_MAX ||
        ttl > MDNSD_TTL_MAX || (txt == NULL && txtLength != 0)) {
        return MDNSD_ERR_BADPARAM;
    }
    /* one instance name per type on this host */
    FOR_SERVICES(md, n) {
        if (dnssd_reg_same_service(&((struct MDNSRegisteredService *)n)->rs_Reg, reg)) {
            return MDNSD_ERR_DUPLICATE;
        }
    }
    /* a host it stands in for is not this host, by its name now or its first */
    if (reg->host_len != 0) {
        UBYTE own[256];
        LONG base = mdnsTextLength(md->md_HostBase, MDNSD_HOST_MAX);

        if ((own_host_name(md, own) > 0 && own[0] == reg->host_len &&
             Strnicmp((STRPTR)own + 1, (STRPTR)reg->host, reg->host_len) == 0) ||
            (base == reg->host_len &&
             Strnicmp((STRPTR)md->md_HostBase, (STRPTR)reg->host, reg->host_len) == 0)) {
            return MDNSD_ERR_BADPARAM;
        }
    }
    rs = (struct MDNSRegisteredService *)AllocMem(sizeof(*rs), MEMF_CLEAR);
    if (rs == NULL) {
        return MDNSD_ERR_NOMEM;
    }
    if (!txt_set(rs, txt, txtLength)) {
        FreeMem(rs, sizeof(*rs));
        return MDNSD_ERR_NOMEM;
    }
    rs->rs_Reg = *reg;
    rs->rs_Reg.state = DNSSD_REG_PENDING;
    names_build(rs);
    rs->rs_TTL = ttl;
    rs->rs_Owner = owner;
    rs->rs_Notify = notify;
    rs->rs_NotifyData = data;
    rs->rs_ClientRef = (APTR)(IPTR)md->md_NextHandle++;
    rs->rs_State = MDNS_NAME_PROBING;
    rs->rs_Step = 0;
    rs->rs_NextTick = mdnsTicks();
    AddTail(&md->md_Services, &rs->rs_Node);
    *ref = rs->rs_ClientRef;
    return MDNSD_OK;
}

/* The goodbyes of a proxied host's addresses that go (those of old not in
 * the new set), on each link they were on (RFC 6762 section 10.1). */
static void
addresses_goodbye(struct MDNSDaemon *md, const struct MDNSRegisteredService *rs,
    const struct dnssd_model_wire_address *addrs, UWORD count)
{
    UBYTE *pkt = md->md_TxBuf;
    LONG slot;

    for (slot = 0; slot < md->md_IfCount; slot++) {
        LONG pos = (LONG)sizeof(struct MDNSHeader);
        UWORD ancount = 0, i, j;

        mdnsInitHeader((struct MDNSHeader *)pkt, DNS_FLAG_QR | DNS_FLAG_AA, 0);
        for (i = 0; i < rs->rs_Reg.address_count; i++) {
            const struct dnssd_model_wire_address *a = &rs->rs_Reg.addresses[i];

            for (j = 0; j < count; j++) {
                if (memcmp(&addrs[j], a, sizeof(*a)) == 0) {
                    break;
                }
            }
            if (j < count || !address_on_link(md, a, slot)) {
                continue;
            }
            mdnsAppendRecordWire(pkt, MDNS_RESPONSE_MAX, &pos, &ancount,
                rs->rs_HostName, address_type(a), DNS_CLASS_IN_FLUSH, 0,
                a->bytes, a->length);
        }
        if (ancount != 0) {
            ((struct MDNSHeader *)pkt)->ancount = htons(ancount);
            mdnsNetSendOn(md, slot, pkt, pos);
            mdnsReflectPacket(md, pkt, pos, slot,
                MDNS_REFLECT_DISCOVERY | MDNS_REFLECT_QUERY, NULL);
        }
    }
}

/*
 * A service changes its TXT data (txt not NULL), its port (not 0), the
 * addresses of the host it stands in for (addrCount not -1) or its time
 * to live (not 0): what changed is announced, an address that goes with
 * its goodbye. An update that changes nothing is not announced: the link
 * is told of changed data only (RFC 6762 sections 8.3 and 8.4).
 */
LONG
mdnsRegUpdateRef(struct MDNSDaemon *md, APTR ref, const UBYTE *txt,
    UWORD txtLength, UWORD port, const struct dnssd_model_wire_address *addrs,
    LONG addrCount, ULONG ttl)
{
    struct MDNSRegisteredService *rs = mdnsRegFind(md, ref);
    BOOL changed = FALSE;

    /* one whose host turned out to be another's has ended */
    if (rs == NULL || rs->rs_State == MDNS_NAME_CONFLICT) {
        return MDNSD_ERR_NOTFOUND;
    }
    if (txtLength > MDNS_TXT_DATA_MAX || ttl > MDNSD_TTL_MAX ||
        (addrCount >= 0 && (!PROXY(rs) || addrCount == 0 ||
            addrCount > DNSSD_REG_ADDRESSES))) {
        return MDNSD_ERR_BADPARAM;
    }
    if (addrCount > 0) {
        /* checked as a registration's are (the parts are the
         * registration's own; check is where they are copied to) */
        const struct dnssd_reg *r = &rs->rs_Reg;
        struct dnssd_reg check;
        struct dnssd_reg_parts p;

        memset(&p, 0, sizeof(p));
        p.instance = r->instance;       p.instance_len = r->instance_len;
        p.type = r->type;               p.type_len = r->type_len;
        p.domain = r->domain;           p.domain_len = r->domain_len;
        p.host = r->host;               p.host_len = r->host_len;
        p.addresses = addrs;            p.address_count = (uint32_t)addrCount;
        p.port = r->port;
        if (dnssd_reg_init(&check, &p) != DNSSD_REG_OK) {
            return MDNSD_ERR_BADPARAM;
        }
    }
    if (txt != NULL && (txtLength != rs->rs_TXTLength ||
            (txtLength != 0 && memcmp(txt, rs->rs_TXT, txtLength) != 0))) {
        if (!txt_set(rs, txt, txtLength)) {
            return MDNSD_ERR_NOMEM;
        }
        changed = TRUE;
    }
    if (port != 0 && port != rs->rs_Reg.port) {
        rs->rs_Reg.port = port;
        changed = TRUE;
    }
    if (ttl != 0 && ttl != rs->rs_TTL) {
        rs->rs_TTL = ttl;
        changed = TRUE;
    }
    if (addrCount >= 0 && (addrCount != rs->rs_Reg.address_count ||
            memcmp(addrs, rs->rs_Reg.addresses, addrCount * sizeof(*addrs)) != 0)) {
        if (rs->rs_State == MDNS_NAME_ANNOUNCED) {
            addresses_goodbye(md, rs, addrs, (UWORD)addrCount);
        }
        memcpy(rs->rs_Reg.addresses, addrs, addrCount * sizeof(*addrs));
        rs->rs_Reg.address_count = (UBYTE)addrCount;
        changed = TRUE;
    }
    if (changed && rs->rs_State == MDNS_NAME_ANNOUNCED) {
        service_send(md, rs, FALSE, all_subtypes(rs));
        rs->rs_Step = 1;
        rs->rs_NextTick = mdnsTicks() + 50;
    }
    return MDNSD_OK;
}

static void
service_remove(struct MDNSDaemon *md, struct MDNSRegisteredService *rs)
{
    Remove(&rs->rs_Node);
    if (rs->rs_State == MDNS_NAME_ANNOUNCED) {
        service_send(md, rs, TRUE, all_subtypes(rs));
    }
    rs->rs_Reg.state = DNSSD_REG_REMOVED;
    txt_set(rs, NULL, 0);
    FreeMem(rs, sizeof(*rs));
}

/* A service goes, with its goodbyes. */
void
mdnsRegRemoveRef(struct MDNSDaemon *md, APTR ref)
{
    struct MDNSRegisteredService *rs = mdnsRegFind(md, ref);

    if (rs != NULL) {
        service_remove(md, rs);
    }
}

/* Every service of one owner goes (a client that is gone). */
void
mdnsRegRemoveOwner(struct MDNSDaemon *md, ULONG owner)
{
    struct Node *n, *next;

    for (n = md->md_Services.lh_Head; n->ln_Succ != NULL; n = next) {
        next = n->ln_Succ;
        if (((struct MDNSRegisteredService *)n)->rs_Owner == owner) {
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
        txt_set((struct MDNSRegisteredService *)n, NULL, 0);
        FreeMem(n, sizeof(struct MDNSRegisteredService));
    }
}
