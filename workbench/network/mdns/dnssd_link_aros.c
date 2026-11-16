/*
 * SPDX-License-Identifier: BSD-2-Clause
 * Copyright 2026 Jaime Dias
 */

#include "dnssd_link_aros.h"

#include <exec/memory.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <net/if.h>
#include <netinet/in.h>
#include <netinet/in_var.h>
#include <proto/bsdsocket.h>
#include <proto/exec.h>
#include <sys/ioctl.h>
#include <sys/sockio.h>

#include <limits.h>
#include <string.h>

#define SNAPSHOT_INITIAL_BYTES 2048u
#define IFCONF_HEADROOM (IFNAMSIZ + 256u + sizeof(unsigned long))

struct raw_snapshot {
    void *ipv4;
    size_t ipv4_size;
    size_t ipv4_used;
    struct in6_ifreq *ipv6;
    size_t ipv6_size;
    size_t ipv6_used;
};

static void free_snapshot(struct raw_snapshot *snapshot)
{
    if (snapshot->ipv4 != NULL)
        FreeVec(snapshot->ipv4);
    if (snapshot->ipv6 != NULL)
        FreeVec(snapshot->ipv6);
    memset(snapshot, 0, sizeof(*snapshot));
}

static int grow_size(size_t current, size_t limit, size_t *next)
{
    if (current >= limit)
        return DNSSD_LINK_AROS_SNAPSHOT_LIMIT;
    if (current > limit / 2u)
        *next = limit;
    else
        *next = current * 2u;
    return DNSSD_LINK_OK;
}

static int snapshot_ipv4(struct Library *SocketBase, int sock,
                         size_t limit, struct raw_snapshot *snapshot)
{
    size_t size = SNAPSHOT_INITIAL_BYTES;

    if (size > limit)
        size = limit;
    for (;;) {
        struct ifconf ifc;
        void *buffer;
        size_t next;

        buffer = AllocVec(size, MEMF_ANY | MEMF_CLEAR);
        if (buffer == NULL)
            return DNSSD_LINK_AROS_NO_MEMORY;
        memset(&ifc, 0, sizeof(ifc));
        ifc.ifc_buf = buffer;
        ifc.ifc_len = (int)size;
        if (IoctlSocket(sock, SIOCGIFCONF, (char *)&ifc) != 0 ||
            ifc.ifc_len < 0 || (size_t)ifc.ifc_len > size) {
            FreeVec(buffer);
            return DNSSD_LINK_AROS_STACK_ERROR;
        }
        if ((size_t)ifc.ifc_len + IFCONF_HEADROOM <= size) {
            snapshot->ipv4 = buffer;
            snapshot->ipv4_size = size;
            snapshot->ipv4_used = (size_t)ifc.ifc_len;
            return DNSSD_LINK_OK;
        }
        FreeVec(buffer);
        if (grow_size(size, limit, &next) != DNSSD_LINK_OK)
            return DNSSD_LINK_AROS_SNAPSHOT_LIMIT;
        size = next;
    }
}

static int snapshot_ipv6(struct Library *SocketBase, int sock,
                         size_t limit, struct raw_snapshot *snapshot)
{
    size_t size = SNAPSHOT_INITIAL_BYTES;

    if (size > limit)
        size = limit;
    size -= size % sizeof(struct in6_ifreq);
    if (size < sizeof(struct in6_ifreq))
        return DNSSD_LINK_AROS_SNAPSHOT_LIMIT;
    for (;;) {
        struct in6_ifconf ifc;
        struct in6_ifreq *buffer;
        size_t next;

        buffer = AllocVec(size, MEMF_ANY | MEMF_CLEAR);
        if (buffer == NULL)
            return DNSSD_LINK_AROS_NO_MEMORY;
        memset(&ifc, 0, sizeof(ifc));
        ifc.ifc6_req = buffer;
        ifc.ifc6_len = (int)size;
        if (IoctlSocket(sock, SIOCGLIFADDR, (char *)&ifc) != 0 ||
            ifc.ifc6_len < 0 || (size_t)ifc.ifc6_len > size ||
            (size_t)ifc.ifc6_len % sizeof(*buffer) != 0u) {
            FreeVec(buffer);
            return DNSSD_LINK_AROS_STACK_ERROR;
        }
        if ((size_t)ifc.ifc6_len + sizeof(*buffer) <= size) {
            snapshot->ipv6 = buffer;
            snapshot->ipv6_size = size;
            snapshot->ipv6_used = (size_t)ifc.ifc6_len;
            return DNSSD_LINK_OK;
        }
        FreeVec(buffer);
        if (grow_size(size, limit, &next) != DNSSD_LINK_OK)
            return DNSSD_LINK_AROS_SNAPSHOT_LIMIT;
        next -= next % sizeof(struct in6_ifreq);
        if (next <= size)
            return DNSSD_LINK_AROS_SNAPSHOT_LIMIT;
        size = next;
    }
}

