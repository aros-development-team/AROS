/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.
*/

#include "discovery.h"
#include "netmidid.h"

#include <stdio.h>
#include <string.h>

#include <exec/libraries.h>
#include <exec/memory.h>
#include <proto/exec.h>
#include <proto/dnssd.h>
#include <dnssd/dnssd.h>

/* The library's calls take their base from this name. */
struct Library *DNSSDBase;

void netmidi_discovery_init(struct netmidi_discovery *discovery)
{
    memset(discovery, 0, sizeof(*discovery));
}

static void stop_browses(struct netmidi_discovery *discovery);

void netmidi_discovery_begin(struct netmidi_discovery *discovery)
{
    int i;

    if (!discovery)
        return;
    for (i = 0; i < discovery->count; ++i)
        discovery->service[i].wanted = 0;
    /* a search is asked for anew by the transport that wants it */
    stop_browses(discovery);
    discovery->browses = 0;
    discovery->reading = 0;
    /* what changed is seen to at once */
    discovery->last_attempt = 0;
}

/* Copies text, its end included, when that fits in size bytes. */
static void copy(char *to, const char *text, size_t size)
{
    const char *end = memchr(text, 0, size);

    if (end)
        memcpy(to, text, (size_t)(end - text) + 1);
}

int netmidi_discovery_add_text(struct netmidi_discovery *discovery,
                               const char *name, const char *type,
                               uint16_t port, const char *text,
                               uint32_t seconds)
{
    int i;

    if (!text)
        text = "";
    if (!discovery || !name || !type || !*name || !*type || !port ||
        !memchr(name, 0, NETMIDI_DISCOVERY_NAME) ||
        !memchr(type, 0, NETMIDI_DISCOVERY_TYPE) ||
        !memchr(text, 0, NETMIDI_DISCOVERY_TEXT))
        return -1;
    for (i = 0; i < discovery->count; ++i)
        if (discovery->service[i].port == port &&
            discovery->service[i].seconds == seconds &&
            strcmp(discovery->service[i].name, name) == 0 &&
            strcmp(discovery->service[i].type, type) == 0 &&
            strcmp(discovery->service[i].text, text) == 0) {
            discovery->service[i].wanted = 1;
            return 0;
        }
    if (discovery->count == NETMIDI_DISCOVERY_SERVICES)
        return -1;
    memset(&discovery->service[discovery->count], 0,
           sizeof(discovery->service[0]));
    copy(discovery->service[discovery->count].name, name,
         NETMIDI_DISCOVERY_NAME);
    copy(discovery->service[discovery->count].type, type,
         NETMIDI_DISCOVERY_TYPE);
    copy(discovery->service[discovery->count].text, text,
         NETMIDI_DISCOVERY_TEXT);
    discovery->service[discovery->count].port = port;
    discovery->service[discovery->count].seconds = seconds;
    discovery->service[discovery->count].wanted = 1;
    ++discovery->count;
    return 0;
}

int netmidi_discovery_add(struct netmidi_discovery *discovery,
                          const char *name, const char *type, uint16_t port)
{
    return netmidi_discovery_add_text(discovery, name, type, port, NULL, 0);
}

int netmidi_discovery_item(char *text, unsigned long size, const char *key,
                           const char *value)
{
    unsigned long start, out;
    const char *part[3];
    int p;

    if (!text || !size || !key || !value)
        return -1;
    for (start = 0; start < size && text[start]; ++start)
        ;
    if (start == size)
        return -1;
    out = start;
    part[0] = key;
    part[1] = "=";
    part[2] = value;
    if (out && out + 1 < size)
        text[out++] = ' ';
    for (p = 0; p < 3; ++p) {
        const char *c;

        for (c = part[p]; *c; ++c) {
            if (out + 3 > size) {
                text[start] = 0;
                return -1;
            }
            if (*c == ' ' || *c == '\\')
                text[out++] = '\\';
            text[out++] = *c;
        }
    }
    text[out] = 0;
    return 0;
}

