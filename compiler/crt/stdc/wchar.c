/*
    Copyright (C) 2025-2026, The AROS Development Team. All rights reserved.

    C99 wchar linklib stubs.
*/

#include <aros/debug.h>

#include <aros/types/mbstate_t.h>

#include <string.h>

#define STDC_NOINLINE_WCHAR

/*****************************************************************************

    NAME */
#include <wchar.h>

int mbsinit(const mbstate_t *ps)
{
    return (ps == NULL) || (ps->__state == 0 && ps->__count == 0 && ps->__value == 0);
}
