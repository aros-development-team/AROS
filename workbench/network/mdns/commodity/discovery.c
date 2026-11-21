/*
 * SPDX-License-Identifier: BSD-2-Clause
 * Copyright 2026 amigazen project
 * Copyright 2026 Jaime Dias
 *
 * discovery.c - NetServices' requests to dnssd.library and what they find:
 * the Nearby list (a browse whose instances are resolved and followed, the
 * finder of sd_common), the Handshake result (one resolve) and the Share
 * (a registration), all operations of one session.
 *
 * The session is opened when it is first needed, so that the commodity
 * starts, and shows why, without mdnsd; it is opened again after mdnsd
 * restarted (DNSSD_ERR_STALE).
 */

#include <exec/types.h>
#include <exec/memory.h>
#include <proto/exec.h>
#include <proto/alib.h>
#include <proto/dnssd.h>
#include <utility/tagitem.h>

#include "netservices.h"
#include "../tools/sd_types.h"

#define CATCOMP_NUMBERS
#define CATCOMP_STRINGS
#include "../strings.h"

struct NSDiscovery NSD;

#define NS_SHARE_ITEMS  16      /* "key=value" items of a Share */

/*------------------------------------------------------------------------*/
/* The lists */

static void
ns_free_list(struct List *list)
{
    struct Node *n;

    while ((n = RemHead(list)) != NULL) {
        FreeVec(n);
    }
}

static void
ns_line(CONST_STRPTR text)
{
    struct NSLine *l = AllocVec(sizeof(*l), MEMF_ANY);

    if (l != NULL) {
        SDStrCopy(l->nl_Text, (const char *)text, (LONG)sizeof(l->nl_Text));
        l->nl_Node.ln_Name = l->nl_Text;
        AddTail(&NSD.ds_Result, &l->nl_Node);
    }
}

static LONG
ns_compare(const char *a, const char *b)
{
    for (;; a++, b++) {
        char x = *a, y = *b;

        if (x >= 'A' && x <= 'Z') {
            x += 'a' - 'A';
        }
        if (y >= 'A' && y <= 'Z') {
            y += 'a' - 'A';
        }
        if (x != y || x == '\0') {
            return (LONG)(UBYTE)x - (LONG)(UBYTE)y;
        }
    }
}

static struct NSRow *
ns_row_find(CONST_STRPTR name, CONST_STRPTR type, BOOL kind)
{
    struct Node *n;

    for (n = NSD.ds_Nearby.lh_Head; n->ln_Succ != NULL; n = n->ln_Succ) {
        struct NSRow *r = (struct NSRow *)n;

        if (r->nr_IsKind == kind && SDStrEqual(r->nr_Name, (const char *)name) &&
            SDStrEqualCI(r->nr_Type, (const char *)type)) {
            return r;
        }
    }
    return NULL;
}

static void
ns_row_label(struct NSRow *r, UWORD port)
{
    CONST_STRPTR kind = SDTypeFriendly((CONST_STRPTR)r->nr_Type);

    if (r->nr_IsKind) {
        SDStrCopy(r->nr_Label, (const char *)kind, (LONG)sizeof(r->nr_Label));
    } else if (NSD.ds_Kinds || port == 0) {
        SDFormat(r->nr_Label, (LONG)sizeof(r->nr_Label),
            SDMsg(MSG_NS_ROW), r->nr_Name, kind);
    } else {
        SDFormat(r->nr_Label, (LONG)sizeof(r->nr_Label),
            SDMsg(MSG_NS_ROW_PORT), r->nr_Name, (LONG)port);
    }
}

/* A row in the list's order (by its label), or one more link for it. */
static void
ns_row_add(CONST_STRPTR name, CONST_STRPTR type, BOOL kind, UWORD port)
{
    struct NSRow *r = ns_row_find(name, type, kind);
    struct Node *n;

    if (r != NULL) {
        r->nr_Links++;
        return;
    }
    if ((r = AllocVec(sizeof(*r), MEMF_ANY | MEMF_CLEAR)) == NULL) {
        return;
    }
    SDStrCopy(r->nr_Name, (const char *)name, (LONG)sizeof(r->nr_Name));
    SDStrCopy(r->nr_Type, (const char *)type, (LONG)sizeof(r->nr_Type));
    r->nr_IsKind = kind;
    r->nr_Links = 1;
    ns_row_label(r, port);
    r->nr_Node.ln_Name = r->nr_Label;
    for (n = NSD.ds_Nearby.lh_Head; n->ln_Succ != NULL; n = n->ln_Succ) {
        if (ns_compare(r->nr_Label, n->ln_Name) < 0) {
            break;
        }
    }
    Insert(&NSD.ds_Nearby, &r->nr_Node, n->ln_Pred);
}

