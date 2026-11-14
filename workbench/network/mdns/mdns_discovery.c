/*
 * SPDX-License-Identifier: BSD-2-Clause
 * Copyright 2026 Jaime Dias
 *
 * mdns_discovery.c - bonami.library's browses and resolves, until it goes
 *
 * Its operations are views of mdnsd's one cache (mdns_cache.c): a browse
 * watches the PTRs of its type; each instance they name, the SRV and TXT of
 * that instance on that link and the A record of the host its SRV names.
 * The cache asks for them (RFC 6762 sections 5.2, 7.1) and tells when they
 * come and go. An instance is told to the browse (MEVENT_ADD) when its PTR,
 * SRV, TXT and an IPv4 address are all there, again when what it says
 * changes, and its MEVENT_REMOVE when it no longer is.
 *
 * DNS names remain counted labels: dots/backslashes in an instance never
 * become separators. All entry points run in mdnsd's task and none calls
 * client code. The one piece of client memory kept is a lent
 * MDNSEventRequest, which is only given back: er_Result is set and the
 * message is replied, once. No client event buffer, Hook or task pointer is
 * retained.
 */
#include <exec/memory.h>
#include <proto/exec.h>
#include <proto/utility.h>
#include <string.h>
#include <netinet/in.h>
#include "mdns_bonami.h"

#define DISC_VIEWS 32
#define DISC_SESSIONS 16
#define DISC_QUEUE 8

/* An instance on a link that a browse reaches, what it was last told as,
 * and the watches that keep its records. */
struct DiscoveryView
{
    UBYTE key[256];
    ULONG link;
    BOOL active, present;
    BOOL type;          /* key is a service type, from type enumeration */
    UWORD browsers;     /* sessions that received the current ADD */
    UWORD activeBrowsers;
    APTR browserHandles[DISC_SESSIONS];
    APTR activeHandles[DISC_SESSIONS];
    APTR srvWatch, txtWatch, addrWatch;
    struct MDNSDiscoveryEvent event;
};
struct DiscoverySession
{
    APTR handle;
    ULONG owner;
    UBYTE key[256];
    BOOL resolve, failed, resolved;
    ULONG deadline;
    UWORD head, count;
    struct MDNSDiscoveryEvent queue[DISC_QUEUE];
    struct MDNSEventRequest *request;   /* lent by the owner, or NULL */
    APTR watch;                         /* browse: its PTRs; resolve: SRV */
    APTR txtWatch, addrWatch;           /* resolve */
};
struct MDNSDiscovery
{
    struct DiscoveryView views[DISC_VIEWS];
    struct DiscoverySession sessions[DISC_SESSIONS];
    IPTR nextHandle;
    /* The views are rebuilt only when the cache or the operations changed. */
    BOOL changed;
};

/* RFC 6763 section 9: the PTRs of this name point at the service types
 * on the link, not at instances. */
static const UBYTE services_key[] = {
    9, '_', 's', 'e', 'r', 'v', 'i', 'c', 'e', 's',
    7, '_', 'd', 'n', 's', '-', 's', 'd',
    4, '_', 'u', 'd', 'p',
    5, 'l', 'o', 'c', 'a', 'l', 0
};

static BOOL due(ULONG now, ULONG deadline)
{
    return (LONG)(now - deadline) >= 0;
}
static UBYTE lower(UBYTE c)
{
    return c >= 'A' && c <= 'Z' ? c + ('a' - 'A') : c;
}
static LONG key_length(const UBYTE *key)
{
    LONG at = 0;

    while (at < 255 && key[at] != 0) {
        at += key[at] + 1;
    }
    return at < 256 ? at + 1 : 0;
}
static BOOL same_key(const UBYTE *a, const UBYTE *b)
{
    LONG length = key_length(a), i;

    if (length == 0 || length != key_length(b)) {
        return FALSE;
    }
    for (i = 0; i < length; i++) {
        if (lower(a[i]) != lower(b[i])) {
            return FALSE;
        }
    }
    return TRUE;
}

