/*
    Copyright (C) 2025, The AROS Development Team. All rights reserved.

    C99 function putwc().
*/


#include <wchar.h>
#include <stdio.h>

wint_t putwc(wchar_t wc, FILE *stream)
{
    return fputwc(wc, stream);
}
