/*
 * SPDX-License-Identifier: BSD-2-Clause
 * Copyright 2026 Jaime Dias
 *
 * dnssd_reg.h - a registration, as mdnsd keeps it (D2/D4)
 *
 * What a client asked to publish, in DNS terms and nothing else: the
 * instance's own label (UTF-8, dots allowed, counted), the service type
 * and the domain as wire names, the subtypes as labels, the host a service
 * stands in for (its label and its addresses, IPv4 and IPv6 alike), the
 * port and the link. The names the records carry are built from it, as
 * wire names; a conflict renames the instance ("Name (2)") here. No client
 * library's types are in it: dnssd.library's requests fill it in
 * (dnssd_server.c).
 *
 * Plain C, as dnssd_model.c: built into mdnsd and into the host test.
 */
#ifndef MDNSD_DNSSD_REG_H
#define MDNSD_DNSSD_REG_H

#include <stddef.h>
#include <stdint.h>

#include "dnssd_model.h"

#define DNSSD_REG_LABEL_MAX     63
#define DNSSD_REG_NAME_MAX      255
#define DNSSD_REG_SUBTYPES      8
#define DNSSD_REG_ADDRESSES     8

/* Where a registration is (RFC 6762 section 8). */
enum dnssd_reg_state {
    DNSSD_REG_PENDING = 1,      /* its name is being probed */
    DNSSD_REG_ESTABLISHED = 2,  /* announced, under the name it has */
    DNSSD_REG_CONFLICT = 3,     /* the host it stands in for is another's: ended */
    DNSSD_REG_REMOVED = 4       /* withdrawn, its goodbyes sent */
};

enum dnssd_reg_result {
    DNSSD_REG_OK = 0,
    DNSSD_REG_BAD_ARGUMENT = -1,
    DNSSD_REG_BAD_NAME = -2,
    DNSSD_REG_TOO_LONG = -3     /* a name it makes would pass 255 bytes */
};

struct dnssd_reg {
    uint8_t  instance[DNSSD_REG_LABEL_MAX];     /* as announced now */
    uint8_t  instance_len;
    uint8_t  type[DNSSD_REG_NAME_MAX];          /* wire, root label included */
    uint16_t type_len;
    uint8_t  domain[DNSSD_REG_NAME_MAX];        /* wire */
    uint16_t domain_len;
    uint8_t  subtype_count;
    uint8_t  subtype_len[DNSSD_REG_SUBTYPES];
    uint8_t  subtypes[DNSSD_REG_SUBTYPES][DNSSD_REG_LABEL_MAX];
    uint8_t  host[DNSSD_REG_LABEL_MAX];         /* a host it stands in for */
    uint8_t  host_len;                          /* 0: this host */
    uint8_t  address_count;
    struct dnssd_model_wire_address addresses[DNSSD_REG_ADDRESSES];
    uint16_t port;
    uint32_t interface_index;                   /* 0: every link */
    uint8_t  state;                             /* enum dnssd_reg_state */
};

/* The parts, as a client gives them; each checked. subtypes: labels, each
 * a length byte and its bytes; addresses: address_count records. */
struct dnssd_reg_parts {
    const uint8_t *instance;    uint32_t instance_len;
    const uint8_t *type;        uint32_t type_len;
    const uint8_t *domain;      uint32_t domain_len;
    const uint8_t *subtypes;    uint32_t subtypes_len;
    const uint8_t *host;        uint32_t host_len;
    const struct dnssd_model_wire_address *addresses; uint32_t address_count;
    uint16_t port;
    uint32_t interface_index;
};

int dnssd_reg_init(struct dnssd_reg *reg, const struct dnssd_reg_parts *parts);

/* Its names as wire names in out (DNSSD_REG_NAME_MAX bytes); their
 * length, or 0 if it would pass 255 bytes. */
size_t dnssd_reg_service_name(const struct dnssd_reg *reg, uint8_t *out);
size_t dnssd_reg_type_name(const struct dnssd_reg *reg, uint8_t *out);
size_t dnssd_reg_subtype_name(const struct dnssd_reg *reg, unsigned subtype, uint8_t *out);
size_t dnssd_reg_host_name(const struct dnssd_reg *reg, uint8_t *out);

/* The next name after a conflict: "Name" becomes "Name (2)", "Name (2)"
 * "Name (3)"; what does not fit 63 bytes is cut before the suffix, at a
 * character's start (UTF-8). */
void dnssd_reg_rename(struct dnssd_reg *reg);

/* The same service: instance, type and domain, ASCII letters in either case. */
int dnssd_reg_same_service(const struct dnssd_reg *a, const struct dnssd_reg *b);

/* Wire names, equal but for ASCII case. */
int dnssd_wire_equal(const uint8_t *a, size_t a_len, const uint8_t *b, size_t b_len);
/* The length of a wire name that ends where size says, or 0. */
size_t dnssd_wire_length(const uint8_t *name, size_t size);

#endif /* MDNSD_DNSSD_REG_H */