int netmidi_discovery_browse(struct netmidi_discovery *discovery,
                             const char *type)
{
    if (!discovery || !type ||
        discovery->browses == NETMIDI_DISCOVERY_BROWSES)
        return -1;
    discovery->browse[discovery->browses].type = type;
    return discovery->browses++;
}

static int same_letters(const uint8_t *text, const char *key, size_t length)
{
    size_t i;

    for (i = 0; i < length; ++i) {
        unsigned a = text[i], b = (unsigned char)key[i];

        if (a >= 'A' && a <= 'Z')
            a += 'a' - 'A';
        if (b >= 'A' && b <= 'Z')
            b += 'a' - 'A';
        if (a != b)
            return 0;
    }
    return 1;
}

int netmidi_discovery_txt_value(const uint8_t *txt, size_t length,
                                const char *key, char *value, size_t size)
{
    size_t at = 0, key_length;
    const char *key_end;

    /* a key is at most a string of the record, 255 bytes */
    if (!txt || !key || !value || !size ||
        !(key_end = memchr(key, 0, 256)))
        return -1;
    key_length = (size_t)(key_end - key);
    while (at < length) {
        size_t n = txt[at++];
        const uint8_t *item = txt + at;

        if (n > length - at)
            return -1;              /* a string longer than the record */
        at += n;
        if (n > key_length && item[key_length] == '=' &&
            same_letters(item, key, key_length)) {
            size_t i, out = 0;

            for (i = key_length + 1; i < n && item[i] && out + 1 < size; ++i)
                value[out++] = (char)item[i];
            value[out] = 0;
            return 0;
        }
    }
    return -1;
}


/* "k=v k2=v2", a space or backslash inside an item after a backslash
 * (netmidi_discovery_item), as TXT data: each item a string after its
 * length (RFC 6763 section 6). Its length. */
static size_t text_to_txt(const char *text, uint8_t *txt, size_t size)
{
    size_t at = 0;

    while (*text) {
        size_t start = at++, n = 0;

        if (start >= size)
            return 0;
        while (*text && *text != ' ') {
            if (*text == '\\' && text[1])
                ++text;
            if (at >= size || n == 255)
                return 0;
            txt[at++] = (uint8_t)*text++;
            ++n;
        }
        txt[start] = (uint8_t)n;
        while (*text == ' ')
            ++text;
    }
    return at;
}

/* The peers a search found are followed no more. */
static void drop_peers(struct netmidi_discovery *discovery, int browse)
{
    int i;

    for (i = 0; i < NETMIDI_DISCOVERY_PEERS; ++i) {
        struct netmidi_discovery_peer *peer = &discovery->peer[i];

        if (!peer->name[0] || (browse >= 0 && peer->browse != browse))
            continue;
        if (peer->resolve && discovery->session)
            DNSSD_Cancel(peer->resolve);
        memset(peer, 0, sizeof(*peer));
    }
}

/* The service went away, or restarted: what it held for us went with it,
 * and is asked for again at the next poll. */
static void forget(struct netmidi_discovery *discovery)
{
    int i;

    drop_peers(discovery, -1);
    if (discovery->session)
        DNSSD_CloseSession(discovery->session);   /* the handles with it */
    discovery->session = NULL;
    for (i = 0; i < discovery->count; ++i)
        discovery->service[i].reference = NULL;
    for (i = 0; i < discovery->browses; ++i)
        discovery->browse[i].reference = NULL;
    discovery->reading = 0;
}

/* What is no longer asked for is withdrawn, with its goodbye. */
static void withdraw_unwanted(struct netmidi_discovery *discovery)
{
    int i, kept = 0;

    for (i = 0; i < discovery->count; ++i) {
        if (!discovery->service[i].wanted) {
            if (discovery->service[i].reference && discovery->session)
                DNSSD_Cancel(discovery->service[i].reference);
            continue;
        }
        if (kept != i)
            discovery->service[kept] = discovery->service[i];
        ++kept;
    }
    discovery->count = kept;
}

