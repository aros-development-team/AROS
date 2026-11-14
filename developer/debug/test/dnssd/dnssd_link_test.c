/*
 * SPDX-License-Identifier: BSD-2-Clause
 * Copyright 2026 Jaime Dias
 */

#include "dnssd_link.h"

#include <stdio.h>
#include <string.h>

static int failures;

#define CHECK(condition, text) do { \
    if (!(condition)) { \
        ++failures; \
        fprintf(stderr, "FAIL %s\n", text); \
    } \
} while (0)

static void set_link(struct dnssd_link_spec *link, uint32_t index,
                     const char *name, uint32_t flags)
{
    size_t length = strnlen(name, 64);

    memset(link, 0, sizeof(*link));
    link->interface_index = index;
    link->flags = flags;
    link->name_length = (uint16_t)length;
    memcpy(link->name, name, length);
}

static void set_v4(struct dnssd_address_spec *address, uint32_t index,
                   uint8_t last, uint32_t flags)
{
    memset(address, 0, sizeof(*address));
    address->interface_index = index;
    address->flags = flags;
    address->family = DNSSD_ADDRESS_IPV4;
    address->length = 4u;
    address->bytes[0] = 192u;
    address->bytes[1] = 0u;
    address->bytes[2] = 2u;
    address->bytes[3] = last;
}

static void set_v6(struct dnssd_address_spec *address, uint32_t index,
                   uint8_t last, uint32_t flags)
{
    memset(address, 0, sizeof(*address));
    address->interface_index = index;
    address->scope_id = index;
    address->flags = flags;
    address->family = DNSSD_ADDRESS_IPV6;
    address->length = 16u;
    address->bytes[0] = 0xfeu;
    address->bytes[1] = 0x80u;
    address->bytes[15] = last;
}

