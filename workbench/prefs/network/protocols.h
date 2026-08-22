/*
    Copyright (C) 2009-2026, The AROS Development Team. All rights reserved.

    protocols.h - Shared protocol-address infrastructure.

    PAWinClass is the base MUI window class for protocol-address
    configuration.  Plugin modules (*.netprefs) subclass it and register
    themselves via the netprefs library API at runtime.
*/

#ifndef _PROTOCOLS_H_
#define _PROTOCOLS_H_

#include <exec/types.h>
#include <exec/nodes.h>
#include <exec/lists.h>
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
 * ProtocolAddress - one protocol object attached to an interface.  It is a
 * List node so an interface can carry any set of protocol objects (IPv4 only,
 * IPv6 only, both, or others); pa_node.ln_Type is the ID the owning plugin was
 * assigned at registration, which is how the core routes a node back to its
 * handler without interpreting the address itself.  The core treats everything
 * below ln_Type as opaque plugin data.
 */
struct ProtocolAddress
{
    struct Node          pa_node;              /* ln_Type = owning plugin's ID           */
    enum ProtocolFamily  pa_family;            /* plugin-internal family tag             */
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

/*--- Protocol-object list helpers (protocols.c) ----------------------------*/

struct ProtoHandlerNode;

struct ProtoHandlerNode *ProtoHandler_ByID(UBYTE id);
struct ProtocolAddress  *ProtoAddr_Find(struct List *list, UBYTE id);
struct ProtocolAddress  *ProtoAddr_FindOrAdd(struct List *list, UBYTE id);
void                     ProtoAddr_FreeList(struct List *list);
void                     ProtoAddr_CopyList(struct List *dst, struct List *src);

/*--- PAWinClass: common protocol-address configuration window --------------*/
/*    Defined in protocols.c; subclassed by plugin modules (.netprefs).      */

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

BOOL PAWin_InitClass(void);
void PAWin_FreeClass(void);

#endif /* _PROTOCOLS_H_ */

