/* Copyright (C) 2026, The AROS Development Team. All rights reserved. */
static unsigned checks, failures;
static void check(int okay, const char *what)
{
    checks++;
    if (!okay) {
        if (failures < 8) fprintf(stderr, "INTMASK FAIL: %s\n", what);
        failures++;
    }
}
static ULONG mask(unsigned bits)
{
    return ((bits & 1) ? AKIKO_CDINT_RXDMA : 0) |
           ((bits & 2) ? AKIKO_CDINT_TXDMA : 0) |
           ((bits & 4) ? AKIKO_CDINT_PBX : 0) |
           ((bits & 8) ? (UINT32_C(1) << 31) : 0);
}
int main(void)
{
    unsigned initial, change, source, enable, nested;
    for (initial = 0; initial < 16; initial++)
        for (change = 0; change < 16; change++)
            for (source = 0; source < 4; source++) {
                if (!(initial & (1u << source))) continue;
                for (enable = 0; enable < 2; enable++)
                    for (nested = 0; nested < 2; nested++) {
                        struct CD32Unit unit = {mask(initial)};
                        ULONG expected = enable ? mask(initial | change) :
                                                  mask(initial & ~change);
                        active = &unit;
                        hardware_mask = unit.cu_IntEnable;
                        pending_bit = mask(1u << source);
                        disable_depth = nested;
                        irq_count = 0;
                        in_irq = 0;
                        inject = 1;
                        if (enable) CD32_IntEnable(&unit, mask(change));
                        else CD32_IntDisable(&unit, mask(change));
                        check(disable_depth == nested, "preserve caller interrupt nesting");
                        if (nested) {
                            check(irq_count == 0, "outer Disable defers the interrupt");
                            Enable();
                        }
                        expected &= ~pending_bit;
                        check(unit.cu_IntEnable == expected, "retain task updates and interrupt acknowledgement");
                        check(hardware_mask == unit.cu_IntEnable, "hardware agrees with the software shadow");
                        check(!(hardware_mask & pending_bit), "acknowledged completion cannot keep IRQ2 asserted");
                    }
            }
    printf("INTMASK TEST: %u/%u pass, %u failures\n", checks - failures, checks, failures);
    return failures ? 20 : 0;
}
