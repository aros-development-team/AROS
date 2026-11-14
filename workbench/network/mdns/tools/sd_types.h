/*
 * SPDX-License-Identifier: BSD-2-Clause
 * Copyright 2026 amigazen project
 *
 * sd_types.h - plain-English service-type aliases shared by the friendly mDNS
 * front-end tools (Nearby / Handshake / Share).
 *
 * The point of these tools is that a user should never have to type or read
 * DNS-SD wire jargon like "_afpovertcp._tcp".  This table maps short everyday
 * words to the real service type, and real types back to a human description.
 */

#ifndef MDNS_TOOLS_SD_TYPES_H
#define MDNS_TOOLS_SD_TYPES_H

#include <exec/types.h>

/*
 * Turn a user-supplied word into a DNS-SD service type.
 *   - a word starting with '_' is assumed to already be a real type,
 *   - a known alias ("web", "printer", ...) maps to its type,
 *   - anything else is returned unchanged so full/unknown types still work.
 * The returned pointer is either the input pointer or a static string.
 */
CONST_STRPTR SDTypeFromAlias(CONST_STRPTR word);

/*
 * Human description for a real service type, e.g. "_http._tcp" -> "Web server".
 * Returns the type itself when it is not in the table.
 */
CONST_STRPTR SDTypeFriendly(CONST_STRPTR type);

/* Case-insensitive string equality (native, no utility.library). */
BOOL SDStrEqualCI(const char *a, const char *b);

#endif /* MDNS_TOOLS_SD_TYPES_H */
