/*
 * mdns_dns.h - DNS wire format helpers (RFC 1035 / mDNS)
 */

#ifndef MDNSD_MDNS_DNS_H
#define MDNSD_MDNS_DNS_H

#include <exec/types.h>

#define MDNS_DNS_PORT           5353
#define MDNS_MCAST_ADDR         "224.0.0.251"
#define MDNS_MAX_PACKET         8972
#define MDNS_DEFAULT_TTL        120
/* RFC 6762 section 10: records that name a host live two minutes, the
 * others 75 minutes. */
#define MDNS_TTL_HOST           120
#define MDNS_TTL_OTHER          4500

#define DNS_TYPE_A              1
#define DNS_TYPE_PTR            12
#define DNS_TYPE_TXT            16
#define DNS_TYPE_SRV            33
#define DNS_TYPE_ANY            255
#define DNS_CLASS_IN            1
#define DNS_CLASS_IN_FLUSH      0x8001
#define DNS_QU_BIT              0x8000  /* RFC 6762 5.4: request unicast reply */

#define DNS_FLAG_QR             0x8000
#define DNS_FLAG_AA             0x0400

struct MDNSHeader
{
    UWORD id;
    UWORD flags;
    UWORD qdcount;
    UWORD ancount;
    UWORD nscount;
    UWORD arcount;
};

struct MDNSQuestion
{
    char  qname[256];
    UWORD qtype;
    UWORD qclass;
};

struct MDNSRecord
{
    char  name[256];
    UWORD type;
    UWORD rclass;
    ULONG ttl;
    UWORD rdlength;
    LONG  rdoffset;     /* absolute offset of rdata within the source packet;
                         * required to resolve DNS name-compression pointers in
                         * PTR/SRV targets, which reference earlier packet bytes
                         * that are not present in the rdata[] copy below */
    UBYTE rdata[512];
};

LONG mdnsNameToWire(const char *name, UBYTE *buf, LONG buflen, LONG *used);
LONG mdnsWireToName(const UBYTE *wire, LONG wirelen, LONG offset,
    const UBYTE *pkt, LONG pktlen, char *name, LONG namelen);
LONG mdnsSkipName(const UBYTE *pkt, LONG pktlen, LONG offset);
LONG mdnsBuildQuery(UBYTE *pkt, LONG pktmax, UWORD id, const char *qname,
    UWORD qtype, UWORD qclass, LONG *pktlen);
LONG mdnsParseQuestion(const UBYTE *pkt, LONG pktlen, LONG offset,
    struct MDNSQuestion *q, LONG *consumed);
LONG mdnsParseRecord(const UBYTE *pkt, LONG pktlen, LONG offset,
    struct MDNSRecord *r, LONG *consumed);
LONG mdnsAppendRecord(UBYTE *pkt, LONG pktmax, LONG *pos, UWORD *ancount,
    const char *name, UWORD type, UWORD rclass, ULONG ttl,
    const UBYTE *rdata, UWORD rdlength);
void mdnsInitHeader(struct MDNSHeader *hdr, UWORD flags, UWORD qdcount);

/*
 * A service's text, as clients give it, is one string of "key=value" items
 * with a space between them. A space that belongs to an item is written
 * with a backslash before it, and a backslash as two; in the TXT record
 * each item is a string of its own with neither (RFC 6763 section 6).
 *
 * mdnsTxtNext() takes the next item of text from *pos, without its
 * backslashes, into item (max bytes, no terminator added) and moves *pos
 * past it. It returns the item's length, cut at max, or -1 at the end.
 */
LONG mdnsTxtNext(const char *text, LONG *pos, UBYTE *item, LONG max);

#endif /* MDNSD_MDNS_DNS_H */
