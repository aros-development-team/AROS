/*
    Copyright (c) 2026, The AROS Development Team. All rights reserved.

    Desc: Early kernel startup for the opensbi-riscv64 target.

    Entered from startup.S in S-mode with the boot hart id and the
    physical address of the device tree blob, as handed over by OpenSBI.

    This is the early bring-up skeleton: it announces itself on the SBI
    console and parks the hart. The real startup sequence (parse the DTB,
    set up the memory map and MMU, boot the remaining harts via HSM,
    populate the boot TagItems and enter the kernel proper) hangs off
    kernel_cstart() as it is brought up.
*/

#include <inttypes.h>

#include <exec/execbase.h>

#include "kernel_sbi.h"

static int sbi_have_dbcn;

/* The hart we were booted on (see kernel_intern.h) */
unsigned long __boot_hartid;

struct ExecBase *SysBase __attribute__((section(".data"))) = NULL;

static void krnSBIPutStr(const char *s)
{
    unsigned long len = 0;

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

static void krnSBIPutHex(uint64_t val)
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

void __attribute__((noreturn)) kernel_cstart(unsigned long hartid, void *fdt)
{
    __boot_hartid = hartid;
    sbi_have_dbcn = sbi_probe_extension(SBI_EXT_DBCN) != 0;

    krnSBIPutStr("AROS64/riscv (OpenSBI)\n");
    krnSBIPutStr("boot hart: ");
    krnSBIPutHex(hartid);
    krnSBIPutStr("\ndtb:       ");
    krnSBIPutHex((uint64_t)(uintptr_t)fdt);
    krnSBIPutStr("\n");

    /*
     * TODO: parse the DTB for the memory map and console,
     *       set up the trap vector and MMU (Sv39/Sv48),
     *       start secondary harts via the HSM extension,
     *       build the KRN_* TagItem list and hand over to the
     *       kernel.resource / exec bring-up.
     */
    krnSBIPutStr("early startup complete - halting.\n");

    for (;;)
        asm volatile("wfi");
}
