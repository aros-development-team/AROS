/*
    Copyright (c) 2026, The AROS Development Team. All rights reserved.

    Desc: Early SBI debug console output for the sbi-riscv64 target.
*/

#include <inttypes.h>

#include "kernel_sbi.h"
#include "kernel_intern.h"

#define SBI_DBCN_ZERO_WRITE_TIMEOUT_MS 5
#define SBI_DBCN_EARLY_ZERO_WRITE_RETRY_LIMIT 4
#define SBI_DBCN_ZERO_WRITE_TIMEOUT_LIMIT 3

static int sbi_have_dbcn = -1;
static unsigned int sbi_dbcn_zero_write_timeouts;

static int krnSBIHaveDBCN(void)
{
    if (sbi_have_dbcn < 0)
        sbi_have_dbcn = sbi_probe_extension(SBI_EXT_DBCN) != 0;
    return sbi_have_dbcn;
}

static void krnSBIHandleDBCNError(long error)
{
    if (error == SBI_ERR_NOT_SUPPORTED)
        sbi_have_dbcn = 0;
}

static uint64_t krnSBIWriteTimeoutTicks(void)
{
    uint64_t frequency = __timebase_freq;
    uint64_t ticks;

    if (frequency == 0)
        return 0;

    ticks = (frequency / 1000) * SBI_DBCN_ZERO_WRITE_TIMEOUT_MS;
    ticks += ((frequency % 1000) * SBI_DBCN_ZERO_WRITE_TIMEOUT_MS + 999) /
             1000;
    return ticks ? ticks : 1;
}

/*
 * DBCN writes are non-blocking and may consume only part of the buffer.
 * The SBI console lock covers one call only, so a string split across
 * calls may be interleaved with output from another hart or a nested trap.
 */
void krnSBIPutBuf(const char *s, unsigned long len)
{
    uint64_t zero_write_start = 0;
    uint64_t zero_write_timeout = krnSBIWriteTimeoutTicks();
    unsigned int early_zero_writes = 0;
    int waiting_for_write = 0;

    while (len && krnSBIHaveDBCN())
    {
        struct sbiret ret = sbi_debug_console_write(s, len);

        if (ret.error != SBI_SUCCESS)
        {
            krnSBIHandleDBCNError(ret.error);
            break;
        }

        if ((unsigned long)ret.value > len)
            break;

        if (ret.value == 0)
        {
            uint64_t now;

            if (zero_write_timeout == 0)
            {
                if (++early_zero_writes >=
                    SBI_DBCN_EARLY_ZERO_WRITE_RETRY_LIMIT)
                    break;
            }
            else if (sbi_dbcn_zero_write_timeouts >=
                     SBI_DBCN_ZERO_WRITE_TIMEOUT_LIMIT)
            {
                break;
            }
            else
            {
                now = krnReadTime();
                if (!waiting_for_write)
                {
                    zero_write_start = now;
                    waiting_for_write = 1;
                }
                else if (now - zero_write_start >= zero_write_timeout)
                {
                    sbi_dbcn_zero_write_timeouts++;
                    break;
                }
            }

            asm volatile("pause");
            continue;
        }

        waiting_for_write = 0;
        sbi_dbcn_zero_write_timeouts = 0;
        s += ret.value;
        len -= ret.value;
    }

    /* On an SBI error ret.value is undefined; DBCN may have emitted part of
       this unconfirmed tail, so the legacy fallback can repeat those bytes. */
    while (len--)
        sbi_console_putchar(*s++);
}

void krnSBIPutC(char c)
{
    long error;

    if (!krnSBIHaveDBCN())
    {
        sbi_console_putchar(c);
        return;
    }

    error = sbi_debug_console_write_byte(c);
    if (error != SBI_SUCCESS)
    {
        krnSBIHandleDBCNError(error);
        sbi_console_putchar(c);
    }
    else
        sbi_dbcn_zero_write_timeouts = 0;
}

/* Returns the next console character, or -1 if none is pending */
int krnSBIGetC(void)
{
    struct sbiret ret;
    int ch;

    if (krnSBIHaveDBCN())
    {
        char c;

        ret = sbi_debug_console_read(&c, 1);
        if (ret.error != SBI_SUCCESS)
        {
            krnSBIHandleDBCNError(ret.error);
            ch = sbi_console_getchar();
            return ch < 0 ? -1 : ch;
        }

        if (ret.value == 1)
            return (unsigned char)c;
        return -1;
    }

    ch = sbi_console_getchar();
    return ch < 0 ? -1 : ch;
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
