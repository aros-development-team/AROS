/*
    Copyright (c) 2026, The AROS Development Team. All rights reserved.

    Desc: Early SBI debug console output for the opensbi-riscv64 target.
*/

#include <inttypes.h>

#include "kernel_sbi.h"
#include "kernel_intern.h"

#define SBI_DBCN_ZERO_WRITE_LIMIT 4

static int sbi_have_dbcn = -1;

static int krnSBIHaveDBCN(void)
{
    if (sbi_have_dbcn < 0)
        sbi_have_dbcn = sbi_probe_extension(SBI_EXT_DBCN) != 0;
    return sbi_have_dbcn;
}

/*
 * DBCN writes are non-blocking and may consume only part of the buffer.
 * The SBI console lock covers one call only, so a string split across
 * calls may be interleaved with output from another hart or a nested trap.
 */
static void krnSBIPutBuf(const char *s, unsigned long len)
{
    unsigned int zero_writes = 0;

    while (len && krnSBIHaveDBCN())
    {
        struct sbiret ret = sbi_debug_console_write(s, len);

        if (ret.error != SBI_SUCCESS || (unsigned long)ret.value > len)
        {
            sbi_have_dbcn = 0;
            break;
        }

        if (ret.value == 0)
        {
            if (++zero_writes >= SBI_DBCN_ZERO_WRITE_LIMIT)
            {
                sbi_have_dbcn = 0;
                break;
            }
            asm volatile("pause");
            continue;
        }

        zero_writes = 0;
        s += ret.value;
        len -= ret.value;
    }

    while (len--)
        sbi_console_putchar(*s++);
}

void krnSBIPutC(char c)
{
    if (!krnSBIHaveDBCN() ||
        sbi_debug_console_write_byte(c) != SBI_SUCCESS)
    {
        sbi_have_dbcn = 0;
        sbi_console_putchar(c);
    }
}

/* Returns the next console character, or -1 if none is pending */
int krnSBIGetC(void)
{
    if (krnSBIHaveDBCN())
    {
        char c;

        if (sbi_debug_console_read(&c, 1) == 1)
            return (unsigned char)c;
        return -1;
    }

    return sbi_console_getchar();
}

void krnSBIPutStr(const char *s)
{
    unsigned long len = 0;

    while (s[len])
        len++;

    krnSBIPutBuf(s, len);
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

/* Compact hex, for dumping cell arrays where 16 digits a cell would
   be unreadable */
void krnSBIPutHex32(uint32_t val)
{
    static const char hexchars[] = "0123456789abcdef";
    char buf[11];
    int i;

    buf[0] = '0';
    buf[1] = 'x';
    for (i = 0; i < 8; i++)
        buf[2 + i] = hexchars[(val >> (28 - i * 4)) & 0xF];
    buf[10] = '\0';

    krnSBIPutStr(buf);
}

void krnSBIPutHex8(unsigned char val)
{
    static const char hexchars[] = "0123456789abcdef";
    char buf[3];

    buf[0] = hexchars[(val >> 4) & 0xF];
    buf[1] = hexchars[val & 0xF];
    buf[2] = '\0';

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