static BOOL type_key(const char *type, UBYTE *key)
{
    LONG length = mdnsTextLength(type, MDNS_MAX_TYPE + 1), split = 0, i;

    while (split < length && type[split] != '.') {
        split++;
    }
    if (length > MDNS_MAX_TYPE || split < 2 || split > 16 ||
        type[0] != '_' || (Stricmp(type + split, "._udp") != 0 &&
        Stricmp(type + split, "._tcp") != 0)) {
        return FALSE;
    }
    for (i = 1; i < split; i++) {
        UBYTE c = lower((UBYTE)type[i]);

        if (!(c >= 'a' && c <= 'z') && !(c >= '0' && c <= '9') && c != '-') {
            return FALSE;
        }
    }
    key[0] = split;
    memcpy(key + 1, type, split);
    key[split + 1] = 4;
    memcpy(key + split + 2, type + split + 1, 4);
    key[split + 6] = 5;
    memcpy(key + split + 7, "local", 6);
    return TRUE;
}

/* Browse accepts either a base type or one RFC 6763 subtype owner such as
 * "_printer._sub._ipp._tcp". The event still reports the base type. */
static BOOL browse_key(const char *type, UBYTE *key)
{
    UBYTE base[256];
    LONG length, split = 0, i, baseLength;

    if (type_key(type, key)) {
        return TRUE;
    }
    length = mdnsTextLength(type, MDNS_MAX_BROWSE_TYPE + 1);
    while (split < length && type[split] != '.') {
        split++;
    }
    if (length > MDNS_MAX_BROWSE_TYPE || split < 2 || split > 63 ||
        type[0] != '_' || Strnicmp(type + split, "._sub.", 6) != 0 ||
        !type_key(type + split + 6, base)) {
        return FALSE;
    }
    for (i = 1; i < split; i++) {
        UBYTE c = lower((UBYTE)type[i]);

        if (!(c >= 'a' && c <= 'z') && !(c >= '0' && c <= '9') &&
            c != '-') {
            return FALSE;
        }
    }
    baseLength = key_length(base);
    if (baseLength == 0 || split + 6 + baseLength > 256) {
        return FALSE;
    }
    key[0] = split;
    memcpy(key + 1, type, split);
    key[split + 1] = 4;
    memcpy(key + split + 2, "_sub", 4);
    memcpy(key + split + 6, base, baseLength);
    return TRUE;
}

/*
 * Gives a lent request back, exactly once. A request whose owner has gone
 * is dropped unread instead: that owner broke the contract, and its port
 * may be gone too. This check narrows, but cannot close, the window in
 * which an owner ends between it and ReplyMsg(); owners must stop first.
 */
static void wake(struct DiscoverySession *s, LONG result)
{
    struct MDNSEventRequest *request = s->request;

    if (request == NULL) {
        return;
    }
    s->request = NULL;
    if (FindTaskByPID(s->owner) == NULL) {
        return;
    }
    request->er_Result = result;
    ReplyMsg(&request->er_Msg);
}

/* A record an operation watches came or went: look again. */
static void changed(APTR data, const struct MDNSCacheEntry *e, BOOL added)
{
    (void)e;
    (void)added;
    ((struct MDNSDiscovery *)data)->changed = TRUE;
}

static void unwatch(struct MDNSDaemon *md, APTR *w)
{
    if (*w != NULL) {
        mdnsCacheUnwatch(md, *w);
        *w = NULL;
    }
}

static void watch(struct MDNSDaemon *md, APTR *w, const UBYTE *name,
    UWORD type, ULONG link)
{
    if (*w == NULL) {
        *w = mdnsCacheWatch(md, name, type, link, changed, md->md_Discovery);
    }
}

/* The address watch follows the host the latest SRV names, if any. */
static void watch_host(struct MDNSDaemon *md, APTR *addrWatch,
    const UBYTE *key, ULONG link)
{
    const struct MDNSCacheEntry *srv =
        mdnsCacheLatest(md, key, DNS_TYPE_SRV, link);

    if (*addrWatch != NULL && (srv == NULL ||
            !same_key(mdnsCacheWatchName(*addrWatch), srv->ce_Data + 6))) {
        unwatch(md, addrWatch);
    }
    if (srv != NULL) {
        watch(md, addrWatch, srv->ce_Data + 6, DNS_TYPE_A, link);
    }
}

static void view_clear(struct MDNSDaemon *md, struct DiscoveryView *v)
{
    unwatch(md, &v->srvWatch);
    unwatch(md, &v->txtWatch);
    unwatch(md, &v->addrWatch);
    memset(v, 0, sizeof(*v));
}