static void
ns_row_remove(CONST_STRPTR name, CONST_STRPTR type, BOOL kind)
{
    struct NSRow *r = ns_row_find(name, type, kind);

    if (r != NULL && --r->nr_Links == 0) {
        Remove(&r->nr_Node);
        FreeVec(r);
    }
}

struct NSRow *
NSRowAt(LONG index)
{
    struct Node *n;

    for (n = NSD.ds_Nearby.lh_Head; n->ln_Succ != NULL; n = n->ln_Succ) {
        if (index-- == 0) {
            return (struct NSRow *)n;
        }
    }
    return NULL;
}

/*------------------------------------------------------------------------*/
/* The session */

static void
ns_status(CONST_STRPTR text)
{
    SDStrCopy(NSD.ds_Status, (const char *)text, (LONG)sizeof(NSD.ds_Status));
}

/* format: a message with a %s for what the error means */
static void
ns_status_error(CONST_STRPTR format, LONG rc)
{
    SDFormat(NSD.ds_Status, (LONG)sizeof(NSD.ds_Status), format, SDErrorText(rc));
}

BOOL
NSDiscoveryInit(void)
{
    NewList(&NSD.ds_Nearby);
    NewList(&NSD.ds_Result);
    NSD.ds_Found = AllocVec(sizeof(struct SDService), MEMF_ANY | MEMF_CLEAR);
    ns_status(SDMsg(MSG_NS_READY));
    return NSD.ds_Found != NULL;
}

/* Ends every operation and the session; what they found stays shown. */
static void
ns_session_close(void)
{
    if (NSD.ds_Browsing) {
        SDFinderStop(&NSD.ds_Finder);
        NSD.ds_Browsing = FALSE;
    }
    if (NSD.ds_Resolve != NULL) {
        DNSSD_Cancel(NSD.ds_Resolve);
        NSD.ds_Resolve = NULL;
    }
    if (NSD.ds_Share != NULL) {
        DNSSD_Cancel(NSD.ds_Share);
        NSD.ds_Share = NULL;
    }
    if (NSD.ds_Open) {
        SDSessionClose(&NSD.ds_Session);
        NSD.ds_Open = FALSE;
    }
}

void
NSDiscoveryFree(void)
{
    ns_session_close();
    ns_free_list(&NSD.ds_Nearby);
    ns_free_list(&NSD.ds_Result);
    if (NSD.ds_Found != NULL) {
        FreeVec(NSD.ds_Found);
        NSD.ds_Found = NULL;
    }
}

static LONG
ns_session(void)
{
    LONG rc = DNSSD_OK;

    if (DNSSDBase == NULL) {
        return DNSSD_ERR_NOENGINE;
    }
    if (!NSD.ds_Open) {
        rc = SDSessionOpen(&NSD.ds_Session, 0);
        NSD.ds_Open = rc == DNSSD_OK;
    }
    return rc;
}

ULONG
NSDiscoverySignal(void)
{
    return NSD.ds_Open ? 1UL << NSD.ds_Session.bs_Port->mp_SigBit : 0;
}

/*------------------------------------------------------------------------*/
/* Nearby */

