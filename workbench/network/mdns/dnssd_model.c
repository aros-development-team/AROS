/*
 * SPDX-License-Identifier: BSD-2-Clause
 * Copyright 2026 Jaime Dias
 */

#include "dnssd_model.h"

#include <limits.h>
#include <string.h>

typedef char dnssd_model_message_size_must_be_96[
    sizeof(struct dnssd_model_message) == 96u ? 1 : -1];
typedef char dnssd_model_address_size_must_be_24[
    sizeof(struct dnssd_model_wire_address) == 24u ? 1 : -1];

static int add_size(size_t *total, size_t amount)
{
    if (amount > SIZE_MAX - *total)
        return DNSSD_MODEL_OVERFLOW;
    *total += amount;
    return DNSSD_MODEL_OK;
}

static int align_size(size_t *total)
{
    size_t padding = (4u - (*total & 3u)) & 3u;

    return add_size(total, padding);
}

static int valid_span(struct dnssd_model_span span)
{
    return span.size == 0u || span.data != NULL;
}

static int valid_kind(uint16_t kind)
{
    return kind >= DNSSD_MODEL_EVENT_ADD &&
        kind <= DNSSD_MODEL_EVENT_TYPE;
}

static int valid_wire_name(const uint8_t *name, uint32_t size)
{
    uint32_t at = 0;

    if (name == NULL || size == 0u || size > 256u)
        return 0;
    while (at < size) {
        uint32_t length = name[at++];

        if (length == 0u)
            return at == size;
        if (length > 63u || length > size - at)
            return 0;
        at += length;
    }
    return 0;
}

static int valid_txt(const uint8_t *txt, uint32_t size)
{
    uint32_t at = 0;

    if (size != 0u && txt == NULL)
        return 0;
    while (at < size) {
        uint32_t length = txt[at++];

        if (length > size - at)
            return 0;
        at += length;
    }
    return 1;
}

static int valid_address(const struct dnssd_model_address *address)
{
    return address != NULL &&
        ((address->family == DNSSD_MODEL_ADDR_IPV4 &&
             address->length == 4u) ||
         (address->family == DNSSD_MODEL_ADDR_IPV6 &&
             address->length == 16u));
}

static int valid_service(const struct dnssd_model_service *service)
{
    uint32_t i;

    if (service == NULL || !valid_kind(service->kind) ||
        !valid_span(service->instance) ||
        !valid_span(service->service_type) ||
        !valid_span(service->domain) || !valid_span(service->target) ||
        !valid_span(service->txt) || service->instance.size > 63u ||
        !valid_wire_name(service->service_type.data,
                         service->service_type.size) ||
        !valid_wire_name(service->domain.data, service->domain.size) ||
        (service->target.size != 0u &&
            !valid_wire_name(service->target.data, service->target.size)) ||
        !valid_txt(service->txt.data, service->txt.size) ||
        service->address_count > UINT16_MAX ||
        (service->address_count != 0u && service->addresses == NULL))
        return 0;
    for (i = 0; i < service->address_count; ++i) {
        if (!valid_address(&service->addresses[i]))
            return 0;
    }
    return 1;
}

static int measure_span(size_t *total, struct dnssd_model_span span)
{
    int result;

    if (span.size == 0u)
        return DNSSD_MODEL_OK;
    result = align_size(total);
    if (result != DNSSD_MODEL_OK)
        return result;
    return add_size(total, span.size);
}

int dnssd_model_measure(const struct dnssd_model_service *service,
                        size_t *required)
{
    size_t total = sizeof(struct dnssd_model_message);
    size_t addresses;
    int result;

    if (required == NULL || !valid_service(service))
        return DNSSD_MODEL_BAD_ARGUMENT;
    result = measure_span(&total, service->instance);
    if (result == DNSSD_MODEL_OK)
        result = measure_span(&total, service->service_type);
    if (result == DNSSD_MODEL_OK)
        result = measure_span(&total, service->domain);
    if (result == DNSSD_MODEL_OK)
        result = measure_span(&total, service->target);
    if (result == DNSSD_MODEL_OK)
        result = measure_span(&total, service->txt);
    if (result != DNSSD_MODEL_OK)
        return result;
    result = align_size(&total);
    if (result != DNSSD_MODEL_OK)
        return result;
    addresses = service->address_count;
    if (addresses != 0u &&
        sizeof(struct dnssd_model_wire_address) > SIZE_MAX / addresses)
        return DNSSD_MODEL_OVERFLOW;
    addresses *= sizeof(struct dnssd_model_wire_address);
    result = add_size(&total, addresses);
    if (result != DNSSD_MODEL_OK || total > UINT32_MAX)
        return DNSSD_MODEL_OVERFLOW;
    *required = total;
    return DNSSD_MODEL_OK;
}

