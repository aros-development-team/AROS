/*
 * SPDX-License-Identifier: BSD-2-Clause
 * Copyright 2026 Jaime Dias
 *
 * Private DNS-SD canonical-model canary.
 *
 * This is not a dnssd.library public header or an installed IPC contract.
 * It tests the pointer-free, sized representation proposed for mdnsd D2.
 */
#ifndef DNSSD_MODEL_PROTOTYPE_H
#define DNSSD_MODEL_PROTOTYPE_H

#include <stddef.h>
#include <stdint.h>

#define DNSSD_MODEL_VERSION 1u

#define DNSSD_MODEL_ADDR_IPV4 4u
#define DNSSD_MODEL_ADDR_IPV6 6u

#define DNSSD_MODEL_EVENT_ADD            1u
#define DNSSD_MODEL_EVENT_REMOVE         2u
#define DNSSD_MODEL_EVENT_RESOLVED       3u
#define DNSSD_MODEL_EVENT_RESOLVE_FAILED 4u
#define DNSSD_MODEL_EVENT_TYPE           5u

#define DNSSD_MODEL_FLAG_TRUNCATED 0x00000001u
#define DNSSD_MODEL_FLAG_LOST      0x00000002u
#define DNSSD_MODEL_FLAG_MORE      0x00000004u

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

struct dnssd_model_service {
    uint16_t kind;
    uint16_t reserved;
    uint32_t flags;
    uint32_t incarnation;
    uint32_t session;
    uint32_t operation;
    uint32_t sequence;
    uint32_t interface_index;
    uint32_t ttl;
    uint16_t priority;
    uint16_t weight;
    uint16_t port;
    uint16_t reserved2;
    struct dnssd_model_span instance;
    struct dnssd_model_span service_type;
    struct dnssd_model_span domain;
    struct dnssd_model_span target;
    struct dnssd_model_span txt;
    const struct dnssd_model_address *addresses;
    uint32_t address_count;
};

/* Every serialized reference is an offset from the message start. */
struct dnssd_model_ref {
    uint32_t offset;
    uint32_t size;
};

/*
 * Same-host private representation. It is deliberately fixed-width and has
 * no pointers, Task *, MsgPort * or borrowed cache storage. Byte order is the
 * host's because this is an Exec IPC canary, not a network packet format.
 */
struct dnssd_model_message {
    uint32_t size;
    uint16_t version;
    uint16_t header_size;
    uint16_t kind;
    uint16_t reserved;
    uint32_t flags;
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
    struct dnssd_model_ref instance;
    struct dnssd_model_ref service_type;
    struct dnssd_model_ref domain;
    struct dnssd_model_ref target;
    struct dnssd_model_ref txt;
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
