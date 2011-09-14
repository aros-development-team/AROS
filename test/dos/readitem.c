/*
 * Copyright (C) 2011, The AROS Development Team.  All rights reserved.
 * Author: Jason S. McMullan <jason.mcmullan@gmail.com>
 *
 * Licensed under the AROS PUBLIC LICENSE (APL) Version 1.1
 */
#include <string.h>

#include <proto/exec.h>
#include <proto/dos.h>

#include <dos/stdio.h>

#define IOERR_UNCHANGED -1

static int ritest_buff(ULONG test, CONST_STRPTR in, LONG ret, LONG ioerr, CONST_STRPTR out, LONG cur)
{
    static char buff[256];
    struct CSource cs;
    int failed = 0;
    LONG err, reterr;

    cs.CS_Buffer = (APTR)in;
    cs.CS_Length = strlen(in);
    cs.CS_CurChr = 0;

    SetIoErr(IOERR_UNCHANGED);
    err = ReadItem(buff, sizeof(buff), &cs);
    reterr = IoErr();
    failed |= (err != ret) ? 1 : 0;
    failed |= (strcmp(buff, out) != 0) ? 1 : 0;
    failed |= (cur != cs.CS_CurChr) ? 1 : 0;
    failed |= (reterr != ioerr) ? 1 : 0;

    if (failed) {
        Printf("%ld: buff: in: (%s)\n", test, in);
        Printf("%ld: buff: expected: ret %ld (%ld), buf (%s), cur %ld\n", test, ret, ioerr, out, cur);
        Printf("%ld: buff: returned: ret %ld (%ld), buf (%s), cur %ld\n", test, err, reterr, buff, cs.CS_CurChr);
    }

    return failed;
}

static int ritest_file(ULONG test, CONST_STRPTR in, LONG ret, LONG ioerr, CONST_STRPTR out, LONG cur)
{
    static char buff[256];
    int failed = 0;
    LONG err, reterr;
    BPTR io, oldin;

    io = Open("RAM:readitem.tmp", MODE_NEWFILE);
    Write(io, in, strlen(in));
    Close(io);
    io = Open("RAM:readitem.tmp", MODE_OLDFILE);

    oldin = Input();
    SelectInput(io);
    SetIoErr(IOERR_UNCHANGED);
    err = ReadItem(buff, sizeof(buff), NULL);
    reterr = IoErr();
    SelectInput(oldin);

    failed |= (err != ret) ? 1 : 0;
    failed |= (strcmp(buff, out) != 0) ? 1 : 0;
    failed |= (reterr != ioerr) ? 1 : 0;

    if (failed) {
        Printf("%ld: file: in: (%s)\n", test, in);
        Printf("%ld: file: expected: ret %ld (%ld), buf (%s)\n", test, ret, ioerr, out);
        Printf("%ld: file: returned: ret %ld (%ld), buf (%s)\n", test, err, reterr, buff);
    }

    Close(io);
    DeleteFile("RAM:readitem.tmp");
    return failed;
}

#define RITEST(in, ret, out, cur) \
    do { \
        failed += ritest_buff(__LINE__, in, ret, IOERR_UNCHANGED, out, cur); \
        tests++; \
        failed += ritest_file(__LINE__, in, ret, RETURN_OK, out, cur); \
        tests++; \
    } while (0)

#define RITERR(in, ret, out, cur, ioerr) \
    do { \
        failed += ritest_buff(__LINE__, in, ret, IOERR_UNCHANGED, out, cur); \
        tests++; \
        failed += ritest_file(__LINE__, in, ret, ioerr, out, cur); \
        tests++; \
    } while (0)