static const char *refusal(long result)
{
    switch (result) {
    case DNSSD_ERR_EXISTS:
        return "this machine already advertises that name";
    case DNSSD_ERR_BADNAME:
    case DNSSD_ERR_BADPARAM:
        return "the name is not one DNS label of at most 63 bytes";
    case DNSSD_ERR_NOMEM:
        return "the mDNS service is out of memory";
    case DNSSD_ERR_BUSY:
        return "the mDNS service has too much to do";
    default:
        return "the mDNS service refused it";
    }
}

/* A session with mdnsd, and whether it is still the one that has it. */
static int session(struct netmidi_discovery *discovery)
{
    ULONG incarnation = 0;

    if (discovery->session) {
        if (DNSSD_GetStatus(DNSSD_Status_Incarnation, (IPTR)&incarnation,
                            TAG_DONE) == DNSSD_OK &&
            incarnation == discovery->incarnation)
            return 1;
        forget(discovery);          /* mdnsd ended, or is another now */
    }
    if (!discovery->port) {
        discovery->port = CreateMsgPort();
        if (!discovery->port)
            return 0;
    }
    if (DNSSD_OpenSessionA((DNSSDSessionRef *)&discovery->session,
                           discovery->port, NULL) != DNSSD_OK) {
        discovery->session = NULL;
        return 0;                   /* mdnsd is not running, yet */
    }
    if (DNSSD_GetStatus(DNSSD_Status_Incarnation, (IPTR)&incarnation,
                        TAG_DONE) != DNSSD_OK) {
        forget(discovery);
        return 0;
    }
    discovery->incarnation = incarnation;
    return 1;
}

int netmidi_discovery_poll(struct netmidi_discovery *discovery, uint64_t now)
{
    int registered = 0;
    int pending = 0;
    int i;

    if (!discovery)
        return -1;
    withdraw_unwanted(discovery);
    for (i = 0; i < discovery->count; ++i) {
        if (discovery->service[i].reference)
            ++registered;
        else
            ++pending;
    }
    for (i = 0; i < discovery->browses; ++i)
        if (!discovery->browse[i].reference)
            ++pending;
    if (!discovery->count && !pending)
        return 0;
    if (discovery->last_attempt && now - discovery->last_attempt < 100000)
        return registered;
    discovery->last_attempt = now ? now : 1;

    if (!discovery->library) {
        discovery->library = OpenLibrary((CONST_STRPTR)DNSSDNAME, DNSSD_VERSION);
        DNSSDBase = discovery->library;
        if (!discovery->library)
            return -1;
    }
    DNSSDBase = discovery->library;
    /* A registration made is no proof that mdnsd is still the process that
       took it: one restarted between polls has another incarnation, and
       the services are registered again. Once in ten seconds at most. */
    if (!session(discovery))
        return 0;
    registered = 0;
    for (i = 0; i < discovery->count; ++i) {
        uint8_t txt[NETMIDI_DISCOVERY_TEXT];
        size_t length = text_to_txt(discovery->service[i].text, txt,
                                    sizeof(txt));
        struct TagItem tags[] = {
            { TAG_IGNORE, (IPTR)txt },
            { TAG_IGNORE, (IPTR)length },
            { TAG_IGNORE, 0 },
            { TAG_DONE, 0 }
        };
        LONG result;

        if (discovery->service[i].reference) {
            ++registered;
            continue;
        }
        if (length) {
            tags[0].ti_Tag = DNSSD_TXT;
            tags[1].ti_Tag = DNSSD_TXTLength;
        }
        if (discovery->service[i].seconds) {
            tags[2].ti_Tag = DNSSD_TimeToLive;
            tags[2].ti_Data = (IPTR)discovery->service[i].seconds;
        }
        result = DNSSD_RegisterA(discovery->session,
                                 (CONST_STRPTR)discovery->service[i].name,
                                 (CONST_STRPTR)discovery->service[i].type,
                                 discovery->service[i].port,
                                 (DNSSDOperationRef *)
                                     &discovery->service[i].reference, tags);
        if (result != DNSSD_OK)
            discovery->service[i].reference = NULL;
        if (discovery->service[i].reference) {
            discovery->service[i].refused = 0;
            discovery->service[i].id =
                DNSSD_OperationID(discovery->service[i].reference);
            discovery->service[i].advertised[0] = 0;
            ++registered;
        } else if (result == DNSSD_ERR_NOENGINE || result == DNSSD_ERR_STALE) {
            forget(discovery);
            return 0;
        } else if (result != discovery->service[i].refused) {
            /* said once for each reason, not at every attempt */
            discovery->service[i].refused = result;
            fprintf(stderr, TAG "\"%s\" (%s) is not advertised: %s\n",
                    discovery->service[i].name, discovery->service[i].type,
                    refusal(result));
        }
    }
    for (i = 0; i < discovery->browses; ++i) {
        LONG result;

        if (discovery->browse[i].reference)
            continue;
        result = DNSSD_BrowseA(discovery->session,
                               (CONST_STRPTR)discovery->browse[i].type,
                               (DNSSDOperationRef *)
                                   &discovery->browse[i].reference, NULL);
        if (result != DNSSD_OK) {
            discovery->browse[i].reference = NULL;
            if (result == DNSSD_ERR_NOENGINE || result == DNSSD_ERR_STALE) {
                forget(discovery);
                return 0;
            }
            continue;
        }
        discovery->browse[i].id =
            DNSSD_OperationID(discovery->browse[i].reference);
    }
    return registered;
}

