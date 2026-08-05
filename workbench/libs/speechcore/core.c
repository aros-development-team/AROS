/* Copyright (C) 2026, The AROS Development Team. All rights reserved. */

#include "speechcore.h"

void SCDefaultParams(struct SCNarratorParams *p)
{
    if (p == NULL)
    {
        return;
    }
    p->rate = 150;
    p->pitch = 110;
    p->mode = 0;
    p->sex = 0;
    p->volume = 64;
    p->sample_frequency = 22200;
    p->mouths = 0;
}

int SCValidateEnglishUtf8(const uint8_t *text, size_t length,
                             size_t *error_offset)
{
    size_t i = 0;
    size_t unsupported_at = SIZE_MAX;

    if (error_offset != NULL)
    {
        *error_offset = 0;
    }
    if (text == NULL && length != 0)
    {
        return SC_ERR_BAD_PARAM;
    }

    while (i < length)
    {
        uint8_t lead = text[i];
        size_t continuation;
        uint32_t codepoint;
        size_t j;

        if (lead < 0x80)
        {
            ++i;
            continue;
        }
        if (lead >= 0xc2 && lead <= 0xdf)
        {
            continuation = 1;
            codepoint = lead & 0x1fU;
        }
        else if (lead >= 0xe0 && lead <= 0xef)
        {
            continuation = 2;
            codepoint = lead & 0x0fU;
        }
        else if (lead >= 0xf0 && lead <= 0xf4)
        {
            continuation = 3;
            codepoint = lead & 0x07U;
        }
        else
        {
            goto malformed;
        }

        if (i + continuation >= length)
        {
            goto malformed;
        }
        for (j = 1; j <= continuation; ++j)
        {
            if ((text[i + j] & 0xc0U) != 0x80U)
            {
                goto malformed;
            }
            codepoint = (codepoint << 6) | (text[i + j] & 0x3fU);
        }
        if ((continuation == 2 && codepoint < 0x800U) ||
                (continuation == 3 && codepoint < 0x10000U) ||
                (codepoint >= 0xd800U && codepoint <= 0xdfffU) ||
                codepoint > 0x10ffffU)
        {
            goto malformed;
        }

        if (unsupported_at == SIZE_MAX)
        {
            unsupported_at = i;
        }
        i += continuation + 1U;
        continue;

malformed:
        if (error_offset != NULL)
        {
            *error_offset = i;
        }
        return SC_ERR_BAD_UTF8;
    }
    if (unsupported_at != SIZE_MAX)
    {
        if (error_offset != NULL)
        {
            *error_offset = unsupported_at;
        }
        return SC_ERR_UNSUPPORTED;
    }
    return SC_OK;
}
