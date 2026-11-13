/*
    Copyright 2009-2026, The AROS Development Team. All rights reserved.
*/

#include <stddef.h>
#include <string.h>

void
util_streaming_load_memcpy(void *restrict dst, void *restrict src, size_t len)
{
    memcpy(dst, src, len);
}
