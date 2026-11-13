/*
 * SPDX-License-Identifier: BSD-2-Clause
 * Copyright 2026 Jaime Dias
 *
 * dnssd_reg.c - a registration, as mdnsd keeps it (dnssd_reg.h)
 */
#include <string.h>

#include "dnssd_reg.h"

static uint8_t
lower(uint8_t c)
{
    return (c >= 'A' && c <= 'Z') ? (uint8_t)(c + 'a' - 'A') : c;
}

static int
bytes_equal_nocase(const uint8_t *a, const uint8_t *b, size_t n)
{
    size_t i;

    for (i = 0; i < n; i++)
        if (lower(a[i]) != lower(b[i]))
            return 0;
    return 1;
}

size_t
dnssd_wire_length(const uint8_t *name, size_t size)
{
    size_t at = 0;

    if (name == NULL || size == 0 || size > DNSSD_REG_NAME_MAX)
        return 0;
    while (at < size) {
        uint8_t len = name[at];

        if (len == 0)
            return at + 1 == size ? size : 0;
        if (len > DNSSD_REG_LABEL_MAX)
            return 0;
        at += 1u + len;
    }
    return 0;
}

int
dnssd_wire_equal(const uint8_t *a, size_t a_len, const uint8_t *b, size_t b_len)
{
    return a_len == b_len && bytes_equal_nocase(a, b, a_len);
}

/* Well-formed UTF-8, no zero byte (RFC 6763 section 4.1.1: an instance's
 * name is UTF-8 text). */
static int
utf8_ok(const uint8_t *s, size_t n)
{
    size_t i = 0;

    while (i < n) {
        uint8_t c = s[i];
        size_t more, k;

        if (c == 0)
            return 0;
        if (c < 0x80)
            more = 0;
        else if (c >= 0xc2 && c <= 0xdf)
            more = 1;
        else if (c >= 0xe0 && c <= 0xef)
            more = 2;
        else if (c >= 0xf0 && c <= 0xf4)
            more = 3;
        else
            return 0;
        if (more != 0 && i + more >= n)
            return 0;                   /* cut short */
        for (k = 1; k <= more; k++)
            if ((s[i + k] & 0xc0) != 0x80)
                return 0;
        i += 1 + more;
    }
    return 1;
}

/* Labels of a wire name, and its first and last label. */
static unsigned
labels(const uint8_t *name, size_t len, const uint8_t **first, const uint8_t **last)
{
    size_t at = 0;
    unsigned n = 0;

    *first = *last = NULL;
    while (at < len && name[at] != 0) {
        if (n == 0)
            *first = name + at;
        *last = name + at;
        n++;
        at += 1u + name[at];
    }
    return n;
}

int
dnssd_reg_init(struct dnssd_reg *reg, const struct dnssd_reg_parts *p)
{
    const uint8_t *first, *last;
    uint32_t at, i, j;

    if (reg == NULL || p == NULL)
        return DNSSD_REG_BAD_ARGUMENT;
    memset(reg, 0, sizeof(*reg));

    /* the instance's own label: UTF-8, dots and backslashes as they are */
    if (p->instance == NULL || p->instance_len == 0 ||
        p->instance_len > DNSSD_REG_LABEL_MAX || !utf8_ok(p->instance, p->instance_len))
        return DNSSD_REG_BAD_NAME;
    /* the type: "_name._tcp" or "_name._udp" (RFC 6763 section 7) */
    if (dnssd_wire_length(p->type, p->type_len) == 0 ||
        labels(p->type, p->type_len, &first, &last) != 2 ||
        first[0] < 2 || first[1] != '_' ||
        last[0] != 4 || !(bytes_equal_nocase(last + 1, (const uint8_t *)"_tcp", 4) ||
                          bytes_equal_nocase(last + 1, (const uint8_t *)"_udp", 4)))
        return DNSSD_REG_BAD_NAME;
    if (dnssd_wire_length(p->domain, p->domain_len) == 0)
        return DNSSD_REG_BAD_NAME;
    if (p->port == 0)
        return DNSSD_REG_BAD_ARGUMENT;

    memcpy(reg->instance, p->instance, p->instance_len);
    reg->instance_len = (uint8_t)p->instance_len;
    memcpy(reg->type, p->type, p->type_len);
    reg->type_len = (uint16_t)p->type_len;
    memcpy(reg->domain, p->domain, p->domain_len);
    reg->domain_len = (uint16_t)p->domain_len;

    /* subtypes: labels of any bytes but none empty (RFC 6763 section 7.1),
     * each once */
    for (at = 0; at < p->subtypes_len; ) {
        uint8_t len = p->subtypes[at];

        if (len == 0 || len > DNSSD_REG_LABEL_MAX || at + 1u + len > p->subtypes_len ||
            reg->subtype_count == DNSSD_REG_SUBTYPES)
            return DNSSD_REG_BAD_NAME;
        for (j = 0; j < reg->subtype_count; j++)
            if (reg->subtype_len[j] == len &&
                bytes_equal_nocase(reg->subtypes[j], p->subtypes + at + 1, len))
                return DNSSD_REG_BAD_NAME;
        memcpy(reg->subtypes[reg->subtype_count], p->subtypes + at + 1, len);
        reg->subtype_len[reg->subtype_count++] = len;
        at += 1u + len;
    }

    /* a host it stands in for: its label, with its addresses, and only so */
    if ((p->host_len != 0) != (p->address_count != 0) ||
        p->address_count > DNSSD_REG_ADDRESSES)
        return DNSSD_REG_BAD_ARGUMENT;
    if (p->host_len != 0) {
        if (p->host == NULL || p->host_len > DNSSD_REG_LABEL_MAX ||
            !utf8_ok(p->host, p->host_len) || memchr(p->host, '.', p->host_len) != NULL)
            return DNSSD_REG_BAD_NAME;
        for (i = 0; i < p->address_count; i++) {
            const struct dnssd_model_wire_address *a = &p->addresses[i];

            if (!((a->family == DNSSD_MODEL_ADDR_IPV4 && a->length == 4) ||
                  (a->family == DNSSD_MODEL_ADDR_IPV6 && a->length == 16)))
                return DNSSD_REG_BAD_ARGUMENT;
            if (a->family == DNSSD_MODEL_ADDR_IPV6 && a->bytes[0] == 0xfe &&
                (a->bytes[1] & 0xc0) == 0x80 && a->interface_index == 0)
                return DNSSD_REG_BAD_ARGUMENT;  /* fe80::/10 needs its link */
        }
        memcpy(reg->host, p->host, p->host_len);
        reg->host_len = (uint8_t)p->host_len;
        memcpy(reg->addresses, p->addresses, p->address_count * sizeof(p->addresses[0]));
        reg->address_count = (uint8_t)p->address_count;
    }
    reg->port = p->port;
    reg->interface_index = p->interface_index;
    reg->state = DNSSD_REG_PENDING;

    /* every name it will make fits */
    {
        uint8_t name[DNSSD_REG_NAME_MAX];

        if (dnssd_reg_service_name(reg, name) == 0 || dnssd_reg_type_name(reg, name) == 0 ||
            (reg->host_len != 0 && dnssd_reg_host_name(reg, name) == 0))
            return DNSSD_REG_TOO_LONG;
        for (i = 0; i < reg->subtype_count; i++)
            if (dnssd_reg_subtype_name(reg, i, name) == 0)
                return DNSSD_REG_TOO_LONG;
    }
    return DNSSD_REG_OK;
}

