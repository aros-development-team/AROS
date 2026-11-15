/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.
*/

#include <dos/dos.h>
#include <proto/dos.h>
#include <stdio.h>

#define LONG_MIN_VALUE (-2147483647L - 1L)

static const struct
{
    const char *text;
    LONG value;
    LONG consumed;
} cases[] =
{
    { "2147483647", 2147483647L, 10 },
    { "-2147483648", LONG_MIN_VALUE, 11 },
    { "+2147483647", 2147483647L, 11 },
    { "2147483646", 2147483646L, 10 },
    { "-2147483647", -2147483647L, 11 },
    { "0", 0, 1 },
    { "-0", 0, 2 },
    { "+0", 0, 2 },
    { "0007", 7, 4 },
    { "+0007", 7, 5 },
    { "-0007", -7, 5 },
    { " \t+42tail", 42, 5 },
    { "\t -42 ", -42, 5 },
    { "42.5", 42, 2 },
    { "42 7", 42, 2 },
    { "42\n", 42, 2 },
    { "0x10", 0, 1 },
    { "", 0, -1 },
    { " \t", 0, -1 },
    { "+", 0, -1 },
    { "-", 0, -1 },
    { "+ 1", 0, -1 },
    { "--1", 0, -1 },
    { "x1", 0, -1 },
    { "\n1", 0, -1 },
    { "\r1", 0, -1 },
    { "\v1", 0, -1 },
    /* Too-large numbers remain unchecked; accumulation wraps as ULONG. */
    { "2147483648", LONG_MIN_VALUE, 10 },
    { "-2147483649", 2147483647L, 11 },
    { "4294967295", -1, 10 },
    { "4294967296", 0, 10 },
    { "-4294967295", 1, 11 },
    { "-4294967296", 0, 11 },
    { "18446744073709551615", -1, 20 }
};

int main(void)
{
    unsigned int i, failures = 0;
    LONG original_error = IoErr();

    for (i = 0; i < sizeof(cases) / sizeof(cases[0]); i++)
    {
        struct
        {
            ULONG before;
            LONG value;
            ULONG after;
        } result = { 0x12345678UL, 123, 0x76543210UL };
        LONG consumed, error;

        SetIoErr(ERROR_OBJECT_NOT_FOUND);
        consumed = StrToLong((CONST_STRPTR)cases[i].text, &result.value);
        error = IoErr();
        if (result.value != cases[i].value || consumed != cases[i].consumed ||
            error != ERROR_OBJECT_NOT_FOUND || result.before != 0x12345678UL ||
            result.after != 0x76543210UL)
        {
            printf("FAIL case %u: value %ld (expected %ld), "
                "consumed %ld (expected %ld), IoErr %ld\n", i,
                (long)result.value, (long)cases[i].value,
                (long)consumed, (long)cases[i].consumed, (long)error);
            failures++;
        }
    }

    SetIoErr(original_error);
    printf("STRTOLONG: %u cases, %u failures\n", i, failures);
    return failures ? RETURN_ERROR : RETURN_OK;
}