static void append_span(uint8_t *bytes, size_t *cursor,
                        struct dnssd_model_ref *reference,
                        struct dnssd_model_span span)
{
    if (span.size == 0u) {
        reference->offset = 0u;
        reference->size = 0u;
        return;
    }
    *cursor = (*cursor + 3u) & ~(size_t)3u;
    reference->offset = (uint32_t)*cursor;
    reference->size = span.size;
    memcpy(bytes + *cursor, span.data, span.size);
    *cursor += span.size;
}

static int compare_address(const struct dnssd_model_wire_address *left,
                           const struct dnssd_model_wire_address *right)
{
    int compared;

    if (left->interface_index != right->interface_index)
        return left->interface_index < right->interface_index ? -1 : 1;
    if (left->family != right->family)
        return left->family < right->family ? -1 : 1;
    compared = memcmp(left->bytes, right->bytes, left->length);
    if (compared != 0)
        return compared;
    return left->length == right->length ? 0 :
        (left->length < right->length ? -1 : 1);
}

static uint16_t append_addresses(uint8_t *bytes, size_t *cursor,
                                 struct dnssd_model_ref *reference,
                                 const struct dnssd_model_address *addresses,
                                 uint32_t count)
{
    struct dnssd_model_wire_address *output;
    uint16_t used = 0;
    uint32_t i;

    *cursor = (*cursor + 3u) & ~(size_t)3u;
    reference->offset = count == 0u ? 0u : (uint32_t)*cursor;
    output = (struct dnssd_model_wire_address *)(void *)(bytes + *cursor);
    for (i = 0; i < count; ++i) {
        struct dnssd_model_wire_address item;
        uint16_t at;

        memset(&item, 0, sizeof(item));
        item.family = addresses[i].family;
        item.length = addresses[i].length;
        item.interface_index = addresses[i].interface_index;
        memcpy(item.bytes, addresses[i].bytes, item.length);
        for (at = 0; at < used; ++at) {
            int compared = compare_address(&item, &output[at]);

            if (compared <= 0) {
                if (compared == 0)
                    break;
                memmove(&output[at + 1u], &output[at],
                        (size_t)(used - at) * sizeof(*output));
                output[at] = item;
                ++used;
                break;
            }
        }
        if (at == used) {
            output[used++] = item;
        }
    }
    reference->size = (uint32_t)((size_t)used * sizeof(*output));
    *cursor += reference->size;
    return used;
}

int dnssd_model_build(const struct dnssd_model_service *service,
                      void *buffer, size_t capacity, size_t *written)
{
    struct dnssd_model_message *message;
    uint8_t *bytes = buffer;
    size_t required;
    size_t cursor = sizeof(*message);
    int result;

    if (written == NULL)
        return DNSSD_MODEL_BAD_ARGUMENT;
    result = dnssd_model_measure(service, &required);
    if (result != DNSSD_MODEL_OK)
        return result;
    *written = required;
    if (buffer == NULL || capacity < required)
        return DNSSD_MODEL_TOO_SMALL;
    memset(buffer, 0, required);
    message = buffer;
    message->version = DNSSD_MODEL_VERSION;
    message->header_size = (uint16_t)sizeof(*message);
    message->kind = service->kind;
    message->flags = service->flags;
    message->incarnation = service->incarnation;
    message->session = service->session;
    message->operation = service->operation;
    message->sequence = service->sequence;
    message->interface_index = service->interface_index;
    message->ttl = service->ttl;
    message->priority = service->priority;
    message->weight = service->weight;
    message->port = service->port;
    append_span(bytes, &cursor, &message->instance, service->instance);
    append_span(bytes, &cursor, &message->service_type,
                service->service_type);
    append_span(bytes, &cursor, &message->domain, service->domain);
    append_span(bytes, &cursor, &message->target, service->target);
    append_span(bytes, &cursor, &message->txt, service->txt);
    message->address_count = append_addresses(bytes, &cursor,
        &message->addresses, service->addresses, service->address_count);
    message->size = (uint32_t)cursor;
    *written = cursor;
    return DNSSD_MODEL_OK;
}

