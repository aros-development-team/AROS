/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.
*/

#include "discovery.h"
#include "netmidid.h"

#include <stdio.h>
#include <string.h>

#include <exec/libraries.h>
#include <exec/memory.h>
#include <proto/exec.h>
#include <proto/bonami.h>
#include <bonami/mdns.h>

/* The library's calls take their base from this name. */
struct Library *BonamiBase;

void netmidi_discovery_init(struct netmidi_discovery *discovery)
{
    memset(discovery, 0, sizeof(*discovery));
}

void netmidi_discovery_begin(struct netmidi_discovery *discovery)
{
    int i;

    if (!discovery)
        return;
    for (i = 0; i < discovery->count; ++i)
        discovery->service[i].wanted = 0;
    /* what changed is seen to at once */
    discovery->last_attempt = 0;
}

/* Copies text, its end included, when that fits in size bytes. */
static void copy(char *to, const char *text, size_t size)
{
    const char *end = memchr(text, 0, size);

    if (end)
        memcpy(to, text, (size_t)(end - text) + 1);
}

int netmidi_discovery_add(struct netmidi_discovery *discovery,
                          const char *name, const char *type, uint16_t port)
{
    int i;

    if (!discovery || !name || !type || !*name || !*type || !port ||
        !memchr(name, 0, NETMIDI_DISCOVERY_NAME) ||
        !memchr(type, 0, NETMIDI_DISCOVERY_TYPE))
        return -1;
    for (i = 0; i < discovery->count; ++i)
        if (discovery->service[i].port == port &&
            strcmp(discovery->service[i].name, name) == 0 &&
            strcmp(discovery->service[i].type, type) == 0) {
            discovery->service[i].wanted = 1;
            return 0;
        }
    if (discovery->count == NETMIDI_DISCOVERY_SERVICES)
        return -1;
    memset(&discovery->service[discovery->count], 0,
           sizeof(discovery->service[0]));
    copy(discovery->service[discovery->count].name, name,
         NETMIDI_DISCOVERY_NAME);
    copy(discovery->service[discovery->count].type, type,
         NETMIDI_DISCOVERY_TYPE);
    discovery->service[discovery->count].port = port;
    discovery->service[discovery->count].wanted = 1;
    ++discovery->count;
    return 0;
}

/* The service went away: what it held for us went with it. */
static void forget(struct netmidi_discovery *discovery)
{
    int i;

    if (discovery->engine)
        CloseMDNSEngine();
    discovery->engine = 0;
    for (i = 0; i < discovery->count; ++i)
        discovery->service[i].reference = NULL;
}

/* What is no longer asked for is withdrawn, with its goodbye. */
static void withdraw_unwanted(struct netmidi_discovery *discovery)
{
    int i, kept = 0;

    for (i = 0; i < discovery->count; ++i) {
        if (!discovery->service[i].wanted) {
            if (discovery->service[i].reference && discovery->library)
                DeregisterService(discovery->service[i].reference);
            continue;
        }
        if (kept != i)
            discovery->service[kept] = discovery->service[i];
        ++kept;
    }
    discovery->count = kept;
}

static const char *refusal(long result)
{
    switch (result) {
    case MDNSERR_DUPLICATE:
        return "this machine already advertises that name";
    case MDNSERR_BADPARAM:
        return "the name is not one DNS label of at most 63 bytes "
               "without dots";
    case MDNSERR_NOMEM:
        return "the mDNS service is out of memory";
    default:
        return "the mDNS service refused it";
    }
}

int netmidi_discovery_poll(struct netmidi_discovery *discovery, uint64_t now)
{
    int registered = 0;
    int i;

    if (!discovery)
        return -1;
    withdraw_unwanted(discovery);
    for (i = 0; i < discovery->count; ++i)
        if (discovery->service[i].reference)
            ++registered;
    if (!discovery->count)
        return 0;
    if (discovery->last_attempt && now - discovery->last_attempt < 100000)
        return registered;
    discovery->last_attempt = now ? now : 1;

    if (!discovery->library) {
        discovery->library = OpenLibrary((CONST_STRPTR)"bonami.library", 40);
        BonamiBase = discovery->library;
        if (!discovery->library)
            return -1;
    }
    if (!discovery->engine) {
        if (OpenMDNSEngine() != MDNS_OK)
            return registered;      /* mdnsd is not running, yet */
        discovery->engine = 1;
    }
    /* A registration made is no proof that mdnsd is still the process
       that took it. An update that changes nothing asks it whether it
       still knows the handle, and sends nothing to the network; a daemon
       that was restarted between polls does not, and the services are
       registered again. Once in ten seconds at most. */
    for (i = 0; i < discovery->count; ++i) {
        struct TagItem unchanged[] = { { TAG_DONE, 0 } };
        LONG result;

        if (!discovery->service[i].reference)
            continue;
        result = UpdateServiceA(discovery->service[i].reference, unchanged);
        if (result == MDNSERR_NOENGINE || result == MDNSERR_NOTFOUND ||
            result == MDNSERR_BADPARAM) {
            forget(discovery);
            return 0;
        }
    }
    registered = 0;
    for (i = 0; i < discovery->count; ++i) {
        /* the service writes the registration where the tag says */
        struct TagItem tags[] = {
            { MDNS_ServiceName, (IPTR)discovery->service[i].name },
            { MDNS_ServiceType, (IPTR)discovery->service[i].type },
            { MDNS_Port, (IPTR)discovery->service[i].port },
            { MDNS_RegisterRef, (IPTR)&discovery->service[i].reference },
            { TAG_DONE, 0 }
        };
        LONG result;

        if (discovery->service[i].reference) {
            ++registered;
            continue;
        }
        result = RegisterServiceA(tags);
        if (result != MDNS_OK)
            discovery->service[i].reference = NULL;
        if (discovery->service[i].reference) {
            discovery->service[i].refused = 0;
            ++registered;
        } else if (result == MDNSERR_NOENGINE) {
            forget(discovery);
            return 0;
        } else if (result != discovery->service[i].refused) {
            /* said once for each reason, not at every attempt */
            discovery->service[i].refused = result;
            fprintf(stderr, TAG "\"%s\" (%s) is not advertised: %s\n",
                    discovery->service[i].name, discovery->service[i].type,
                    refusal(result));
        }
    }
    return registered;
}

void netmidi_discovery_close(struct netmidi_discovery *discovery)
{
    if (!discovery)
        return;
    if (discovery->library) {
        int i;

        BonamiBase = discovery->library;
        for (i = 0; i < discovery->count; ++i)
            if (discovery->service[i].reference)
                DeregisterService(discovery->service[i].reference);
        if (discovery->engine)
            CloseMDNSEngine();
        CloseLibrary(discovery->library);
    }
    memset(discovery, 0, sizeof(*discovery));
    BonamiBase = NULL;
}