/* label(s) + type without its root + domain */
static size_t
compose(const struct dnssd_reg *reg, const uint8_t *prefix, size_t prefix_len,
    int with_type, uint8_t *out)
{
    size_t n = prefix_len;

    if (prefix_len + (with_type ? reg->type_len - 1u : 0) + reg->domain_len > DNSSD_REG_NAME_MAX)
        return 0;
    if (prefix_len != 0)
        memcpy(out, prefix, prefix_len);
    if (with_type) {
        memcpy(out + n, reg->type, reg->type_len - 1u);
        n += reg->type_len - 1u;
    }
    memcpy(out + n, reg->domain, reg->domain_len);
    return n + reg->domain_len;
}

size_t
dnssd_reg_service_name(const struct dnssd_reg *reg, uint8_t *out)
{
    uint8_t label[1 + DNSSD_REG_LABEL_MAX];

    label[0] = reg->instance_len;
    memcpy(label + 1, reg->instance, reg->instance_len);
    return compose(reg, label, 1u + reg->instance_len, 1, out);
}

size_t
dnssd_reg_type_name(const struct dnssd_reg *reg, uint8_t *out)
{
    return compose(reg, NULL, 0, 1, out);
}

size_t
dnssd_reg_subtype_name(const struct dnssd_reg *reg, unsigned subtype, uint8_t *out)
{
    uint8_t prefix[1 + DNSSD_REG_LABEL_MAX + 5];
    uint8_t len;

    if (subtype >= reg->subtype_count)
        return 0;
    len = reg->subtype_len[subtype];
    prefix[0] = len;
    memcpy(prefix + 1, reg->subtypes[subtype], len);
    memcpy(prefix + 1 + len, "\4_sub", 5);
    return compose(reg, prefix, 1u + len + 5u, 1, out);
}

size_t
dnssd_reg_host_name(const struct dnssd_reg *reg, uint8_t *out)
{
    uint8_t label[1 + DNSSD_REG_LABEL_MAX];

    if (reg->host_len == 0)
        return 0;
    label[0] = reg->host_len;
    memcpy(label + 1, reg->host, reg->host_len);
    return compose(reg, label, 1u + reg->host_len, 0, out);
}

void
dnssd_reg_rename(struct dnssd_reg *reg)
{
    uint8_t *name = reg->instance;
    size_t len = reg->instance_len, cut = len;
    unsigned long number = 1;
    char digits[12];
    size_t n = 0;

    /* a " (n)" it has already */
    if (len >= 4 && name[len - 1] == ')') {
        size_t i = len - 2;

        while (i > 0 && name[i] >= '0' && name[i] <= '9')
            i--;
        if (i < len - 2 && i > 0 && name[i] == '(' && name[i - 1] == ' ') {
            size_t d;

            number = 0;
            for (d = i + 1; d < len - 1; d++)
                number = number * 10 + (unsigned long)(name[d] - '0');
            cut = i - 1;
        }
    }
    number++;
    do {
        digits[n++] = (char)('0' + number % 10);
        number /= 10;
    } while (number != 0);
    /* room for " (" digits ")": cut the name before, at a character's start */
    if (cut + 3 + n > DNSSD_REG_LABEL_MAX) {
        cut = DNSSD_REG_LABEL_MAX - 3 - n;
        while (cut > 0 && (name[cut] & 0xc0) == 0x80)
            cut--;
    }
    name[cut++] = ' ';
    name[cut++] = '(';
    while (n > 0)
        name[cut++] = (uint8_t)digits[--n];
    name[cut++] = ')';
    reg->instance_len = (uint8_t)cut;
}

int
dnssd_reg_same_service(const struct dnssd_reg *a, const struct dnssd_reg *b)
{
    return a->instance_len == b->instance_len &&
           bytes_equal_nocase(a->instance, b->instance, a->instance_len) &&
           dnssd_wire_equal(a->type, a->type_len, b->type, b->type_len) &&
           dnssd_wire_equal(a->domain, a->domain_len, b->domain, b->domain_len);
}
