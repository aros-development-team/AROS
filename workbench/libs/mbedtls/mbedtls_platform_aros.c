/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    mbedTLS platform hooks for AROS.
    mbedTLS only knows how to read a millisecond clock on POSIX and Windows,
    so provide it here from posixc's monotonic clock.
*/

#include "mbedtls/build_info.h"
#include "mbedtls/platform_time.h"

#include <time.h>

#if defined(MBEDTLS_HAVE_TIME) && defined(MBEDTLS_PLATFORM_MS_TIME_ALT)

mbedtls_ms_time_t mbedtls_ms_time(void)
{
    struct timespec tv;

    if (clock_gettime(CLOCK_MONOTONIC, &tv) != 0)
        return (mbedtls_ms_time_t)time(NULL) * 1000;

    return (mbedtls_ms_time_t)tv.tv_sec * 1000 + tv.tv_nsec / 1000000;
}

#endif