static int valid_ref(struct dnssd_model_ref reference, uint32_t total,
                     uint32_t header)
{
    if (reference.size == 0u)
        return reference.offset == 0u;
    return reference.offset >= header && (reference.offset & 3u) == 0u &&
        reference.offset <= total && reference.size <= total - reference.offset;
}

static int refs_overlap(struct dnssd_model_ref left,
                        struct dnssd_model_ref right)
{
    uint32_t left_end;
    uint32_t right_end;

    if (left.size == 0u || right.size == 0u)
        return 0;
    left_end = left.offset + left.size;
    right_end = right.offset + right.size;
    return left.offset < right_end && right.offset < left_end;
}

static int validate_refs(const struct dnssd_model_message *message)
{
    const struct dnssd_model_ref refs[] = {
        message->instance, message->service_type, message->domain,
        message->target, message->txt, message->addresses
    };
    size_t i, j;

    for (i = 0; i < sizeof(refs) / sizeof(refs[0]); ++i) {
        if (!valid_ref(refs[i], message->size, message->header_size))
            return 0;
        for (j = 0; j < i; ++j) {
            if (refs_overlap(refs[i], refs[j]))
                return 0;
        }
    }
    return 1;
}

int dnssd_model_validate(const void *buffer, size_t capacity)
{
    struct dnssd_model_message message;
    const uint8_t *bytes = buffer;
    uint32_t i;

    if (buffer == NULL || capacity < sizeof(message))
        return DNSSD_MODEL_BAD_MESSAGE;
    memcpy(&message, buffer, sizeof(message));
    if (message.version != DNSSD_MODEL_VERSION ||
        message.header_size != sizeof(message) ||
        !valid_kind(message.kind) ||
        message.size < sizeof(message) || message.size > capacity ||
        !validate_refs(&message) || message.instance.size > 63u ||
        message.addresses.size != (uint32_t)message.address_count *
            sizeof(struct dnssd_model_wire_address) ||
        !valid_wire_name(bytes + message.service_type.offset,
                         message.service_type.size) ||
        !valid_wire_name(bytes + message.domain.offset, message.domain.size) ||
        (message.target.size != 0u &&
            !valid_wire_name(bytes + message.target.offset,
                             message.target.size)) ||
        !valid_txt(message.txt.size == 0u ? NULL :
                      bytes + message.txt.offset,
                   message.txt.size))
        return DNSSD_MODEL_BAD_MESSAGE;
    for (i = 0; i < message.address_count; ++i) {
        struct dnssd_model_wire_address address;
        const uint8_t *source = bytes + message.addresses.offset +
            (size_t)i * sizeof(address);

        memcpy(&address, source, sizeof(address));
        if (!((address.family == DNSSD_MODEL_ADDR_IPV4 &&
                  address.length == 4u) ||
              (address.family == DNSSD_MODEL_ADDR_IPV6 &&
                  address.length == 16u)))
            return DNSSD_MODEL_BAD_MESSAGE;
    }
    return DNSSD_MODEL_OK;
}

int dnssd_model_copy_address(const void *buffer, size_t capacity,
                             uint32_t index,
                             struct dnssd_model_address *address)
{
    struct dnssd_model_message message;
    struct dnssd_model_wire_address source;
    const uint8_t *bytes = buffer;

    if (address == NULL || dnssd_model_validate(buffer, capacity) !=
            DNSSD_MODEL_OK)
        return DNSSD_MODEL_BAD_MESSAGE;
    memcpy(&message, buffer, sizeof(message));
    if (index >= message.address_count)
        return DNSSD_MODEL_BAD_ARGUMENT;
    memcpy(&source, bytes + message.addresses.offset +
        (size_t)index * sizeof(source), sizeof(source));
    memset(address, 0, sizeof(*address));
    address->family = source.family;
    address->length = source.length;
    address->interface_index = source.interface_index;
    memcpy(address->bytes, source.bytes, source.length);
    return DNSSD_MODEL_OK;
}
