/*
 * SPDX-License-Identifier: BSD-2-Clause
 * Copyright 2026 Jaime Dias
 *
 * dnssd_ipc.c - building and checking dnssd.library's requests to mdnsd
 * (dnssd_ipc.h). Plain C: the same code is built into the library, into
 * mdnsd and into the host test.
 */
#include <string.h>

#include "dnssd_ipc.h"
#include "dnssd_model.h"

#define ALIGN4(x) (((x) + 3u) & ~(uint32_t)3u)

/* What a command takes: each part required, allowed or refused. */
#define P_NAME     0x001u
#define P_INSTANCE 0x002u
#define P_TYPE     0x004u
#define P_DOMAIN   0x008u
#define P_SUBTYPES 0x010u
#define P_TXT      0x020u
#define P_HOST     0x040u
#define P_ADDRS    0x080u
#define P_REPLY    0x100u
#define N_SESSION  0x1000u      /* needs a session */
#define N_OPERATION 0x2000u     /* needs an operation */

static const struct {
    uint16_t required;
    uint16_t allowed;
} commands[DNSSD_IPC_COMMANDS + 1] = {
    { 0, 0 },
    /* OPEN */     { 0, 0 },
    /* CLOSE */    { N_SESSION, 0 },
    /* CANCEL */   { N_SESSION | N_OPERATION, 0 },
    /* READ */     { N_SESSION | P_REPLY, P_REPLY },
    /* STATUS */   { 0, 0 },
    /* LOOKUP */   { N_SESSION | P_NAME, P_NAME },
    /* BROWSE */   { N_SESSION | P_TYPE | P_DOMAIN, P_TYPE | P_DOMAIN | P_SUBTYPES },
    /* RESOLVE */  { N_SESSION | P_INSTANCE | P_TYPE | P_DOMAIN, P_INSTANCE | P_TYPE | P_DOMAIN },
    /* QUERY */    { N_SESSION | P_NAME, P_NAME },
    /* REGISTER */ { N_SESSION | P_TYPE | P_DOMAIN,
                     P_INSTANCE | P_TYPE | P_DOMAIN | P_SUBTYPES | P_TXT | P_HOST | P_ADDRS },
    /* UPDATE */   { N_SESSION | N_OPERATION, P_TXT | P_ADDRS },
};

int
dnssd_ipc_wire_name_ok(const uint8_t *name, uint32_t size)
{
    uint32_t at = 0;

    if (name == NULL || size < 1u || size > 255u)
        return 0;
    while (at < size) {
        uint8_t len = name[at];

        if (len == 0)
            return at + 1u == size;     /* the root label ends it, exactly */
        if (len > 63u)
            return 0;                   /* no compression, no extended labels */
        at += 1u + len;
    }
    return 0;
}

int
dnssd_ipc_txt_ok(const uint8_t *txt, uint32_t size)
{
    uint32_t at = 0;

    if (size == 0)
        return 1;
    if (txt == NULL)
        return 0;
    while (at < size)
        at += 1u + txt[at];
    return at == size;
}

int
dnssd_ipc_labels_ok(const uint8_t *labels, uint32_t size, uint32_t max)
{
    uint32_t at = 0, count = 0;

    if (labels == NULL || size == 0)
        return 0;
    while (at < size) {
        uint8_t len = labels[at];

        if (len == 0 || len > 63u || ++count > max)
            return 0;
        at += 1u + len;
    }
    return at == size;
}

/* The references, in the order the parts are laid out. */
static struct dnssd_ipc_ref *
ref_at(struct dnssd_ipc_request *r, unsigned i)
{
    struct dnssd_ipc_ref *refs[9];

    refs[0] = &r->name;
    refs[1] = &r->instance;
    refs[2] = &r->service_type;
    refs[3] = &r->domain;
    refs[4] = &r->subtypes;
    refs[5] = &r->txt;
    refs[6] = &r->host;
    refs[7] = &r->addresses;
    refs[8] = &r->reply;
    return refs[i];
}

static void
part_at(const struct dnssd_ipc_parts *p, unsigned i, const uint8_t **data, uint32_t *size)
{
    switch (i) {
    case 0: *data = p->name; *size = p->name_size; break;
    case 1: *data = p->instance; *size = p->instance_size; break;
    case 2: *data = p->service_type; *size = p->service_type_size; break;
    case 3: *data = p->domain; *size = p->domain_size; break;
    case 4: *data = p->subtypes; *size = p->subtypes_size; break;
    case 5: *data = p->txt; *size = p->txt_size; break;
    case 6: *data = p->host; *size = p->host_size; break;
    case 7:
        *data = (const uint8_t *)p->addresses;
        *size = p->address_count * (uint32_t)sizeof(struct dnssd_model_wire_address);
        break;
    default: *data = NULL; *size = p->reply_room; break;
    }
}