static void session_unwatch(struct MDNSDaemon *md, struct DiscoverySession *s)
{
    unwatch(md, &s->watch);
    unwatch(md, &s->txtWatch);
    unwatch(md, &s->addrWatch);
}

static void session_end(struct MDNSDaemon *md, struct DiscoverySession *s)
{
    session_unwatch(md, s);
    memset(s, 0, sizeof(*s));
}

/* An operation that ran out of room: the caller must stop and begin it
 * again. Nothing else is failed with it. */
static void fail(struct MDNSDaemon *md, struct DiscoverySession *s)
{
    if (s->handle == NULL || s->failed) {
        return;
    }
    s->failed = TRUE;
    s->count = 0;
    session_unwatch(md, s);
    md->md_Discovery->changed = TRUE;
    wake(s, MDNSERR_BUSY);
}

/* Whether an operation still wants records: a browse until it is stopped
 * or failed, a resolve until it has its answer. */
static BOOL live(const struct DiscoverySession *s)
{
    return s->handle != NULL && !s->failed && !(s->resolve && s->resolved);
}

static void enqueue(struct MDNSDaemon *md, struct DiscoverySession *s,
    const struct MDNSDiscoveryEvent *event)
{
    UWORD i;

    md->md_DiscoveryNotify++;
    if (s->failed) {
        return;
    }
    /* A pending snapshot for the same instance/link is replaced atomically.
     * Overflow is explicit: the caller must stop and resubscribe. */
    for (i = 0; i < s->count; i++) {
        struct MDNSDiscoveryEvent *old = &s->queue[(s->head + i) % DISC_QUEUE];

        if (old->de_Interface == event->de_Interface &&
            Stricmp(old->de_Name, event->de_Name) == 0 &&
            Stricmp(old->de_Type, event->de_Type) == 0) {
            *old = *event;
            wake(s, MDNS_OK);
            return;
        }
    }
    if (s->count == DISC_QUEUE) {
        s->count = 0;
        s->failed = TRUE;
        wake(s, MDNSERR_BUSY);
        return;
    }
    s->queue[(s->head + s->count++) % DISC_QUEUE] = *event;
    wake(s, MDNS_OK);
}

static void notify(struct MDNSDaemon *md, const struct DiscoveryView *view,
    ULONG code, UWORD browsers)
{
    struct MDNSDiscovery *d = md->md_Discovery;
    LONG i;
    APTR const *handles = code == MEVENT_ADD ?
        view->activeHandles : view->browserHandles;
    struct MDNSDiscoveryEvent event = view->event;

    event.de_Code = code;
    for (i = 0; i < DISC_SESSIONS; i++) {
        struct DiscoverySession *s = &d->sessions[i];

        if ((browsers & (1U << i)) != 0 && s->handle != NULL &&
            s->handle == handles[i] && !s->resolve) {
            enqueue(md, s, &event);
        }
    }
}

static UWORD browse_sessions(const struct MDNSDiscovery *d,
    const UBYTE *type)
{
    LONG i;
    UWORD result = 0;

    for (i = 0; i < DISC_SESSIONS; i++) {
        const struct DiscoverySession *s = &d->sessions[i];

        if (live(s) && !s->resolve && same_key(s->key, type)) {
            result |= (UWORD)(1U << i);
        }
    }
    return result;
}
static ULONG interface_address4(const struct MDNSDaemon *md, ULONG link)
{
    LONG i;

    for (i = 0; i < md->md_IfCount; i++) {
        if (md->md_Ifs[i].mi_Index == link) {
            return mdnsNetAddress4(md, i);
        }
    }
    return 0;
}

static BOOL is_services(const UBYTE *key)
{
    return same_key(key, services_key);
}

/* A service type as labels, "_http", "_tcp", "local". */
static BOOL valid_type(const UBYTE *key)
{
    UBYTE expected[256];
    char type[MDNS_MAX_TYPE + 1];
    LONG n = key[0];

    if (n < 2 || n + 5 > MDNS_MAX_TYPE || key[n + 1] != 4) {
        return FALSE;
    }
    memcpy(type, key + 1, n);
    type[n] = '.';
    memcpy(type + n + 1, key + n + 2, 4);
    type[n + 5] = '\0';
    return type_key(type, expected) && same_key(expected, key);
}

