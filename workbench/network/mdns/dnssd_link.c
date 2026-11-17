/*
 * SPDX-License-Identifier: BSD-2-Clause
 * Copyright 2026 Jaime Dias
 */

#include "dnssd_link.h"

#include <string.h>

#define DNSSD_LINK_KNOWN_FLAGS \
    (DNSSD_LINK_UP | DNSSD_LINK_MULTICAST | DNSSD_LINK_LOOPBACK)
#define DNSSD_ADDRESS_KNOWN_FLAGS \
    (DNSSD_ADDRESS_TENTATIVE | DNSSD_ADDRESS_DUPLICATED | \
     DNSSD_ADDRESS_DETACHED | DNSSD_ADDRESS_DEPRECATED)

static uint32_t next_generation(struct dnssd_link_inventory *inventory)
{
    uint32_t generation = ++inventory->next_generation;

    if (generation == 0u)
        generation = ++inventory->next_generation;
    return generation;
}

static int same_link_key(const struct dnssd_link_spec *left,
                         const struct dnssd_link_spec *right)
{
    return left->interface_index == right->interface_index;
}

static int same_link(const struct dnssd_link_spec *left,
                     const struct dnssd_link_spec *right)
{
    return same_link_key(left, right) && left->flags == right->flags &&
        left->name_length == right->name_length &&
        memcmp(left->name, right->name, left->name_length) == 0;
}

static int same_link_identity(const struct dnssd_link_spec *left,
                              const struct dnssd_link_spec *right)
{
    return same_link_key(left, right) &&
        left->name_length == right->name_length &&
        memcmp(left->name, right->name, left->name_length) == 0;
}

static int valid_link(const struct dnssd_link_spec *link)
{
    return link != NULL && link->interface_index != 0u &&
        link->name_length != 0u &&
        link->name_length <= DNSSD_LINK_NAME_MAX && link->reserved == 0u &&
        (link->flags & ~DNSSD_LINK_KNOWN_FLAGS) == 0u &&
        memchr(link->name, '\0', link->name_length) == NULL;
}

static int address_is_unspecified_or_multicast(
    const struct dnssd_address_spec *address)
{
    uint16_t i;
    int all_zero = 1;

    for (i = 0; i < address->length; ++i) {
        if (address->bytes[i] != 0u) {
            all_zero = 0;
            break;
        }
    }
    if (all_zero)
        return 1;
    if (address->family == DNSSD_ADDRESS_IPV4)
        return address->bytes[0] >= 224u && address->bytes[0] <= 239u;
    return address->bytes[0] == 0xffu;
}

static int address_is_link_local6(const struct dnssd_address_spec *address)
{
    return address->family == DNSSD_ADDRESS_IPV6 &&
        address->bytes[0] == 0xfeu && (address->bytes[1] & 0xc0u) == 0x80u;
}

static int valid_address(const struct dnssd_address_spec *address)
{
    if (address == NULL || address->interface_index == 0u ||
        (address->flags & ~DNSSD_ADDRESS_KNOWN_FLAGS) != 0u ||
        !((address->family == DNSSD_ADDRESS_IPV4 && address->length == 4u) ||
          (address->family == DNSSD_ADDRESS_IPV6 && address->length == 16u)) ||
        address_is_unspecified_or_multicast(address))
        return 0;
    if (address->family == DNSSD_ADDRESS_IPV4 && address->scope_id != 0u)
        return 0;
    if (address_is_link_local6(address) &&
        address->scope_id != address->interface_index)
        return 0;
    return 1;
}

static int compare_links(const struct dnssd_link_spec *left,
                         const struct dnssd_link_spec *right)
{
    return left->interface_index < right->interface_index ? -1 :
        (left->interface_index > right->interface_index ? 1 : 0);
}

static int same_address_key(const struct dnssd_address_spec *left,
                            const struct dnssd_address_spec *right)
{
    return left->interface_index == right->interface_index &&
        left->scope_id == right->scope_id && left->family == right->family &&
        left->length == right->length &&
        memcmp(left->bytes, right->bytes, left->length) == 0;
}

static int same_address(const struct dnssd_address_spec *left,
                        const struct dnssd_address_spec *right)
{
    return same_address_key(left, right) && left->flags == right->flags;
}