/* The searches end, and the peers they found with them. */
static void stop_browses(struct netmidi_discovery *discovery)
{
    int i;

    if (discovery->library)
        DNSSDBase = discovery->library;
    drop_peers(discovery, -1);
    for (i = 0; i < discovery->browses; ++i) {
        if (discovery->browse[i].reference && discovery->session)
            DNSSD_Cancel(discovery->browse[i].reference);
        discovery->browse[i].reference = NULL;
    }
    discovery->reading = 0;
}

/* A search found an instance: it is followed - its host, port and TXT -
 * until it goes, by a resolve with no time limit. */
static void peer_found(struct netmidi_discovery *discovery, int browse,
                       const struct DNSSDEvent *event)
{
    const UBYTE *label = (const UBYTE *)event + event->de_Instance.dr_Offset;
    ULONG length = event->de_Instance.dr_Size;
    char type[256];
    int i, slot = -1;

    if (length == 0 || length > 63 ||
        DNSSD_NameToText((const UBYTE *)event + event->de_Type.dr_Offset,
                         event->de_Type.dr_Size, (STRPTR)type,
                         sizeof(type)) < 0)
        return;
    for (i = 0; i < NETMIDI_DISCOVERY_PEERS; ++i) {
        const struct netmidi_discovery_peer *peer = &discovery->peer[i];

        if (!peer->name[0]) {
            if (slot < 0)
                slot = i;
        } else if (peer->browse == browse && peer->link == event->de_Interface &&
                   strnlen(peer->name, sizeof(peer->name)) == length &&
                   memcmp(peer->name, label, length) == 0) {
            return;                 /* followed already */
        }
    }
    if (slot < 0)
        return;                     /* more peers than are followed */
    memcpy(discovery->peer[slot].name, label, length);
    discovery->peer[slot].name[length] = 0;
    discovery->peer[slot].link = event->de_Interface;
    discovery->peer[slot].browse = browse;
    if (DNSSD_Resolve(discovery->session,
                      (CONST_STRPTR)discovery->peer[slot].name,
                      (CONST_STRPTR)type,
                      (DNSSDOperationRef *)&discovery->peer[slot].resolve,
                      DNSSD_Interface, event->de_Interface,
                      DNSSD_Timeout, 0, TAG_DONE) != DNSSD_OK) {
        memset(&discovery->peer[slot], 0, sizeof(discovery->peer[slot]));
        return;
    }
    discovery->peer[slot].resolve_id =
        DNSSD_OperationID(discovery->peer[slot].resolve);
}

