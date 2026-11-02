/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.
    Author: Fabian Schmieder (@metaneutrons)

    On-target test for m68kemu.library.
    Writes a tiny m68k program (MOVEQ #42,D0; RTS) to T:,
    calls RunFile, and verifies the emulator returns 42.
*/

#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/m68kemu.h>
#include <exec/memory.h>
#include <dos/dostags.h>
#include <workbench/startup.h>
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

/* A program as Workbench starts it, with no CLI: FindTask(NULL), WaitPort
   and GetMsg on its pr_MsgPort, ReplyMsg of the WBStartup, return */
static const UBYTE m68k_wb_binary[] = {
    0x00, 0x00, 0x03, 0xF3,  /* HUNK_HEADER */
    0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x01,
    0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x0A,  /* hunk 0 size: 10 longwords */
    0x00, 0x00, 0x03, 0xE9,  /* HUNK_CODE */
    0x00, 0x00, 0x00, 0x0A,
    0x2C, 0x78, 0x00, 0x04,  /* MOVEA.L 4.W,A6 */
    0x93, 0xC9,              /* SUBA.L A1,A1 */
    0x4E, 0xAE, 0xFE, 0xDA,  /* JSR FindTask(A6) */
    0x24, 0x40,              /* MOVEA.L D0,A2 */
    0x47, 0xEA, 0x00, 0x5C,  /* LEA pr_MsgPort(A2),A3 */
    0x20, 0x4B,              /* MOVEA.L A3,A0 */
    0x4E, 0xAE, 0xFE, 0x80,  /* JSR WaitPort(A6) */
    0x20, 0x4B,              /* MOVEA.L A3,A0 */
    0x4E, 0xAE, 0xFE, 0x8C,  /* JSR GetMsg(A6) */
    0x22, 0x40,              /* MOVEA.L D0,A1 */
    0x4E, 0xAE, 0xFE, 0x86,  /* JSR ReplyMsg(A6) */
    0x70, 0x00,              /* MOVEQ #0,D0 */
    0x4E, 0x75,              /* RTS */
    0x00, 0x00,
    0x00, 0x00, 0x03, 0xF2,  /* HUNK_END */
};

/* Started the way workbench.library starts a tool: CreateNewProc() of the
   seglist, then the WBStartup to its pr_MsgPort. The program has to get the
   message and reply it; the reply must come back within ten seconds. */
static int wb_test(void)
{
    const char *tmpfile = "RAM:m68kemu_wbtest.bin";
    struct WBStartup *wbs;
    struct MsgPort *reply;
    struct Process *proc;
    BPTR fh, seg;
    int i, ok = 0;

    fh = Open(tmpfile, MODE_NEWFILE);
    if (!fh) {
        printf("FAIL: cannot create %s\n", tmpfile);
        return 20;
    }
    Write(fh, (APTR)m68k_wb_binary, sizeof(m68k_wb_binary));
    Close(fh);

    seg = LoadSeg(tmpfile);
    reply = CreateMsgPort();
    wbs = AllocVec(sizeof(struct WBStartup) + sizeof(struct WBArg), MEMF_PUBLIC | MEMF_CLEAR);
    if (!seg || !reply || !wbs) {
        printf("FAIL: LoadSeg/CreateMsgPort/AllocVec for the Workbench start\n");
        if (seg) UnLoadSeg(seg);
        if (reply) DeleteMsgPort(reply);
        FreeVec(wbs);
        DeleteFile(tmpfile);
        return 20;
    }
    wbs->sm_Message.mn_ReplyPort = reply;
    wbs->sm_Message.mn_Length = sizeof(struct WBStartup);
    wbs->sm_Segment = seg;
    wbs->sm_NumArgs = 1;
    wbs->sm_ArgList = (struct WBArg *)(wbs + 1);
    wbs->sm_ArgList[0].wa_Lock = Lock("RAM:", SHARED_LOCK);
    wbs->sm_ArgList[0].wa_Name = "m68kemu_wbtest.bin";

    proc = CreateNewProcTags(NP_Seglist, (IPTR)seg, NP_Name, (IPTR)"m68kemu wbtest",
                             NP_Input, (IPTR)BNULL, NP_Output, (IPTR)BNULL,
                             NP_CurrentDir, (IPTR)BNULL, TAG_DONE);
    if (!proc) {
        printf("FAIL: CreateNewProc() of the 68k seglist\n");
    } else {
        wbs->sm_Process = &proc->pr_MsgPort;
        PutMsg(&proc->pr_MsgPort, &wbs->sm_Message);
        for (i = 0; i < 100 && !ok; i++) {
            if (GetMsg(reply))
                ok = 1;
            else
                Delay(5);
        }
        printf("  Workbench start: WBStartup %s\n", ok ? "replied" : "not replied in 10 s");
    }
    if (ok) {
        UnLock(wbs->sm_ArgList[0].wa_Lock);
        FreeVec(wbs);
    }
    DeleteMsgPort(reply);
    DeleteFile(tmpfile);
    return ok ? 0 : 20;
}

int main(void)
{
    struct Library *M68KEmuBase;
    LONG result;
    BPTR fh;
    const char *tmpfile = "RAM:m68kemu_test.bin";

    printf("m68kemu on-target test\n");

    /* Open the library */
    M68KEmuBase = OpenLibrary("m68kemu.library", 0);
    if (!M68KEmuBase) {
        printf("FAIL: cannot open m68kemu.library\n");
        return 20;
    }
    printf("  m68kemu.library opened at %p\n", M68KEmuBase);

    /* Write test binary to T: */
    fh = Open(tmpfile, MODE_NEWFILE);
    if (!fh) {
        printf("FAIL: cannot create %s\n", tmpfile);
        CloseLibrary(M68KEmuBase);
        return 20;
    }
    Write(fh, (APTR)m68k_test_binary, sizeof(m68k_test_binary));
    Close(fh);

    result = RunFile(tmpfile, 0, "\n", 1);

    /* Clean up */
    DeleteFile(tmpfile);
    CloseLibrary(M68KEmuBase);

    if (result != 42) {
        printf("  MOVEQ #42,D0; RTS -> %ld (expected 42)\n", (long)result);
        printf("FAIL\n");
        return 20;
    }
    printf("  MOVEQ #42,D0; RTS -> %ld\n", (long)result);

    if (wb_test() != 0) {
        printf("FAIL\n");
        return 20;
    }
    printf("PASS\n");
    return 0;
}