/* Return the base type portion of a valid subtype owner, or NULL. */
static const UBYTE *subtype_base(const UBYTE *key)
{
    LONG i;
    const UBYTE *sub;
    const UBYTE *base;

    if (key[0] < 2 || key[0] > 63 || key[1] != '_') {
        return NULL;
    }
    for (i = 2; i <= key[0]; i++) {
        UBYTE c = lower(key[i]);

        if (!(c >= 'a' && c <= 'z') && !(c >= '0' && c <= '9') &&
            c != '-') {
            return NULL;
        }
    }
    sub = key + key[0] + 1;
    if (sub[0] != 4 || lower(sub[1]) != '_' || lower(sub[2]) != 's' ||
        lower(sub[3]) != 'u' || lower(sub[4]) != 'b') {
        return NULL;
    }
    base = sub + 5;
    return valid_type(base) ? base : NULL;
}

static BOOL ptr_owner_matches(const UBYTE *owner, const UBYTE *instance)
{
    const UBYTE *base = instance + instance[0] + 1;
    const UBYTE *subtype = subtype_base(owner);

    return same_key(owner, base) ||
        (subtype != NULL && same_key(subtype, base));
}

/* What a type found by enumeration is told as: its type, no instance. */
static void type_event(const struct MDNSDaemon *md, const UBYTE *key,
    ULONG link, struct MDNSDiscoveryEvent *out)
{
    LONG n = key[0];

    memset(out, 0, sizeof(*out));
    memcpy(out->de_Type, key + 1, n);
    out->de_Type[n] = '.';
    memcpy(out->de_Type + n + 1, key + n + 2, 4);
    out->de_Interface = htonl(interface_address4(md, link));
}

static BOOL valid_instance(const UBYTE *key)
{
    if (key[0] == 0 || key[0] > 63 || memchr(key + 1, 0, key[0]) != NULL) {
        return FALSE;
    }
    return valid_type(key + key[0] + 1);
}

/* Does a PTR of owner on link name instance? */
static BOOL ptr_targets(struct MDNSDaemon *md, const UBYTE *owner,
    const UBYTE *instance, ULONG link)
{
    const struct MDNSCacheEntry *e = NULL;

    while ((e = mdnsCacheFind(md, e, owner, DNS_TYPE_PTR, link)) != NULL) {
        if (same_key(e->ce_Data, instance)) {
            return TRUE;
        }
    }
    return FALSE;
}

/* TXT data as RFC 6763 section 6 has it - strings, one at least, that fill
 * it exactly - and no more than an event holds. */
static BOOL txt_valid(const struct MDNSCacheEntry *txt)
{
    LONG at = 0;

    if (txt->ce_DataLength == 0 ||
        txt->ce_DataLength > MDNS_DISCOVERY_TXT_MAX) {
        return FALSE;
    }
    while (at < txt->ce_DataLength) {
        at += txt->ce_Data[at] + 1;
    }
    return at == txt->ce_DataLength;
}

static BOOL snapshot(struct MDNSDaemon *md, const UBYTE *key, ULONG link,
    struct MDNSDiscoveryEvent *out)
{
    const struct MDNSCacheEntry *srv =
        mdnsCacheLatest(md, key, DNS_TYPE_SRV, link);
    const struct MDNSCacheEntry *txt =
        mdnsCacheLatest(md, key, DNS_TYPE_TXT, link);
    const struct MDNSCacheEntry *addr;
    const UBYTE *type = key + key[0] + 1;
    LONG n;

    if (srv == NULL || txt == NULL || key[0] == 0 || !txt_valid(txt)) {
        return FALSE;
    }
    addr = mdnsCacheLatest(md, srv->ce_Data + 6, DNS_TYPE_A, link);
    if (addr == NULL || addr->ce_DataLength != 4 ||
        (srv->ce_Data[4] == 0 && srv->ce_Data[5] == 0)) {
        return FALSE;
    }
    memset(out, 0, sizeof(*out));
    memcpy(out->de_Name, key + 1, key[0]);
    n = type[0];
    if (n + 5 > MDNS_MAX_TYPE || type[n + 1] != 4) {
        return FALSE;
    }
    memcpy(out->de_Type, type + 1, n);
    out->de_Type[n] = '.';
    memcpy(out->de_Type + n + 1, type + n + 2, 4);
    out->de_Port = ((UWORD)srv->ce_Data[4] << 8) | srv->ce_Data[5];
    memcpy(&out->de_Address, addr->ce_Data, 4);
    out->de_Interface = htonl(interface_address4(md, link));
    out->de_TXTLength = txt->ce_DataLength;
    memcpy(out->de_TXT, txt->ce_Data, txt->ce_DataLength);
    return TRUE;
}