static int compare_addresses(const struct dnssd_address_spec *left,
                             const struct dnssd_address_spec *right)
{
    int compared;

    if (left->interface_index != right->interface_index)
        return left->interface_index < right->interface_index ? -1 : 1;
    if (left->family != right->family)
        return left->family < right->family ? -1 : 1;
    if (left->scope_id != right->scope_id)
        return left->scope_id < right->scope_id ? -1 : 1;
    compared = memcmp(left->bytes, right->bytes, left->length);
    if (compared != 0)
        return compared;
    return 0;
}

static const struct dnssd_link_spec *find_link_spec(
    const struct dnssd_link_spec *links, size_t count, uint32_t index)
{
    size_t i;

    for (i = 0; i < count; ++i) {
        if (links[i].interface_index == index)
            return &links[i];
    }
    return NULL;
}

static int count_links(const struct dnssd_link_spec *links, size_t count,
                       size_t *unique)
{
    size_t i, j;

    *unique = 0;
    for (i = 0; i < count; ++i) {
        if (!valid_link(&links[i]))
            return DNSSD_LINK_BAD_ARGUMENT;
        for (j = 0; j < i; ++j) {
            if (same_link_key(&links[i], &links[j])) {
                if (!same_link(&links[i], &links[j]))
                    return DNSSD_LINK_BAD_ARGUMENT;
                break;
            }
        }
        if (j == i)
            ++*unique;
    }
    return DNSSD_LINK_OK;
}

static int count_addresses(const struct dnssd_address_spec *addresses,
                           size_t count, const struct dnssd_link_spec *links,
                           size_t link_count, size_t *unique)
{
    size_t i, j;

    *unique = 0;
    for (i = 0; i < count; ++i) {
        if (!valid_address(&addresses[i]) ||
            find_link_spec(links, link_count,
                           addresses[i].interface_index) == NULL)
            return DNSSD_LINK_BAD_ARGUMENT;
        for (j = 0; j < i; ++j) {
            if (same_address_key(&addresses[i], &addresses[j])) {
                if (!same_address(&addresses[i], &addresses[j]))
                    return DNSSD_LINK_BAD_ARGUMENT;
                break;
            }
        }
        if (j == i)
            ++*unique;
    }
    return DNSSD_LINK_OK;
}

static void build_links(struct dnssd_link_inventory *inventory,
                        const struct dnssd_link_spec *links, size_t count)
{
    size_t i, used = 0;

    memset(inventory->link_scratch, 0,
        inventory->link_capacity * sizeof(*inventory->link_scratch));
    for (i = 0; i < count; ++i) {
        struct dnssd_link_entry item;
        size_t at, old;

        for (at = 0; at < used; ++at) {
            if (same_link_key(&links[i],
                    &inventory->link_scratch[at].value))
                break;
        }
        if (at != used)
            continue;
        memset(&item, 0, sizeof(item));
        item.value = links[i];
        for (old = 0; old < inventory->link_count; ++old) {
            if (same_link_identity(&item.value,
                    &inventory->links[old].value)) {
                item.generation = inventory->links[old].generation;
                break;
            }
        }
        if (item.generation == 0u)
            item.generation = next_generation(inventory);
        for (at = 0; at < used; ++at) {
            if (compare_links(&item.value,
                    &inventory->link_scratch[at].value) < 0)
                break;
        }
        if (at < used)
            memmove(&inventory->link_scratch[at + 1u],
                    &inventory->link_scratch[at],
                    (used - at) * sizeof(*inventory->link_scratch));
        inventory->link_scratch[at] = item;
        ++used;
    }
}

static void build_addresses(struct dnssd_link_inventory *inventory,
    const struct dnssd_address_spec *addresses, size_t count)
{
    size_t i, used = 0;

    memset(inventory->address_scratch, 0,
        inventory->address_capacity * sizeof(*inventory->address_scratch));
    for (i = 0; i < count; ++i) {
        struct dnssd_address_entry item;
        size_t at;

        for (at = 0; at < used; ++at) {
            if (same_address_key(&addresses[i],
                    &inventory->address_scratch[at].value))
                break;
        }
        if (at != used)
            continue;
        memset(&item, 0, sizeof(item));
        item.value = addresses[i];
        for (at = 0; at < used; ++at) {
            if (compare_addresses(&item.value,
                    &inventory->address_scratch[at].value) < 0)
                break;
        }
        if (at < used)
            memmove(&inventory->address_scratch[at + 1u],
                    &inventory->address_scratch[at],
                    (used - at) * sizeof(*inventory->address_scratch));
        inventory->address_scratch[at] = item;
        ++used;
    }
}

