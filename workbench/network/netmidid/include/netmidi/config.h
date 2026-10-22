/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.
*/

#ifndef NETMIDI_CONFIG_H
#define NETMIDI_CONFIG_H

#include <stddef.h>
#include <stdint.h>


/* What the service is told, a text file of "key = value" lines: the keys
 * every transport shares, and each transport's own. */
struct netmidi_network_config {
    char session_name[64];
    int session_name_given;     /* the file says it: not the default */
    /* "latency_ms": added to the time each received event is due, 0 to
     * 2000. */
    int latency_ms;
};

void netmidi_config_defaults(struct netmidi_network_config *config);

/* Returns 0 on success, 1 if the file does not exist, -1 on invalid input
 * or another I/O error. A failed parse leaves *config unchanged. */
int netmidi_config_load(const char *path, struct netmidi_network_config *config);

/* Writes every setting to path, in the form netmidi_config_load reads.
 * Returns 0, or -1 when the file could not be written whole. */
int netmidi_config_save(const char *path,
                        const struct netmidi_network_config *config);

/* For the transports' keys. Each returns 0, or -1 with nothing changed. */
/* text into a field of size bytes, its zero byte with it */
int netmidi_config_take(char *field, size_t size, const char *text);
/* a UDP port, 1 to 65534 */
int netmidi_config_port(const char *text, uint16_t *port);
/* "<driver>.port.<n> = <name>": number is the text after the last dot */
int netmidi_config_port_name(const char *number, const char *value,
                             char (*names)[64]);

#endif