static int next_ipv4(const struct raw_snapshot *snapshot, size_t *offset,
                     const struct ifreq **request)
{
    const struct ifreq *item;
    const struct sockaddr *address;
    size_t step;

    if (*offset == snapshot->ipv4_used)
        return 0;
    if (*offset > snapshot->ipv4_used ||
        snapshot->ipv4_used - *offset < IFNAMSIZ + sizeof(*address))
        return -1;
    item = (const struct ifreq *)((const char *)snapshot->ipv4 + *offset);
    address = &item->ifr_addr;
    step = address->sa_len < sizeof(*address) ?
        IFNAMSIZ + sizeof(*address) : IFNAMSIZ + (size_t)address->sa_len;
    if (step == 0u || step > snapshot->ipv4_used - *offset)
        return -1;
    *request = item;
    *offset += step;
    return 1;
}

static uint32_t canonical_link_flags(short flags)
{
    uint32_t result = 0u;

    if ((flags & IFF_UP) != 0)
        result |= DNSSD_LINK_UP;
    if ((flags & IFF_MULTICAST) != 0)
        result |= DNSSD_LINK_MULTICAST;
    if ((flags & IFF_LOOPBACK) != 0)
        result |= DNSSD_LINK_LOOPBACK;
    return result;
}

static uint32_t canonical_address_flags(int flags)
{
    uint32_t result = 0u;

    if ((flags & IN6_IFF_TENTATIVE) != 0)
        result |= DNSSD_ADDRESS_TENTATIVE;
    if ((flags & IN6_IFF_DUPLICATED) != 0)
        result |= DNSSD_ADDRESS_DUPLICATED;
    if ((flags & IN6_IFF_DETACHED) != 0)
        result |= DNSSD_ADDRESS_DETACHED;
    if ((flags & IN6_IFF_DEPRECATED) != 0)
        result |= DNSSD_ADDRESS_DEPRECATED;
    return result;
}

static int find_link_by_name(const struct dnssd_link_spec *links,
                             size_t count, const char *name, size_t length)
{
    size_t i;

    for (i = 0; i < count; ++i) {
        if (links[i].name_length == length &&
            memcmp(links[i].name, name, length) == 0)
            return (int)i;
    }
    return -1;
}

static int add_link(struct Library *SocketBase, int sock,
                    struct dnssd_link_spec *links, size_t capacity,
                    size_t *count, const char *name)
{
    struct ifreq request;
    size_t length = 0u;
    int found;

    while (length < IFNAMSIZ && name[length] != '\0')
        ++length;
    if (length == 0u || length > DNSSD_LINK_NAME_MAX)
        return DNSSD_LINK_BAD_ARGUMENT;
    found = find_link_by_name(links, *count, name, length);
    if (found >= 0)
        return found;
    if (*count >= capacity)
        return DNSSD_LINK_NO_SPACE;

    memset(&request, 0, sizeof(request));
    memcpy(request.ifr_name, name, length);
    if (IoctlSocket(sock, SIOCGIFFLAGS, (char *)&request) != 0)
        return DNSSD_LINK_AROS_STACK_ERROR;
    memset(&links[*count], 0, sizeof(links[*count]));
    links[*count].flags = canonical_link_flags(request.ifr_flags);
    memset(&request, 0, sizeof(request));
    memcpy(request.ifr_name, name, length);
    if (IoctlSocket(sock, SIOCGIFINDEX, (char *)&request) != 0 ||
        request.ifr_index <= 0)
        return DNSSD_LINK_AROS_STACK_ERROR;
    links[*count].interface_index = (uint32_t)request.ifr_index;
    links[*count].name_length = (uint16_t)length;
    memcpy(links[*count].name, name, length);
    return (int)(*count)++;
}

static int address6_flags(struct Library *SocketBase, int sock,
                          const struct in6_ifreq *source)
{
    struct in6_ifreq request;

    memset(&request, 0, sizeof(request));
    memcpy(request.ifr_name, source->ifr_name, IFNAMSIZ);
    request.ifr6_addr = source->ifr6_addr;
    if (IoctlSocket(sock, SIOCGIFAFLAG_IN6, (char *)&request) != 0)
        return DNSSD_LINK_AROS_STACK_ERROR;
    return request.ifr_ifru.ifru_flags6;
}