static int inventory_changed(const struct dnssd_link_inventory *inventory,
                             size_t links, size_t addresses)
{
    size_t i;

    if (inventory->link_count != links ||
        inventory->address_count != addresses)
        return 1;
    for (i = 0; i < links; ++i) {
        if (!same_link(&inventory->links[i].value,
                       &inventory->link_scratch[i].value) ||
            inventory->links[i].generation !=
                inventory->link_scratch[i].generation)
            return 1;
    }
    for (i = 0; i < addresses; ++i) {
        if (!same_address(&inventory->addresses[i].value,
                          &inventory->address_scratch[i].value))
            return 1;
    }
    return 0;
}

int dnssd_link_inventory_init(struct dnssd_link_inventory *inventory,
    struct dnssd_link_entry *links, struct dnssd_link_entry *link_scratch,
    size_t link_capacity, struct dnssd_address_entry *addresses,
    struct dnssd_address_entry *address_scratch, size_t address_capacity)
{
    if (inventory == NULL || links == NULL || link_scratch == NULL ||
        link_capacity == 0u || addresses == NULL || address_scratch == NULL ||
        address_capacity == 0u)
        return DNSSD_LINK_BAD_ARGUMENT;
    memset(inventory, 0, sizeof(*inventory));
    memset(links, 0, link_capacity * sizeof(*links));
    memset(link_scratch, 0, link_capacity * sizeof(*link_scratch));
    memset(addresses, 0, address_capacity * sizeof(*addresses));
    memset(address_scratch, 0,
        address_capacity * sizeof(*address_scratch));
    inventory->links = links;
    inventory->link_scratch = link_scratch;
    inventory->link_capacity = link_capacity;
    inventory->addresses = addresses;
    inventory->address_scratch = address_scratch;
    inventory->address_capacity = address_capacity;
    return DNSSD_LINK_OK;
}

/* Is an address usable with these links' state (as
 * dnssd_link_address_usable(), on entries not yet published)? */
static int usable_among(const struct dnssd_link_entry *links, size_t link_count,
                        const struct dnssd_address_spec *address)
{
    uint32_t unusable = DNSSD_ADDRESS_TENTATIVE | DNSSD_ADDRESS_DUPLICATED |
        DNSSD_ADDRESS_DETACHED | DNSSD_ADDRESS_DEPRECATED;
    size_t i;

    if ((address->flags & unusable) != 0u)
        return 0;
    for (i = 0; i < link_count; ++i) {
        if (links[i].value.interface_index == address->interface_index)
            return (links[i].value.flags & (DNSSD_LINK_UP |
                    DNSSD_LINK_MULTICAST | DNSSD_LINK_LOOPBACK)) ==
                (DNSSD_LINK_UP | DNSSD_LINK_MULTICAST);
    }
    return 0;
}

/* The usable addresses of one link and family among entries. */
static size_t usable_count(const struct dnssd_link_entry *links,
    size_t link_count, const struct dnssd_address_entry *addresses,
    size_t address_count, uint32_t index, uint16_t family)
{
    size_t i, count = 0;

    for (i = 0; i < address_count; ++i) {
        if (addresses[i].value.interface_index == index &&
            addresses[i].value.family == family &&
            usable_among(links, link_count, &addresses[i].value))
            ++count;
    }
    return count;
}

/* Is address usable among the entries (the same address, usable there)? */
static int usable_in(const struct dnssd_link_entry *links, size_t link_count,
    const struct dnssd_address_entry *addresses, size_t address_count,
    const struct dnssd_address_spec *address)
{
    size_t i;

    for (i = 0; i < address_count; ++i) {
        if (same_address_key(&addresses[i].value, address))
            return usable_among(links, link_count, &addresses[i].value);
    }
    return 0;
}

