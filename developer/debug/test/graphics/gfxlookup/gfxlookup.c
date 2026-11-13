/* Copyright (C) 2026, The AROS Development Team. All rights reserved. */

#include <aros/asmcall.h>
#include <aros/debug.h>
#include <exec/interrupts.h>
#include <graphics/gfxbase.h>
#include <graphics/gfxnodes.h>
#include <graphics/view.h>
#include <hardware/intbits.h>
#include <proto/dos.h>
#include <proto/exec.h>
#include <proto/graphics.h>

struct LookupTest
{
    struct View key, missing;
    struct ExtendedNode *expected;
    struct ExtendedNode *found, *not_found;
    volatile BOOL done;
};

static AROS_INTH1(lookup_interrupt, struct LookupTest *, test)
{
    AROS_INTFUNC_INIT

    if (!test->done)
    {
        test->found = GfxLookUp(&test->key);
        test->not_found = GfxLookUp(&test->missing);
        test->done = TRUE;
    }
    return 0;

    AROS_INTFUNC_EXIT
}

int main(void)
{
    struct LookupTest test = {0};
    struct Interrupt interrupt = {0};
    ULONG i, failures = 0;

    test.expected = GfxNew(VIEW_EXTRA_TYPE);
    if (!test.expected)
        return 20;
    GfxAssociate(&test.key, test.expected);
    if (GfxLookUp(&test.key) != test.expected)
        failures++;
    interrupt.is_Code = (APTR)lookup_interrupt;
    interrupt.is_Data = &test;
    interrupt.is_Node.ln_Type = NT_INTERRUPT;
    interrupt.is_Node.ln_Name = "graphics lookup regression";

    /* An interrupt must look up both a present and an absent association
     * even when it interrupts the task holding the hash semaphore. */
    ObtainSemaphore(GfxBase->HashTableSemaphore);
    AddIntServer(INTB_VERTB, &interrupt);
    for (i = 0; i < 8 && !test.done; i++)
        WaitTOF();
    RemIntServer(INTB_VERTB, &interrupt);
    ReleaseSemaphore(GfxBase->HashTableSemaphore);
    /* Give a deferred supervisor alert a chance to reach the normal alert
     * task before this regression exits. A clean run must produce no alert. */
    Delay(2);
    if (!test.done || test.found != test.expected || test.not_found)
        failures++;
    GfxFree(test.expected);
    if (GfxLookUp(&test.key))
        failures++;
    bug("GFX LOOKUP TEST: %lu failures\n", (unsigned long)failures);
    return failures ? 20 : 0;
}