ULONG
NSBrowse(CONST_STRPTR type)
{
    LONG rc;

    NSStopBrowse();
    ns_free_list(&NSD.ds_Nearby);
    NSD.ds_Kinds = SDStrEqual((const char *)type, SD_TYPE_SERVICES);
    if ((rc = ns_session()) == DNSSD_OK) {
        rc = SDFinderStart(&NSD.ds_Finder, &NSD.ds_Session, type);
    }
    if (rc != DNSSD_OK) {
        ns_status_error(SDMsg(MSG_NS_BROWSE_FAILED), rc);
    } else {
        NSD.ds_Browsing = TRUE;
        if (NSD.ds_Kinds) {
            ns_status(SDMsg(MSG_NS_BROWSING_KINDS));
        } else {
            SDFormat(NSD.ds_Status, (LONG)sizeof(NSD.ds_Status),
                SDMsg(MSG_NS_BROWSING), SDTypeFriendly(type));
        }
    }
    return NS_CHANGED_NEARBY | NS_CHANGED_STATE | NS_CHANGED_STATUS;
}

void
NSClearNearby(void)
{
    ns_free_list(&NSD.ds_Nearby);
}

ULONG
NSStopBrowse(void)
{
    if (!NSD.ds_Browsing) {
        return 0;
    }
    SDFinderStop(&NSD.ds_Finder);
    NSD.ds_Browsing = FALSE;
    ns_status(SDMsg(MSG_NS_STOPPED));
    return NS_CHANGED_STATE | NS_CHANGED_STATUS;
}

static ULONG
ns_found(const struct DNSSDEvent *e)
{
    struct SDService *sv = NSD.ds_Found;
    struct NSRow *r;
    LONG rc;

    switch (SDFinderEvent(&NSD.ds_Finder, e, sv, &rc)) {
    case SD_FOUND_TYPE:
        ns_row_add((CONST_STRPTR)"", (CONST_STRPTR)sv->sv_Type, TRUE, 0);
        return NS_CHANGED_NEARBY;
    case SD_FOUND_NOTYPE:
        ns_row_remove((CONST_STRPTR)"", (CONST_STRPTR)sv->sv_Type, TRUE);
        return NS_CHANGED_NEARBY;
    case SD_FOUND_ADD:
        ns_row_add((CONST_STRPTR)sv->sv_Name, (CONST_STRPTR)sv->sv_Type,
            FALSE, sv->sv_Port);
        return NS_CHANGED_NEARBY;
    case SD_FOUND_UPDATE:
        /* the port may have changed: the label again, in its place */
        if ((r = ns_row_find((CONST_STRPTR)sv->sv_Name,
                (CONST_STRPTR)sv->sv_Type, FALSE)) != NULL) {
            UWORD links = r->nr_Links;

            Remove(&r->nr_Node);
            FreeVec(r);
            ns_row_add((CONST_STRPTR)sv->sv_Name, (CONST_STRPTR)sv->sv_Type,
                FALSE, sv->sv_Port);
            if ((r = ns_row_find((CONST_STRPTR)sv->sv_Name,
                    (CONST_STRPTR)sv->sv_Type, FALSE)) != NULL) {
                r->nr_Links = links;
            }
        }
        return NS_CHANGED_NEARBY;
    case SD_FOUND_REMOVE:
        ns_row_remove((CONST_STRPTR)sv->sv_Name, (CONST_STRPTR)sv->sv_Type, FALSE);
        return NS_CHANGED_NEARBY;
    case SD_FOUND_END:
        SDFinderStop(&NSD.ds_Finder);
        NSD.ds_Browsing = FALSE;
        ns_status_error(SDMsg(MSG_NS_BROWSE_FAILED), rc);
        return NS_CHANGED_STATE | NS_CHANGED_STATUS;
    }
    return 0;
}

/*------------------------------------------------------------------------*/
/* Handshake */

ULONG
NSReach(CONST_STRPTR name, CONST_STRPTR type)
{
    LONG rc;

    if (NSD.ds_Resolve != NULL) {
        DNSSD_Cancel(NSD.ds_Resolve);
        NSD.ds_Resolve = NULL;
    }
    ns_free_list(&NSD.ds_Result);
    SDStrCopy(NSD.ds_ResolveName, (const char *)name,
        (LONG)sizeof(NSD.ds_ResolveName));
    if ((rc = ns_session()) == DNSSD_OK) {
        rc = DNSSD_Resolve(NSD.ds_Session.bs_Session, name, type,
            &NSD.ds_Resolve, DNSSD_Timeout, 10, TAG_DONE);
    }
    if (rc != DNSSD_OK) {
        NSD.ds_Resolve = NULL;
        ns_status_error(SDMsg(MSG_NS_REACH_FAILED), rc);
    } else {
        NSD.ds_ResolveID = DNSSD_OperationID(NSD.ds_Resolve);
        SDFormat(NSD.ds_Status, (LONG)sizeof(NSD.ds_Status),
            SDMsg(MSG_NS_REACHING), name);
    }
    return NS_CHANGED_RESULT | NS_CHANGED_STATE | NS_CHANGED_STATUS;
}

