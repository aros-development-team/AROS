/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.
*/

#ifndef NETMIDI_TEXT_H
#define NETMIDI_TEXT_H

#include <stddef.h>
#include <stdint.h>

/* How much of a name of length bytes fits in limit bytes without cutting a
 * UTF-8 character in two: limit, or a few bytes less. A name is whatever a
 * peer sent, so bytes that are no UTF-8 at all are cut where limit falls. */
size_t netmidi_name_fit(const uint8_t *name, size_t length, size_t limit);

/* The length of a string that ends in a zero byte, looked for in its
 * first limit + 1 bytes: limit + 1 when there is none there, which a
 * caller takes for "too long". */
size_t netmidi_text_length(const char *text, size_t limit);

#endif
