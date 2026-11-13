/*
 * SPDX-License-Identifier: BSD-2-Clause
 * Copyright 2026 Jaime Dias
 *
 * dnssd_reg_test - a registration as mdnsd keeps it (dnssd_reg.h): what is
 * accepted and refused, the wire names it makes, and the names a conflict
 * gives it.
 */

#include "dnssd_reg.h"

#include <stdio.h>
#include <string.h>

static int failures;

#define CHECK(condition, text) do { \
    if (!(condition)) { \
        ++failures; \
        fprintf(stderr, "FAIL %s\n", text); \
    } \
} while (0)

static const uint8_t type[] = { 4, '_', 'i', 'p', 'p', 4, '_', 't', 'c', 'p', 0 };
static const uint8_t domain[] = { 5, 'l', 'o', 'c', 'a', 'l', 0 };

static struct dnssd_reg_parts base(const char *instance)
{
    struct dnssd_reg_parts p;

    memset(&p, 0, sizeof(p));
    p.instance = (const uint8_t *)instance;
    p.instance_len = (uint32_t)strnlen(instance, 256);
    p.type = type;
    p.type_len = sizeof(type);
    p.domain = domain;
    p.domain_len = sizeof(domain);
    p.port = 631;
    return p;
}

static int name_is(const uint8_t *name, size_t len, const char *expect, size_t expect_len)
{
    return len == expect_len && memcmp(name, expect, len) == 0;
}

static int instance_is(const struct dnssd_reg *r, const char *text)
{
    return r->instance_len == strnlen(text, 64) &&
           memcmp(r->instance, text, r->instance_len) == 0;
}