static void
ns_reached(const struct DNSSDEvent *e)
{
    struct DNSSDAddress addresses[SD_ADDRESSES];
    ULONG count = SDEventAddresses(e, addresses, SD_ADDRESSES), i;
    const UBYTE *txt = SDEventPart(e, &e->de_TXT);
    LONG length = (LONG)e->de_TXT.dr_Size, pos = 0;
    char text[160], part[128];

    SDEventName(e, &e->de_Target, part, (LONG)sizeof(part));
    SDFormat(text, (LONG)sizeof(text), SDMsg(MSG_NS_HOST), part);
    ns_line((CONST_STRPTR)text);
    for (i = 0; i < count; i++) {
        SDAddressText(&addresses[i], part, (LONG)sizeof(part));
        SDFormat(text, (LONG)sizeof(text), SDMsg(MSG_NS_ADDRESS), part);
        ns_line((CONST_STRPTR)text);
    }
    SDFormat(text, (LONG)sizeof(text), SDMsg(MSG_NS_PORT), (LONG)e->de_Port);
    ns_line((CONST_STRPTR)text);
    /* the TXT data a line for each string; one empty string is none */
    while (txt != NULL && length > 1 && pos < length) {
        LONG n = txt[pos];

        if (n > length - pos - 1) {
            break;
        }
        SDTXTText(txt + pos, n + 1, part, (LONG)sizeof(part));
        SDFormat(text, (LONG)sizeof(text),
            pos == 0 ? SDMsg(MSG_NS_INFO) : SDMsg(MSG_NS_INFO_MORE), part);
        ns_line((CONST_STRPTR)text);
        pos += n + 1;
    }
}

static ULONG
ns_resolve_event(const struct DNSSDEvent *e)
{
    if (e->de_Kind == DNSSD_EVENT_RESOLVED) {
        ns_free_list(&NSD.ds_Result);
        ns_reached(e);
        DNSSD_Cancel(NSD.ds_Resolve);
        NSD.ds_Resolve = NULL;
        SDFormat(NSD.ds_Status, (LONG)sizeof(NSD.ds_Status),
            SDMsg(MSG_NS_REACHED), NSD.ds_ResolveName);
        return NS_CHANGED_RESULT | NS_CHANGED_STATE | NS_CHANGED_STATUS;
    }
    if (e->de_Kind == DNSSD_EVENT_END) {
        NSD.ds_Resolve = NULL;      /* over: nothing to cancel */
        SDFormat(NSD.ds_Status, (LONG)sizeof(NSD.ds_Status),
            SDMsg(MSG_NS_NOT_REACHED), NSD.ds_ResolveName);
        return NS_CHANGED_STATE | NS_CHANGED_STATUS;
    }
    return 0;
}

/*------------------------------------------------------------------------*/
/* Share */

