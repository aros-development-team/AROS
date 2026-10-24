/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.
*/

#include <netmidi/text.h>
#include "netmidi/ports.h"

#include <string.h>

void netmidi_ports_init(struct netmidi_ports *ports, int count)
{
    memset(ports, 0, sizeof(*ports));
    ports->count = count < 0 ? 0 :
                   count > NETMIDI_PORTS_MAX ? NETMIDI_PORTS_MAX : count;
}

static int kept_for(const struct netmidi_ports *ports, const uint8_t *name,
                    size_t length)
{
    int port;

    if (!name || !length)
        return -1;
    /* kept[port] has MAX + 1 bytes: text_length also reads text[MAX]. */
    for (port = 0; port < ports->count; ++port)
        if (netmidi_text_length(ports->kept[port],
                                  NETMIDI_PORT_NAME_MAX) == length &&
            memcmp(ports->kept[port], name, length) == 0)
            return port;
    return -1;
}

int netmidi_ports_keep(struct netmidi_ports *ports, int port,
                         const char *name)
{
    size_t length;

    if (!ports || !name || port < 0 || port >= ports->count)
        return -1;
    /* name must include a readable byte at MAX unless terminated earlier. */
    length = netmidi_text_length(name, NETMIDI_PORT_NAME_MAX);
    if (!length || length > NETMIDI_PORT_NAME_MAX || ports->kept[port][0] ||
        kept_for(ports, (const uint8_t *)name, length) >= 0)
        return -1;
    memcpy(ports->kept[port], name, length + 1);
    return 0;
}

int netmidi_ports_take(struct netmidi_ports *ports, const uint8_t *name,
                         size_t name_length, int owner)
{
    int port;

    if (!ports || !owner)
        return -1;
    port = netmidi_ports_of(ports, owner);
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

int netmidi_ports_of(const struct netmidi_ports *ports, int owner)
{
    int port;

    if (!ports || !owner)
        return -1;
    for (port = 0; port < ports->count; ++port)
        if (ports->owner[port] == owner)
            return port;
    return -1;
}

int netmidi_ports_owner(const struct netmidi_ports *ports, int port)
{
    if (!ports || port < 0 || port >= ports->count)
        return 0;
    return ports->owner[port];
}

void netmidi_ports_release(struct netmidi_ports *ports, int owner)
{
    int port = netmidi_ports_of(ports, owner);

    if (port >= 0)
        ports->owner[port] = 0;
}