/* A PTR a browse can be told of: one of _services._dns-sd._udp naming a
 * type, or one of a type (or a subtype of it) naming an instance of it. */
static BOOL ptr_valid(const struct MDNSCacheEntry *e)
{
    UBYTE target[256];

    if (e->ce_DataLength > sizeof(target)) {
        return FALSE;
    }
    memset(target, 0, sizeof(target));
    memcpy(target, e->ce_Data, e->ce_DataLength);
    return is_services(e->ce_Name) ? valid_type(target) :
        valid_instance(target) && ptr_owner_matches(e->ce_Name, target);
}

static struct DiscoveryView *view_of(struct MDNSDiscovery *d,
    const UBYTE *instance, ULONG link)
{
    LONG j;

    for (j = 0; j < DISC_VIEWS; j++) {
        if (d->views[j].key[0] != 0 && d->views[j].link == link &&
            same_key(d->views[j].key, instance)) {
            return &d->views[j];
        }
    }
    return NULL;
}

static void release_type_views(struct MDNSDaemon *md, const UBYTE *type)
{
    struct MDNSDiscovery *d = md->md_Discovery;
    LONG i;
    BOOL services = is_services(type);

    for (i = 0; i < DISC_VIEWS; i++) {
        struct DiscoveryView *view = &d->views[i];

        if (view->key[0] != 0 && view->type == services &&
            (services || ptr_targets(md, type, view->key, view->link))) {
            view_clear(md, view);
        }
    }
}

