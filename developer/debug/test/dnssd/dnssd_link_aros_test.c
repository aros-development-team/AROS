/*
 * SPDX-License-Identifier: BSD-2-Clause
 * Copyright 2026 Jaime Dias
 */

#include "dnssd_link_aros.h"

#include <bsdsocket/socketbasetags.h>
#include <exec/types.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <net/if.h>
#include <netinet/in.h>
#include <netinet/in_var.h>
#include <proto/bsdsocket.h>
#include <proto/dos.h>
#include <proto/exec.h>
#include <sys/ioctl.h>

#include <stdio.h>
#include <string.h>

struct Library *SocketBase;

static int failures;

#define CHECK(condition, text) do { \
    if (!(condition)) { \
        ++failures; \
        printf("FAIL %s\n", text); \
    } \
} while (0)

static int find_address(const struct dnssd_link_inventory *inventory,
                        uint32_t index, uint16_t family,
                        const uint8_t *bytes, size_t length)
{
    size_t i;

    for (i = 0; i < inventory->address_count; ++i) {
        const struct dnssd_address_spec *address =
            &inventory->addresses[i].value;

        if (address->interface_index == index &&
            address->family == family && address->length == length &&
            memcmp(address->bytes, bytes, length) == 0)
            return 1;
    }
    return 0;
}

static const struct dnssd_link_entry *first_usable_link(
    const struct dnssd_link_inventory *inventory)
{
    size_t i;

    for (i = 0; i < inventory->link_count; ++i) {
        uint32_t flags = inventory->links[i].value.flags;

        if ((flags & (DNSSD_LINK_UP | DNSSD_LINK_MULTICAST)) ==
                (DNSSD_LINK_UP | DNSSD_LINK_MULTICAST) &&
            (flags & DNSSD_LINK_LOOPBACK) == 0u)
            return &inventory->links[i];
    }
    return NULL;
}

static int change_address4(const char *name, uint32_t host_address, int add)
{
    int sock = socket(AF_INET, SOCK_DGRAM, 0), result;

    if (sock < 0)
        return -1;
    if (add) {
        struct ifaliasreq request;
        struct sockaddr_in *address;

        memset(&request, 0, sizeof(request));
        snprintf(request.ifra_name, sizeof(request.ifra_name), "%s", name);
        address = (struct sockaddr_in *)&request.ifra_addr;
        address->sin_len = sizeof(*address);
        address->sin_family = AF_INET;
        address->sin_addr.s_addr = htonl(host_address);
        address = (struct sockaddr_in *)&request.ifra_mask;
        address->sin_len = sizeof(*address);
        address->sin_family = AF_INET;
        address->sin_addr.s_addr = htonl(0xffffff00u);
        result = IoctlSocket(sock, SIOCAIFADDR, (char *)&request);
    } else {
        struct ifreq request;
        struct sockaddr_in *address;

        memset(&request, 0, sizeof(request));
        snprintf(request.ifr_name, sizeof(request.ifr_name), "%s", name);
        address = (struct sockaddr_in *)&request.ifr_addr;
        address->sin_len = sizeof(*address);
        address->sin_family = AF_INET;
        address->sin_addr.s_addr = htonl(host_address);
        result = IoctlSocket(sock, SIOCDIFADDR, (char *)&request);
    }
    CloseSocket(sock);
    return result;
}

static int change_address6(const char *name, const struct in6_addr *value,
                           int add)
{
    int sock = socket(AF_INET6, SOCK_DGRAM, 0), result;

    if (sock < 0)
        return -1;
    if (add) {
        struct in6_aliasreq request;
        size_t i;

        memset(&request, 0, sizeof(request));
        snprintf(request.ifra_name, sizeof(request.ifra_name), "%s", name);
        request.ifra_addr.sin6_len = sizeof(request.ifra_addr);
        request.ifra_addr.sin6_family = AF_INET6;
        request.ifra_addr.sin6_addr = *value;
        request.ifra_prefixmask.sin6_len = sizeof(request.ifra_prefixmask);
        request.ifra_prefixmask.sin6_family = AF_INET6;
        for (i = 0; i < 8u; ++i)
            request.ifra_prefixmask.sin6_addr.s6_addr[i] = 0xffu;
        request.ifra_lifetime.ia6t_vltime = 600u;
        request.ifra_lifetime.ia6t_pltime = 600u;
        result = IoctlSocket(sock, SIOCAIFADDR_IN6, (char *)&request);
    } else {
        struct in6_ifreq request;

        memset(&request, 0, sizeof(request));
        snprintf(request.ifr_name, sizeof(request.ifr_name), "%s", name);
        request.ifr6_addr.sin6_len = sizeof(request.ifr6_addr);
        request.ifr6_addr.sin6_family = AF_INET6;
        request.ifr6_addr.sin6_addr = *value;
        result = IoctlSocket(sock, SIOCDIFADDR_IN6, (char *)&request);
    }
    CloseSocket(sock);
    return result;
}

static int wait_signal(ULONG mask)
{
    int i;

    for (i = 0; i < 40; ++i) {
        if ((SetSignal(0u, mask) & mask) != 0u) {
            SetSignal(0u, mask);
            return 1;
        }
        Delay(5);
    }
    return 0;
}