ULONG
NSShare(CONST_STRPTR name, CONST_STRPTR type, LONG port, CONST_STRPTR info)
{
    static UBYTE txt[1300];         /* off a 68k task's small stack */
    static char words[400];
    char *items[NS_SHARE_ITEMS + 1];
    LONG n = 0, i, length, rc;

    NSUnshare();
    if (port < 1 || port > 65535) {
        ns_status(SDMsg(MSG_NS_BAD_PORT));
        return NS_CHANGED_STATUS;
    }
    /* the items, each where a space ends the one before */
    SDStrCopy(words, (const char *)info, (LONG)sizeof(words));
    for (i = 0; words[i] != '\0'; i++) {
        if (words[i] == ' ') {
            words[i] = '\0';
        } else if ((i == 0 || words[i - 1] == '\0') && n < NS_SHARE_ITEMS) {
            items[n++] = &words[i];
        }
    }
    items[n] = NULL;
    if ((length = SDTXTFromItems(txt, (LONG)sizeof(txt), items)) < 0) {
        ns_status(SDMsg(MSG_NS_BAD_INFO));
        return NS_CHANGED_STATUS;
    }
    if ((rc = ns_session()) == DNSSD_OK) {
        if (length != 0) {
            rc = DNSSD_Register(NSD.ds_Session.bs_Session, name, type,
                (UWORD)port, &NSD.ds_Share,
                DNSSD_TXT, (IPTR)txt, DNSSD_TXTLength, length, TAG_DONE);
        } else {
            rc = DNSSD_Register(NSD.ds_Session.bs_Session, name, type,
                (UWORD)port, &NSD.ds_Share, TAG_DONE);
        }
    }
    if (rc != DNSSD_OK) {
        NSD.ds_Share = NULL;
        ns_status_error(SDMsg(MSG_NS_SHARE_FAILED), rc);
    } else {
        NSD.ds_ShareID = DNSSD_OperationID(NSD.ds_Share);
        NSD.ds_SharePort = (UWORD)port;
        SDStrCopy(NSD.ds_ShareType, (const char *)type,
            (LONG)sizeof(NSD.ds_ShareType));
        SDFormat(NSD.ds_Status, (LONG)sizeof(NSD.ds_Status),
            SDMsg(MSG_NS_SHARING), name);
    }
    return NS_CHANGED_STATE | NS_CHANGED_STATUS;
}

ULONG
NSUnshare(void)
{
    if (NSD.ds_Share == NULL) {
        return 0;
    }
    DNSSD_Cancel(NSD.ds_Share);
    NSD.ds_Share = NULL;
    ns_status(SDMsg(MSG_NS_UNSHARED));
    return NS_CHANGED_STATE | NS_CHANGED_STATUS;
}

static ULONG
ns_share_event(const struct DNSSDEvent *e)
{
    if (e->de_Kind == DNSSD_EVENT_REGISTERED) {
        const UBYTE *label = SDEventPart(e, &e->de_Instance);
        char as[64];
        ULONG i;

        /* the name it is announced under: another if a host had it */
        for (i = 0; label != NULL && i < e->de_Instance.dr_Size && i < 63; i++) {
            as[i] = (char)label[i];
        }
        as[i] = '\0';
        SDFormat(NSD.ds_Status, (LONG)sizeof(NSD.ds_Status),
            SDMsg(MSG_NS_SHARED), as, SDTypeFriendly((CONST_STRPTR)NSD.ds_ShareType),
            (LONG)NSD.ds_SharePort);
        return NS_CHANGED_STATUS;
    }
    if (e->de_Kind == DNSSD_EVENT_END) {
        NSD.ds_Share = NULL;
        ns_status_error(SDMsg(MSG_NS_SHARE_FAILED), e->de_Result);
        return NS_CHANGED_STATE | NS_CHANGED_STATUS;
    }
    return 0;
}

/*------------------------------------------------------------------------*/
/* The events */

ULONG
NSDiscoveryEvents(void)
{
    const struct DNSSDEvent *e;
    ULONG changed = 0;
    LONG rc;

    if (!NSD.ds_Open) {
        return 0;
    }
    /* the library's messages say there are events; they are not replied */
    while (GetMsg(NSD.ds_Session.bs_Port) != NULL) {
    }
    while ((e = SDSessionRead(&NSD.ds_Session, &rc)) != NULL) {
        if (NSD.ds_Resolve != NULL && e->de_Operation == NSD.ds_ResolveID) {
            changed |= ns_resolve_event(e);
        } else if (NSD.ds_Share != NULL && e->de_Operation == NSD.ds_ShareID) {
            changed |= ns_share_event(e);
        } else if (NSD.ds_Browsing) {
            changed |= ns_found(e);
        }
    }
    if (rc != DNSSD_ERR_EMPTY) {
        /* mdnsd restarted, or the session failed: all of it is over */
        ns_session_close();
        ns_status_error(SDMsg(MSG_NS_SESSION_FAILED), rc);
        changed |= NS_CHANGED_STATE | NS_CHANGED_STATUS;
    }
    return changed;
}
