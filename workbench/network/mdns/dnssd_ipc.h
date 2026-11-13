/*
 * SPDX-License-Identifier: BSD-2-Clause
 * Copyright 2026 Jaime Dias
 *
 * dnssd_ipc.h - the requests dnssd.library sends mdnsd
 *
 * Private to the two, both built from this file. A request is one block:
 * a fixed-width header, then the data it refers to. It has no pointers -
 * nothing of the caller's is lent - so mdnsd reads only what the block
 * holds, and checks it all (dnssd_ipc_validate()) before it reads any of
 * it: the size, the version, what the command needs and must not have,
 * every reference inside the block and apart from the others, names,
 * labels, TXT data and addresses well formed. As the events
 * (dnssd_model.h), whose conventions this follows: fixed-width fields,
 * references as offset and size from the start of the request, host byte
 * order (an Exec message, not a packet).
 *
 * The Exec side - the message around the request, the port - is in
 * dnssd_ipc_port.h; this part builds and is tested on any host.
 *
 * Sessions and operations are numbers mdnsd gives out (a slot and a
 * generation) under its incarnation, so that a number from before a
 * restart, or of a slot used again since, is refused.
 */
#ifndef MDNSD_DNSSD_IPC_H
#define MDNSD_DNSSD_IPC_H

#include <stddef.h>
#include <stdint.h>

#define DNSSD_IPC_VERSION   2u

/* commands */
#define DNSSD_IPC_OPEN      1u  /* -> incarnation, session */
#define DNSSD_IPC_CLOSE     2u  /* cancels every operation of the session */
#define DNSSD_IPC_CANCEL    3u  /* operation */
#define DNSSD_IPC_READ      4u  /* the next event, into reply */
#define DNSSD_IPC_STATUS    5u  /* -> incarnation, sessions, operations */
#define DNSSD_IPC_LOOKUP    6u  /* name; family, interface_index, timeout */
#define DNSSD_IPC_BROWSE    7u  /* service_type, domain, [subtypes: one]; interface_index */
#define DNSSD_IPC_RESOLVE   8u  /* instance, service_type, domain; interface_index, timeout */
#define DNSSD_IPC_QUERY     9u  /* name; rrtype, interface_index, timeout */
#define DNSSD_IPC_REGISTER 10u  /* [instance], service_type, domain, port, [txt], [subtypes],
                                 * [host with addresses]; interface_index */
#define DNSSD_IPC_UPDATE   11u  /* operation; [txt], [addresses], [port] */
#define DNSSD_IPC_COMMANDS 11u

#define DNSSD_IPC_MAX_SIZE      16384u  /* the largest request */
#define DNSSD_IPC_MAX_ADDRESSES 8u      /* addresses of a host registered for */
#define DNSSD_IPC_MAX_SUBTYPES  8u

enum dnssd_ipc_result {
    DNSSD_IPC_OK = 0,
    DNSSD_IPC_BAD_ARGUMENT = -1,
    DNSSD_IPC_TOO_SMALL = -2,
    DNSSD_IPC_BAD_MESSAGE = -3
};

struct dnssd_ipc_ref {
    uint32_t offset;            /* from the start of the request */
    uint32_t size;
};

/*
 * The header. size: bytes the request holds as sent; capacity: bytes of
 * the block, at least size - room for what a READ writes. The fields
 * after result are the request's; the reply sets result and, as the
 * command says, incarnation, session, operation, the two counts or reply.
 */
struct dnssd_ipc_request {
    uint32_t size;
    uint32_t capacity;
    uint16_t version;
    uint16_t header_size;
    uint16_t command;
    uint16_t flags;             /* none defined: 0 */
    int32_t  result;
    uint32_t task;              /* the caller's GetETaskID() */
    uint32_t incarnation;
    uint32_t session;
    uint32_t operation;
    uint32_t family;            /* LOOKUP: DNSSD_FAMILY_* */
    uint32_t interface_index;   /* 0: every link */
    uint32_t timeout;           /* seconds */
    uint16_t rrtype;            /* QUERY */
    uint16_t port;              /* REGISTER, UPDATE (0: unchanged) */
    uint32_t count[2];          /* STATUS: sessions, operations */
    struct dnssd_ipc_ref name;          /* LOOKUP, QUERY: wire name */
    struct dnssd_ipc_ref instance;      /* the instance label, UTF-8, 1-63 bytes */
    struct dnssd_ipc_ref service_type;  /* wire name: _service._tcp/_udp */
    struct dnssd_ipc_ref domain;        /* wire name */
    struct dnssd_ipc_ref subtypes;      /* labels, each a length byte and its bytes */
    struct dnssd_ipc_ref txt;           /* TXT data (strings, each with its length byte) */
    struct dnssd_ipc_ref host;          /* the host's label, 1-63 bytes */
    struct dnssd_ipc_ref addresses;     /* struct dnssd_model_wire_address records */
    struct dnssd_ipc_ref reply;         /* READ: where the event goes (capacity, not size) */
};

/* What a request carries, for dnssd_ipc_build(): each part's bytes and
 * size, NULL/0 for none. addresses: address_count records. */
struct dnssd_ipc_parts {
    const uint8_t *name;          uint32_t name_size;
    const uint8_t *instance;      uint32_t instance_size;
    const uint8_t *service_type;  uint32_t service_type_size;
    const uint8_t *domain;        uint32_t domain_size;
    const uint8_t *subtypes;      uint32_t subtypes_size;
    const uint8_t *txt;           uint32_t txt_size;
    const uint8_t *host;          uint32_t host_size;
    const void    *addresses;     uint32_t address_count;
    uint32_t reply_room;          /* READ: bytes for the event */
};

/* Bytes a request with these parts takes (its capacity). */
int dnssd_ipc_measure(const struct dnssd_ipc_parts *parts, size_t *required);
/* Write the header (zeroed, but for the sizes, version, command and the
 * references) and copy the parts after it. The caller sets the rest. */
int dnssd_ipc_build(uint16_t command, const struct dnssd_ipc_parts *parts,
                    void *buffer, size_t capacity, size_t *written);
/* Is the request in buffer (capacity bytes there) one mdnsd may read? */
int dnssd_ipc_validate(const void *buffer, size_t capacity);
/* The bytes of a reference of a valid request. */
const uint8_t *dnssd_ipc_data(const void *buffer, const struct dnssd_ipc_ref *ref);

/* Well formed: a wire name of size bytes exactly; TXT data; labels. */
int dnssd_ipc_wire_name_ok(const uint8_t *name, uint32_t size);
int dnssd_ipc_txt_ok(const uint8_t *txt, uint32_t size);
int dnssd_ipc_labels_ok(const uint8_t *labels, uint32_t size, uint32_t max);

#endif /* MDNSD_DNSSD_IPC_H */
