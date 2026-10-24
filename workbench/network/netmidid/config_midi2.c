/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.
*/

#include <netmidi/config.h>

#include <string.h>

void netmidi_midi2_config_defaults(struct netmidi_midi2_config *c)
{
    memset(c, 0, sizeof(*c));
}

int netmidi_midi2_config_key(struct netmidi_midi2_config *c,
                             const char *key, const char *value)
{
    if (strcmp(key, "midi2_port") == 0)
        return netmidi_config_port(value, &c->port) ? -1 : 1;
    if (strcmp(key, "midi2_peer_port") == 0)
        return netmidi_config_port(value, &c->peer_port) ? -1 : 1;
    if (strcmp(key, "midi2_peer_ip") == 0)
        return !*value || netmidi_config_take(c->peer_ip, sizeof(c->peer_ip),
                                              value) ? -1 : 1;
    if (strcmp(key, "midi2_peer_name") == 0)
        return !*value || netmidi_config_take(c->peer_name,
                                              sizeof(c->peer_name),
                                              value) ? -1 : 1;
    if (strncmp(key, "networkmidi2.port.", 18) == 0)
        return netmidi_config_port_name(key + 18, value,
                                        c->port_name) ? -1 : 1;
    return 0;
}

int netmidi_midi2_config_check(const struct netmidi_midi2_config *c)
{
    /* a Host to invite has an address and a port, or neither, and both
       ways of naming it need a port of our own to be invited from */
    if (!!c->peer_ip[0] != !!c->peer_port || (c->peer_ip[0] && !c->port))
        return -1;
    if (c->peer_name[0] && !c->port)
        return -1;
    return 0;
}

void netmidi_midi2_config_save(FILE *file,
                               const struct netmidi_midi2_config *c)
{
    int port;

    if (c->port)
        fprintf(file, "midi2_port=%u\n", (unsigned)c->port);
    if (c->peer_ip[0])
        fprintf(file, "midi2_peer_ip=%s\nmidi2_peer_port=%u\n", c->peer_ip,
                (unsigned)c->peer_port);
    if (c->peer_name[0])
        fprintf(file, "midi2_peer_name=%s\n", c->peer_name);
    for (port = 0; port < 16; ++port)
        if (c->port_name[port][0])
            fprintf(file, "networkmidi2.port.%d=%s\n", port,
                    c->port_name[port]);
}
