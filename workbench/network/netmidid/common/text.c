/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.
*/

#include <netmidi/text.h>
#include "netmidi/text.h"

size_t netmidi_name_fit(const uint8_t *name, size_t length, size_t limit)
{
    size_t cut = limit;
    unsigned back;

    if (!name || length <= limit)
        return length;
    /* a continuation byte is 10xxxxxx: step back to the byte that begins
       the character the limit falls inside, at most three such bytes */
    for (back = 0; back < 3 && cut && (name[cut] & 0xc0) == 0x80; ++back)
        --cut;
    if ((name[cut] & 0xc0) == 0x80)
        return limit;           /* not UTF-8: nothing to keep whole */
    return cut;
}

size_t netmidi_text_length(const char *text, size_t limit)
{
    size_t length = 0;

    while (length <= limit && text[length])
        ++length;
    return length;
}