static void add_change(struct dnssd_link_changes *changes, uint32_t index,
    uint16_t family, uint16_t kind, const struct dnssd_address_spec *address)
{
    struct dnssd_link_change *change;

    if (changes->count >= changes->capacity) {
        changes->overflow = 1;
        return;
    }
    change = &changes->items[changes->count++];
    memset(change, 0, sizeof(*change));
    change->interface_index = index;
    change->family = family;
    change->kind = kind;
    if (address != NULL)
        memcpy(change->bytes, address->bytes, address->length);
}

/* Is a usable address of one link and family among the first entries not
 * usable among the second? */
static int usable_gone(uint32_t index, uint16_t family,
    const struct dnssd_link_entry *links, size_t link_count,
    const struct dnssd_address_entry *addresses, size_t address_count,
    const struct dnssd_link_entry *other_links, size_t other_link_count,
    const struct dnssd_address_entry *others, size_t other_count)
{
    size_t i;

    for (i = 0; i < address_count; ++i) {
        const struct dnssd_address_spec *a = &addresses[i].value;

        if (a->interface_index == index && a->family == family &&
            usable_among(links, link_count, a) &&
            !usable_in(other_links, other_link_count, others, other_count, a))
            return 1;
    }
    return 0;
}

/* One link and family: what changed between the published entries (old)
 * and the scratch ones (new). */
static void link_family_changes(const struct dnssd_link_inventory *inventory,
    size_t links, size_t addresses, uint32_t index, uint16_t family,
    struct dnssd_link_changes *changes)
{
    const struct dnssd_link_entry *old_links = inventory->links;
    const struct dnssd_link_entry *new_links = inventory->link_scratch;
    const struct dnssd_address_entry *old_addresses = inventory->addresses;
    const struct dnssd_address_entry *new_addresses =
        inventory->address_scratch;
    size_t before = usable_count(old_links, inventory->link_count,
        old_addresses, inventory->address_count, index, family);
    size_t after = usable_count(new_links, links, new_addresses, addresses,
        index, family);

    if (before == 0u && after == 0u)
        return;
    if (before == 0u) {
        add_change(changes, index, family, DNSSD_LINK_FAMILY_UP, NULL);
    } else if (after == 0u) {
        size_t i;

        add_change(changes, index, family, DNSSD_LINK_FAMILY_DOWN, NULL);
        for (i = 0; i < inventory->address_count; ++i) {
            const struct dnssd_address_spec *a = &old_addresses[i].value;

            if (a->interface_index == index && a->family == family &&
                usable_among(old_links, inventory->link_count, a))
                add_change(changes, index, family, DNSSD_LINK_ADDRESS_LOST, a);
        }
    } else if (usable_gone(index, family, old_links, inventory->link_count,
                   old_addresses, inventory->address_count,
                   new_links, links, new_addresses, addresses) ||
               usable_gone(index, family, new_links, links, new_addresses,
                   addresses, old_links, inventory->link_count,
                   old_addresses, inventory->address_count)) {
        add_change(changes, index, family, DNSSD_LINK_ADDRESSES, NULL);
    }
}

/* Every link either snapshot has, each family. */
static void inventory_changes(const struct dnssd_link_inventory *inventory,
    size_t links, size_t addresses, struct dnssd_link_changes *changes)
{
    static const uint16_t families[2] = {
        DNSSD_ADDRESS_IPV4, DNSSD_ADDRESS_IPV6
    };
    size_t i, j, f;

    for (f = 0; f < 2u; ++f) {
        for (i = 0; i < inventory->link_count; ++i)
            link_family_changes(inventory, links, addresses,
                inventory->links[i].value.interface_index, families[f],
                changes);
        for (i = 0; i < links; ++i) {
            uint32_t index = inventory->link_scratch[i].value.interface_index;

            for (j = 0; j < inventory->link_count; ++j) {
                if (inventory->links[j].value.interface_index == index)
                    break;
            }
            if (j == inventory->link_count)     /* a link only new has */
                link_family_changes(inventory, links, addresses, index,
                    families[f], changes);
        }
    }
}

/* A change per link and family at most - of the links published and those
 * to come, as many as there is room for - and an ADDRESS_LOST for each
 * address published. */
size_t dnssd_link_changes_bound(const struct dnssd_link_inventory *inventory)
{
    return 2u * (inventory->link_count + inventory->link_capacity) +
        inventory->address_count;
}