int main(void)
{
    struct dnssd_reg r, r2;
    struct dnssd_reg_parts p;
    uint8_t name[DNSSD_REG_NAME_MAX];
    size_t n;

    /* ---- accepted, and the names it makes ---- */
    p = base("My.Printer\\A");
    CHECK(dnssd_reg_init(&r, &p) == DNSSD_REG_OK && r.state == DNSSD_REG_PENDING,
          "an instance with a dot and a backslash");
    n = dnssd_reg_service_name(&r, name);
    CHECK(name_is(name, n, "\x0cMy.Printer\\A\x04_ipp\x04_tcp\x05local", 30),
          "its name: the label as it is, then the type and the domain");
    n = dnssd_reg_type_name(&r, name);
    CHECK(name_is(name, n, "\x04_ipp\x04_tcp\x05local", 17), "the type's name");
    CHECK(dnssd_reg_host_name(&r, name) == 0, "no host of its own: this host's");
    {
        static const uint8_t subs[] = { 8, '_', 'p', 'r', 'i', 'n', 't', 'e', 'r', 4, '_', 'f', 'a', 'x' };

        p.subtypes = subs;
        p.subtypes_len = sizeof(subs);
        CHECK(dnssd_reg_init(&r, &p) == DNSSD_REG_OK && r.subtype_count == 2, "two subtypes");
        n = dnssd_reg_subtype_name(&r, 1, name);
        CHECK(name_is(name, n, "\x04_fax\x04_sub\x04_ipp\x04_tcp\x05local", 27),
              "a subtype's name: <subtype>._sub.<type>.<domain>");
        CHECK(dnssd_reg_subtype_name(&r, 2, name) == 0, "no third subtype");
    }
    {
        static struct dnssd_model_wire_address a[2];

        memset(a, 0, sizeof(a));
        a[0].family = DNSSD_MODEL_ADDR_IPV4; a[0].length = 4;
        a[0].bytes[0] = 192; a[0].bytes[1] = 168; a[0].bytes[2] = 166; a[0].bytes[3] = 77;
        a[1].family = DNSSD_MODEL_ADDR_IPV6; a[1].length = 16; a[1].interface_index = 2;
        a[1].bytes[0] = 0xfe; a[1].bytes[1] = 0x80; a[1].bytes[15] = 0x77;
        p = base("Proxy");
        p.host = (const uint8_t *)"printer";
        p.host_len = 7;
        p.addresses = a;
        p.address_count = 2;
        CHECK(dnssd_reg_init(&r, &p) == DNSSD_REG_OK && r.address_count == 2,
              "for another host: IPv4 and link-local IPv6");
        n = dnssd_reg_host_name(&r, name);
        CHECK(name_is(name, n, "\x07printer\x05local", 15), "the host's name");

        p.address_count = 0;
        CHECK(dnssd_reg_init(&r, &p) == DNSSD_REG_BAD_ARGUMENT, "a host without its addresses");
        p.address_count = 2;
        p.host_len = 0;
        CHECK(dnssd_reg_init(&r, &p) == DNSSD_REG_BAD_ARGUMENT, "addresses without a host");
        p.host = (const uint8_t *)"a.b";
        p.host_len = 3;
        CHECK(dnssd_reg_init(&r, &p) == DNSSD_REG_BAD_NAME, "a host label with a dot");
        p.host = (const uint8_t *)"printer";
        p.host_len = 7;
        a[1].interface_index = 0;
        CHECK(dnssd_reg_init(&r, &p) == DNSSD_REG_BAD_ARGUMENT, "a link-local address without its link");
        a[1].interface_index = 2;
        a[0].length = 16;
        CHECK(dnssd_reg_init(&r, &p) == DNSSD_REG_BAD_ARGUMENT, "an IPv4 address 16 bytes long");
    }

    /* ---- refused ---- */
    p = base("");
    CHECK(dnssd_reg_init(&r, &p) == DNSSD_REG_BAD_NAME, "an empty instance");
    {
        char longname[65];

        memset(longname, 'x', 64);
        longname[64] = 0;
        p = base(longname);
        CHECK(dnssd_reg_init(&r, &p) == DNSSD_REG_BAD_NAME, "a 64-byte instance");
        longname[63] = 0;
        p = base(longname);
        CHECK(dnssd_reg_init(&r, &p) == DNSSD_REG_OK, "a 63-byte instance");
    }
    p = base("Bad\xff");
    CHECK(dnssd_reg_init(&r, &p) == DNSSD_REG_BAD_NAME, "an instance that is not UTF-8");
    p = base("Cut\xc3");
    CHECK(dnssd_reg_init(&r, &p) == DNSSD_REG_BAD_NAME, "a UTF-8 sequence cut short");
    p = base("Caf\xc3\xa9");
    CHECK(dnssd_reg_init(&r, &p) == DNSSD_REG_OK, "UTF-8 text");
    p = base("Zero");
    p.instance_len = 5;                 /* the terminating zero too */
    CHECK(dnssd_reg_init(&r, &p) == DNSSD_REG_BAD_NAME, "a zero byte in the instance");
    {
        static const uint8_t notype[] = { 3, 'i', 'p', 'p', 4, '_', 't', 'c', 'p', 0 };
        static const uint8_t sctp[] = { 4, '_', 'i', 'p', 'p', 5, '_', 's', 'c', 't', 'p', 0 };
        static const uint8_t three[] = { 1, 'a', 4, '_', 'i', 'p', 'p', 4, '_', 't', 'c', 'p', 0 };
        static const uint8_t upper[] = { 4, '_', 'i', 'p', 'p', 4, '_', 'T', 'C', 'P', 0 };

        p = base("T");
        p.type = notype; p.type_len = sizeof(notype);
        CHECK(dnssd_reg_init(&r, &p) == DNSSD_REG_BAD_NAME, "a type without its underscore");
        p.type = sctp; p.type_len = sizeof(sctp);
        CHECK(dnssd_reg_init(&r, &p) == DNSSD_REG_BAD_NAME, "a type that is not _tcp or _udp");
        p.type = three; p.type_len = sizeof(three);
        CHECK(dnssd_reg_init(&r, &p) == DNSSD_REG_BAD_NAME, "a type of three labels");
        p.type = upper; p.type_len = sizeof(upper);
        CHECK(dnssd_reg_init(&r, &p) == DNSSD_REG_OK, "_TCP in capitals");
    }
    {
        static const uint8_t dup[] = { 2, '_', 'a', 2, '_', 'A' };
        static const uint8_t empty[] = { 2, '_', 'a', 0 };

        p = base("S");
        p.subtypes = dup; p.subtypes_len = sizeof(dup);
        CHECK(dnssd_reg_init(&r, &p) == DNSSD_REG_BAD_NAME, "the same subtype twice");
        p.subtypes = empty; p.subtypes_len = sizeof(empty);
        CHECK(dnssd_reg_init(&r, &p) == DNSSD_REG_BAD_NAME, "an empty subtype label");
    }
    p = base("P");
    p.port = 0;
    CHECK(dnssd_reg_init(&r, &p) == DNSSD_REG_BAD_ARGUMENT, "port 0");
    {
        /* a domain long enough that the service's name passes 255 bytes */
        uint8_t big[DNSSD_REG_NAME_MAX];
        size_t at = 0;
        int k;

        for (k = 0; k < 3; k++) {
            big[at] = 60;
            memset(big + at + 1, 'd', 60);
            at += 61;
        }
        big[at++] = 0;
        p = base("Long");
        p.domain = big;
        p.domain_len = (uint32_t)at;
        CHECK(dnssd_reg_init(&r, &p) == DNSSD_REG_OK, "a long domain that fits");
        p = base("An instance of sixty-three bytes, which with that domain passes");
        p.domain = big;
        p.domain_len = (uint32_t)at;
        CHECK(dnssd_reg_init(&r, &p) == DNSSD_REG_TOO_LONG, "a service's name past 255 bytes");
    }

    /* ---- conflicts ---- */
    p = base("Name");
    dnssd_reg_init(&r, &p);
    dnssd_reg_rename(&r);
    CHECK(instance_is(&r, "Name (2)"), "Name -> Name (2)");
    dnssd_reg_rename(&r);
    CHECK(instance_is(&r, "Name (3)"), "Name (2) -> Name (3)");
    p = base("Name (9)");
    dnssd_reg_init(&r, &p);
    dnssd_reg_rename(&r);
    CHECK(instance_is(&r, "Name (10)"), "Name (9) -> Name (10)");
    p = base("Name (x)");
    dnssd_reg_init(&r, &p);
    dnssd_reg_rename(&r);
    CHECK(instance_is(&r, "Name (x) (2)"), "a parenthesis that is no number stays");
    {
        char longname[64];

        memset(longname, 'x', 63);
        longname[63] = 0;
        p = base(longname);
        dnssd_reg_init(&r, &p);
        dnssd_reg_rename(&r);
        CHECK(r.instance_len == 63 && memcmp(r.instance + 59, " (2)", 4) == 0,
              "a 63-byte name: cut before the suffix");
        /* the cut falls on a two-byte character: it goes whole */
        memset(longname, 'x', 63);
        longname[58] = (char)0xc3;      /* an e acute at the cut (59) */
        longname[59] = (char)0xa9;
        longname[61] = 0;
        p = base(longname);
        dnssd_reg_init(&r, &p);
        dnssd_reg_rename(&r);
        CHECK(r.instance_len == 62 && memcmp(r.instance + 58, " (2)", 4) == 0,
              "a character at the cut goes whole: UTF-8 stays");
        dnssd_reg_rename(&r);
        p.instance = r.instance;
        p.instance_len = r.instance_len;
        CHECK(dnssd_reg_init(&r2, &p) == DNSSD_REG_OK, "the renamed name is a valid instance");
    }

    /* ---- the same service ---- */
    p = base("Printer");
    dnssd_reg_init(&r, &p);
    p = base("PRINTER");
    dnssd_reg_init(&r2, &p);
    CHECK(dnssd_reg_same_service(&r, &r2), "the same service, letters in either case");
    p = base("Printer 2");
    dnssd_reg_init(&r2, &p);
    CHECK(!dnssd_reg_same_service(&r, &r2), "another instance");

    if (failures == 0)
        printf("DNS-SD registrations: 0 failures\n");
    return failures != 0;
}
