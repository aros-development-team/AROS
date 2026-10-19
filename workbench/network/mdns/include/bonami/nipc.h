/* SPDX-License-Identifier: BSD-2-Clause
 * Copyright 2026 amigazen project
 * Copyright 2026 Jaime Dias
 */
#ifndef BONAMI_NIPC_H
#define BONAMI_NIPC_H
/*------------------------------------------------------------------------*/
/*
 * nipc.h - Envoy / nipc.library integration (bonami.library)
 *
 * These entry points adapt NIPCInquiryA tag lists and public entities to mDNS.
 * nipc.library calls them at runtime (dual stack); application code normally
 * uses NIPCInquiryA only.
 *
 * LVO offsets must match SDK/FD/bonami_lib.fd (bias 30, -6 per slot).
 */
/*------------------------------------------------------------------------*/

#include <exec/types.h>
#include <utility/tagitem.h>
#include <utility/hooks.h>

#define BONAMI_LVO_INQUIRY      102
#define BONAMI_LVO_PUBLISH      108
#define BONAMI_LVO_UNPUBLISH    114

#define MDNS_NIPC_Base          (TAG_USER + 0xB3020)

/*------------------------------------------------------------------------*/
/* Function prototypes: clib/bonami_protos.h (via proto/bonami.h) */

/*------------------------------------------------------------------------*/

#endif /* BONAMI_NIPC_H */
