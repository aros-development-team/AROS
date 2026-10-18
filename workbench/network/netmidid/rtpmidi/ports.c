/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.
*/

#include "rtpmidi/ports.h"
#include "rtpmidi/applemidi.h"

#include <string.h>

void applemidi_ports_init(struct applemidi_ports *ports, int count)
{
    memset(ports, 0, sizeof(*ports));
    ports->count = count < 0 ? 0 :
                   count > APPLEMIDI_PORTS_MAX ? APPLEMIDI_PORTS_MAX : count;
}

static int kept_for(const struct applemidi_ports *ports, const uint8_t *name,
                    size_t length)
{
    int port;

    if (!name || !length)
        return -1;
    /* kept[port] has MAX + 1 bytes: text_length also reads text[MAX]. */
    for (port = 0; port < ports->count; ++port)
        if (applemidi_text_length(ports->kept[port],
                                  APPLEMIDI_PORT_NAME_MAX) == length &&
            memcmp(ports->kept[port], name, length) == 0)
            return port;
    return -1;
}

int applemidi_ports_keep(struct applemidi_ports *ports, int port,
                         const char *name)
{
    size_t length;

    if (!ports || !name || port < 0 || port >= ports->count)
        return -1;
    /* name must include a readable byte at MAX unless terminated earlier. */
    length = applemidi_text_length(name, APPLEMIDI_PORT_NAME_MAX);
    if (!length || length > APPLEMIDI_PORT_NAME_MAX || ports->kept[port][0] ||
        kept_for(ports, (const uint8_t *)name, length) >= 0)
        return -1;
    memcpy(ports->kept[port], name, length + 1);
    return 0;
}

int applemidi_ports_take(struct applemidi_ports *ports, const uint8_t *name,
                         size_t name_length, int owner)
{
    int port;

    if (!ports || !owner)
        return -1;
    port = applemidi_ports_of(ports, owner);
    if (port >= 0)
        return port;
    port = kept_for(ports, name, name_length);
    if (port >= 0) {
        if (ports->owner[port])
            return -1;
        ports->owner[port] = owner;
        return port;
    }
    for (port = 0; port < ports->count; ++port)
        if (!ports->kept[port][0] && !ports->owner[port]) {
            ports->owner[port] = owner;
            return port;
        }
    return -1;
}

int applemidi_ports_of(const struct applemidi_ports *ports, int owner)
{
    int port;

    if (!ports || !owner)
        return -1;
    for (port = 0; port < ports->count; ++port)
        if (ports->owner[port] == owner)
            return port;
    return -1;
}

int applemidi_ports_owner(const struct applemidi_ports *ports, int port)
{
    if (!ports || port < 0 || port >= ports->count)
        return 0;
    return ports->owner[port];
}

void applemidi_ports_release(struct applemidi_ports *ports, int owner)
{
    int port = applemidi_ports_of(ports, owner);

    if (port >= 0)
        ports->owner[port] = 0;
}
