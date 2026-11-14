/*
 * SPDX-License-Identifier: BSD-2-Clause
 * Copyright 2026 Jaime Dias
 */

#include "dnssd_model.h"

#include <stdio.h>
#include <string.h>

static int failures;

#define CHECK(condition, text) do { \
    if (!(condition)) { \
        ++failures; \
        fprintf(stderr, "FAIL %s\n", text); \
    } \
} while (0)

static struct dnssd_model_span span(const void *data, size_t size)
{
    struct dnssd_model_span result;

    result.data = data;
    result.size = (uint32_t)size;
    return result;
}

static void copy_header(struct dnssd_model_message *message,
                        const uint8_t *buffer)
{
    memcpy(message, buffer, sizeof(*message));
}

int main(void)
{
    static const uint8_t instance[] = {
        'S', 't', 'u', 'd', 'i', 'o', '.', '\\', 'A'
    };
    static const uint8_t type[] = {
        11, '_', 'a', 'p', 'p', 'l', 'e', '-', 'm', 'i', 'd', 'i',
        4, '_', 'u', 'd', 'p', 0
    };
    static const uint8_t domain[] = { 5, 'l', 'o', 'c', 'a', 'l', 0 };
    static const uint8_t target[] = {
        4, 'h', 'o', 's', 't', 5, 'l', 'o', 'c', 'a', 'l', 0
    };
    static const uint8_t txt[] = {
        3, 'a', '=', 'b', 5, 'x', '=', 0xff, 0, 'y'
    };
    struct dnssd_model_address addresses[4];
    struct dnssd_model_service service;
    struct dnssd_model_message message;
    struct dnssd_model_address address;
    uint8_t buffer[2048], malformed[2048];
    size_t required = 0, written = 0;

    memset(addresses, 0, sizeof(addresses));
    addresses[0].family = DNSSD_MODEL_ADDR_IPV6;
    addresses[0].length = 16;
    addresses[0].interface_index = 7;
    addresses[0].bytes[0] = 0xfe;
    addresses[0].bytes[1] = 0x80;
    addresses[0].bytes[15] = 2;
    addresses[1].family = DNSSD_MODEL_ADDR_IPV4;
    addresses[1].length = 4;
    addresses[1].interface_index = 7;
    addresses[1].bytes[0] = 192;
    addresses[1].bytes[1] = 0;
    addresses[1].bytes[2] = 2;
    addresses[1].bytes[3] = 20;
    addresses[2] = addresses[1];
    addresses[2].bytes[3] = 10;
    addresses[3] = addresses[1];

    memset(&service, 0, sizeof(service));
    service.kind = DNSSD_MODEL_EVENT_RESOLVED;
    service.incarnation = 0x11223344u;
    service.session = 3;
    service.operation = 9;
    service.sequence = 17;
    service.interface_index = 7;
    service.ttl = 120;
    service.priority = 1;
    service.weight = 2;
    service.port = 5004;
    service.instance = span(instance, sizeof(instance));
    service.service_type = span(type, sizeof(type));
    service.domain = span(domain, sizeof(domain));
    service.target = span(target, sizeof(target));
    service.txt = span(txt, sizeof(txt));
    service.addresses = addresses;
    service.address_count = 4;

    CHECK(sizeof(struct dnssd_model_message) == 116u &&
          sizeof(struct dnssd_model_wire_address) == 24u,
          "serialized layout is fixed across pointer widths");
    CHECK(dnssd_model_measure(&service, &required) == DNSSD_MODEL_OK &&
          required > sizeof(message), "measure valid canonical event");
    CHECK(dnssd_model_build(&service, buffer, sizeof(message), &written) ==
              DNSSD_MODEL_TOO_SMALL && written == required,
          "sized builder reports required storage without partial output");
    CHECK(dnssd_model_build(&service, buffer, sizeof(buffer), &written) ==
              DNSSD_MODEL_OK && written <= required,
          "build pointer-free canonical event");
    CHECK(dnssd_model_validate(buffer, written) == DNSSD_MODEL_OK,
          "validate canonical event envelope and payloads");

    copy_header(&message, buffer);
    CHECK(message.version == DNSSD_MODEL_VERSION &&
          message.header_size == 116u && message.size == written &&
          message.incarnation == service.incarnation &&
          message.session == service.session &&
          message.operation == service.operation &&
          message.sequence == service.sequence,
          "versioned envelope correlates daemon session and operation");
    CHECK(message.instance.size == sizeof(instance) &&
          memcmp(buffer + message.instance.offset, instance,
                 sizeof(instance)) == 0,
          "instance is counted data and preserves dots and backslashes");
    CHECK(message.txt.size == sizeof(txt) &&
          memcmp(buffer + message.txt.offset, txt, sizeof(txt)) == 0,
          "TXT remains counted binary data");
    CHECK(message.address_count == 3u &&
          message.addresses.size == 3u *
              sizeof(struct dnssd_model_wire_address),
          "address set is deduplicated without a legacy projection");
    CHECK(dnssd_model_copy_address(buffer, written, 0, &address) ==
              DNSSD_MODEL_OK &&
          address.family == DNSSD_MODEL_ADDR_IPV4 &&
          address.interface_index == 7u && address.bytes[3] == 10u,
          "addresses sort deterministically by interface family and bytes");
    CHECK(dnssd_model_copy_address(buffer, written, 2, &address) ==
              DNSSD_MODEL_OK &&
          address.family == DNSSD_MODEL_ADDR_IPV6 &&
          address.bytes[0] == 0xfe && address.bytes[15] == 2,
          "IPv6 address and link scope survive serialization");

    memcpy(malformed, buffer, written);
    ((struct dnssd_model_message *)(void *)malformed)->version++;
    CHECK(dnssd_model_validate(malformed, written) ==
              DNSSD_MODEL_BAD_MESSAGE,
          "reject unknown private protocol version");
    memcpy(malformed, buffer, written);
    ((struct dnssd_model_message *)(void *)malformed)->kind = 0xffffu;
    CHECK(dnssd_model_validate(malformed, written) ==
              DNSSD_MODEL_BAD_MESSAGE,
          "reject unknown event kind within a known version");
    memcpy(malformed, buffer, written);
    ((struct dnssd_model_message *)(void *)malformed)->txt.offset = 4u;
    CHECK(dnssd_model_validate(malformed, written) ==
              DNSSD_MODEL_BAD_MESSAGE,
          "reject payload reference into the header");
    memcpy(malformed, buffer, written);
    ((struct dnssd_model_message *)(void *)malformed)->domain =
        ((struct dnssd_model_message *)(void *)malformed)->service_type;
    CHECK(dnssd_model_validate(malformed, written) ==
              DNSSD_MODEL_BAD_MESSAGE,
          "reject overlapping counted payloads");
    memcpy(malformed, buffer, written);
    malformed[((struct dnssd_model_message *)(void *)malformed)->
        service_type.offset] = 64u;
    CHECK(dnssd_model_validate(malformed, written) ==
              DNSSD_MODEL_BAD_MESSAGE,
          "reject malformed DNS wire names");
    memcpy(malformed, buffer, written);
    malformed[((struct dnssd_model_message *)(void *)malformed)->txt.offset] =
        (uint8_t)((struct dnssd_model_message *)(void *)malformed)->txt.size;
    CHECK(dnssd_model_validate(malformed, written) ==
              DNSSD_MODEL_BAD_MESSAGE,
          "reject malformed binary TXT lengths");
    memcpy(malformed, buffer, written);
    ((struct dnssd_model_message *)(void *)malformed)->address_count++;
    CHECK(dnssd_model_validate(malformed, written) ==
              DNSSD_MODEL_BAD_MESSAGE,
          "reject inconsistent address count and extent");
    CHECK(dnssd_model_copy_address(buffer, written, 3, &address) ==
              DNSSD_MODEL_BAD_ARGUMENT,
          "reject address access beyond the counted set");

    service.service_type = span(type, sizeof(type) - 1u);
    CHECK(dnssd_model_measure(&service, &required) ==
              DNSSD_MODEL_BAD_ARGUMENT,
          "reject unterminated DNS wire names before serialization");
    service.service_type = span(type, sizeof(type));
    service.txt = span(txt, sizeof(txt) - 1u);
    CHECK(dnssd_model_measure(&service, &required) ==
              DNSSD_MODEL_BAD_ARGUMENT,
          "reject malformed TXT before serialization");

    /* version 2: what each kind must carry */
    {
        static const uint8_t host[] = { 7, 'p', 'a', 'l', 'e', 'r', 'm', 'a',
                                        5, 'l', 'o', 'c', 'a', 'l', 0 };
        static const uint8_t a4[] = { 192, 168, 166, 1 };
        struct dnssd_model_service other;

        memset(&other, 0, sizeof(other));
        other.kind = DNSSD_MODEL_EVENT_ADDRESS;
        CHECK(dnssd_model_measure(&other, &required) == DNSSD_MODEL_BAD_ARGUMENT,
              "an ADDRESS event needs a name");
        other.name = span(host, sizeof(host));
        other.addresses = addresses;
        other.address_count = 1;
        CHECK(dnssd_model_build(&other, buffer, sizeof(buffer), &written) ==
                  DNSSD_MODEL_OK &&
              dnssd_model_validate(buffer, written) == DNSSD_MODEL_OK,
              "an ADDRESS event: a name and its addresses, no service");
        other.kind = DNSSD_MODEL_EVENT_RECORD;
        other.address_count = 0;
        other.addresses = NULL;
        other.rdata = span(a4, sizeof(a4));
        CHECK(dnssd_model_measure(&other, &required) == DNSSD_MODEL_BAD_ARGUMENT,
              "a RECORD event needs a record type");
        other.rrtype = 1;
        other.flags = DNSSD_MODEL_FLAG_REMOVED;
        CHECK(dnssd_model_build(&other, buffer, sizeof(buffer), &written) ==
                  DNSSD_MODEL_OK &&
              dnssd_model_validate(buffer, written) == DNSSD_MODEL_OK,
              "a RECORD event, removed: name, type and data");
        copy_header(&message, buffer);
        CHECK(message.rrtype == 1u && message.rdata.size == sizeof(a4) &&
              memcmp(buffer + message.rdata.offset, a4, sizeof(a4)) == 0,
              "the record's type and data are carried");
        memset(&other, 0, sizeof(other));
        other.kind = DNSSD_MODEL_EVENT_END;
        other.result = -8;
        CHECK(dnssd_model_build(&other, buffer, sizeof(buffer), &written) ==
                  DNSSD_MODEL_OK && written == sizeof(message),
              "an END event is the header and its result");
        copy_header(&message, buffer);
        CHECK(message.result == -8, "the result is carried");
        other.kind = DNSSD_MODEL_EVENT_TYPE;
        other.service_type = span(type, sizeof(type));
        other.domain = span(domain, sizeof(domain));
        other.instance = span(instance, sizeof(instance));
        CHECK(dnssd_model_measure(&other, &required) == DNSSD_MODEL_BAD_ARGUMENT,
              "a TYPE event names no instance");
        other.instance = span(NULL, 0);
        CHECK(dnssd_model_measure(&other, &required) == DNSSD_MODEL_OK,
              "a TYPE event: a type and its domain");
        other.kind = DNSSD_MODEL_EVENT_ADD;
        CHECK(dnssd_model_measure(&other, &required) == DNSSD_MODEL_BAD_ARGUMENT,
              "an instance's event needs the instance");
    }

    if (failures == 0)
        printf("DNS-SD canonical model: 0 failures\n");
    return failures != 0;
}