int main(void)
{
    static const uint8_t address4_bytes[4] = { 192u, 0u, 2u, 211u };
    struct dnssd_link_inventory inventory;
    struct dnssd_link_entry links[32], link_scratch[32];
    struct dnssd_address_entry addresses[256], address_scratch[256];
    struct in6_addr address6_value;
    const struct dnssd_link_entry *selected;
    uint8_t address6_bytes[16];
    char name[DNSSD_LINK_NAME_MAX + 1u];
    uint32_t index = 0u, revision;
    BYTE signal_number = -1;
    ULONG signal_mask = 0u;
    int added4 = 0, added6 = 0;

    memset(&address6_value, 0, sizeof(address6_value));
    address6_value.s6_addr[0] = 0x20u;
    address6_value.s6_addr[1] = 0x01u;
    address6_value.s6_addr[2] = 0x0du;
    address6_value.s6_addr[3] = 0xb8u;
    address6_value.s6_addr[4] = 0x00u;
    address6_value.s6_addr[5] = 0x43u;
    address6_value.s6_addr[6] = 0x49u;
    address6_value.s6_addr[7] = 0x01u;
    address6_value.s6_addr[15] = 0xd3u;
    memcpy(address6_bytes, &address6_value, sizeof(address6_bytes));

    CHECK(dnssd_link_inventory_init(&inventory, links, link_scratch, 32u,
              addresses, address_scratch, 256u) == DNSSD_LINK_OK,
          "initialize native inventory storage");
    CHECK(dnssd_link_aros_reconcile(NULL, NULL, 65536u) ==
              DNSSD_LINK_BAD_ARGUMENT,
          "reject missing inventory and stack base");

    SocketBase = OpenLibrary("bsdsocket.library", 4u);
    CHECK(SocketBase != NULL, "open bsdsocket.library");
    if (SocketBase == NULL)
        goto done;

    CHECK(dnssd_link_aros_reconcile(&inventory, SocketBase, 65536u) ==
              DNSSD_LINK_OK,
          "collect a complete AROSTCP snapshot");
    CHECK(inventory.link_count != 0u && inventory.address_count != 0u,
          "publish at least one link and address from the running stack");
    revision = inventory.revision;
    CHECK(dnssd_link_aros_reconcile(&inventory, SocketBase,
              sizeof(struct in6_ifreq)) ==
              DNSSD_LINK_AROS_SNAPSHOT_LIMIT &&
              inventory.revision == revision,
          "reject a saturated snapshot without publishing partial state");
    selected = first_usable_link(&inventory);
    CHECK(selected != NULL, "find an up multicast non-loopback link");
    if (selected == NULL)
        goto close;

    index = selected->value.interface_index;
    memset(name, 0, sizeof(name));
    memcpy(name, selected->value.name, selected->value.name_length);
    signal_number = AllocSignal(-1);
    CHECK(signal_number != -1, "allocate address-change signal");
    if (signal_number == -1)
        goto close;
    signal_mask = 1UL << signal_number;
    SocketBaseTags(SBTM_SETVAL(SBTC_SIG_ADDRESS_CHANGE_MASK), signal_mask,
                   TAG_END);
    SetSignal(0u, signal_mask);

    added4 = change_address4(name, 0xc00002d3u, 1) == 0;
    CHECK(added4, "add an IPv4 alias through AROSTCP");
    CHECK(!added4 || wait_signal(signal_mask),
          "receive address-change notification for IPv4 alias");
    added6 = change_address6(name, &address6_value, 1) == 0;
    CHECK(added6, "add an IPv6 address through AROSTCP");
    CHECK(!added6 || wait_signal(signal_mask),
          "receive address-change notification for IPv6 address");

    revision = inventory.revision;
    CHECK(dnssd_link_aros_reconcile(&inventory, SocketBase, 65536u) ==
              DNSSD_LINK_OK,
          "reconcile the stack after adding both address families");
    CHECK(!added4 || find_address(&inventory, index, DNSSD_ADDRESS_IPV4,
              address4_bytes, sizeof(address4_bytes)),
          "retain the added IPv4 alias in the canonical snapshot");
    CHECK(!added6 || find_address(&inventory, index, DNSSD_ADDRESS_IPV6,
              address6_bytes, sizeof(address6_bytes)),
          "retain the added IPv6 address in the canonical snapshot");
    CHECK((!added4 && !added6) || inventory.revision > revision,
          "publish one new inventory revision after address additions");

    SetSignal(0u, signal_mask);
    if (added6) {
        int removed = change_address6(name, &address6_value, 0) == 0;

        CHECK(removed, "remove the IPv6 test address");
        if (removed)
            added6 = 0;
    }
    if (added4) {
        int removed = change_address4(name, 0xc00002d3u, 0) == 0;

        CHECK(removed, "remove the IPv4 test alias");
        if (removed)
            added4 = 0;
    }
    CHECK(wait_signal(signal_mask),
          "receive address-change notification for address removal");
    revision = inventory.revision;
    CHECK(dnssd_link_aros_reconcile(&inventory, SocketBase, 65536u) ==
              DNSSD_LINK_OK,
          "reconcile the stack after removing test addresses");
    CHECK(!find_address(&inventory, index, DNSSD_ADDRESS_IPV4,
              address4_bytes, sizeof(address4_bytes)) &&
              !find_address(&inventory, index, DNSSD_ADDRESS_IPV6,
                  address6_bytes, sizeof(address6_bytes)),
          "remove both test addresses from the canonical snapshot");
    CHECK(inventory.revision > revision,
          "publish a new inventory revision after address removal");

close:
    if (added6)
        change_address6(name, &address6_value, 0);
    if (added4)
        change_address4(name, 0xc00002d3u, 0);
    if (signal_number != -1) {
        SocketBaseTags(SBTM_SETVAL(SBTC_SIG_ADDRESS_CHANGE_MASK), 0u,
                       TAG_END);
        FreeSignal(signal_number);
    }
    CloseLibrary(SocketBase);
done:
    if (failures == 0)
        printf("DNS-SD AROSTCP link collector: 0 failures\n");
    return failures != 0;
}