int
dnssd_ipc_measure(const struct dnssd_ipc_parts *parts, size_t *required)
{
    uint32_t total = ALIGN4((uint32_t)sizeof(struct dnssd_ipc_request));
    unsigned i;

    if (parts == NULL || required == NULL ||
        parts->address_count > DNSSD_IPC_MAX_ADDRESSES)
        return DNSSD_IPC_BAD_ARGUMENT;
    for (i = 0; i < 9; i++) {
        const uint8_t *data;
        uint32_t size;

        part_at(parts, i, &data, &size);
        if (size > DNSSD_IPC_MAX_SIZE || (i < 8 && size != 0 && data == NULL))
            return DNSSD_IPC_BAD_ARGUMENT;
        total = ALIGN4(total + size);
    }
    if (total > DNSSD_IPC_MAX_SIZE)
        return DNSSD_IPC_BAD_ARGUMENT;
    *required = total;
    return DNSSD_IPC_OK;
}

int
dnssd_ipc_build(uint16_t command, const struct dnssd_ipc_parts *parts,
                void *buffer, size_t capacity, size_t *written)
{
    struct dnssd_ipc_request *r = (struct dnssd_ipc_request *)buffer;
    uint8_t *base = (uint8_t *)buffer;
    size_t need;
    uint32_t at;
    unsigned i;
    int rc;

    if (buffer == NULL || written == NULL || command == 0 ||
        command > DNSSD_IPC_COMMANDS || ((uintptr_t)buffer & 3u) != 0)
        return DNSSD_IPC_BAD_ARGUMENT;
    if ((rc = dnssd_ipc_measure(parts, &need)) != DNSSD_IPC_OK)
        return rc;
    if (capacity < need)
        return DNSSD_IPC_TOO_SMALL;
    memset(r, 0, sizeof(*r));
    r->version = DNSSD_IPC_VERSION;
    r->header_size = (uint16_t)sizeof(*r);
    r->command = command;
    at = ALIGN4((uint32_t)sizeof(*r));
    for (i = 0; i < 9; i++) {
        struct dnssd_ipc_ref *ref = ref_at(r, i);
        const uint8_t *data;
        uint32_t size;

        part_at(parts, i, &data, &size);
        if (size != 0) {
            ref->offset = at;
            ref->size = size;
            if (data != NULL)
                memcpy(base + at, data, size);
            else
                memset(base + at, 0, size);
        }
        at = ALIGN4(at + size);
    }
    /* the reply's room is not sent */
    r->size = parts->reply_room != 0 ? r->reply.offset : at;
    r->capacity = at;
    *written = at;
    return DNSSD_IPC_OK;
}

const uint8_t *
dnssd_ipc_data(const void *buffer, const struct dnssd_ipc_ref *ref)
{
    return ref->size == 0 ? NULL : (const uint8_t *)buffer + ref->offset;
}

/* Each address: a family and its length, an IPv6 link-local one with its
 * link, the bytes past the address zero. */
static int
addresses_ok(const uint8_t *data, uint32_t size)
{
    uint32_t i, n = size / (uint32_t)sizeof(struct dnssd_model_wire_address);

    if (size % sizeof(struct dnssd_model_wire_address) != 0 || n == 0 ||
        n > DNSSD_IPC_MAX_ADDRESSES)
        return 0;
    for (i = 0; i < n; i++) {
        struct dnssd_model_wire_address a;
        uint32_t k;

        memcpy(&a, data + i * sizeof(a), sizeof(a));
        if (!((a.family == DNSSD_MODEL_ADDR_IPV4 && a.length == 4u) ||
              (a.family == DNSSD_MODEL_ADDR_IPV6 && a.length == 16u)))
            return 0;
        if (a.family == DNSSD_MODEL_ADDR_IPV6 && a.bytes[0] == 0xfe &&
            (a.bytes[1] & 0xc0) == 0x80 && a.interface_index == 0)
            return 0;                   /* fe80::/10 needs its link */
        for (k = a.length; k < sizeof(a.bytes); k++)
            if (a.bytes[k] != 0)
                return 0;
    }
    return 1;
}

