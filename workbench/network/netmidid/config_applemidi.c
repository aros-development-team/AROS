/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.
*/

#include <netmidi/config.h>

#include <errno.h>
#include <stdlib.h>
#include <string.h>

void netmidi_applemidi_config_defaults(struct netmidi_applemidi_config *c)
{
    memset(c, 0, sizeof(*c));
    c->local_port = 5004;
}

int netmidi_applemidi_config_key(struct netmidi_applemidi_config *c,
                                 const char *key, const char *value)
{
    if (strcmp(key, "local_port") == 0)
        return netmidi_config_port(value, &c->local_port) ? -1 : 1;
    if (strcmp(key, "peer_port") == 0)
        return netmidi_config_port(value, &c->peer_port) ? -1 : 1;
    if (strcmp(key, "peer_ip") == 0)
        return netmidi_config_take(c->peer_ip, sizeof(c->peer_ip),
                                   value) ? -1 : 1;
    if (strcmp(key, "peer_name") == 0)
        return !*value || netmidi_config_take(c->peer_name,
                                              sizeof(c->peer_name),
                                              value) ? -1 : 1;
    if (strcmp(key, "max_peers") == 0) {
        char *end;
        unsigned long peers;

        errno = 0;
        peers = strtoul(value, &end, 10);
        if (errno || end == value || *end || peers < 1 || peers > 8)
            return -1;
        c->max_peers = (int)peers;
        return 1;
    }
    if (strcmp(key, "accept") == 0)
        return !*value || netmidi_config_take(c->accept, sizeof(c->accept),
                                              value) ? -1 : 1;
    if (strncmp(key, "applemidi.port.", 15) == 0)
        return netmidi_config_port_name(key + 15, value,
                                        c->port_name) ? -1 : 1;
    return 0;
}

int netmidi_applemidi_config_check(const struct netmidi_applemidi_config *c)
{
    /* a peer to invite has an address and a port, or neither */
    return !!c->peer_ip[0] != !!c->peer_port ? -1 : 0;
}

void netmidi_applemidi_config_save(FILE *file,
                                   const struct netmidi_applemidi_config *c)
{
    int port;

    fprintf(file, "local_port=%u\n", (unsigned)c->local_port);
    if (c->peer_ip[0])
        fprintf(file, "peer_ip=%s\npeer_port=%u\n", c->peer_ip,
                (unsigned)c->peer_port);
    if (c->peer_name[0])
        fprintf(file, "peer_name=%s\n", c->peer_name);
    if (c->max_peers)
        fprintf(file, "max_peers=%d\n", c->max_peers);
    if (c->accept[0])
        fprintf(file, "accept=%s\n", c->accept);
    for (port = 0; port < 16; ++port)
        if (c->port_name[port][0])
            fprintf(file, "applemidi.port.%d=%s\n", port, c->port_name[port]);
}