int dnssd_link_inventory_reconcile(struct dnssd_link_inventory *inventory,
    const struct dnssd_link_spec *links, size_t link_count,
    const struct dnssd_address_spec *addresses, size_t address_count)
{
    return dnssd_link_inventory_reconcile_changes(inventory, links,
        link_count, addresses, address_count, NULL);
}

int dnssd_link_inventory_reconcile_changes(
    struct dnssd_link_inventory *inventory,
    const struct dnssd_link_spec *links, size_t link_count,
    const struct dnssd_address_spec *addresses, size_t address_count,
    struct dnssd_link_changes *changes)
{
    size_t unique_links, unique_addresses;
    int result;

    if (inventory == NULL || (link_count != 0u && links == NULL) ||
        (address_count != 0u && addresses == NULL))
        return DNSSD_LINK_BAD_ARGUMENT;
    if (changes != NULL) {
        changes->count = 0u;
        changes->overflow = 0;
    }
    result = count_links(links, link_count, &unique_links);
    if (result != DNSSD_LINK_OK)
        return result;
    result = count_addresses(addresses, address_count, links, link_count,
                             &unique_addresses);
    if (result != DNSSD_LINK_OK)
        return result;
    inventory->last_required_links = unique_links;
    inventory->last_required_addresses = unique_addresses;
    if (unique_links > inventory->link_capacity ||
        unique_addresses > inventory->address_capacity)
        return DNSSD_LINK_NO_SPACE;
    build_links(inventory, links, link_count);
    build_addresses(inventory, addresses, address_count);
    if (!inventory_changed(inventory, unique_links, unique_addresses))
        return DNSSD_LINK_OK;
    if (changes != NULL)
        inventory_changes(inventory, unique_links, unique_addresses, changes);
    memset(inventory->links, 0,
           inventory->link_capacity * sizeof(*inventory->links));
    memset(inventory->addresses, 0,
           inventory->address_capacity * sizeof(*inventory->addresses));
    memcpy(inventory->links, inventory->link_scratch,
           unique_links * sizeof(*inventory->links));
    memcpy(inventory->addresses, inventory->address_scratch,
           unique_addresses * sizeof(*inventory->addresses));
    inventory->link_count = unique_links;
    inventory->address_count = unique_addresses;
    if (++inventory->revision == 0u)
        ++inventory->revision;
    return DNSSD_LINK_OK;
}

const struct dnssd_link_entry *dnssd_link_inventory_find(
    const struct dnssd_link_inventory *inventory, uint32_t interface_index)
{
    size_t i;

    if (inventory == NULL)
        return NULL;
    for (i = 0; i < inventory->link_count; ++i) {
        if (inventory->links[i].value.interface_index == interface_index)
            return &inventory->links[i];
    }
    return NULL;
}

int dnssd_link_address_usable(const struct dnssd_link_inventory *inventory,
    const struct dnssd_address_entry *address)
{
    const struct dnssd_link_entry *link;
    uint32_t unusable = DNSSD_ADDRESS_TENTATIVE | DNSSD_ADDRESS_DUPLICATED |
        DNSSD_ADDRESS_DETACHED | DNSSD_ADDRESS_DEPRECATED;

    if (inventory == NULL || address == NULL)
        return 0;
    link = dnssd_link_inventory_find(inventory,
        address->value.interface_index);
    return link != NULL &&
        (link->value.flags & (DNSSD_LINK_UP | DNSSD_LINK_MULTICAST)) ==
            (DNSSD_LINK_UP | DNSSD_LINK_MULTICAST) &&
        (link->value.flags & DNSSD_LINK_LOOPBACK) == 0u &&
        (address->value.flags & unusable) == 0u;
}

size_t dnssd_link_usable_address_count(
    const struct dnssd_link_inventory *inventory, uint32_t interface_index)
{
    size_t i, count = 0;

    if (inventory == NULL)
        return 0;
    for (i = 0; i < inventory->address_count; ++i) {
        if ((interface_index == 0u ||
             inventory->addresses[i].value.interface_index ==
                 interface_index) &&
            dnssd_link_address_usable(inventory, &inventory->addresses[i]))
            ++count;
    }
    return count;
}
