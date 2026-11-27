/*
 * SPDX-License-Identifier: BSD-2-Clause
 * Copyright 2026 Jaime Dias
 *
 * Private canonical link/address inventory for the native DNS-SD daemon.
 * This is engine state, not a public dnssd.library contract.
 */
#ifndef DNSSD_LINK_H
#define DNSSD_LINK_H

#include <stddef.h>
#include <stdint.h>

#define DNSSD_LINK_NAME_MAX 64u

#define DNSSD_LINK_UP        0x00000001u
#define DNSSD_LINK_MULTICAST 0x00000002u
#define DNSSD_LINK_LOOPBACK  0x00000004u

#define DNSSD_ADDRESS_TENTATIVE  0x00000001u
#define DNSSD_ADDRESS_DUPLICATED 0x00000002u
#define DNSSD_ADDRESS_DETACHED   0x00000004u
#define DNSSD_ADDRESS_DEPRECATED 0x00000008u

#define DNSSD_ADDRESS_IPV4 4u
#define DNSSD_ADDRESS_IPV6 6u

enum dnssd_link_result {
    DNSSD_LINK_OK = 0,
    DNSSD_LINK_BAD_ARGUMENT = -1,
    DNSSD_LINK_NO_SPACE = -2
};

struct dnssd_link_spec {
    uint32_t interface_index;
    uint32_t flags;
    uint16_t name_length;
    uint16_t reserved;
    char name[DNSSD_LINK_NAME_MAX];
};

struct dnssd_address_spec {
    uint32_t interface_index;
    uint32_t scope_id;
    uint32_t flags;
    uint16_t family;
    uint16_t length;
    uint8_t bytes[16];
};

struct dnssd_link_entry {
    struct dnssd_link_spec value;
    uint32_t generation;
};

struct dnssd_address_entry {
    struct dnssd_address_spec value;
};

/*
 * What a reconcile changed, per link and family, in the addresses that are
 * usable (dnssd_link_address_usable()) - one change at most for each:
 * FAMILY_UP, a family that a link now has its first usable address of;
 * FAMILY_DOWN, no longer any - followed by an ADDRESS_LOST for each address
 * it had, so that their records can be withdrawn; ADDRESSES, the family is
 * there still but its addresses changed. A responder probes on a link and
 * family that came (RFC 6762 section 8.3) and only announces again where
 * the addresses changed (section 8.4). Room for
 * dnssd_link_changes_bound() changes is enough for any reconcile; with
 * less, overflow may be set: take everything as changed.
 */
#define DNSSD_LINK_FAMILY_UP      1u
#define DNSSD_LINK_FAMILY_DOWN    2u
#define DNSSD_LINK_ADDRESSES      3u
#define DNSSD_LINK_ADDRESS_LOST   4u

struct dnssd_link_change {
    uint32_t interface_index;
    uint16_t family;            /* DNSSD_ADDRESS_IPV4 or _IPV6 */
    uint16_t kind;              /* DNSSD_LINK_* */
    uint8_t bytes[16];          /* _ADDRESS_LOST: the address, 4 bytes for
                                 * IPv4, 16 for IPv6 */
};

struct dnssd_link_changes {
    struct dnssd_link_change *items;
    size_t capacity;
    size_t count;
    int overflow;
};

struct dnssd_link_inventory {
    struct dnssd_link_entry *links;
    struct dnssd_link_entry *link_scratch;
    size_t link_capacity;
    size_t link_count;
    struct dnssd_address_entry *addresses;
    struct dnssd_address_entry *address_scratch;
    size_t address_capacity;
    size_t address_count;
    size_t last_required_links;
    size_t last_required_addresses;
    uint32_t revision;
    uint32_t next_generation;
};

/*
 * The daemon service task owns and serializes each inventory. Reconcile
 * validates and builds in scratch storage before publishing a new snapshot;
 * readers must use the same service-task serialization boundary.
 */

int dnssd_link_inventory_init(struct dnssd_link_inventory *inventory,
    struct dnssd_link_entry *links, struct dnssd_link_entry *link_scratch,
    size_t link_capacity, struct dnssd_address_entry *addresses,
    struct dnssd_address_entry *address_scratch, size_t address_capacity);

int dnssd_link_inventory_reconcile(struct dnssd_link_inventory *inventory,
    const struct dnssd_link_spec *links, size_t link_count,
    const struct dnssd_address_spec *addresses, size_t address_count);
/* The most changes the next reconcile of inventory can tell. */
size_t dnssd_link_changes_bound(const struct dnssd_link_inventory *inventory);
/* The same, telling what changed in changes (may be NULL): emptied first,
 * filled only when a new snapshot is published. */
int dnssd_link_inventory_reconcile_changes(
    struct dnssd_link_inventory *inventory,
    const struct dnssd_link_spec *links, size_t link_count,
    const struct dnssd_address_spec *addresses, size_t address_count,
    struct dnssd_link_changes *changes);

const struct dnssd_link_entry *dnssd_link_inventory_find(
    const struct dnssd_link_inventory *inventory, uint32_t interface_index);
int dnssd_link_address_usable(const struct dnssd_link_inventory *inventory,
    const struct dnssd_address_entry *address);
size_t dnssd_link_usable_address_count(
    const struct dnssd_link_inventory *inventory, uint32_t interface_index);

#endif