static void refresh_views(struct MDNSDaemon *md)
{
    struct MDNSDiscovery *d = md->md_Discovery;
    const struct MDNSCacheEntry *r;
    LONG i, j;

    /* Which views the browsed PTRs still have; the others go first, so
     * that their room can be taken by new instances. */
    for (i = 0; i < DISC_VIEWS; i++) {
        d->views[i].active = FALSE;
        d->views[i].activeBrowsers = 0;
        memset(d->views[i].activeHandles, 0,
            sizeof(d->views[i].activeHandles));
    }
    for (r = NULL; (r = mdnsCacheFind(md, r, NULL, DNS_TYPE_PTR, 0)) != NULL; ) {
        struct DiscoveryView *view;
        UWORD browsers = browse_sessions(d, r->ce_Name);

        if (browsers != 0 && ptr_valid(r) &&
            (view = view_of(d, r->ce_Data, r->ce_Link)) != NULL) {
            view->active = TRUE;
            view->activeBrowsers |= browsers;
            for (j = 0; j < DISC_SESSIONS; j++) {
                if ((browsers & (1U << j)) != 0) {
                    view->activeHandles[j] = d->sessions[j].handle;
                }
            }
        }
    }
    for (i = 0; i < DISC_VIEWS; i++) {
        struct DiscoveryView *v = &d->views[i];

        if (v->key[0] != 0 && !v->active) {
            if (v->present) {
                notify(md, v, MEVENT_REMOVE, v->browsers);
            }
            view_clear(md, v);
        }
    }
    for (r = NULL; (r = mdnsCacheFind(md, r, NULL, DNS_TYPE_PTR, 0)) != NULL; ) {
        struct DiscoveryView *view;
        UWORD browsers = browse_sessions(d, r->ce_Name);

        if (browsers == 0 || !ptr_valid(r) ||
            view_of(d, r->ce_Data, r->ce_Link) != NULL) {
            continue;
        }
        for (view = NULL, j = 0; j < DISC_VIEWS; j++) {
            if (d->views[j].key[0] == 0) {
                LONG k;

                view = &d->views[j];
                memcpy(view->key, r->ce_Data, r->ce_DataLength);
                view->link = r->ce_Link;
                view->active = TRUE;
                view->activeBrowsers = browsers;
                for (k = 0; k < DISC_SESSIONS; k++) {
                    if ((browsers & (1U << k)) != 0) {
                        view->activeHandles[k] = d->sessions[k].handle;
                    }
                }
                view->type = is_services(r->ce_Name);
                break;
            }
        }
        if (view == NULL) {
            /* More instances of the type than there are views: those who
             * browse it are failed, once, and its records are no longer
             * watched; nobody else is touched. */
            for (j = 0; j < DISC_SESSIONS; j++) {
                struct DiscoverySession *s = &d->sessions[j];

                if (live(s) && !s->resolve && same_key(s->key, r->ce_Name)) {
                    fail(md, s);
                }
            }
            /* The failed type no longer has a live browse. Return its views
             * now so a later PTR in this rebuild can use them. */
            release_type_views(md, r->ce_Name);
        }
    }
    for (i = 0; i < DISC_VIEWS; i++) {
        struct DiscoveryView *v = &d->views[i];
        struct MDNSDiscoveryEvent event;
        BOOL present, differs;
        UWORD added = 0, removed = 0;

        if (v->key[0] == 0) {
            continue;
        }
        if (v->type) {
            type_event(md, v->key, v->link, &event);
            present = TRUE;
        } else {
            /* an instance's records are asked for and kept while it is
             * seen */
            watch(md, &v->srvWatch, v->key, DNS_TYPE_SRV, v->link);
            watch(md, &v->txtWatch, v->key, DNS_TYPE_TXT, v->link);
            watch_host(md, &v->addrWatch, v->key, v->link);
            present = snapshot(md, v->key, v->link, &event);
        }
        differs = present && v->present &&
            memcmp(&event, &v->event, sizeof(event)) != 0;
        for (j = 0; present && v->present && !differs &&
                j < DISC_SESSIONS; j++) {
            BOOL was = (v->browsers & (1U << j)) != 0;
            BOOL is = (v->activeBrowsers & (1U << j)) != 0;

            if (was && (!is || v->browserHandles[j] !=
                    v->activeHandles[j])) {
                removed |= (UWORD)(1U << j);
            }
            if (is && (!was || v->browserHandles[j] !=
                    v->activeHandles[j])) {
                added |= (UWORD)(1U << j);
            }
        }
        if (v->present && (!present || differs)) {
            notify(md, v, MEVENT_REMOVE, v->browsers);
        } else if (removed != 0) {
            notify(md, v, MEVENT_REMOVE, removed);
        }
        if (present && (!v->present || differs)) {
            v->event = event;
            notify(md, v, MEVENT_ADD, v->activeBrowsers);
        } else if (added != 0) {
            notify(md, v, MEVENT_ADD, added);
        }
        v->present = present;
        v->browsers = v->activeBrowsers;
        memcpy(v->browserHandles, v->activeHandles,
            sizeof(v->browserHandles));
    }
}

/* Operations ended: the views only they reached go now, with their
 * watches, before the cache's next turn would ask for them. */
static void settle(struct MDNSDaemon *md)
{
    refresh_views(md);
    md->md_Discovery->changed = TRUE;   /* and the resolves, in the turn */
}

/* After the cache's turn (mdnsCacheTick()), which has told what came and
 * went. */
void mdnsDiscoveryTick(struct MDNSDaemon *md)
{
    struct MDNSDiscovery *d = md->md_Discovery;
    ULONG now = mdnsTicks();
    LONG i, j;
    BOOL rebuild;

    if (d == NULL) {
        return;
    }
    /* This runs on every turn of mdnsd's loop. The views and resolutions
     * are looked at again only when something they depend on changed. */
    rebuild = d->changed;
    if (rebuild) {
        d->changed = FALSE;
        refresh_views(md);
    }
    for (i = 0; i < DISC_SESSIONS; i++) {
        struct DiscoverySession *s = &d->sessions[i];
        struct MDNSDiscoveryEvent event;

        if (s->handle == NULL || !s->resolve || s->resolved || s->failed) {
            continue;
        }
        if (rebuild) {
            watch_host(md, &s->addrWatch, s->key, 0);
            for (j = 0; j < md->md_IfCount; j++) {
                if (snapshot(md, s->key, md->md_Ifs[j].mi_Index, &event)) {
                    event.de_Code = MEVENT_RESOLVED;
                    enqueue(md, s, &event);
                    s->resolved = TRUE;
                    break;
                }
            }
        }
        if (!s->resolved && due(now, s->deadline)) {
            memset(&event, 0, sizeof(event));
            event.de_Code = MEVENT_RESOLVE_FAILED;
            enqueue(md, s, &event);
            s->resolved = TRUE;
        }
        if (s->resolved) {
            session_unwatch(md, s);
        }
    }
}

