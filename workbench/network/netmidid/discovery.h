/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.
*/

#ifndef NETMIDI_DISCOVERY_H
#define NETMIDI_DISCOVERY_H

#include <stddef.h>
#include <stdint.h>

/* Our name on the network, through the system's mDNS service:
 * dnssd.library and mdnsd. We hold no socket on port 5353 and answer no
 * question ourselves; the service does, for every program on the machine.
 * Where there is no such service nothing is advertised and the calls say
 * so. */

#define NETMIDI_DISCOVERY_SERVICES 2
#define NETMIDI_DISCOVERY_NAME 64   /* one DNS label and its end */
#define NETMIDI_DISCOVERY_TYPE 32
#define NETMIDI_DISCOVERY_TEXT 256
#define NETMIDI_DISCOVERY_BROWSES 2
#define NETMIDI_DISCOVERY_ENDPOINT 99   /* a UMP Endpoint Name, 98 bytes,
                                           and its end (M2-124-UM 4.4) */
#define NETMIDI_DISCOVERY_PEERS 16      /* instances followed at a time: with
                                           the registrations and searches,
                                           2 + 2 + 16 = 20 of the 24
                                           operations mdnsd allows a session
                                           on the 68k */

#define NETMIDI_DISCOVERY_ADDRESSES 4

/* An address of a peer: IPv4 or IPv6, and the interface it was learned on
 * (the zone of an IPv6 link-local one). */
struct netmidi_address {
    uint8_t family;                 /* 4 or 6 */
    uint8_t bytes[16];              /* 4 or 16 of them, network order */
    uint32_t link;
};

/* A service instance someone else advertises, as it came, changed or
 * went. */
struct netmidi_discovered {
    char name[64];
    uint32_t address;               /* its first IPv4, host order; 0 when
                                       it has none */
    int address_count;              /* every address it has, IPv4 first */
    struct netmidi_address addresses[NETMIDI_DISCOVERY_ADDRESSES];
    uint16_t port;
    int present;                    /* 1 it is there, 0 it left */
    int browse;                     /* which search found it: what
                                       netmidi_discovery_browse returned */
    char endpoint[NETMIDI_DISCOVERY_ENDPOINT];  /* the UMPEndpointName of
                                                   its text record, or
                                                   empty */
};

/* An instance a search found, followed until it goes: what it was last
 * reported as. */
struct netmidi_discovery_peer {
    char name[64];
    uint32_t link;                  /* the interface it was found on */
    int browse;                     /* the search that found it */
    void *resolve;                  /* the operation that follows it */
    uint32_t resolve_id;
    int reported;                   /* told as present */
    struct netmidi_discovered last; /* what it was told as */
};

struct netmidi_discovery {
    void *library;                  /* dnssd.library, opened */
    void *session;                  /* our session with mdnsd, or NULL */
    void *port;                     /* where mdnsd tells of events */
    uint32_t incarnation;           /* the mdnsd that has the session */
    struct {
        char name[NETMIDI_DISCOVERY_NAME];
        char type[NETMIDI_DISCOVERY_TYPE];
        uint16_t port;
        char text[NETMIDI_DISCOVERY_TEXT];  /* its text record, or empty */
        uint32_t seconds;           /* its records' time to live, 0 for
                                       the service's own choice */
        int wanted;                 /* asked for since the last begin */
        long refused;               /* why it was last refused, or 0 */
        void *reference;            /* the registration, or NULL */
        uint32_t id;
        char advertised[NETMIDI_DISCOVERY_NAME];  /* the name it has on
                                       the network now, once announced */
    } service[NETMIDI_DISCOVERY_SERVICES];
    int count;
    uint64_t last_attempt;
    struct {
        const char *type;           /* what to look for, or NULL */
        void *reference;            /* the search, or NULL */
        uint32_t id;
    } browse[NETMIDI_DISCOVERY_BROWSES];
    int browses;
    struct netmidi_discovery_peer peer[NETMIDI_DISCOVERY_PEERS];
    int reading;                    /* mdnsd said there are events: read
                                       until there are none */
};

/* Once, before anything else. */
void netmidi_discovery_init(struct netmidi_discovery *discovery);
/* Before the services are asked for again, as when the configuration
 * changed: one asked for again as it was keeps its registration, and the
 * network sees no change; the others are withdrawn at the next poll.
 * Searches end here, and are asked for again. */
void netmidi_discovery_begin(struct netmidi_discovery *discovery);
/* Asks for an instance of type on port to be advertised under name. The
 * strings are copied. Nothing is sent here. */
int netmidi_discovery_add(struct netmidi_discovery *discovery,
                          const char *name, const char *type, uint16_t port);
/* The same with a text record and a time to live for the instance's
 * records. text is the items with a space between, a space or backslash
 * inside an item written with a backslash before it; netmidi_discovery_item
 * writes one so. */
int netmidi_discovery_add_text(struct netmidi_discovery *discovery,
                               const char *name, const char *type,
                               uint16_t port, const char *text,
                               uint32_t seconds);
/* Appends "key=value" to text as one item. Returns -1, with text as it
 * was, when it does not fit in size. */
int netmidi_discovery_item(char *text, unsigned long size, const char *key,
                           const char *value);
/* Withdraws what is no longer asked for and registers what is not yet
 * registered. The service may start after the program, or stop and start
 * again with the network, so this is called regularly; it tries at most
 * once in ten seconds. now is in ten-thousandths of a second. Returns the
 * number of services registered now, -1 where there is no service to
 * ask. */
int netmidi_discovery_poll(struct netmidi_discovery *discovery,
                           uint64_t now);
/* Asks for instances of type to be looked for. Like a registration, the
 * search begins at the next poll at which the service answers. Returns
 * the number events of this search carry in their browse field, -1 when
 * there is no room for another. */
int netmidi_discovery_browse(struct netmidi_discovery *discovery,
                             const char *type);
/* The signal mdnsd raises when it has events for us, to wait on with the
 * others; -1 while there is no session. */
int netmidi_discovery_signal(const struct netmidi_discovery *discovery);
/* The next instance that appeared, changed or left, 1, or 0 when there is
 * none waiting. Called until it returns 0, after the signal (or on any
 * turn: without the signal it does nothing). An instance is reported as
 * present once its port and an address are known, and again when its
 * addresses, port or endpoint name change; it is followed until it goes.
 * A search whose events were lost is begun again, and its instances
 * reported anew. Our own registrations come back here too
 * (netmidi_discovery_own()). */
int netmidi_discovery_next(struct netmidi_discovery *discovery,
                           struct netmidi_discovered *event);
/* Is name one of ours, as mdnsd announced it? A conflict may have given a
 * service another name than the one asked for ("AROS MIDI (2)"), and then
 * an instance under the name asked for is another host's. */
int netmidi_discovery_own(const struct netmidi_discovery *discovery,
                          const char *name);
/* An address as text: "192.168.1.2", "fe80::1%2". */
void netmidi_discovery_address_text(const struct netmidi_address *address,
                                    char *text, size_t size);
/* The value of key in a text record as it is on the wire: strings each
 * after its length (RFC 6763 section 6). Keys match without regard to
 * ASCII case. The value is copied as a string, cut at size - 1 bytes or at
 * a NUL. 0 when the key has a value, -1 when not. */
int netmidi_discovery_txt_value(const uint8_t *txt, size_t length,
                                const char *key, char *value, size_t size);
/* Withdraws everything, with the goodbye the service sends for it. */
void netmidi_discovery_close(struct netmidi_discovery *discovery);

#endif