/* What a resolve says of a peer, as it is reported: every address, IPv4
 * first, its port and the endpoint name of its TXT record. */
static void describe(const struct netmidi_discovery_peer *peer,
                     const struct DNSSDEvent *event,
                     struct netmidi_discovered *seen)
{
    const struct DNSSDAddress *a = (const struct DNSSDAddress *)
        ((const UBYTE *)event + event->de_Addresses.dr_Offset);
    int pass;
    ULONG k;

    memset(seen, 0, sizeof(*seen));
    memcpy(seen->name, peer->name, sizeof(seen->name));
    seen->browse = peer->browse;
    seen->port = event->de_Port;
    for (pass = 0; pass < 2; ++pass)
        for (k = 0; k < event->de_AddressCount &&
                    seen->address_count < NETMIDI_DISCOVERY_ADDRESSES; ++k) {
            struct netmidi_address *to;

            if ((a[k].da_Family == DNSSD_FAMILY_IPV4) != (pass == 0))
                continue;
            to = &seen->addresses[seen->address_count++];
            to->family = (uint8_t)a[k].da_Family;
            memcpy(to->bytes, a[k].da_Bytes, sizeof(to->bytes));
            to->link = a[k].da_Interface;
        }
    /* the transports speak IPv4: its first one, in host order */
    if (seen->address_count && seen->addresses[0].family == 4)
        seen->address = ((uint32_t)seen->addresses[0].bytes[0] << 24) |
                        ((uint32_t)seen->addresses[0].bytes[1] << 16) |
                        ((uint32_t)seen->addresses[0].bytes[2] << 8) |
                        seen->addresses[0].bytes[3];
    netmidi_discovery_txt_value(
        (const uint8_t *)event + event->de_TXT.dr_Offset,
        event->de_TXT.dr_Size, "UMPEndpointName", seen->endpoint,
        sizeof(seen->endpoint));
}

/* Is what a peer says now not what it was told as? */
static int differs(const struct netmidi_discovered *a,
                   const struct netmidi_discovered *b)
{
    return a->port != b->port || a->address_count != b->address_count ||
           memcmp(a->addresses, b->addresses,
                  sizeof(a->addresses[0]) * (size_t)a->address_count) != 0 ||
           strcmp(a->endpoint, b->endpoint) != 0;
}

/* An event of one of our registrations: the name it has on the network,
   or its end (another host has the name of a host it stands in for).
   0: nothing to report. */
static int registration_event(struct netmidi_discovery *discovery, int i,
                              const struct DNSSDEvent *event)
{
    if (event->de_Kind == DNSSD_EVENT_REGISTERED) {
        ULONG length = event->de_Instance.dr_Size;

        if (length < NETMIDI_DISCOVERY_NAME) {
            memcpy(discovery->service[i].advertised,
                   (const UBYTE *)event + event->de_Instance.dr_Offset, length);
            discovery->service[i].advertised[length] = 0;
            if (strcmp(discovery->service[i].advertised,
                       discovery->service[i].name) != 0)
                printf("%s\"%s\" (%s) is advertised as \"%s\": another "
                       "machine has the name\n", TAG, discovery->service[i].name,
                       discovery->service[i].type,
                       discovery->service[i].advertised);
        }
    } else if (event->de_Kind == DNSSD_EVENT_END) {
        /* registered again at the next poll, and why said once */
        DNSSD_Cancel(discovery->service[i].reference);
        discovery->service[i].reference = NULL;
        discovery->service[i].advertised[0] = 0;
        if (event->de_Result != discovery->service[i].refused) {
            discovery->service[i].refused = event->de_Result;
            fprintf(stderr, "%s\"%s\" (%s) is no longer advertised: %s\n",
                    TAG, discovery->service[i].name, discovery->service[i].type,
                    refusal(event->de_Result));
        }
        discovery->last_attempt = 0;
    }
    return 0;
}