int main(int argc, char **argv)
{
    int failed = 0, tests = 0;
    LONG ret;

    /* The following behaviour is versus AOS 3.1,
     * which should be used as the reference for
     * the AROS implementation
     */
    RITEST(";",ITEM_NOTHING,"",0);
    RITEST(";word",ITEM_NOTHING,"",0);
    RITEST(" ;word",ITEM_NOTHING,"",1);
    RITEST("\t;word",ITEM_NOTHING,"",1);
    RITEST("word",ITEM_UNQUOTED,"word",3);
    RITEST("word\n",ITEM_UNQUOTED,"word",4);
    RITEST("word=thing\n",ITEM_UNQUOTED,"word",5);
    RITEST("word crazy\n",ITEM_UNQUOTED,"word",5);
    RITEST("word\tcrazy\n",ITEM_UNQUOTED,"word",5);
    RITEST("\"word\"=thing\n",ITEM_QUOTED,"word",6);
    RITEST("\"word\" crazy\n",ITEM_QUOTED,"word",6);
    RITEST("\"word\"\tcrazy\n",ITEM_QUOTED,"word",6);
    RITERR("\"word",ITEM_ERROR,"word",4,0);
    RITERR("\"word\n\"\n",ITEM_ERROR,"word",5,0);
    RITEST("\"word=\"\n",ITEM_QUOTED,"word=",7);
    RITEST("\"word \"\n",ITEM_QUOTED,"word ",7);
    RITERR("\"word*",ITEM_ERROR,"word",5,0);
    RITERR("\"word*\n",ITEM_ERROR,"word",6,0);
    RITERR("\"word**",ITEM_ERROR,"word*",6,0);
    RITERR("\"word*e",ITEM_ERROR,"word\e",6,0);
    RITERR("\"word*E",ITEM_ERROR,"word\e",6,0);
    RITERR("\"word*n",ITEM_ERROR,"word\n",6,0);
    RITERR("\"word*N",ITEM_ERROR,"word\n",6,0);
    RITERR("\"word**\n",ITEM_ERROR,"word*",7,0);
    RITERR("\"word*e\n",ITEM_ERROR,"word\e",7,0);
    RITERR("\"word*E\n",ITEM_ERROR,"word\e",7,0);
    RITERR("\"word*n\n",ITEM_ERROR,"word\n",7,0);
    RITERR("\"word*N\n",ITEM_ERROR,"word\n",7,0);
    RITEST("\"word**\"",ITEM_QUOTED,"word*",8);
    RITEST("\"word*e\"",ITEM_QUOTED,"word\e",8);
    RITEST("\"word*E\"",ITEM_QUOTED,"word\e",8);
    RITEST("\"word*n\"",ITEM_QUOTED,"word\n",8);
    RITEST("\"word*N\"",ITEM_QUOTED,"word\n",8);
    RITEST(" word\n",ITEM_UNQUOTED,"word",5);
    RITEST("\nword\n",ITEM_NOTHING,"",0);
    RITEST("\"word\"\n",ITEM_QUOTED,"word",6);
    RITEST("\n",ITEM_NOTHING,"",0);
    RITEST("",ITEM_NOTHING,"",0);
    RITEST("word\"hello \"world",ITEM_UNQUOTED,"word\"hello",11);
    RITEST("",ITEM_NOTHING,"",0);

    /* Explicit tests for edge conditions */
    {
        int lfailed = 0;
        LONG ioerr;
        SetIoErr(IOERR_UNCHANGED);
        ret = ReadItem(NULL, 128, NULL);
        ioerr = IoErr();
        tests++;
        lfailed |= (ret != ITEM_NOTHING) ? 1 : 0;
        lfailed |= (ioerr != IOERR_UNCHANGED) ? 1 : 0;
        if (lfailed) {
            Printf("Edge1: expected %ld (%ld)\n", 0, IOERR_UNCHANGED);
            Printf("Edge1: returned %ld (%ld)\n", ret, ioerr);
        }
        failed += lfailed;
    }

    {
        int lfailed = 0;
        LONG ioerr;
        BYTE buff[] = { 0x11, 0x22, 0x33, 0x44 };
        SetIoErr(IOERR_UNCHANGED);
        ret = ReadItem(buff, 0, NULL);
        ioerr = IoErr();
        tests++;
        lfailed |= (ret != ITEM_NOTHING) ? 1 : 0;
        lfailed |= (ioerr != IOERR_UNCHANGED) ? 1 : 0;
        lfailed |= (buff[0] != 0x0) ? 1 : 0;
        if (lfailed) {
            Printf("Edge2: expected %ld (%ld), buff[0] = 0x00\n", 0, IOERR_UNCHANGED);
            Printf("Edge2: returned %ld (%ld), buff[0] = 0x%02lx\n", ret, ioerr, (LONG)buff[0]);
        }
        failed += lfailed;
    }

    {
        int lfailed = 0;
        LONG ioerr;
        BPTR io, oldio;

        BYTE buff[] = { 0x11, 0x22, 0x33, 0x44 };

        io = Open("NIL:", MODE_OLDFILE);
        oldio = Input();
        SelectInput(io);
        SetIoErr(IOERR_UNCHANGED);
        ret = ReadItem(buff, sizeof(buff), NULL);
        ioerr = IoErr();
        SelectInput(oldio);
        Close(io);
        tests++;
        lfailed |= (ret != ITEM_NOTHING) ? 1 : 0;
        lfailed |= (ioerr != IOERR_UNCHANGED) ? 1 : 0;
        lfailed |= (buff[0] != 0x0) ? 1 : 0;
        if (lfailed) {
            Printf("Edge3: expected %ld (%ld), buff[0] = 0x00\n", ITEM_NOTHING, IOERR_UNCHANGED);
            Printf("Edge3: returned %ld (%ld), buff[0] = 0x%02lx\n", ret, ioerr, (LONG)buff[0]);
        }
        failed += lfailed;
    }

    {
        int lfailed = 0;
        LONG ioerr;
        BPTR oldio;
        BYTE buff[] = { 0x11, 0x22, 0x33, 0x44 };

        oldio = Input();
        SelectInput(BNULL);
        SetIoErr(IOERR_UNCHANGED);
        ret = ReadItem(buff, sizeof(buff), NULL);
        ioerr = IoErr();
        SelectInput(oldio);
        tests++;
        lfailed |= (ret != ITEM_NOTHING) ? 1 : 0;
        lfailed |= (ioerr != IOERR_UNCHANGED) ? 1 : 0;
        lfailed |= (buff[0] != 0x0) ? 1 : 0;
        if (lfailed) {
            Printf("Edge4: expected %ld (%ld), buff[0] = 0x00\n", ITEM_NOTHING, IOERR_UNCHANGED);
            Printf("Edge4: returned %ld (%ld), buff[0] = 0x%02lx\n", ret, ioerr, (LONG)buff[0]);
        }
        failed += lfailed;
    }

    {
        int lfailed = 0;
        LONG ioerr;
        BPTR oldio;
        BYTE buff[] = { 0x11, 0x22, 0x33, 0x44 };

        oldio = Input();
        SelectInput(BNULL);
        SetIoErr(IOERR_UNCHANGED);
        ret = ReadItem(buff, -1, NULL);
        ioerr = IoErr();
        SelectInput(oldio);
        tests++;
        lfailed |= (ret != ITEM_NOTHING) ? 1 : 0;
        lfailed |= (ioerr != IOERR_UNCHANGED) ? 1 : 0;
        lfailed |= (buff[0] != 0x0) ? 1 : 0;
        if (lfailed) {
            Printf("Edge5: expected %ld (%ld), buff[0] = 0x00\n", ITEM_NOTHING, IOERR_UNCHANGED);
            Printf("Edge5: returned %ld (%ld), buff[0] = 0x%02lx\n", ret, ioerr, (LONG)buff[0]);
        }
        failed += lfailed;
    }

    if (failed == 0)
        Printf("All %ld tests passed\n", tests);
    else
        Printf("FAILED: %ld out of %ld tests.\n", failed, tests);

    return (failed) ? RETURN_WARN : RETURN_OK;
}
