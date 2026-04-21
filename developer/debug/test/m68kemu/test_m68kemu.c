/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.
    Author: Fabian Schmieder

    On-target test for m68kemu.library.
    Writes a tiny m68k program (MOVEQ #42,D0; RTS) to T:,
    calls RunFile, and verifies the emulator returns 42.
*/

#include <proto/exec.h>
#include <proto/dos.h>
#include <exec/memory.h>
#include <stdio.h>

/* Minimal m68k hunk binary: MOVEQ #42,D0 (0x702A) + RTS (0x4E75) */
static const UBYTE m68k_test_binary[] = {
    0x00, 0x00, 0x03, 0xF3,  /* HUNK_HEADER */
    0x00, 0x00, 0x00, 0x00,  /* no resident libs */
    0x00, 0x00, 0x00, 0x01,  /* 1 hunk */
    0x00, 0x00, 0x00, 0x00,  /* first = 0 */
    0x00, 0x00, 0x00, 0x00,  /* last = 0 */
    0x00, 0x00, 0x00, 0x01,  /* hunk 0 size: 1 longword */
    0x00, 0x00, 0x03, 0xE9,  /* HUNK_CODE */
    0x00, 0x00, 0x00, 0x01,  /* 1 longword of code */
    0x70, 0x2A,              /* MOVEQ #42,D0 */
    0x4E, 0x75,              /* RTS */
    0x00, 0x00, 0x03, 0xF2,  /* HUNK_END */
};

int main(void)
{
    struct Library *emubase;
    LONG result;
    BPTR fh;
    const char *tmpfile = "RAM:m68kemu_test.bin";

    printf("m68kemu on-target test\n");

    /* Open the library */
    emubase = OpenLibrary("m68kemu.library", 0);
    if (!emubase) {
        printf("FAIL: cannot open m68kemu.library\n");
        return 20;
    }
    printf("  m68kemu.library opened at %p\n", emubase);

    /* Write test binary to T: */
    fh = Open(tmpfile, MODE_NEWFILE);
    if (!fh) {
        printf("FAIL: cannot create %s\n", tmpfile);
        CloseLibrary(emubase);
        return 20;
    }
    Write(fh, (APTR)m68k_test_binary, sizeof(m68k_test_binary));
    Close(fh);

    /* Call RunFile (LVO 6) */
    LONG (*RunFile)(CONST_STRPTR, ULONG, CONST_STRPTR, ULONG) =
        (LONG (*)(CONST_STRPTR, ULONG, CONST_STRPTR, ULONG))__AROS_GETVECADDR(emubase, 6);

    result = RunFile(tmpfile, 0, "\n", 1);

    /* Clean up */
    DeleteFile(tmpfile);
    CloseLibrary(emubase);

    if (result == 42) {
        printf("  MOVEQ #42,D0; RTS -> %ld\n", (long)result);
        printf("PASS\n");
        return 0;
    } else {
        printf("  MOVEQ #42,D0; RTS -> %ld (expected 42)\n", (long)result);
        printf("FAIL\n");
        return 20;
    }
}
