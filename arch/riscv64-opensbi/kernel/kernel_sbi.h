#ifndef KERNEL_SBI_H
#define KERNEL_SBI_H

/*
    Copyright (c) 2026, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Minimal SBI (Supervisor Binary Interface) call helpers for the
          opensbi-riscv64 target.
*/

/* SBI extension IDs */
#define SBI_EXT_BASE            0x10
#define SBI_EXT_TIME            0x54494D45
#define SBI_EXT_IPI             0x735049
#define SBI_EXT_HSM             0x48534D
#define SBI_EXT_SRST            0x53525354
#define SBI_EXT_DBCN            0x4442434E

/* SBI_EXT_BASE function IDs */
#define SBI_BASE_GET_SPEC_VERSION   0
#define SBI_BASE_GET_IMPL_ID        1
#define SBI_BASE_GET_IMPL_VERSION   2
#define SBI_BASE_PROBE_EXTENSION    3

/* SBI_EXT_DBCN function IDs */
#define SBI_DBCN_WRITE              0
#define SBI_DBCN_READ               1
#define SBI_DBCN_WRITE_BYTE         2

/* SBI_EXT_SRST function IDs */
#define SBI_SRST_SYSTEM_RESET       0
#define SBI_SRST_TYPE_SHUTDOWN      0
#define SBI_SRST_TYPE_COLD_REBOOT   1
#define SBI_SRST_TYPE_WARM_REBOOT   2

/* Legacy (v0.1) extension IDs */
#define SBI_LEGACY_CONSOLE_PUTCHAR  0x01
#define SBI_LEGACY_CONSOLE_GETCHAR  0x02
#define SBI_LEGACY_SHUTDOWN         0x08

/* SBI return errors */
#define SBI_SUCCESS                 0
#define SBI_ERR_FAILED              -1
#define SBI_ERR_NOT_SUPPORTED       -2
#define SBI_ERR_INVALID_PARAM       -3

struct sbiret
{
    long error;
    long value;
};

static inline struct sbiret sbi_ecall(unsigned long eid, unsigned long fid,
                                      unsigned long arg0, unsigned long arg1,
                                      unsigned long arg2, unsigned long arg3,
                                      unsigned long arg4, unsigned long arg5)
{
    register unsigned long r_a0 asm("a0") = arg0;
    register unsigned long r_a1 asm("a1") = arg1;
    register unsigned long r_a2 asm("a2") = arg2;
    register unsigned long r_a3 asm("a3") = arg3;
    register unsigned long r_a4 asm("a4") = arg4;
    register unsigned long r_a5 asm("a5") = arg5;
    register unsigned long r_a6 asm("a6") = fid;
    register unsigned long r_a7 asm("a7") = eid;
    struct sbiret ret;

    asm volatile("ecall"
                 : "+r"(r_a0), "+r"(r_a1)
                 : "r"(r_a2), "r"(r_a3), "r"(r_a4), "r"(r_a5),
                   "r"(r_a6), "r"(r_a7)
                 : "memory");

    ret.error = r_a0;
    ret.value = r_a1;
    return ret;
}

static inline long sbi_probe_extension(unsigned long eid)
{
    struct sbiret ret = sbi_ecall(SBI_EXT_BASE, SBI_BASE_PROBE_EXTENSION,
                                  eid, 0, 0, 0, 0, 0);
    return ret.error == SBI_SUCCESS ? ret.value : 0;
}

/* Write to the SBI debug console (DBCN extension, SBI >= 2.0) */
static inline long sbi_debug_console_write(const char *buf, unsigned long len)
{
    struct sbiret ret = sbi_ecall(SBI_EXT_DBCN, SBI_DBCN_WRITE,
                                  len, (unsigned long)buf, 0, 0, 0, 0);
    return ret.error;
}

/* Legacy (v0.1) console putchar - always available on OpenSBI */
static inline void sbi_console_putchar(int ch)
{
    sbi_ecall(SBI_LEGACY_CONSOLE_PUTCHAR, 0, ch, 0, 0, 0, 0, 0);
}

/* Program the next timer interrupt (TIME extension) */
static inline void sbi_set_timer(unsigned long stime_value)
{
    sbi_ecall(SBI_EXT_TIME, 0, stime_value, 0, 0, 0, 0, 0);
}

static inline void sbi_system_reset(unsigned long type, unsigned long reason)
{
    sbi_ecall(SBI_EXT_SRST, SBI_SRST_SYSTEM_RESET, type, reason, 0, 0, 0, 0);
}

#endif /* KERNEL_SBI_H */
