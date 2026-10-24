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
#include <proto/bonami.h>
#include <bonami/mdns.h>

/* The library's calls take their base from this name. */
struct Library *BonamiBase;

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


/* The service went away: what it held for us went with it. */
static void forget(struct netmidi_discovery *discovery)
{
    int i;
    if (discovery->engine)
        CloseMDNSEngine();
    discovery->engine = 0;
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
            if (discovery->service[i].reference && discovery->library)
                DeregisterService(discovery->service[i].reference);
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
    case MDNSERR_DUPLICATE:
        return "this machine already advertises that name";
    case MDNSERR_BADPARAM:
        return "the name is not one DNS label of at most 63 bytes "
               "without dots";
    case MDNSERR_NOMEM:
        return "the mDNS service is out of memory";
    default:
        return "the mDNS service refused it";
    }
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
        discovery->library = OpenLibrary((CONST_STRPTR)"bonami.library", 40);
        BonamiBase = discovery->library;
        if (!discovery->library)
            return -1;
    }
    if (!discovery->engine) {
        if (OpenMDNSEngine() != MDNS_OK)
            return registered;      /* mdnsd is not running, yet */
        discovery->engine = 1;
    }
    /* A registration made is no proof that mdnsd is still the process
       that took it. An update that changes nothing asks it whether it
       still knows the handle, and sends nothing to the network; a daemon
       that was restarted between polls does not, and the services are
       registered again. Once in ten seconds at most. */
    for (i = 0; i < discovery->count; ++i) {
        struct TagItem unchanged[] = { { TAG_DONE, 0 } };
        LONG result;

        if (!discovery->service[i].reference)
            continue;
        result = UpdateServiceA(discovery->service[i].reference, unchanged);
        if (result == MDNSERR_NOENGINE || result == MDNSERR_NOTFOUND ||
            result == MDNSERR_BADPARAM) {
            forget(discovery);
            return 0;
        }
    }
    registered = 0;
    for (i = 0; i < discovery->count; ++i) {
        /* the service writes the registration where the tag says */
        struct TagItem tags[] = {
            { MDNS_ServiceName, (IPTR)discovery->service[i].name },
            { MDNS_ServiceType, (IPTR)discovery->service[i].type },
            { MDNS_Port, (IPTR)discovery->service[i].port },
            { MDNS_RegisterRef, (IPTR)&discovery->service[i].reference },
            { TAG_IGNORE, 0 },
            { TAG_IGNORE, 0 },
            { TAG_DONE, 0 }
        };
        LONG result;

        if (discovery->service[i].reference) {
            ++registered;
            continue;
        }
        if (discovery->service[i].text[0]) {
            tags[4].ti_Tag = MDNS_TXTRecord;
            tags[4].ti_Data = (IPTR)discovery->service[i].text;
        }
        if (discovery->service[i].seconds) {
            tags[5].ti_Tag = MDNS_TimeToLive;
            tags[5].ti_Data = (IPTR)discovery->service[i].seconds;
        }
        result = RegisterServiceA(tags);
        if (result != MDNS_OK)
            discovery->service[i].reference = NULL;
        if (discovery->service[i].reference) {
            discovery->service[i].refused = 0;
            ++registered;
        } else if (result == MDNSERR_NOENGINE) {
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
    /* Each search has a queue of its own, read in our task: no hook, and
       nothing of ours runs in mdnsd (DISCOVERY.md of mdnsd). */
    for (i = 0; i < discovery->browses; ++i) {
        struct TagItem tags[] = {
            { MDNS_EventMode, MDNS_EVENT_QUEUE },
            { MDNS_BrowseRef, (IPTR)&discovery->browse[i].reference },
            { TAG_DONE, 0 }
        };
        LONG result;

        if (discovery->browse[i].reference)
            continue;
        result = BrowseServiceTypeA((STRPTR)discovery->browse[i].type, NULL,
                                    tags);
        if (result != MDNS_OK)
            discovery->browse[i].reference = NULL;
        if (result == MDNSERR_NOENGINE) {
            forget(discovery);
            return 0;
        }
    }
    return registered;
}

/* The searches end, and what their queues held with them. */
static void stop_browses(struct netmidi_discovery *discovery)
{
    int i;

    if (discovery->library) {
        BonamiBase = discovery->library;
        for (i = 0; i < discovery->browses; ++i)
            if (discovery->browse[i].reference)
                StopBrowse(discovery->browse[i].reference);
    }
    for (i = 0; i < discovery->browses; ++i)
        discovery->browse[i].reference = NULL;
    discovery->reading = 0;
}

/* What the event says, as the transports are told it. */
static void take(struct netmidi_discovered *seen,
                 const struct MDNSDiscoveryEvent *event, int browse)
{
    const UBYTE *b = (const UBYTE *)&event->de_Address;
    size_t length = event->de_TXTLength;

    memset(seen, 0, sizeof(*seen));
    memcpy(seen->name, event->de_Name, sizeof(seen->name) - 1);
    /* the address arrives in network order, the port in ours */
    seen->address = ((uint32_t)b[0] << 24) | ((uint32_t)b[1] << 16) |
                    ((uint32_t)b[2] << 8) | b[3];
    seen->port = event->de_Port;
    seen->present = event->de_Code == MEVENT_ADD;
    seen->browse = browse;
    if (length > sizeof(event->de_TXT))
        length = sizeof(event->de_TXT);
    netmidi_discovery_txt_value(event->de_TXT, length, "UMPEndpointName",
                                seen->endpoint, sizeof(seen->endpoint));
}

int netmidi_discovery_next(struct netmidi_discovery *discovery, uint64_t now,
                           struct netmidi_discovered *seen)
{
    if (!discovery || !seen || !discovery->library || !discovery->engine)
        return 0;
    if (!discovery->reading) {
        if (discovery->last_read && now - discovery->last_read < 10000)
            return 0;
        discovery->last_read = now ? now : 1;
        discovery->reading = 1;
        discovery->cursor = 0;
    }
    BonamiBase = discovery->library;
    while (discovery->cursor < discovery->browses) {
        int i = discovery->cursor;
        struct MDNSDiscoveryEvent event;
        LONG result;

        if (!discovery->browse[i].reference) {
            ++discovery->cursor;
            continue;
        }
        result = ReadMDNSEvent(discovery->browse[i].reference, &event);
        if (result == MDNS_OK) {
            if (event.de_Code != MEVENT_ADD && event.de_Code != MEVENT_REMOVE)
                continue;
            take(seen, &event, i);
            return 1;
        }
        if (result == MDNSERR_NOENGINE) {
            /* mdnsd ended: what it held for us is gone, and is asked
               for again when it is back */
            forget(discovery);
            return 0;
        }
        if (result != MDNSERR_NOTREADY) {
            /* MDNSERR_BUSY, the queue overflowed, is the end of the
               search; MDNSERR_BADPARAM, it is no longer ours. Either way
               it is begun again at the next poll, which then reports
               what is there anew. */
            StopBrowse(discovery->browse[i].reference);
            discovery->browse[i].reference = NULL;
            discovery->last_attempt = 0;
        }
        ++discovery->cursor;
    }
    discovery->reading = 0;
    return 0;
}

void netmidi_discovery_close(struct netmidi_discovery *discovery)
{
    if (!discovery)
        return;
    stop_browses(discovery);
    if (discovery->library) {
        int i;

        BonamiBase = discovery->library;
        for (i = 0; i < discovery->count; ++i)
            if (discovery->service[i].reference)
                DeregisterService(discovery->service[i].reference);
        if (discovery->engine)
            CloseMDNSEngine();
        CloseLibrary(discovery->library);
    }
    memset(discovery, 0, sizeof(*discovery));
    BonamiBase = NULL;
}