static int build_specs(struct Library *SocketBase, int query_socket,
                       int socket6, const struct raw_snapshot *snapshot,
                       struct dnssd_link_spec *links, size_t link_capacity,
                       size_t *link_count,
                       struct dnssd_address_spec *addresses,
                       size_t address_capacity, size_t *address_count)
{
    size_t offset = 0u;
    const struct ifreq *request;
    int next;

    *link_count = 0u;
    *address_count = 0u;
    while ((next = next_ipv4(snapshot, &offset, &request)) > 0) {
        const struct sockaddr_in *source;
        struct dnssd_address_spec *address;
        int link;

        if (request->ifr_addr.sa_family != AF_INET)
            continue;
        source = (const struct sockaddr_in *)&request->ifr_addr;
        if (source->sin_addr.s_addr == htonl(INADDR_ANY))
            continue;
        link = add_link(SocketBase, query_socket, links, link_capacity,
                        link_count, request->ifr_name);
        if (link < 0)
            return link;
        if (*address_count >= address_capacity)
            return DNSSD_LINK_NO_SPACE;
        address = &addresses[(*address_count)++];
        memset(address, 0, sizeof(*address));
        address->interface_index = links[link].interface_index;
        address->family = DNSSD_ADDRESS_IPV4;
        address->length = 4u;
        memcpy(address->bytes, &source->sin_addr, 4u);
    }
    if (next < 0)
        return DNSSD_LINK_AROS_STACK_ERROR;

    if (snapshot->ipv6 != NULL) {
        size_t i, count = snapshot->ipv6_used / sizeof(*snapshot->ipv6);

        for (i = 0; i < count; ++i) {
            const struct in6_ifreq *source = &snapshot->ipv6[i];
            const struct sockaddr_in6 *source_address = &source->ifr6_addr;
            struct dnssd_address_spec *address;
            int flags, link;

            if (IN6_IS_ADDR_UNSPECIFIED(&source_address->sin6_addr) ||
                IN6_IS_ADDR_MULTICAST(&source_address->sin6_addr))
                continue;
            link = add_link(SocketBase, query_socket, links, link_capacity,
                            link_count, source->ifr_name);
            if (link < 0)
                return link;
            flags = address6_flags(SocketBase, socket6, source);
            if (flags < 0)
                return flags;
            if (*address_count >= address_capacity)
                return DNSSD_LINK_NO_SPACE;
            address = &addresses[(*address_count)++];
            memset(address, 0, sizeof(*address));
            address->interface_index = links[link].interface_index;
            address->scope_id = source_address->sin6_scope_id;
            address->flags = canonical_address_flags(flags);
            address->family = DNSSD_ADDRESS_IPV6;
            address->length = 16u;
            memcpy(address->bytes, &source_address->sin6_addr, 16u);
            if (IN6_IS_ADDR_LINKLOCAL(&source_address->sin6_addr))
                address->scope_id = address->interface_index;
        }
    }
    return DNSSD_LINK_OK;
}

int dnssd_link_aros_reconcile(struct dnssd_link_inventory *inventory,
    struct Library *SocketBase, size_t snapshot_limit)
{
    struct raw_snapshot snapshot;
    struct dnssd_link_spec *links = NULL;
    struct dnssd_address_spec *addresses = NULL;
    size_t candidates, link_count, address_count;
    int socket4 = -1, socket6 = -1, query_socket, result;

    if (inventory == NULL || SocketBase == NULL ||
        snapshot_limit < sizeof(struct in6_ifreq) ||
        snapshot_limit > (size_t)INT_MAX)
        return DNSSD_LINK_BAD_ARGUMENT;
    memset(&snapshot, 0, sizeof(snapshot));
    socket4 = socket(AF_INET, SOCK_DGRAM, 0);
    if (socket4 >= 0) {
        result = snapshot_ipv4(SocketBase, socket4, snapshot_limit,
                               &snapshot);
        if (result != DNSSD_LINK_OK)
            goto out;
    }
    socket6 = socket(AF_INET6, SOCK_DGRAM, 0);
    if (socket6 >= 0) {
        result = snapshot_ipv6(SocketBase, socket6, snapshot_limit,
                               &snapshot);
        if (result != DNSSD_LINK_OK)
            goto out;
    }
    if (socket4 < 0 && socket6 < 0) {
        result = DNSSD_LINK_AROS_STACK_ERROR;
        goto out;
    }
    query_socket = socket4 >= 0 ? socket4 : socket6;
    candidates = snapshot.ipv4_used / sizeof(struct ifreq) +
        snapshot.ipv6_used / sizeof(struct in6_ifreq) + 1u;
    if (candidates > (size_t)-1 / sizeof(*links) ||
        candidates > (size_t)-1 / sizeof(*addresses)) {
        result = DNSSD_LINK_AROS_NO_MEMORY;
        goto out;
    }
    links = AllocVec(candidates * sizeof(*links), MEMF_ANY | MEMF_CLEAR);
    addresses = AllocVec(candidates * sizeof(*addresses),
                         MEMF_ANY | MEMF_CLEAR);
    if (links == NULL || addresses == NULL) {
        result = DNSSD_LINK_AROS_NO_MEMORY;
        goto out;
    }
    result = build_specs(SocketBase, query_socket, socket6, &snapshot,
                         links, candidates, &link_count,
                         addresses, candidates, &address_count);
    if (result == DNSSD_LINK_OK)
        result = dnssd_link_inventory_reconcile(inventory, links, link_count,
                                                 addresses, address_count);
out:
    if (addresses != NULL)
        FreeVec(addresses);
    if (links != NULL)
        FreeVec(links);
    free_snapshot(&snapshot);
    if (socket6 >= 0)
        CloseSocket(socket6);
    if (socket4 >= 0)
        CloseSocket(socket4);
    return result;
}
