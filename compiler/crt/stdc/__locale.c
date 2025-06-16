/*
    Copyright (C) 2025, The AROS Development Team. All rights reserved.
*/

#include <aros/debug.h>
#include <proto/exec.h>

#include <string.h>
#include <locale.h>

#include "__stdc_intbase.h"
#include "debug.h"

struct __locale __locale_C = {
    .name = "C",
    .mb_cur_max = 1
};

struct __locale __locale_UTF8 = {
    .name = "C.UTF-8",
    .mb_cur_max = 4
};

struct __locale *__get_current_locale(void) {
    struct StdCIntBase *StdCBase =
        (struct StdCIntBase *)__aros_getbase_StdCBase();

    return StdCBase->__locale_cur;
}

locale_t __get_setlocale_internal(const char *name) {
    struct StdCIntBase *StdCBase =
        (struct StdCIntBase *)__aros_getbase_StdCBase();

    if (!name || strcmp(name, "C") == 0)
        StdCBase->__locale_cur = &__locale_C;
    else if (strcmp(name, "C.UTF-8") == 0)
        StdCBase->__locale_cur = &__locale_UTF8;
    else
        return NULL;

    return StdCBase->__locale_cur;
}

static int __init_stdclocale(struct StdCIntBase *StdCBase)
{
    D(bug("[%s] %s()\n", STDCNAME, __func__));
	StdCBase->__locale_cur = &__locale_C;

    return 1;
}

ADD2OPENLIB(__init_stdclocale, 0);
