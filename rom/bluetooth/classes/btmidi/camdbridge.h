/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.
*/

#ifndef BTMIDI_CAMDBRIDGE_H
#define BTMIDI_CAMDBRIDGE_H

#include <stddef.h>
#include <stdint.h>

#include "btmidi_limits.h"

struct btmidi_camd {
    void *camd_base;        /* each bridge holds camd.library open itself */
    void *node;
    void *to_clients;
    void *from_clients;
    uint8_t *sysex_buffer;
    int signal_bit;
};

typedef int (*btmidi_camd_output)(void *context, const uint8_t *message,
                              size_t length);

int btmidi_camd_open_named(struct btmidi_camd *bridge,
                              char *node_name, char *incoming_name,
                              char *outgoing_name);
void btmidi_camd_close(struct btmidi_camd *bridge);
void btmidi_camd_deliver(struct btmidi_camd *bridge,
                            const uint8_t *message, size_t length);
void btmidi_camd_deliver_sysex(struct btmidi_camd *bridge,
                                  const uint8_t *message, size_t length);
void btmidi_camd_poll(struct btmidi_camd *bridge,
                         btmidi_camd_output output, void *context);

#endif
