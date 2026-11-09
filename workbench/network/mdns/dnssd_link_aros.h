/*
 * SPDX-License-Identifier: BSD-2-Clause
 * Copyright 2026 Jaime Dias
 *
 * AROSTCP snapshot provider for the private native DNS-SD link inventory.
 * This is daemon-engine code, not a public dnssd.library contract.
 */
#ifndef DNSSD_LINK_AROS_H
#define DNSSD_LINK_AROS_H

#include <exec/libraries.h>
#include <stddef.h>

#include "dnssd_link.h"

enum dnssd_link_aros_result {
    DNSSD_LINK_AROS_NO_MEMORY = -10,
    DNSSD_LINK_AROS_STACK_ERROR = -11,
    DNSSD_LINK_AROS_SNAPSHOT_LIMIT = -12
};

/*
 * Read one complete AROSTCP interface/address snapshot and reconcile it.
 * snapshot_limit is the maximum temporary byte buffer used for either
 * address family. Saturated snapshots grow up to that limit; a partial
 * snapshot is never passed to the inventory.
 */
int dnssd_link_aros_reconcile(struct dnssd_link_inventory *inventory,
    struct Library *SocketBase, size_t snapshot_limit);

#endif
