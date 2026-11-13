/*
 * SPDX-License-Identifier: BSD-2-Clause
 * Copyright 2026 Jaime Dias
 *
 * The canonical DNS-SD event: the pointer-free, sized representation mdnsd
 * builds every event in (D2), checked here against what it says. dnssd.library
 * hands it to its clients as it is: <dnssd/dnssd.h>'s struct DNSSDEvent has
 * this layout. Version 2 adds the operations that are not about a service
 * (a name's addresses, a record) and the operation's end (D6).
 */
#ifndef DNSSD_MODEL_PROTOTYPE_H
#define DNSSD_MODEL_PROTOTYPE_H

#include <stddef.h>
#include <stdint.h>

#define DNSSD_MODEL_VERSION 2u

#define DNSSD_MODEL_ADDR_IPV4 4u
#define DNSSD_MODEL_ADDR_IPV6 6u

/*
 * Event kinds. Version 2 adds, for the operations that are not about a
 * service (D6: a name's addresses, a record of any type) and for the
 * life of an operation, the kinds after TYPE; the first five are
 * version 1's.
 */
#define DNSSD_MODEL_EVENT_ADD            1u   /* an instance on interface_index */
#define DNSSD_MODEL_EVENT_REMOVE         2u   /* that instance is gone */
#define DNSSD_MODEL_EVENT_RESOLVED       3u   /* target, port, TXT, addresses */
#define DNSSD_MODEL_EVENT_RESOLVE_FAILED 4u
#define DNSSD_MODEL_EVENT_TYPE           5u   /* a service type on the link */
#define DNSSD_MODEL_EVENT_ADDRESS        6u   /* addresses of name */
#define DNSSD_MODEL_EVENT_RECORD         7u   /* a record of name: rrtype, rdata */
#define DNSSD_MODEL_EVENT_REGISTERED     8u   /* the instance's name, announced */
#define DNSSD_MODEL_EVENT_END            9u   /* the operation is over: result */
#define DNSSD_MODEL_EVENT_LOST          10u   /* events were dropped */

#define DNSSD_MODEL_FLAG_TRUNCATED 0x00000001u
#define DNSSD_MODEL_FLAG_LOST      0x00000002u
#define DNSSD_MODEL_FLAG_MORE      0x00000004u
#define DNSSD_MODEL_FLAG_REMOVED   0x00000008u  /* TYPE, ADDRESS, RECORD: gone */

enum dnssd_model_result {
    DNSSD_MODEL_OK = 0,
    DNSSD_MODEL_BAD_ARGUMENT = -1,
    DNSSD_MODEL_TOO_SMALL = -2,
    DNSSD_MODEL_BAD_MESSAGE = -3,
    DNSSD_MODEL_OVERFLOW = -4
};

struct dnssd_model_span {
    const uint8_t *data;
    uint32_t size;
};

struct dnssd_model_address {
    uint16_t family;
    uint16_t length;
    uint32_t interface_index;
    uint8_t bytes[16];
};

/*
 * What an event says. Names are DNS wire labels (name, service_type,
 * domain, target); instance is the instance's own label, as it is shown
 * (UTF-8, at most 63 bytes, dots allowed); txt is TXT data; rdata a
 * record's data with the names in it uncompressed.
 */
struct dnssd_model_service {
    uint16_t kind;
    uint16_t rrtype;
    uint32_t flags;
    int32_t result;
    uint32_t incarnation;
    uint32_t session;
    uint32_t operation;
    uint32_t sequence;
    uint32_t interface_index;
    uint32_t ttl;
    uint16_t priority;
    uint16_t weight;
    uint16_t port;
    uint16_t reserved;
    struct dnssd_model_span name;
    struct dnssd_model_span instance;
    struct dnssd_model_span service_type;
    struct dnssd_model_span domain;
    struct dnssd_model_span target;
    struct dnssd_model_span txt;
    struct dnssd_model_span rdata;
    const struct dnssd_model_address *addresses;
    uint32_t address_count;
};

/* Every serialized reference is an offset from the message start. */
struct dnssd_model_ref {
    uint32_t offset;
    uint32_t size;
};

/*
 * Same-host private representation, and what dnssd.library hands its
 * clients (struct DNSSDEvent has this layout). It is fixed-width and has
 * no pointers, Task *, MsgPort * or borrowed cache storage. Byte order is
 * the host's because this is an Exec message, not a network packet.
 */
struct dnssd_model_message {
    uint32_t size;
    uint16_t version;
    uint16_t header_size;
    uint16_t kind;
    uint16_t rrtype;
    uint32_t flags;
    int32_t result;
    uint32_t incarnation;
    uint32_t session;
    uint32_t operation;
    uint32_t sequence;
    uint32_t interface_index;
    uint32_t ttl;
    uint16_t priority;
    uint16_t weight;
    uint16_t port;
    uint16_t address_count;
    struct dnssd_model_ref name;
    struct dnssd_model_ref instance;
    struct dnssd_model_ref service_type;
    struct dnssd_model_ref domain;
    struct dnssd_model_ref target;
    struct dnssd_model_ref txt;
    struct dnssd_model_ref rdata;
    struct dnssd_model_ref addresses;
};

struct dnssd_model_wire_address {
    uint16_t family;
    uint16_t length;
    uint32_t interface_index;
    uint8_t bytes[16];
};

int dnssd_model_measure(const struct dnssd_model_service *service,
                        size_t *required);
int dnssd_model_build(const struct dnssd_model_service *service,
                      void *buffer, size_t capacity, size_t *written);
int dnssd_model_validate(const void *buffer, size_t capacity);
int dnssd_model_copy_address(const void *buffer, size_t capacity,
                             uint32_t index,
                             struct dnssd_model_address *address);

#endif