LONG mdnsDiscoveryMessage(struct MDNSDaemon *md, struct MDNSIPCMsg *msg)
{
    struct MDNSDiscovery *d = md->md_Discovery;
    struct DiscoverySession *s = NULL;
    LONG i;
    BOOL create = msg->mi_Command == MDNS_IPC_BROWSE || msg->mi_Command == MDNS_IPC_RESOLVE;

    if (!create && msg->mi_DaemonID != md->md_DaemonID) {
        return MDNSERR_NOENGINE;
    }
    if (create && d == NULL) {
        d = AllocMem(sizeof(*d), MEMF_CLEAR);
        if (d == NULL) {
            return MDNSERR_NOMEM;
        }
        d->nextHandle = 1;
        md->md_Discovery = d;
    }
    if (d == NULL) {
        return MDNSERR_BADPARAM;
    }
    if (create) {
        LONG owned = 0;

        for (i = 0; i < DISC_SESSIONS; i++) {
            if (d->sessions[i].handle != NULL && d->sessions[i].owner == msg->mi_TaskID) {
                owned++;
            }
        }
        if (owned >= 4) {
            return MDNSERR_BUSY;
        }
        for (i = 0; i < DISC_SESSIONS; i++) {
            if (d->sessions[i].handle == NULL) {
                s = &d->sessions[i];
                break;
            }
        }
        if (s == NULL || d->nextHandle == 0) {
            return MDNSERR_BUSY;
        }
        memset(s, 0, sizeof(*s));
        s->resolve = msg->mi_Command == MDNS_IPC_RESOLVE;
        if (!s->resolve && Stricmp(msg->mi_ServiceType,
                MDNS_TYPE_SERVICES_DNS_SD) == 0) {
            /* type enumeration: the types on the link, each once */
            memcpy(s->key, services_key, sizeof(services_key));
        } else if (s->resolve ?
                !type_key(msg->mi_ServiceType, s->key) :
                !browse_key(msg->mi_ServiceType, s->key)) {
            return MDNSERR_BADTYPE;
        }
        if (s->resolve) {
            LONG length = mdnsTextLength(msg->mi_Name, 64);
            LONG typeLen = key_length(s->key);

            if (length == 0 || length > 63) {
                return MDNSERR_BADPARAM;
            }
            memmove(s->key + length + 1, s->key, typeLen);
            s->key[0] = length;
            memcpy(s->key + 1, msg->mi_Name, length);
        }
        s->owner = msg->mi_TaskID;
        s->handle = (APTR)d->nextHandle++;
        s->deadline = mdnsTicks() + 250; /* bounded asynchronous resolution */
        msg->mi_Handle = s->handle;
        msg->mi_DaemonID = md->md_DaemonID;
        d->changed = TRUE;
        /* this host's own services are in the cache as it announces them;
         * once more, so that all it announced is there */
        mdnsRegAnswerLocal(md, MDNS_REFLECT_DISCOVERY, NULL);
        /* the cache asks for what the operation wants, and keeps it */
        if (s->resolve) {
            watch(md, &s->watch, s->key, DNS_TYPE_SRV, 0);
            watch(md, &s->txtWatch, s->key, DNS_TYPE_TXT, 0);
        } else {
            watch(md, &s->watch, s->key, DNS_TYPE_PTR, 0);
        }
        for (i = 0; i < DISC_VIEWS; i++) {
            const struct DiscoveryView *v = &d->views[i];

            if (!s->resolve && v->present &&
                (v->type ? same_key(s->key, services_key) :
                    ptr_targets(md, s->key, v->key, v->link))) {
                struct MDNSDiscoveryEvent event = v->event;

                event.de_Code = MEVENT_ADD;
                enqueue(md, s, &event);
            }
        }
        return MDNS_OK;
    }
    for (i = 0; i < DISC_SESSIONS; i++) {
        if (d->sessions[i].handle == msg->mi_Handle &&
            d->sessions[i].owner == msg->mi_TaskID) {
            s = &d->sessions[i];
            break;
        }
    }
    if (s == NULL) {
        return MDNSERR_BADPARAM;
    }
    if (msg->mi_Command == MDNS_IPC_READ) {
        if (s->failed) {
            return MDNSERR_BUSY;
        }
        if (s->count == 0) {
            return MDNSERR_NOTREADY;
        }
        msg->mi_Event = s->queue[s->head];
        s->head = (s->head + 1) % DISC_QUEUE;
        s->count--;
        return MDNS_OK;
    }
    if (msg->mi_Command == MDNS_IPC_NOTIFY) {
        /* One lent request per operation. What is already waiting
         * answers it at once, before this call's own reply. */
        if (msg->mi_Request == NULL || s->request != NULL) {
            return MDNSERR_BADPARAM;
        }
        s->request = msg->mi_Request;
        if (s->failed) {
            wake(s, MDNSERR_BUSY);
        } else if (s->count != 0) {
            wake(s, MDNS_OK);
        }
        return MDNS_OK;
    }
    if ((msg->mi_Command == MDNS_IPC_STOPRESOLVE) != s->resolve) {
        return MDNSERR_BADPARAM;
    }
    /* The request goes back before the stop's own reply, so the owner has
     * it again when StopBrowse()/StopResolve() returns. */
    wake(s, MDNSERR_CANCELLED);
    session_end(md, s);
    settle(md);
    return MDNS_OK;
}

