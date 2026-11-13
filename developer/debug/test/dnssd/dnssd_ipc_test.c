/*
 * SPDX-License-Identifier: BSD-2-Clause
 * Copyright 2026 Jaime Dias
 *
 * dnssd_ipc_test - dnssd.library's requests to mdnsd (dnssd_ipc.h): every
 * command built and accepted; every way a request can be malformed
 * refused - the header, references outside the request, over one another
 * or out of alignment, what a command needs and must not have, names,
 * labels, TXT data and addresses - and every byte of a valid request
 * changed in turn, which may be refused but must be read only inside the
 * request (run it under AddressSanitizer on the host).
 */

#include "dnssd_ipc.h"
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

static const uint8_t name[] = { 7, 'p', 'a', 'l', 'e', 'r', 'm', 'a', 5, 'l', 'o', 'c', 'a', 'l', 0 };
static const uint8_t type[] = { 5, '_', 'h', 't', 't', 'p', 4, '_', 't', 'c', 'p', 0 };
static const uint8_t domain[] = { 5, 'l', 'o', 'c', 'a', 'l', 0 };
static const uint8_t instance[] = { 'M', 'y', '.', 'P', 'r', 'i', 'n', 't', 'e', 'r' };
static const uint8_t subtypes[] = { 8, '_', 'p', 'r', 'i', 'n', 't', 'e', 'r', 4, '_', 'f', 'a', 'x' };
static const uint8_t txt[] = { 3, 'a', '=', 'b', 0, 4, 'x', '=', 0xff, 0 };
static const uint8_t host[] = { 'p', 'r', 'o', 'x', 'y' };

/* An 8-byte-aligned buffer, larger than any request here. */
static struct { uint8_t b[4096]; } __attribute__((aligned(8))) buf, bad;

static struct dnssd_model_wire_address addrs[3];

static struct dnssd_ipc_request *hdr(void) { return (struct dnssd_ipc_request *)bad.b; }

/* Build a valid request of command with parts into buf; its ids set. */
static size_t build(uint16_t command, const struct dnssd_ipc_parts *p)
{
    size_t written = 0;
    struct dnssd_ipc_request *r = (struct dnssd_ipc_request *)buf.b;

    if (dnssd_ipc_build(command, p, buf.b, sizeof(buf.b), &written) != DNSSD_IPC_OK)
        return 0;
    r->session = 0x00010001u;
    r->operation = 0x00020003u;
    r->port = 631;
    r->task = 42;
    return written;
}

/* A copy of the last request built, to break. */
static void copy(size_t n) { memcpy(bad.b, buf.b, n); }

static int valid(size_t n) { return dnssd_ipc_validate(bad.b, n) == DNSSD_IPC_OK; }

static void parts_register(struct dnssd_ipc_parts *p, int for_host)
{
    memset(p, 0, sizeof(*p));
    p->instance = instance; p->instance_size = sizeof(instance);
    p->service_type = type; p->service_type_size = sizeof(type);
    p->domain = domain; p->domain_size = sizeof(domain);
    p->subtypes = subtypes; p->subtypes_size = sizeof(subtypes);
    p->txt = txt; p->txt_size = sizeof(txt);
    if (for_host) {
        p->host = host; p->host_size = sizeof(host);
        p->addresses = addrs; p->address_count = 3;
    }
}

