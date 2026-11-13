/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Behaviour table for sizePCIBaseReg(), extracted verbatim by
    run_sizebar.py behind this file. Bar scripting and the four
    HIDD_PCIDriver_* macros are fixture-provided; every BAR value and
    size mask here is a test input. Expected values differ by width
    only where the size is 0x100000000 or more: a 32-bit IPTR cannot
    represent it and the routine answers 0.
*/

extern int printf(const char *fmt, ...);
extern int strcmp(const char *l, const char *r);

#define NULL ((void *)0)
#define CMD_ON (PCICMF_IODECODE | PCICMF_MEMDECODE | PCICMF_BUSMASTER)

static int fails;

#define CHECK(cond, msg)                                  \
    do {                                                  \
        if (!(cond)) { printf("FAIL %s\n", msg); fails++; } \
        else         { printf("ok %s\n", msg); }          \
    } while (0)

static struct DeviceData tdev;

static void corpus(void)
{
    static const unsigned int szhis[5] = {0x00000000u, 0xFFFFFFFFu,
        0xFFFFFFFEu, 0x80000000u, 0x0000FF00u};
    int a;
    unsigned int k;

    for (a = 0; a < 5; a++)
    {
        unsigned int szhi = szhis[a];

        for (k = 4; k < 32; k++)
        {
            unsigned int run = 0xFFFFFFFFu << k;
            unsigned int b;
            IPTR r;

            setup_bar(1, 0x00000004u, run | 0x4u);
            setup_bar(2, 0x00000001u, szhi);
            r = sizePCIBaseReg(NULL, NULL, &tdev, 0, 2, 0, 1);
            printf("%08x %08x -> %016llx\n", szhi,
                   (unsigned int)(run & 0xFFFFFFF0u),
                   (unsigned long long)r);
            for (b = 4; b < 32; b++)
            {
                unsigned int m = run & ~(1u << b);

                setup_bar(1, 0x00000004u, m | 0x4u);
                setup_bar(2, 0x00000001u, szhi);
                r = sizePCIBaseReg(NULL, NULL, &tdev, 0, 2, 0, 1);
                printf("%08x %08x -> %016llx\n", szhi,
                       (unsigned int)(m & 0xFFFFFFF0u),
                       (unsigned long long)r);
            }
            for (b = 4; b < k; b++)
            {
                unsigned int m = run | (1u << b);

                setup_bar(1, 0x00000004u, m | 0x4u);
                setup_bar(2, 0x00000001u, szhi);
                r = sizePCIBaseReg(NULL, NULL, &tdev, 0, 2, 0, 1);
                printf("%08x %08x -> %016llx\n", szhi,
                       (unsigned int)(m & 0xFFFFFFF0u),
                       (unsigned long long)r);
            }
        }

        setup_bar(1, 0x00000004u, 0x4u);
        setup_bar(2, 0x00000001u, szhi);
        {
            IPTR r = sizePCIBaseReg(NULL, NULL, &tdev, 0, 2, 0, 1);

            printf("%08x %08x -> %016llx\n", szhi, 0u,
                   (unsigned long long)r);
        }
    }
}

