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
 * (IPv4 or IPv6) on an interface.
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

void ProtoAddr_FromInterface(struct ProtocolAddress *pa,
                             struct Interface *iface,
                             enum ProtocolFamily fam);

void ProtoAddr_ToInterface(struct Interface *iface,
                           struct ProtocolAddress *ipv4,
                           struct ProtocolAddress *ipv6);

/*--- PAWinClass: common protocol-address configuration window --------------*/
/*    Defined in protocols.c; subclassed by Net4WinClass and Net6WinClass.   */

#define MUIB_PAWin                  (TAG_USER | 0x11000000)

/* Init-only attributes (set in OM_NEW tags) */
#define MUIA_PAWin_ProtocolName     (MUIB_PAWin | 0x0001) /* STRPTR  */
#define MUIA_PAWin_Content          (MUIB_PAWin | 0x0002) /* Object* */

/* Read-only attributes */
#define MUIA_PAWin_UseButton        (MUIB_PAWin | 0x0003) /* Object* */
#define MUIA_PAWin_CancelButton     (MUIB_PAWin | 0x0004) /* Object* */

/* Methods */
#define MUIM_PAWin_Show             (MUIB_PAWin | 0x0010)
#define MUIM_PAWin_Apply            (MUIB_PAWin | 0x0011)
#define MUIM_PAWin_ModeChanged      (MUIB_PAWin | 0x0012)

struct MUIP_PAWin_Show        { STACKED ULONG MethodID; STACKED struct ProtocolAddress *pa; };
struct MUIP_PAWin_Apply       { STACKED ULONG MethodID; STACKED struct ProtocolAddress *pa; };
struct MUIP_PAWin_ModeChanged { STACKED ULONG MethodID; STACKED ULONG newMode; };

extern struct MUI_CustomClass *PAWinClass;
extern struct MUI_CustomClass *Net4WinClass;
extern struct MUI_CustomClass *Net6WinClass;

BOOL PAWin_InitClass(void);
void PAWin_FreeClass(void);

/*--- Net4WinClass (net4.c) -------------------------------------------------*/

BOOL Net4Win_InitClass(void);
void Net4Win_FreeClass(void);

/* Write the IP= / NETMASK= / GW= tokens for this address to a FILE. */
void Net4_WriteTokens(FILE *f, struct ProtocolAddress *pa);

/*--- Net6WinClass (net6.c) -------------------------------------------------*/

BOOL Net6Win_InitClass(void);
void Net6Win_FreeClass(void);

/* Write the IP6= / GW6= tokens for this address to a FILE. */
void Net6_WriteTokens(FILE *f, struct ProtocolAddress *pa);

#endif /* _PROTOCOLS_H_ */