/* One event of the session: 1 with *seen to report, 0 for nothing. */
static int take(struct netmidi_discovery *discovery,
                const struct DNSSDEvent *event,
                struct netmidi_discovered *seen)
{
    int i;

    for (i = 0; i < discovery->count; ++i)
        if (discovery->service[i].reference &&
            event->de_Operation == discovery->service[i].id)
            return registration_event(discovery, i, event);
    for (i = 0; i < discovery->browses; ++i) {
        if (!discovery->browse[i].reference ||
            event->de_Operation != discovery->browse[i].id)
            continue;
        if (event->de_Kind == DNSSD_EVENT_ADD) {
            peer_found(discovery, i, event);
        } else if (event->de_Kind == DNSSD_EVENT_REMOVE) {
            const UBYTE *label =
                (const UBYTE *)event + event->de_Instance.dr_Offset;
            int k;

            for (k = 0; k < NETMIDI_DISCOVERY_PEERS; ++k) {
                struct netmidi_discovery_peer *peer = &discovery->peer[k];
                int told;

                if (!peer->name[0] || peer->browse != i ||
                    peer->link != event->de_Interface ||
                    strnlen(peer->name, sizeof(peer->name)) !=
                        event->de_Instance.dr_Size ||
                    memcmp(peer->name, label, event->de_Instance.dr_Size) != 0)
                    continue;
                told = peer->reported;
                if (told) {
                    *seen = peer->last;
                    seen->present = 0;
                }
                DNSSD_Cancel(peer->resolve);
                memset(peer, 0, sizeof(*peer));
                return told;
            }
        } else if (event->de_Kind == DNSSD_EVENT_END) {
            /* begun again at the next poll */
            drop_peers(discovery, i);
            DNSSD_Cancel(discovery->browse[i].reference);
            discovery->browse[i].reference = NULL;
            discovery->last_attempt = 0;
        }
        return 0;
    }
    for (i = 0; i < NETMIDI_DISCOVERY_PEERS; ++i) {
        struct netmidi_discovery_peer *peer = &discovery->peer[i];

        if (!peer->name[0] || !peer->resolve ||
            event->de_Operation != peer->resolve_id)
            continue;
        if (event->de_Kind == DNSSD_EVENT_RESOLVED) {
            struct netmidi_discovered now;

            describe(peer, event, &now);
            /* one that lost its last address can be reached no more: gone,
               until one comes back */
            if (!now.address_count && peer->reported) {
                peer->reported = 0;
                *seen = peer->last;
                seen->present = 0;
                return 1;
            }
            /* told when its port and an address are known, and again when
               what it advertises changes */
            if (!now.address_count || !now.port ||
                (peer->reported && !differs(&now, &peer->last)))
                return 0;
            now.present = 1;
            peer->last = now;
            peer->reported = 1;
            *seen = now;
            return 1;
        }
        if (event->de_Kind == DNSSD_EVENT_REMOVE && peer->reported) {
            peer->reported = 0;
            *seen = peer->last;
            seen->present = 0;
            return 1;
        }
        return 0;
    }
    return 0;
}

int netmidi_discovery_signal(const struct netmidi_discovery *discovery)
{
    if (!discovery || !discovery->session || !discovery->port)
        return -1;
    return ((const struct MsgPort *)discovery->port)->mp_SigBit;
}

int netmidi_discovery_own(const struct netmidi_discovery *discovery,
                          const char *name)
{
    int i;

    if (!discovery || !name)
        return 0;
    /* only a name mdnsd has announced: it puts our instance in its cache
       as it announces it, with DNSSD_EVENT_REGISTERED, so no browse finds
       it before; another host's instance under the name we asked for (the
       one that made ours "Name (2)") is a peer */
    for (i = 0; i < discovery->count; ++i)
        if (discovery->service[i].advertised[0] &&
            strcmp(name, discovery->service[i].advertised) == 0)
            return 1;
    return 0;
}

