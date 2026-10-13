#ifndef BONAMI_MDNS_H
#define BONAMI_MDNS_H
/*
 * SPDX-License-Identifier: BSD-2-Clause
 * Copyright 2026 amigazen project
 * Copyright 2026 Jaime Dias
 *
 * mdns.h - multicast DNS and DNS-based service discovery (RFC 6762, RFC
 * 6763) through bonami.library
 *
 * The library holds no state of its own: each call is a message to mdnsd,
 * the system's one responder, which the stack's service manager runs.
 */

#include <exec/types.h>
#include <utility/tagitem.h>
#include <libraries/bonamibase.h>

#define MDNS_DOMAIN_LOCAL       "local"

/* The longest texts, without the NUL that ends them. */
#define MDNS_MAX_NAME           64      /* an instance: "Living Room" */
#define MDNS_MAX_TYPE           32      /* a type: "_http._tcp" */
#define MDNS_MAX_DOMAIN         16
#define MDNS_MAX_HOST           64
#define MDNS_MAX_TXT            256

/*
 * Tags of RegisterServiceA() and UpdateServiceA().
 *
 * MDNS_TXTRecord is one string of "key=value" items with a space between
 * them. A space that belongs to an item is written with a backslash before
 * it, and a backslash as two; in the TXT record each item is a string of
 * its own with neither (RFC 6763 section 6).
 */
#define MDNS_Dummy              (TAG_USER + 0xB3000)

#define MDNS_ServiceName        (MDNS_Dummy + 1)   /* STRPTR, the instance */
#define MDNS_ServiceType        (MDNS_Dummy + 2)   /* STRPTR, "_http._tcp" */
#define MDNS_Port               (MDNS_Dummy + 3)   /* UWORD, host order */
#define MDNS_TXTRecord          (MDNS_Dummy + 4)   /* STRPTR, see above */
#define MDNS_Domain             (MDNS_Dummy + 5)   /* STRPTR, "local" when
                                                    * not given */
#define MDNS_HostName           (MDNS_Dummy + 6)   /* STRPTR: with
                                                    * MDNS_HostAddr, the
                                                    * service is another
                                                    * host's, one that does
                                                    * not speak mDNS */
#define MDNS_RegisterRef        (MDNS_Dummy + 11)  /* MDNSRegisterRef *,
                                                    * filled in */
#define MDNS_HostAddr           (MDNS_Dummy + 14)  /* ULONG, IPv4 in network
                                                    * order */
#define MDNS_TimeToLive         (MDNS_Dummy + 17)  /* ULONG seconds, 1 to
                                                    * 4500, of the service's
                                                    * own records; without
                                                    * it RFC 6762's 75
                                                    * minutes, and 2 for
                                                    * the one that names
                                                    * the host */

/* What the functions return. */
#define MDNS_OK                 0
#define MDNSERR_BADPARAM        (-1)
#define MDNSERR_NOMEM           (-2)
#define MDNSERR_DUPLICATE       (-4)    /* this task or another already
                                         * registered the name and type */
#define MDNSERR_NOTFOUND        (-5)
#define MDNSERR_NOENGINE        (-8)    /* mdnsd is not running */
#define MDNSERR_NOTREADY        (-9)

#endif /* BONAMI_MDNS_H */