/* mdnsd is ending: every lent request goes back with MDNSERR_NOENGINE. */
void mdnsDiscoveryCancel(struct MDNSDaemon *md)
{
    LONG i;

    if (md->md_Discovery == NULL) {
        return;
    }
    for (i = 0; i < DISC_SESSIONS; i++) {
        wake(&md->md_Discovery->sessions[i], MDNSERR_NOENGINE);
    }
}

/* The owner's last close of the library. A request it still lent is
 * dropped unread, like one of a vanished task: it should have stopped the
 * operation first, and its reply port may already be gone. */
void mdnsDiscoveryRelease(struct MDNSDaemon *md, ULONG owner)
{
    LONG i;

    if (md->md_Discovery == NULL) {
        return;
    }
    for (i = 0; i < DISC_SESSIONS; i++) {
        if (md->md_Discovery->sessions[i].owner == owner) {
            session_end(md, &md->md_Discovery->sessions[i]);
        }
    }
    settle(md);
}

/* On mdnsd's reap timer: the operations of a task that ended without
 * stopping them go, a request it lent with them, unread. */
void mdnsDiscoveryReap(struct MDNSDaemon *md)
{
    struct MDNSDiscovery *d = md->md_Discovery;
    LONG i;

    if (d == NULL) {
        return;
    }
    for (i = 0; i < DISC_SESSIONS; i++) {
        struct DiscoverySession *s = &d->sessions[i];

        if (s->handle != NULL && FindTaskByPID(s->owner) == NULL) {
            session_end(md, s);
        }
    }
    settle(md);
}

/* The links went or changed: every instance told is withdrawn; what is
 * still there is told again, as the cache has it. */
void mdnsDiscoveryReset(struct MDNSDaemon *md)
{
    struct MDNSDiscovery *d = md->md_Discovery;
    LONG i;

    if (d == NULL) {
        return;
    }
    for (i = 0; i < DISC_VIEWS; i++) {
        if (d->views[i].present) {
            notify(md, &d->views[i], MEVENT_REMOVE,
                d->views[i].browsers);
        }
        view_clear(md, &d->views[i]);
    }
    d->changed = TRUE;
}

/* At the end, before the cache goes. */
void mdnsDiscoveryFree(struct MDNSDaemon *md)
{
    struct MDNSDiscovery *d = md->md_Discovery;
    LONG i;

    if (d == NULL) {
        return;
    }
    for (i = 0; i < DISC_VIEWS; i++) {
        view_clear(md, &d->views[i]);
    }
    for (i = 0; i < DISC_SESSIONS; i++) {
        session_unwatch(md, &d->sessions[i]);
    }
    FreeMem(d, sizeof(*d));
    md->md_Discovery = NULL;
}
