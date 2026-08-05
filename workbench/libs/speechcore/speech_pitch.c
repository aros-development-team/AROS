/* Copyright (C) 2026, The AROS Development Team. All rights reserved. */
#include "speechcore.h"

#define FIXED_ONE 0x00010000UL

uint32_t SCSemitoneRatio(int32_t pitch)
{
    uint32_t ratio = FIXED_ONE;
    uint32_t magnitude, fraction;
    uint32_t step;
    int32_t interpolated;
    unsigned whole, i;

    if (pitch < -(24L << 16))
    {
        pitch = -(24L << 16);
    }
    if (pitch > (24L << 16))
    {
        pitch = (24L << 16);
    }
    magnitude = pitch < 0 ? (uint32_t) - pitch : (uint32_t)pitch;
    whole = magnitude >> 16;
    fraction = magnitude & 0xffffU;
    step = pitch < 0 ? 61858UL : 69433UL;
    for (i = 0; i < whole; ++i)
    {
        ratio = (uint32_t)(((uint64_t)ratio * step) >> 16);
    }
    interpolated = (int32_t)FIXED_ONE +
                   (((int32_t)step - (int32_t)FIXED_ONE) * (int32_t)fraction >> 16);
    ratio = (uint32_t)(((uint64_t)ratio * (uint32_t)interpolated) >> 16);
    return ratio;
}
