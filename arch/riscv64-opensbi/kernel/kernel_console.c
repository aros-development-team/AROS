/*
    Copyright (c) 2026, The AROS Development Team. All rights reserved.

    Desc: Early SBI debug console output for the opensbi-riscv64 target.
*/

#include <inttypes.h>

#include "kernel_sbi.h"
#include "kernel_intern.h"

static int sbi_have_dbcn = -1;

void krnSBIPutStr(const char *s)
{
    unsigned long len = 0;

    if (sbi_have_dbcn < 0)
        sbi_have_dbcn = sbi_probe_extension(SBI_EXT_DBCN) != 0;

    while (s[len])
        len++;

    if (sbi_have_dbcn)
    {
        sbi_debug_console_write(s, len);
    }
    else
    {
        while (*s)
            sbi_console_putchar(*s++);
    }
}

void krnSBIPutHex(uint64_t val)
{
    static const char hexchars[] = "0123456789abcdef";
    char buf[19];
    int i;

    buf[0] = '0';
    buf[1] = 'x';
    for (i = 0; i < 16; i++)
        buf[2 + i] = hexchars[(val >> (60 - i * 4)) & 0xF];
    buf[18] = '\0';

    krnSBIPutStr(buf);
}

void krnSBIPutDec(uint64_t val)
{
    char buf[21];
    int i = 20;

    buf[i] = '\0';
    do
    {
        buf[--i] = '0' + (val % 10);
        val /= 10;
    } while (val && i > 0);

    krnSBIPutStr(&buf[i]);
}