int main(void)
{
    struct dnssd_link_inventory inventory, small;
    struct dnssd_link_entry links[4], link_scratch[4];
    struct dnssd_address_entry addresses[8], address_scratch[8];
    struct dnssd_link_entry small_links[1], small_link_scratch[1];
    struct dnssd_address_entry small_addresses[1];
    struct dnssd_address_entry small_address_scratch[1];
    struct dnssd_link_spec link_input[3], reordered_links[3];
    struct dnssd_address_spec address_input[6], reordered_addresses[6];
    struct dnssd_link_spec one_link[1];
    struct dnssd_address_spec one_address[1];
    const struct dnssd_link_entry *link7, *link9;
    uint32_t generation7, generation9, revision;

    CHECK(dnssd_link_inventory_init(&inventory, links, link_scratch, 4u,
              addresses, address_scratch, 8u) == DNSSD_LINK_OK,
          "initialize caller-owned active and scratch storage");

    set_link(&link_input[0], 9u, "wifi0",
             DNSSD_LINK_UP | DNSSD_LINK_MULTICAST);
    set_link(&link_input[1], 7u, "eth0",
             DNSSD_LINK_UP | DNSSD_LINK_MULTICAST);
    link_input[2] = link_input[1];
    set_v6(&address_input[0], 7u, 2u, 0u);
    set_v4(&address_input[1], 9u, 30u, 0u);
    set_v4(&address_input[2], 7u, 20u, 0u);
    set_v4(&address_input[3], 7u, 10u, 0u);
    address_input[4] = address_input[2];
    set_v6(&address_input[5], 9u, 3u, 0u);

    CHECK(dnssd_link_inventory_reconcile(&inventory, link_input, 3u,
              address_input, 6u) == DNSSD_LINK_OK,
          "reconcile unordered dual-stack link state");
    CHECK(inventory.link_count == 2u && inventory.address_count == 5u &&
              inventory.revision == 1u,
          "deduplicate inputs and publish one revision");
    CHECK(inventory.links[0].value.interface_index == 7u &&
              inventory.links[1].value.interface_index == 9u &&
              inventory.addresses[0].value.interface_index == 7u &&
              inventory.addresses[0].value.family == DNSSD_ADDRESS_IPV4 &&
              inventory.addresses[0].value.bytes[3] == 10u &&
              inventory.addresses[3].value.interface_index == 9u,
          "sort links and addresses deterministically");
    CHECK(dnssd_link_usable_address_count(&inventory, 7u) == 3u &&
              dnssd_link_usable_address_count(&inventory, 0u) == 5u,
          "make every healthy address independently usable");

    link7 = dnssd_link_inventory_find(&inventory, 7u);
    link9 = dnssd_link_inventory_find(&inventory, 9u);
    CHECK(link7 != NULL && link9 != NULL &&
              link7->generation != 0u && link9->generation != 0u &&
              link7->generation != link9->generation,
          "assign distinct stable link generations");
    generation7 = link7 != NULL ? link7->generation : 0u;
    generation9 = link9 != NULL ? link9->generation : 0u;

    reordered_links[0] = link_input[2];
    reordered_links[1] = link_input[0];
    reordered_links[2] = link_input[1];
    reordered_addresses[0] = address_input[5];
    reordered_addresses[1] = address_input[4];
    reordered_addresses[2] = address_input[3];
    reordered_addresses[3] = address_input[2];
    reordered_addresses[4] = address_input[1];
    reordered_addresses[5] = address_input[0];
    revision = inventory.revision;
    CHECK(dnssd_link_inventory_reconcile(&inventory, reordered_links, 3u,
              reordered_addresses, 6u) == DNSSD_LINK_OK &&
              inventory.revision == revision &&
              dnssd_link_inventory_find(&inventory, 7u)->generation ==
                  generation7,
          "ignore input ordering and preserve an unchanged snapshot");

    reordered_addresses[2].flags = DNSSD_ADDRESS_DEPRECATED;
    CHECK(dnssd_link_inventory_reconcile(&inventory, reordered_links, 3u,
              reordered_addresses, 6u) == DNSSD_LINK_OK &&
              inventory.revision == revision + 1u &&
              dnssd_link_usable_address_count(&inventory, 7u) == 2u &&
              dnssd_link_inventory_find(&inventory, 7u)->generation ==
                  generation7,
          "publish address-state changes without changing link identity");

    reordered_links[0].flags = DNSSD_LINK_MULTICAST;
    reordered_links[2].flags = DNSSD_LINK_MULTICAST;
    revision = inventory.revision;
    CHECK(dnssd_link_inventory_reconcile(&inventory, reordered_links, 3u,
              reordered_addresses, 6u) == DNSSD_LINK_OK &&
              inventory.revision == revision + 1u &&
              dnssd_link_usable_address_count(&inventory, 7u) == 0u &&
              dnssd_link_inventory_find(&inventory, 7u)->generation ==
                  generation7,
          "link availability gates addresses without changing identity");

    one_link[0] = link_input[1];
    one_address[0] = address_input[3];
    CHECK(dnssd_link_inventory_reconcile(&inventory, one_link, 1u,
              one_address, 1u) == DNSSD_LINK_OK &&
              inventory.link_count == 1u && inventory.address_count == 1u &&
              inventory.links[1].generation == 0u &&
              inventory.addresses[1].value.interface_index == 0u,
          "remove links and clear stale active-storage entries");
    CHECK(dnssd_link_inventory_reconcile(&inventory, link_input, 3u,
              address_input, 6u) == DNSSD_LINK_OK &&
              dnssd_link_inventory_find(&inventory, 7u)->generation ==
                  generation7 &&
              dnssd_link_inventory_find(&inventory, 9u)->generation !=
                  generation9,
          "assign a new generation after observed removal and re-addition");

    revision = inventory.revision;
    address_input[0].scope_id = 0u;
    CHECK(dnssd_link_inventory_reconcile(&inventory, link_input, 3u,
              address_input, 6u) == DNSSD_LINK_BAD_ARGUMENT &&
              inventory.revision == revision && inventory.link_count == 2u,
          "reject wrong IPv6 link-local scope atomically");
    address_input[0].scope_id = 7u;
    address_input[0].interface_index = 44u;
    CHECK(dnssd_link_inventory_reconcile(&inventory, link_input, 3u,
              address_input, 6u) == DNSSD_LINK_BAD_ARGUMENT &&
              inventory.revision == revision,
          "reject addresses for unknown links atomically");
    address_input[0].interface_index = 7u;
    address_input[4].flags = DNSSD_ADDRESS_TENTATIVE;
    CHECK(dnssd_link_inventory_reconcile(&inventory, link_input, 3u,
              address_input, 6u) == DNSSD_LINK_BAD_ARGUMENT &&
              inventory.revision == revision,
          "reject duplicate addresses with conflicting state");
    address_input[4] = address_input[2];
    link_input[0].flags |= 0x80000000u;
    CHECK(dnssd_link_inventory_reconcile(&inventory, link_input, 3u,
              address_input, 6u) == DNSSD_LINK_BAD_ARGUMENT &&
              inventory.revision == revision,
          "reject unknown link flags without publishing partial state");
    link_input[0].flags &= ~0x80000000u;

    CHECK(dnssd_link_inventory_init(&small, small_links,
              small_link_scratch, 1u, small_addresses,
              small_address_scratch, 1u) == DNSSD_LINK_OK &&
              dnssd_link_inventory_reconcile(&small, one_link, 1u,
                  one_address, 1u) == DNSSD_LINK_OK,
          "initialize a constrained inventory");
    revision = small.revision;
    CHECK(dnssd_link_inventory_reconcile(&small, link_input, 3u,
              address_input, 6u) == DNSSD_LINK_NO_SPACE &&
              small.revision == revision && small.link_count == 1u &&
              small.address_count == 1u &&
              small.links[0].value.interface_index == 7u &&
              small.last_required_links == 2u &&
              small.last_required_addresses == 5u,
          "report capacity pressure without mutating active state");

    if (failures == 0)
        printf("DNS-SD link inventory: 0 failures\n");
    return failures != 0;
}