int main(int argc, char **argv)
{
    IPTR sz;
    int before;

    if (argc > 1 && strcmp(argv[1], "--corpus") == 0)
    {
        corpus();
        return 0;
    }

    /* 16GB 64-bit BAR1: low mask 0, high mask 0xFFFFFFFC; combined
     * run of ones bit34..bit63 -> size 0x400000000 on a 64-bit IPTR,
     * unrepresentable on 32-bit (0). */
    setup_bar(1, 0x00000004u, 0x00000000u);
    setup_bar(2, 0x38000000u, 0xFFFFFFFCu);
    sz = sizePCIBaseReg(NULL, NULL, &tdev, 0, 2, 0, 1);
    if (sizeof(IPTR) == 8)
        CHECK(sz == (IPTR)0x400000000ULL, "16GB 64-bit BAR -> 0x400000000");
    else
        CHECK(sz == (IPTR)0, "16GB -> 0 (32-bit IPTR: unrepresentable)");

    /* 6GB shape 0xFFFFFFFE80000000: a gap at bit32 between the low
     * half ending bit31-set and a high half starting bit0-clear; not
     * a contiguous BAR mask, so "cannot size" is the answer. */
    setup_bar(1, 0x00000004u, 0x80000000u);
    setup_bar(2, 0x00000001u, 0xFFFFFFFEu);
    sz = sizePCIBaseReg(NULL, NULL, &tdev, 0, 2, 0, 1);
    CHECK(sz == (IPTR)0, "gap mask 0xFFFFFFFE80000000 -> 0 (both widths)");

    /* 4GB 64-bit BAR: combined mask all ones in the high dword, 0 low
     * -> 0x100000000 (64-bit IPTR); 0 at 32-bit width. */
    setup_bar(1, 0x00000004u, 0x00000000u);
    setup_bar(2, 0x00000001u, 0xFFFFFFFFu);
    sz = sizePCIBaseReg(NULL, NULL, &tdev, 0, 2, 0, 1);
    if (sizeof(IPTR) == 8)
        CHECK(sz == (IPTR)0x100000000ULL, "4GB 64-bit BAR -> 0x100000000");
    else
        CHECK(sz == (IPTR)0, "4GB -> 0 (32-bit IPTR: >= 4GB)");

    /* 8GB 64-bit BAR: high half nor zeroes nor ones (0xFFFFFFFE):
     * a legal 64-bit BAR, sized 0x200000000. */
    setup_bar(1, 0x00000004u, 0x00000000u);
    setup_bar(2, 0x00000001u, 0xFFFFFFFEu);
    sz = sizePCIBaseReg(NULL, NULL, &tdev, 0, 2, 0, 1);
    if (sizeof(IPTR) == 8)
        CHECK(sz == (IPTR)0x200000000ULL, "8GB 64-bit BAR -> 0x200000000");
    else
        CHECK(sz == (IPTR)0, "8GB -> 0 (32-bit IPTR: >= 4GB)");

    /* 256MB 64-bit memory BAR sized the same at both widths. */
    setup_bar(1, 0x00000004u, 0xF0000006u);
    setup_bar(2, 0x00000000u, 0xFFFFFFFFu);
    sz = sizePCIBaseReg(NULL, NULL, &tdev, 0, 2, 0, 1);
    CHECK(sz == (IPTR)0x10000000u, "256MB 64-bit BAR -> 0x10000000 (both widths)");

    /* 64MB plain 32-bit BAR0 unchanged. */
    setup_bar(0, 0x80000000u, 0xFC000000u);
    sz = sizePCIBaseReg(NULL, NULL, &tdev, 0, 2, 0, 0);
    CHECK(sz == (IPTR)0x4000000ULL, "64MB BAR0 -> 0x4000000");

    /* I/O BAR sizing unchanged (0x20 of ports). */
    setup_bar(4, 0x00006001u, 0xFFFFFFE1u);
    sz = sizePCIBaseReg(NULL, NULL, &tdev, 0, 2, 0, 4);
    CHECK(sz == (IPTR)0x20ULL, "IO BAR -> 0x20");

    /* 64MB 64-bit memory BAR whose upper dword reads back zero. */
    setup_bar(1, 0x00000004u, 0xFC000004u);
    setup_bar(2, 0x00000000u, 0x00000000u);
    sz = sizePCIBaseReg(NULL, NULL, &tdev, 0, 2, 0, 1);
    CHECK(sz == (IPTR)0x4000000ULL, "64MB upper-zero 64-bit BAR -> 0x4000000");

    /* same upper-zero shape at 256MB and 2GB. */
    setup_bar(1, 0x00000004u, 0xF0000004u);
    setup_bar(2, 0x00000000u, 0x00000000u);
    sz = sizePCIBaseReg(NULL, NULL, &tdev, 0, 2, 0, 1);
    CHECK(sz == (IPTR)0x10000000u, "upper-zero shape at 256MB -> 0x10000000");
    setup_bar(1, 0x00000004u, 0x80000004u);
    setup_bar(2, 0x00000000u, 0x00000000u);
    sz = sizePCIBaseReg(NULL, NULL, &tdev, 0, 2, 0, 1);
    CHECK(sz == (IPTR)0x80000000u, "upper-zero shape at 2GB -> 0x80000000");

    /* combined mask with a gap (0x0000ff00fc000000): cannot size -> 0 */
    setup_bar(1, 0x00000004u, 0xFC000000u);
    setup_bar(2, 0x00000000u, 0x0000FF00u);
    sz = sizePCIBaseReg(NULL, NULL, &tdev, 0, 2, 0, 1);
    CHECK(sz == (IPTR)0, "gap combined mask -> 0 (cannot size)");

    /* R1: upper dword all ones, low mask 0xfc000010 (not a contiguous
     * run). An unchecked ~sz + 1 at 32-bit width would give 0x3fffff0;
     * the answer is 0 at both widths. */
    setup_bar(1, 0x00000004u, 0xfc000014u);
    setup_bar(2, 0x00000001u, 0xFFFFFFFFu);
    sz = sizePCIBaseReg(NULL, NULL, &tdev, 0, 2, 0, 1);
    CHECK(sz == (IPTR)0, "review R1: szhi all ones, low 0xfc000010 -> 0");

    /* R2: single bit 4 alone, gap above: also not a contiguous
     * low-half run; 0 at both widths. */
    setup_bar(1, 0x00000004u, 0x00000014u);
    setup_bar(2, 0x00000001u, 0xFFFFFFFFu);
    sz = sizePCIBaseReg(NULL, NULL, &tdev, 0, 2, 0, 1);
    CHECK(sz == (IPTR)0, "R2: bit4 alone, upper all ones -> 0");

    /* R3: szhi all ones, contiguous low run 0xfff00000 (1MB): a
     * 32-bit BAR answer, same size at both widths. */
    setup_bar(1, 0x00000004u, 0xfff00004u);
    setup_bar(2, 0x00000001u, 0xFFFFFFFFu);
    sz = sizePCIBaseReg(NULL, NULL, &tdev, 0, 2, 0, 1);
    CHECK(sz == (IPTR)0x100000u, "R3: contiguous 0xfff00000, all-ones upper -> 0x100000");

    /* R4: same shape as R1 on the szhi == 0 path: 0 at both widths
     * (the already-correct answer must stay). */
    setup_bar(1, 0x00000004u, 0xfc000014u);
    setup_bar(2, 0x00000001u, 0x00000000u);
    sz = sizePCIBaseReg(NULL, NULL, &tdev, 0, 2, 0, 1);
    CHECK(sz == (IPTR)0, "R4: szhi 0, low 0xfc000010 -> 0");

    /* decode discipline on every probe above, and the command
     * register restored on the regular (non-host-bridge) paths. */
    CHECK(decode_violations == 0, "decode off during every BAR write");
    CHECK(fake_cmd == CMD_ON, "command register restored");

    /* PCI-PCI bridge: BAR1 has no BAR2 slot (slot 2 is the bridge
     * bus byte); slot 2 must never be written. */
    tdev.isBridge = 1;
    setup_bar(1, 0x00000004u, 0xFFF00000u);
    setup_bar(2, 0x12345678u, 0xFFFFFFFFu);
    cfg_last_write[2] = 0xAAAAAAAAu;
    sz = sizePCIBaseReg(NULL, NULL, &tdev, 0, 2, 0, 1);
    CHECK(cfg_last_write[2] == 0xAAAAAAAAu, "bridge: slot 2 never written");
    CHECK(sz == (IPTR)0x100000ULL, "bridge: low-half size only");
    tdev.isBridge = 0;

    /* host bridge: decoding is left on (Linux mmio_always_on), the
     * command register untouched, size unchanged. */
    tdev.Class = 0x06;
    tdev.SubClass = 0x00;
    before = decode_violations;
    setup_bar(0, 0x80000000u, 0xFC000000u);
    sz = sizePCIBaseReg(NULL, NULL, &tdev, 0, 0, 0, 0);
    CHECK(decode_violations == before + 2, "host bridge: decode left on");
    CHECK(fake_cmd == CMD_ON, "host bridge: command register untouched");
    CHECK(sz == (IPTR)0x4000000ULL, "host bridge: size unchanged");

    if (fails == 0)
        printf("PCI_SIZEBAR ALL GREEN\n");
    return fails != 0;
}
