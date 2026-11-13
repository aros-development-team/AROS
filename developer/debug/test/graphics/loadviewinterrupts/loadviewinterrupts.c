/* Copyright (C) 2026, The AROS Development Team. All rights reserved. */

#include <aros/debug.h>
#include <graphics/gfxbase.h>
#include <hardware/custom.h>
#include <hardware/intbits.h>
#include <proto/exec.h>
#include <proto/graphics.h>

struct GfxBase *GfxBase;
static ULONG checks, failures;

static void check(BOOL okay, CONST_STRPTR text)
{
    checks++;
    if (!okay)
    {
        failures++;
        bug("LOADVIEW INTERRUPTS FAIL: %s\n", text);
    }
}

static void repeated_null(BOOL clear_all, BOOL disabled)
{
    volatile struct Custom *custom = (APTR)0xdff000;
    UWORD saved, observed, before_dma, expected_other;
    ULONG before;

    Forbid();
    saved = custom->intenar;
    before_dma = custom->dmaconr & 0x07ff;
    if (disabled)
        Disable();
    custom->intena = clear_all ? 0x7fff : INTF_VERTB;
    expected_other = clear_all ? 0 : saved & ~(INTF_INTEN | INTF_VERTB);
    LoadView(NULL);
    observed = custom->intenar;

    check((observed & INTF_VERTB) != 0,
          "repeated LoadView(NULL) enables vertical blank");
    check(((observed & INTF_INTEN) != 0) == !disabled,
          "master interrupt enable respects Disable nesting");
    check((observed & ~(INTF_INTEN | INTF_VERTB)) == expected_other,
          "unrelated interrupt enables are preserved");
    check((custom->dmaconr & 0x07ff) == before_dma,
          "repeated NULL leaves DMA enables unchanged");

    /* Restore the hardware even on the old ROM, so a failed check cannot
     * trap the test in WaitTOF or leave its caller without interrupts.
     */
    if (disabled)
        Enable();
    custom->intena = 0x7fff;
    custom->intena = INTF_SETCLR | saved | INTF_INTEN | INTF_VERTB;
    Permit();
    before = GfxBase->VBCounter;
    WaitTOF();
    check(GfxBase->VBCounter != before, "WaitTOF observes a later vertical blank");
}

int main(void)
{
    struct View *saved;

    GfxBase = (APTR)OpenLibrary("graphics.library", 0);
    if (!GfxBase)
        return 20;
    saved = GfxBase->ActiView;
    LoadView(NULL);
    WaitTOF();
    WaitTOF();

    repeated_null(FALSE, FALSE);
    repeated_null(TRUE, FALSE);
    repeated_null(FALSE, TRUE);

    LoadView(saved);
    CloseLibrary((APTR)GfxBase);
    bug("LOADVIEW INTERRUPTS TEST: %lu/%lu pass, %lu failures\n",
        checks - failures, checks, failures);
    return failures ? 20 : 0;
}
