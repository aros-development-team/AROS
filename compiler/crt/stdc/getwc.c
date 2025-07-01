/*
    Copyright (C) 2025, The AROS Development Team. All rights reserved.

    C99 function getwc().
*/
#include <wchar.h>
#include <stdio.h>

wint_t getwc(FILE *stream)
{
    return fgetwc(stream);
}
