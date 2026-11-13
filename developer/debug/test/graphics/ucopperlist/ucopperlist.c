/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Build user copper instructions without a preceding CINIT, as classic
    callers do. Verify both lazy entry points and explicitly initialized lists.
*/
#include <aros/debug.h>
#include <proto/graphics.h>
#include <graphics/copper.h>

static ULONG failures;

static void check(BOOL okay, CONST_STRPTR what)
{
    if (!okay)
    {
        bug("USER COPPER FAIL: %s\n", what);
        failures++;
    }
}

static void wait_first(void)
{
    struct UCopList ucl = {0};
    struct CopIns *ins;

    CWait(&ucl, 0, 0);
    check(ucl.FirstCopList && ucl.CopList && ucl.CopList->CopIns,
          "CWait initializes a zeroed user copper list");
    if (!ucl.FirstCopList || !ucl.CopList || !ucl.CopList->CopIns)
        goto end;
    CBump(&ucl);
    CWait(&ucl, 134, 0);
    CBump(&ucl);
    CMove(&ucl, (APTR)0x00dff09c, 0x8010);
    CBump(&ucl);
    CWait(&ucl, 10000, 0xff);
    CBump(&ucl);

    ins = ucl.FirstCopList->CopIns;
    check(ucl.FirstCopList->Count == 4, "four instructions recorded");
    check(ins[0].OpCode == COPPER_WAIT && ins[0].VWAITPOS == 0 &&
          ins[0].HWAITPOS == 0, "first wait retained");
    check(ins[1].OpCode == COPPER_WAIT && ins[1].VWAITPOS == 134,
          "second wait retained");
    check(ins[2].OpCode == COPPER_MOVE && ins[2].DESTADDR == 0x9c &&
          (UWORD)ins[2].DESTDATA == 0x8010, "interrupt move retained");
    check(ins[3].OpCode == COPPER_WAIT && ins[3].VWAITPOS == 10000 &&
          ins[3].HWAITPOS == 0xff, "terminator retained");
end:
    FreeCopList(ucl.FirstCopList);
}

static void move_first(void)
{
    struct UCopList ucl = {0};

    CMove(&ucl, (APTR)0x0180, 0x0123);
    check(ucl.FirstCopList && ucl.CopList && ucl.CopList->CopIns,
          "CMove initializes a zeroed user copper list");
    if (ucl.FirstCopList && ucl.CopList && ucl.CopList->CopIns)
    {
        CBump(&ucl);
        check(ucl.FirstCopList->Count == 1 &&
              ucl.FirstCopList->CopIns[0].OpCode == COPPER_MOVE &&
              ucl.FirstCopList->CopIns[0].DESTADDR == 0x180 &&
              ucl.FirstCopList->CopIns[0].DESTDATA == 0x0123,
              "first move recorded");
    }
    FreeCopList(ucl.FirstCopList);
}

static void initialized(void)
{
    struct UCopList ucl = {0};
    struct CopList *first = UCopperListInit(&ucl, 8);

    check(first && first->CopIns, "explicit initialization succeeds");
    if (first && first->CopIns)
    {
        CMove(&ucl, (APTR)0x0180, 0x0456);
        CBump(&ucl);
        CWait(&ucl, 40, 0);
        CBump(&ucl);
        check(ucl.FirstCopList == first && first->Count == 2 &&
              first->CopIns[0].DESTDATA == 0x0456 &&
              first->CopIns[1].VWAITPOS == 40,
              "initialized list retains both instructions");
    }
    FreeCopList(ucl.FirstCopList);
}

static void failed_allocation(void)
{
    struct CopList cl = {0};
    struct UCopList ucl = {0};

    /* UCopperListInit can allocate the header but fail to allocate CopIns. */
    cl.MaxCount = 4;
    ucl.FirstCopList = &cl;
    ucl.CopList = &cl;
    CBump(&ucl);
    check(cl.Count == 0 && cl.CopPtr == NULL,
          "failed instruction allocation does not advance the list");
}

int main(void)
{
    wait_first();
    move_first();
    initialized();
    failed_allocation();
    bug("USER COPPER TEST: %lu failures\n", (unsigned long)failures);
    return failures ? 20 : 0;
}