void netmidi_discovery_address_text(const struct netmidi_address *address,
                                    char *text, size_t size)
{
    size_t at = 0;
    int i, best = -1, best_length = 0;

    if (!size)
        return;
    text[0] = 0;
    if (address->family == 4) {
        snprintf(text, size, "%u.%u.%u.%u", address->bytes[0],
                 address->bytes[1], address->bytes[2], address->bytes[3]);
        return;
    }
    /* the longest run of zero groups as "::" (RFC 5952) */
    for (i = 0; i < 8; ) {
        int j = i;

        while (j < 8 && !address->bytes[2 * j] && !address->bytes[2 * j + 1])
            ++j;
        if (j - i > best_length && j - i > 1) {
            best = i;
            best_length = j - i;
        }
        i = j == i ? i + 1 : j;
    }
    for (i = 0; i < 8 && at + 6 < size; ++i) {
        unsigned group = ((unsigned)address->bytes[2 * i] << 8) |
                         address->bytes[2 * i + 1];

        if (i == best) {
            at += (size_t)snprintf(text + at, size - at, "::");
            i += best_length - 1;
            continue;
        }
        at += (size_t)snprintf(text + at, size - at, "%s%x",
                               i && i != best + best_length ? ":" : "", group);
    }
    if (address->bytes[0] == 0xfe && (address->bytes[1] & 0xc0) == 0x80 &&
        address->link && at < size)
        snprintf(text + at, size - at, "%%%lu", (unsigned long)address->link);
}

int netmidi_discovery_next(struct netmidi_discovery *discovery,
                           struct netmidi_discovered *seen)
{
    if (!discovery || !seen || !discovery->library || !discovery->session)
        return 0;
    /* only when mdnsd said there are events: the session's message (the
       library's, taken off our port, never replied) */
    if (!discovery->reading) {
        if (!GetMsg((struct MsgPort *)discovery->port))
            return 0;
        discovery->reading = 1;
    }
    DNSSDBase = discovery->library;
    for (;;) {
        /* as large as an event can be (a TXT of 8900 bytes and the names),
           off the stack: netmidid has one task */
        static union {
            struct DNSSDEvent event;
            UBYTE bytes[16384];
        } buffer;
        ULONG size = sizeof(buffer);
        LONG result = DNSSD_ReadEvent(discovery->session, &buffer.event,
                                      &size);

        if (result == DNSSD_OK) {
            if (buffer.event.de_Kind == DNSSD_EVENT_LOST) {
                /* events were dropped: the searches are begun again at
                   the next poll, which reports what is there anew */
                stop_browses(discovery);
                discovery->last_attempt = 0;
                continue;
            }
            if (take(discovery, &buffer.event, seen))
                return 1;
            continue;
        }
        if (result == DNSSD_ERR_STALE || result == DNSSD_ERR_NOENGINE) {
            /* mdnsd ended: what it held for us is gone, and is asked for
               again when it is back */
            forget(discovery);
            discovery->last_attempt = 0;
        }
        /* DNSSD_ERR_EMPTY: the session's port is told of the next one */
        break;
    }
    discovery->reading = 0;
    return 0;
}

void netmidi_discovery_close(struct netmidi_discovery *discovery)
{
    if (!discovery)
        return;
    if (discovery->library) {
        DNSSDBase = discovery->library;
        /* the session's end withdraws the services, with their goodbyes,
           and ends the searches */
        forget(discovery);
        if (discovery->port)
            DeleteMsgPort((struct MsgPort *)discovery->port);
        CloseLibrary(discovery->library);
    }
    memset(discovery, 0, sizeof(*discovery));
    DNSSDBase = NULL;
}