int main(void)
{
    struct dnssd_ipc_parts p;
    size_t n, need;
    uint32_t i;

    memset(addrs, 0, sizeof(addrs));
    addrs[0].family = DNSSD_MODEL_ADDR_IPV4; addrs[0].length = 4;
    addrs[0].bytes[0] = 192; addrs[0].bytes[1] = 168; addrs[0].bytes[2] = 166; addrs[0].bytes[3] = 77;
    addrs[1].family = DNSSD_MODEL_ADDR_IPV6; addrs[1].length = 16; addrs[1].interface_index = 2;
    addrs[1].bytes[0] = 0xfe; addrs[1].bytes[1] = 0x80; addrs[1].bytes[15] = 0x77;
    addrs[2].family = DNSSD_MODEL_ADDR_IPV6; addrs[2].length = 16;
    addrs[2].bytes[0] = 0x20; addrs[2].bytes[1] = 0x01; addrs[2].bytes[2] = 0x0d; addrs[2].bytes[3] = 0xb8;
    addrs[2].bytes[15] = 0x77;

    /* the same layout on every host: fixed-width fields, natural alignment */
    CHECK(sizeof(struct dnssd_ipc_request) == 132u &&
          offsetof(struct dnssd_ipc_request, name) == 60u, "the header's layout");

    /* ---- every command, built and accepted ---- */
    memset(&p, 0, sizeof(p));
    n = build(DNSSD_IPC_OPEN, &p);
    copy(n);
    CHECK(n == sizeof(struct dnssd_ipc_request) && valid(n), "OPEN: the header alone");
    n = build(DNSSD_IPC_STATUS, &p);
    copy(n);
    CHECK(valid(n), "STATUS");
    n = build(DNSSD_IPC_CLOSE, &p);
    copy(n);
    CHECK(valid(n), "CLOSE");
    n = build(DNSSD_IPC_CANCEL, &p);
    copy(n);
    CHECK(valid(n), "CANCEL");

    p.reply_room = 1024;
    n = build(DNSSD_IPC_READ, &p);
    copy(n);
    CHECK(valid(n) && hdr()->size == sizeof(struct dnssd_ipc_request) &&
          hdr()->capacity == n && hdr()->reply.size == 1024,
          "READ: the reply's room is in the capacity, not in what is sent");
    p.reply_room = 0;
    n = build(DNSSD_IPC_READ, &p);
    copy(n);
    CHECK(!valid(n), "READ without room for the reply");

    memset(&p, 0, sizeof(p));
    p.name = name; p.name_size = sizeof(name);
    n = build(DNSSD_IPC_LOOKUP, &p);
    copy(n);
    CHECK(valid(n) && memcmp(dnssd_ipc_data(bad.b, &hdr()->name), name, sizeof(name)) == 0,
          "LOOKUP: the name where its reference says");
    n = build(DNSSD_IPC_QUERY, &p);
    copy(n);
    CHECK(valid(n), "QUERY");

    memset(&p, 0, sizeof(p));
    p.service_type = type; p.service_type_size = sizeof(type);
    p.domain = domain; p.domain_size = sizeof(domain);
    n = build(DNSSD_IPC_BROWSE, &p);
    copy(n);
    CHECK(valid(n), "BROWSE");
    p.subtypes = subtypes; p.subtypes_size = 9;
    n = build(DNSSD_IPC_BROWSE, &p);
    copy(n);
    CHECK(valid(n), "BROWSE a subtype");
    p.subtypes_size = sizeof(subtypes);
    n = build(DNSSD_IPC_BROWSE, &p);
    copy(n);
    CHECK(!valid(n), "BROWSE two subtypes");

    memset(&p, 0, sizeof(p));
    p.instance = instance; p.instance_size = sizeof(instance);
    p.service_type = type; p.service_type_size = sizeof(type);
    p.domain = domain; p.domain_size = sizeof(domain);
    n = build(DNSSD_IPC_RESOLVE, &p);
    copy(n);
    CHECK(valid(n), "RESOLVE: an instance with a dot");

    parts_register(&p, 0);
    n = build(DNSSD_IPC_REGISTER, &p);
    copy(n);
    CHECK(valid(n), "REGISTER: subtypes, TXT with an empty string and a 0xff");
    parts_register(&p, 1);
    n = build(DNSSD_IPC_REGISTER, &p);
    copy(n);
    CHECK(valid(n) && hdr()->addresses.size == 3 * sizeof(struct dnssd_model_wire_address),
          "REGISTER for another host: IPv4, link-local and global IPv6");

    memset(&p, 0, sizeof(p));
    p.txt = txt; p.txt_size = sizeof(txt);
    n = build(DNSSD_IPC_UPDATE, &p);
    copy(n);
    CHECK(valid(n), "UPDATE the TXT");
    memset(&p, 0, sizeof(p));
    n = build(DNSSD_IPC_UPDATE, &p);
    copy(n);
    CHECK(valid(n), "UPDATE the port alone");
    hdr()->port = 0;
    CHECK(!valid(n), "UPDATE nothing");

    /* building: what does not fit, what cannot be */
    parts_register(&p, 1);
    CHECK(dnssd_ipc_measure(&p, &need) == DNSSD_IPC_OK &&
          dnssd_ipc_build(DNSSD_IPC_REGISTER, &p, buf.b, need - 1, &n) == DNSSD_IPC_TOO_SMALL,
          "a buffer one byte short");
    CHECK(dnssd_ipc_build(DNSSD_IPC_REGISTER, &p, buf.b + 2, sizeof(buf.b) - 2, &n) ==
              DNSSD_IPC_BAD_ARGUMENT, "an unaligned buffer");
    p.address_count = DNSSD_IPC_MAX_ADDRESSES + 1;
    CHECK(dnssd_ipc_measure(&p, &need) == DNSSD_IPC_BAD_ARGUMENT, "nine addresses");
    parts_register(&p, 0);
    p.txt = NULL;
    CHECK(dnssd_ipc_measure(&p, &need) == DNSSD_IPC_BAD_ARGUMENT, "a size with no data");
    p.txt = txt;
    p.txt_size = DNSSD_IPC_MAX_SIZE;
    CHECK(dnssd_ipc_measure(&p, &need) == DNSSD_IPC_BAD_ARGUMENT, "larger than a request");

    /* ---- the header ---- */
    parts_register(&p, 1);
    n = build(DNSSD_IPC_REGISTER, &p);
    copy(n);
    CHECK(!valid(sizeof(struct dnssd_ipc_request) - 1), "shorter than the header");
    CHECK(dnssd_ipc_validate(bad.b + 4, n - 4) != DNSSD_IPC_OK, "an unaligned request");
    copy(n); hdr()->version = 1;
    CHECK(!valid(n), "version 1");
    copy(n); hdr()->header_size += 4;
    CHECK(!valid(n), "another header size");
    copy(n); hdr()->command = 0;
    CHECK(!valid(n), "command 0");
    copy(n); hdr()->command = DNSSD_IPC_COMMANDS + 1;
    CHECK(!valid(n), "an unknown command");
    copy(n); hdr()->flags = 1;
    CHECK(!valid(n), "a flag not defined");
    copy(n);
    CHECK(!valid(n - 4), "a capacity larger than what arrived");
    copy(n); hdr()->size = hdr()->capacity + 4;
    CHECK(!valid(n), "a size larger than the capacity");
    copy(n); hdr()->size = sizeof(struct dnssd_ipc_request) - 1;
    CHECK(!valid(n), "a size smaller than the header");
    copy(n); hdr()->session = 0;
    CHECK(!valid(n), "no session");

    /* ---- the references ---- */
    copy(n); hdr()->txt.offset = 8;
    CHECK(!valid(n), "a reference into the header");
    copy(n); hdr()->txt.offset = hdr()->size;
    CHECK(!valid(n), "a reference past the request");
    copy(n); hdr()->txt.size = 0xffffffffu;
    CHECK(!valid(n), "a size that wraps around");
    copy(n); hdr()->txt.offset = hdr()->subtypes.offset;
    hdr()->txt.size = hdr()->subtypes.size;
    CHECK(!valid(n), "two references to the same bytes");
    copy(n); hdr()->domain.offset = hdr()->service_type.offset + 4;
    CHECK(!valid(n), "references that overlap");
    copy(n); hdr()->addresses.offset += 2;
    CHECK(!valid(n), "an unaligned reference");
    copy(n); hdr()->txt.size = 0;
    CHECK(!valid(n), "an empty reference with an offset");
    {
        /* the reply's room may only be used by READ, and lies past size */
        memset(&p, 0, sizeof(p));
        p.name = name; p.name_size = sizeof(name);
        p.reply_room = 512;
        n = build(DNSSD_IPC_LOOKUP, &p);
        copy(n);
        CHECK(!valid(n), "a LOOKUP with room for a reply");
    }

    /* ---- what a command needs and must not have ---- */
    memset(&p, 0, sizeof(p));
    n = build(DNSSD_IPC_LOOKUP, &p);
    copy(n);
    CHECK(!valid(n), "LOOKUP without a name");
    p.name = name; p.name_size = sizeof(name);
    p.txt = txt; p.txt_size = sizeof(txt);
    n = build(DNSSD_IPC_LOOKUP, &p);
    copy(n);
    CHECK(!valid(n), "LOOKUP with TXT data");
    n = build(DNSSD_IPC_CLOSE, &p);
    copy(n);
    CHECK(!valid(n), "CLOSE with a name");
    memset(&p, 0, sizeof(p));
    n = build(DNSSD_IPC_CANCEL, &p);
    copy(n); hdr()->operation = 0;
    CHECK(!valid(n), "CANCEL without an operation");
    p.service_type = type; p.service_type_size = sizeof(type);
    p.domain = domain; p.domain_size = sizeof(domain);
    n = build(DNSSD_IPC_RESOLVE, &p);
    copy(n);
    CHECK(!valid(n), "RESOLVE without an instance");
    parts_register(&p, 0);
    n = build(DNSSD_IPC_REGISTER, &p);
    copy(n); hdr()->port = 0;
    CHECK(!valid(n), "REGISTER without a port");
    p.host = host; p.host_size = sizeof(host);
    n = build(DNSSD_IPC_REGISTER, &p);
    copy(n);
    CHECK(!valid(n), "a host without its addresses");
    p.host = NULL; p.host_size = 0;
    p.addresses = addrs; p.address_count = 1;
    n = build(DNSSD_IPC_REGISTER, &p);
    copy(n);
    CHECK(!valid(n), "addresses without a host");

    /* ---- names, labels, TXT, instance, host ---- */
    {
        static const uint8_t no_root[] = { 5, 'l', 'o', 'c', 'a', 'l' };
        static const uint8_t past_root[] = { 5, 'l', 'o', 'c', 'a', 'l', 0, 0, 0, 0 };
        static const uint8_t pointer[] = { 0xc0, 12, 0, 0 };
        uint8_t long_label[66], long_name[260], long_instance[64];

        CHECK(dnssd_ipc_wire_name_ok(name, sizeof(name)), "a name");
        CHECK(!dnssd_ipc_wire_name_ok(no_root, sizeof(no_root)), "a name without its root label");
        CHECK(!dnssd_ipc_wire_name_ok(past_root, sizeof(past_root)), "bytes after the root label");
        CHECK(!dnssd_ipc_wire_name_ok(pointer, sizeof(pointer)), "a compression pointer");
        memset(long_label, 'a', sizeof(long_label));
        long_label[0] = 64; long_label[65] = 0;
        CHECK(!dnssd_ipc_wire_name_ok(long_label, sizeof(long_label)), "a 64-byte label");
        for (i = 0; i + 64 < sizeof(long_name); i += 64) {
            memset(long_name + i, 'b', 64);
            long_name[i] = 63;
        }
        long_name[i] = 0;
        CHECK(!dnssd_ipc_wire_name_ok(long_name, i + 1), "a name longer than 255 bytes");
        CHECK(dnssd_ipc_txt_ok(txt, sizeof(txt)), "TXT data");
        CHECK(!dnssd_ipc_txt_ok(txt, sizeof(txt) - 1), "a TXT string past the end");
        CHECK(!dnssd_ipc_labels_ok(domain, sizeof(domain), 8), "a zero-length label");

        memset(&p, 0, sizeof(p));
        p.name = no_root; p.name_size = sizeof(no_root);
        n = build(DNSSD_IPC_LOOKUP, &p);
        copy(n);
        CHECK(!valid(n), "LOOKUP of a name without its root label");

        memset(long_instance, 'i', sizeof(long_instance));
        memset(&p, 0, sizeof(p));
        p.instance = long_instance; p.instance_size = sizeof(long_instance);
        p.service_type = type; p.service_type_size = sizeof(type);
        p.domain = domain; p.domain_size = sizeof(domain);
        n = build(DNSSD_IPC_RESOLVE, &p);
        copy(n);
        CHECK(!valid(n), "a 64-byte instance");
        long_instance[3] = 0;
        p.instance_size = 10;
        n = build(DNSSD_IPC_RESOLVE, &p);
        copy(n);
        CHECK(!valid(n), "an instance with a zero byte");
        parts_register(&p, 1);
        p.host = (const uint8_t *)"a.b"; p.host_size = 3;
        n = build(DNSSD_IPC_REGISTER, &p);
        copy(n);
        CHECK(!valid(n), "a host label with a dot");
    }

    /* ---- addresses ---- */
    {
        struct dnssd_model_wire_address a[2];

        parts_register(&p, 1);
        p.addresses = a; p.address_count = 1;
        memcpy(a, addrs, sizeof(a[0]));
        a[0].family = 5;
        n = build(DNSSD_IPC_REGISTER, &p);
        copy(n);
        CHECK(!valid(n), "an address of no family");
        memcpy(a, addrs, sizeof(a[0]));
        a[0].length = 16;
        n = build(DNSSD_IPC_REGISTER, &p);
        copy(n);
        CHECK(!valid(n), "an IPv4 address 16 bytes long");
        memcpy(a, addrs + 1, sizeof(a[0]));
        a[0].interface_index = 0;
        n = build(DNSSD_IPC_REGISTER, &p);
        copy(n);
        CHECK(!valid(n), "a link-local address without its link");
        memcpy(a, addrs, sizeof(a[0]));
        a[0].bytes[9] = 1;
        n = build(DNSSD_IPC_REGISTER, &p);
        copy(n);
        CHECK(!valid(n), "bytes past an IPv4 address");
        memcpy(a, addrs, sizeof(a[0]));
        n = build(DNSSD_IPC_REGISTER, &p);
        copy(n);
        hdr()->addresses.size -= 4;
        CHECK(!valid(n), "a part of an address record");
    }

    /* ---- every byte of a valid request changed: read inside, or refused ---- */
    {
        uint32_t accepted = 0, refused = 0, k;
        static const uint8_t values[] = { 0x00, 0x01, 0x3f, 0x40, 0x7f, 0x80, 0xc0, 0xff };

        parts_register(&p, 1);
        n = build(DNSSD_IPC_REGISTER, &p);
        for (i = 0; i < n; i++)
            for (k = 0; k < sizeof(values); k++) {
                copy(n);
                bad.b[i] = values[k];
                if (valid(n))
                    accepted++;
                else
                    refused++;
            }
        printf("every byte of a REGISTER (%lu bytes) changed: %lu accepted, %lu refused\n",
               (unsigned long)n, (unsigned long)accepted, (unsigned long)refused);
        CHECK(refused > 0 && accepted > 0, "changes are told apart");
    }

    if (failures == 0)
        printf("DNS-SD requests: 0 failures\n");
    return failures != 0;
}
