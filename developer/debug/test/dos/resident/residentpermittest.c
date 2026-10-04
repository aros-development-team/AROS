/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Run the built-in Resident command in this process and check that it
    preserves the caller's Forbid nesting count on success and error paths.
    Run beside residentpermitpayload, or pass its path as the first argument.
*/
#include <aros/debug.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <exec/execbase.h>
#include <dos/dosextens.h>
#include <stdio.h>
#include <string.h>

#define TEST_NAME "__ResidentPermitRegression"
#define INTERNAL_NAME "__ResidentPermitInternalRegression"

static BPTR residentCommand;
static ULONG failures;

static void check(BOOL okay, CONST_STRPTR what)
{
    if (!okay)
    {
        bug("RESIDENT PERMIT FAIL: %s\n", what);
        failures++;
    }
}

static void command(STRPTR args, LONG expected, CONST_STRPTR what)
{
    BYTE before = SysBase->TDNestCnt;
    BYTE after;
    LONG result = RunCommand(residentCommand, 8192, args, strlen(args));

    after = SysBase->TDNestCnt;
    check(result == expected, what);
    if (after != before)
    {
        bug("RESIDENT PERMIT FAIL: %s changed nesting from %ld to %ld\n",
            what, (long)before, (long)after);
        failures++;
    }
    /* Recover after a failing implementation so later cases and the shell
     * are not poisoned by its leaked Forbid level. */
    while (SysBase->TDNestCnt > before) Permit();
    while (SysBase->TDNestCnt < before) Forbid();
}

static BOOL usecount(CONST_STRPTR name, LONG count)
{
    struct Segment *segment;

    Forbid();
    segment = FindSegment(name, NULL, FALSE);
    if (segment) segment->seg_UC = count;
    Permit();
    return segment != NULL;
}

static LONG systemcount(CONST_STRPTR name)
{
    struct Segment *segment;
    LONG count = 0;

    Forbid();
    segment = FindSegment(name, NULL, TRUE);
    if (segment) count = segment->seg_UC;
    Permit();
    return count;
}

int main(int argc, char **argv)
{
    CONST_STRPTR payload = argc > 1 ? argv[1] : "C:residentpermitpayload";
    struct Segment *segment;
    BPTR internalPayload = BNULL;
    char add[512], replace[512];
    char remove[] = TEST_NAME " REMOVE\n";
    char removeInternal[] = INTERNAL_NAME " REMOVE\n";
    char replaceInternal[512], rejectBuiltin[512];

    Forbid();
    segment = FindSegment("Resident", NULL, TRUE);
    if (segment) residentCommand = segment->seg_Seg;
    check(FindSegment(TEST_NAME, NULL, FALSE) == NULL &&
          FindSegment(INTERNAL_NAME, NULL, TRUE) == NULL,
          "temporary names are unused");
    Permit();
    if (!residentCommand || failures)
    {
        check(residentCommand != BNULL, "built-in Resident is available");
        goto end;
    }
    if (strlen(payload) > 400)
    {
        check(FALSE, "payload path fits command buffer");
        goto end;
    }
    snprintf(add, sizeof(add), TEST_NAME " \"%s\" FORCE\n", payload);
    snprintf(replace, sizeof(replace), TEST_NAME " \"%s\" REPLACE FORCE\n", payload);
    snprintf(replaceInternal, sizeof(replaceInternal),
             INTERNAL_NAME " \"%s\" REPLACE FORCE\n", payload);
    snprintf(rejectBuiltin, sizeof(rejectBuiltin), "resident \"%s\" FORCE\n", payload);

    command(remove, RETURN_WARN, "remove missing command");
    command(add, RETURN_OK, "register new command");
    check(usecount(TEST_NAME, 0), "registered segment exists");
    command(replace, RETURN_OK, "replace existing command");
    check(usecount(TEST_NAME, 2), "mark segment in use");
    command(remove, RETURN_WARN, "reject removal of busy command");
    command(replace, RETURN_FAIL, "reject replacement of busy command");
    check(usecount(TEST_NAME, 0), "busy segment remains registered");
    command(remove, RETURN_OK, "remove existing command");
    command(remove, RETURN_WARN, "removed command is absent");
    command(rejectBuiltin, 1, "protect built-in Resident name");

    /* Repeat the two leaking paths with an outer Forbid held by the caller. */
    Forbid();
    command(add, RETURN_OK, "register while caller is forbidden");
    command(remove, RETURN_OK, "remove while caller is forbidden");
    Permit();

    internalPayload = LoadSeg(payload);
    if (internalPayload && AddSegment(INTERNAL_NAME, internalPayload, CMD_INTERNAL))
    {
        internalPayload = BNULL; /* resident list owns the segment now */
        command(removeInternal, RETURN_OK, "disable internal command");
        check(systemcount(INTERNAL_NAME) == CMD_DISABLED, "internal command was disabled");
        command(replaceInternal, RETURN_OK, "restore disabled internal command");
        check(systemcount(INTERNAL_NAME) == CMD_INTERNAL, "internal command was restored");
        Forbid();
        segment = FindSegment(INTERNAL_NAME, NULL, TRUE);
        if (segment)
        {
            segment->seg_UC = 0;
            check(RemSegment(segment), "remove temporary internal segment");
        }
        Permit();
    }
    else
        check(FALSE, "create temporary internal command");
    /* Cleanup also covers an unexpected command failure during the test. */
    Forbid();
    segment = FindSegment(TEST_NAME, NULL, FALSE);
    if (segment)
    {
        segment->seg_UC = 0;
        RemSegment(segment);
    }
    Permit();
end:
    if (internalPayload) UnLoadSeg(internalPayload);
    bug("RESIDENT PERMIT TEST: %lu failures\n", (unsigned long)failures);
    return failures ? RETURN_FAIL : RETURN_OK;
}