int
dnssd_ipc_validate(const void *buffer, size_t capacity)
{
    struct dnssd_ipc_request r;
    const uint8_t *base = (const uint8_t *)buffer;
    uint32_t have = 0, end[9], start[9];
    unsigned i, j;

    if (buffer == NULL || capacity < sizeof(r) || ((uintptr_t)buffer & 3u) != 0)
        return DNSSD_IPC_BAD_MESSAGE;
    memcpy(&r, buffer, sizeof(r));
    if (r.version != DNSSD_IPC_VERSION || r.header_size != sizeof(r) ||
        r.command == 0 || r.command > DNSSD_IPC_COMMANDS || r.flags != 0 ||
        r.size < sizeof(r) || r.size > r.capacity || r.capacity > capacity ||
        r.capacity > DNSSD_IPC_MAX_SIZE)
        return DNSSD_IPC_BAD_MESSAGE;

    /* every reference inside: the request's data in size, the reply's
     * room in capacity; none before the data, none over another */
    for (i = 0; i < 9; i++) {
        const struct dnssd_ipc_ref *ref = ref_at(&r, i);
        uint32_t limit = i == 8 ? r.capacity : r.size;

        start[i] = end[i] = 0;
        if (ref->size == 0) {
            if (ref->offset != 0)
                return DNSSD_IPC_BAD_MESSAGE;
            continue;
        }
        if (ref->offset < sizeof(r) || ref->offset > limit ||
            ref->size > limit - ref->offset || (ref->offset & 3u) != 0)
            return DNSSD_IPC_BAD_MESSAGE;
        start[i] = ref->offset;
        end[i] = ref->offset + ref->size;
        for (j = 0; j < i; j++)
            if (end[j] != 0 && start[i] < end[j] && start[j] < end[i])
                return DNSSD_IPC_BAD_MESSAGE;
        have |= 1u << i;
    }

    /* what the command needs, and nothing it does not take */
    if ((have & commands[r.command].required & 0x1ffu) !=
            (commands[r.command].required & 0x1ffu) ||
        (have & ~(uint32_t)commands[r.command].allowed) != 0 ||
        ((commands[r.command].required & N_SESSION) && r.session == 0) ||
        ((commands[r.command].required & N_OPERATION) && r.operation == 0))
        return DNSSD_IPC_BAD_MESSAGE;

    /* what each part holds */
    if ((have & P_NAME) && !dnssd_ipc_wire_name_ok(base + r.name.offset, r.name.size))
        return DNSSD_IPC_BAD_MESSAGE;
    if ((have & P_INSTANCE) && (r.instance.size > 63u ||
            memchr(base + r.instance.offset, 0, r.instance.size) != NULL))
        return DNSSD_IPC_BAD_MESSAGE;
    if ((have & P_TYPE) &&
        !dnssd_ipc_wire_name_ok(base + r.service_type.offset, r.service_type.size))
        return DNSSD_IPC_BAD_MESSAGE;
    if ((have & P_DOMAIN) && !dnssd_ipc_wire_name_ok(base + r.domain.offset, r.domain.size))
        return DNSSD_IPC_BAD_MESSAGE;
    if ((have & P_SUBTYPES) && !dnssd_ipc_labels_ok(base + r.subtypes.offset, r.subtypes.size,
            r.command == DNSSD_IPC_BROWSE ? 1u : DNSSD_IPC_MAX_SUBTYPES))
        return DNSSD_IPC_BAD_MESSAGE;
    if ((have & P_TXT) && !dnssd_ipc_txt_ok(base + r.txt.offset, r.txt.size))
        return DNSSD_IPC_BAD_MESSAGE;
    if ((have & P_HOST) && (r.host.size > 63u ||
            memchr(base + r.host.offset, 0, r.host.size) != NULL ||
            memchr(base + r.host.offset, '.', r.host.size) != NULL))
        return DNSSD_IPC_BAD_MESSAGE;
    if ((have & P_ADDRS) && !addresses_ok(base + r.addresses.offset, r.addresses.size))
        return DNSSD_IPC_BAD_MESSAGE;
    /* a host is registered for with its addresses, and only then */
    if (r.command == DNSSD_IPC_REGISTER && !!(have & P_HOST) != !!(have & P_ADDRS))
        return DNSSD_IPC_BAD_MESSAGE;
    if (r.command == DNSSD_IPC_REGISTER && r.port == 0)
        return DNSSD_IPC_BAD_MESSAGE;
    if (r.command == DNSSD_IPC_UPDATE && !(have & (P_TXT | P_ADDRS)) && r.port == 0)
        return DNSSD_IPC_BAD_MESSAGE;
    return DNSSD_IPC_OK;
}
