/*
    Copyright (C) 2009-2026, The AROS Development Team. All rights reserved.
*/

#ifndef _PROTOCOLS_H_
#define _PROTOCOLS_H_

#include <exec/types.h>
#include <libraries/mui.h>
#include <utility/hooks.h>
#include <stdio.h>

#include "prefsdata.h"

/*
 * ProtocolFamily - identifies the IP protocol for a ProtocolAddress entry.
 */
enum ProtocolFamily
{
    PROTO_FAMILY_IPV4 = 0,
    PROTO_FAMILY_IPV6 = 1,
};

/*
 * ProtocolAddress - holds the complete address configuration for one protocol
 * (IPv4 or IPv6) on an interface.  Two instances (one per family) replace the
 * flat IPv4/IPv6 fields that were formerly scattered across struct Interface
 * and the interface-window gadgets.
 */
struct ProtocolAddress
{
    enum ProtocolFamily  pa_family;            /* PROTO_FAMILY_IPV4 or PROTO_FAMILY_IPV6 */
    enum IPMode          pa_mode;              /* DHCP, Auto, or Manual                  */
    TEXT                 pa_addr[IP6BUFLEN];   /* IP or IPv6 address (Manual mode)       */
    TEXT                 pa_mask[IPBUFLEN];    /* netmask (IPv4 only)                    */
    LONG                 pa_prefix;            /* prefix length (IPv6 only)              */
    TEXT                 pa_gate[IP6BUFLEN];   /* default gateway (any mode)             */
};

/*--- MUI list hooks (defined in protocols.c, shared by all callers) --------*/
extern struct Hook proto_constructHook;
extern struct Hook proto_destructHook;
extern struct Hook proto_displayHook;

/*--- Shared init / conversion (protocols.c) --------------------------------*/

/* Populate a ProtocolAddress from an Interface struct for the given family. */
void ProtoAddr_FromInterface(struct ProtocolAddress *pa,
                             struct Interface *iface,
                             enum ProtocolFamily fam);

/* Write IPv4 and IPv6 ProtocolAddresses back into an Interface struct. */
void ProtoAddr_ToInterface(struct Interface *iface,
                           struct ProtocolAddress *ipv4,
                           struct ProtocolAddress *ipv6);

/*--- IPv4 implementation (net4.c) ------------------------------------------*/

/*
 * Create the IPv4 configuration window.
 * Returns the Window Object; the six output pointers receive references to
 * the individual gadgets inside it.
 */
Object *Net4_CreateWindow(Object **modeOut,
                          Object **addrOut,
                          Object **maskOut,
                          Object **gateOut,
                          Object **applyOut,
                          Object **closeOut);

/* Populate the IPv4 config-window gadgets from a ProtocolAddress. */
void Net4_ShowWindow(struct ProtocolAddress *pa,
                     Object *mode, Object *addr,
                     Object *mask, Object *gate);

/* Read the IPv4 config-window gadgets back into a ProtocolAddress. */
void Net4_ApplyWindow(struct ProtocolAddress *pa,
                      Object *mode, Object *addr,
                      Object *mask, Object *gate);

/*
 * Handle an IPv4 mode cycle change: save current field values if needed,
 * then enable or disable the addr/mask gadgets accordingly.
 */
void Net4_ModeChanged(struct ProtocolAddress *pa, ULONG newMode,
                      Object *addr, Object *mask);

/* Write the IP= / NETMASK= / GW= tokens for this address to a FILE. */
void Net4_WriteTokens(FILE *f, struct ProtocolAddress *pa);

/*--- IPv6 implementation (net6.c) ------------------------------------------*/

/*
 * Create the IPv6 configuration window.
 * Returns the Window Object; six output pointers receive gadget references.
 */
Object *Net6_CreateWindow(Object **modeOut,
                          Object **addrOut,
                          Object **prefixOut,
                          Object **gateOut,
                          Object **applyOut,
                          Object **closeOut);

/* Populate the IPv6 config-window gadgets from a ProtocolAddress. */
void Net6_ShowWindow(struct ProtocolAddress *pa,
                     Object *mode, Object *addr,
                     Object *prefix, Object *gate);

/* Read the IPv6 config-window gadgets back into a ProtocolAddress. */
void Net6_ApplyWindow(struct ProtocolAddress *pa,
                      Object *mode, Object *addr,
                      Object *prefix, Object *gate);

/*
 * Handle an IPv6 mode cycle change: save current field values if needed,
 * then enable or disable the addr/prefix gadgets accordingly.
 */
void Net6_ModeChanged(struct ProtocolAddress *pa, ULONG newMode,
                      Object *addr, Object *prefix);

/* Write the IP6= / GW6= tokens for this address to a FILE. */
void Net6_WriteTokens(FILE *f, struct ProtocolAddress *pa);

#endif /* _PROTOCOLS_H_ */
