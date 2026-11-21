/*
 * SPDX-License-Identifier: BSD-2-Clause
 * Copyright 2026 amigazen project
 * Copyright 2026 Jaime Dias
 *
 * netservices.h - NetServices, the commodity that finds, reaches and shares
 * services on the local network (DNS-SD over mDNS) on dnssd.library: what
 * its parts share.
 *
 * All of it runs in the commodity's own task. Its requests are operations of
 * one dnssd.library session; mdnsd signals the session's port when there
 * are events, and the main loop reads them (NSDiscoveryEvents()) between
 * the window's and the broker's messages - no hook, no other task, no
 * polling.
 */

#ifndef NETSERVICES_H
#define NETSERVICES_H

#include <exec/types.h>
#include <exec/lists.h>
#include <dnssd/dnssd.h>

#include "../tools/sd_common.h"

#define NS_NAME         "NetServices"   /* the broker's: not translated */
#define NS_POPKEY       "control alt n"

/*------------------------------------------------------------------------*/
/* What the window shows: the model (discovery.c) */

/* A row of the Nearby list: an instance (Name, Kind), or a kind of service
 * when the list is of the kinds on the network. */
struct NSRow
{
    struct Node nr_Node;        /* ln_Name: nr_Label */
    char        nr_Label[160];
    char        nr_Name[64];    /* the instance's own label; "" for a kind */
    char        nr_Type[256];   /* "_http._tcp" */
    UWORD       nr_Links;       /* the links it was found on */
    BOOL        nr_IsKind;
};

/* A line of the Handshake result. */
struct NSLine
{
    struct Node nl_Node;        /* ln_Name: nl_Text */
    char        nl_Text[160];
};

/* What NSDiscoveryEvents() changed, for the window to show. */
#define NS_CHANGED_NEARBY   0x01
#define NS_CHANGED_RESULT   0x02
#define NS_CHANGED_STATE    0x04    /* browsing, reaching or sharing, started or ended */
#define NS_CHANGED_STATUS   0x08    /* the status line */

/*
 * A session can always have DNSSD_SESSION_OPERATIONS operations: the
 * browse with its resolves (the finder's, sd_common.h), the Handshake's
 * resolve and the Share's registration.
 */
#if 1 + SD_FINDER_FOLLOWED + SD_FINDER_RESOLVES + 2 > DNSSD_SESSION_OPERATIONS
#error "NetServices needs more operations than a session is promised"
#endif

struct NSDiscovery
{
    struct SDSession    ds_Session;
    BOOL                ds_Open;
    struct SDFinder     ds_Finder;
    BOOL                ds_Browsing;
    BOOL                ds_Kinds;       /* the browse is of the kinds */
    struct SDService   *ds_Found;       /* the finder's, off the stack */
    struct List         ds_Nearby;      /* struct NSRow */
    DNSSDOperationRef   ds_Resolve;     /* Handshake */
    ULONG               ds_ResolveID;
    char                ds_ResolveName[64];
    struct List         ds_Result;      /* struct NSLine */
    DNSSDOperationRef   ds_Share;
    ULONG               ds_ShareID;
    UWORD               ds_SharePort;
    char                ds_ShareType[256];
    char                ds_Status[160];
};

extern struct NSDiscovery NSD;

BOOL NSDiscoveryInit(void);
void NSDiscoveryFree(void);
/* The session's signal, 0 while there is no session. */
ULONG NSDiscoverySignal(void);
/* Reads the session's events; NS_CHANGED_*. The window must have its lists
 * off the gadgets while this runs. */
ULONG NSDiscoveryEvents(void);

/* type: a DNS-SD type, or SD_TYPE_SERVICES for the kinds. */
ULONG NSBrowse(CONST_STRPTR type);
ULONG NSStopBrowse(void);
void NSClearNearby(void);
ULONG NSReach(CONST_STRPTR name, CONST_STRPTR type);
/* info: "key=value" items with a space between them. */
ULONG NSShare(CONST_STRPTR name, CONST_STRPTR type, LONG port, CONST_STRPTR info);
ULONG NSUnshare(void);

struct NSRow *NSRowAt(LONG index);

/*------------------------------------------------------------------------*/
/* The window (window.c) */

extern struct MsgPort *NSWindowPort;    /* NULL while the window is closed */

void NSWindowOpen(void);
void NSWindowClose(void);
/* FALSE: quit. */
BOOL NSWindowMessages(void);
/* Shows what NSDiscoveryEvents() or a command changed. */
void NSWindowShow(ULONG changed);
/* Lists off and back on the gadgets, around a change of them. */
void NSWindowDetach(void);
void NSWindowAttach(void);

extern CONST_STRPTR NSPopKey;

#endif /* NETSERVICES_H */
