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

#ifndef BNULL
#define BNULL 0
#endif

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
    RITEST("\"word\"",ITEM_QUOTED,"word",6);
    RITEST("\"word word2\"",ITEM_QUOTED,"word word2",12);
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

    /* Edge1: Buffer is NULL */
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

    /* Edge2: Buffer size is 0 */
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

    /* Edge3: Input is NIL: */
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

    /* Edge4: Input is BNULL */
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

    /* Edge5: Buffer length < 0 */
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

    /* Edge6: Buffer size is equal to input length */
    {
        int lfailed = 0;
        LONG ioerr;
        BYTE buff[] = { 0x11, 0x22, 0x33, 0x44, 0x55 };
        BYTE input[] = { '1', '2', '3', '4' };
        struct CSource cs = {
            .CS_Buffer = input,
            .CS_Length = 4,
            .CS_CurChr = 0,
        };
        SetIoErr(IOERR_UNCHANGED);
        ret = ReadItem(buff, 4, &cs);
        ioerr = IoErr();
        tests++;
        lfailed |= (ret != ITEM_ERROR) ? 1 : 0;
        lfailed |= (ioerr != IOERR_UNCHANGED) ? 1 : 0;
        lfailed |= (buff[0] != '1') ? 1 : 0;
        lfailed |= (buff[1] != '2') ? 1 : 0;
        lfailed |= (buff[2] != '3') ? 1 : 0;
        lfailed |= (buff[3] != 0) ? 1 : 0;
        lfailed |= (buff[4] != 0x55) ? 1 : 0;
        if (lfailed) {
            Printf("Edge6: expected %ld (%ld), buff[] = \"123\",0x0,0x55\n", ITEM_ERROR, IOERR_UNCHANGED);
            Printf("Edge6: returned %ld (%ld), buff[] = \"%lc%lc%lc\",0x%lx,0x%lx\n", ret, ioerr, (LONG)buff[0], (LONG)buff[1], (LONG)buff[2], (LONG)buff[3], (LONG)buff[4]);
        }
        failed += lfailed;
    }

    /* Edge7: Buffer size is one less than input length */
    {
        int lfailed = 0;
        LONG ioerr;
        BYTE buff[] = { 0x11, 0x22, 0x33, 0x44, 0x55 };
        BYTE input[] = { '1', '2', '3', '4' };
        struct CSource cs = {
            .CS_Buffer = input,
            .CS_Length = 4,
            .CS_CurChr = 0
        };
        SetIoErr(IOERR_UNCHANGED);
        ret = ReadItem(buff, 3, &cs);
        ioerr = IoErr();
        tests++;
        lfailed |= (ret != ITEM_ERROR) ? 1 : 0;
        lfailed |= (ioerr != IOERR_UNCHANGED) ? 1 : 0;
        lfailed |= (buff[0] != '1') ? 1 : 0;
        lfailed |= (buff[1] != '2') ? 1 : 0;
        lfailed |= (buff[2] != 0) ? 1 : 0;
        lfailed |= (buff[3] != 0x44) ? 1 : 0;
        lfailed |= (buff[4] != 0x55) ? 1 : 0;
        if (lfailed) {
            Printf("Edge7: expected %ld (%ld), buff[] = \"12\",0x00,0x44,0x55\n", ITEM_ERROR, IOERR_UNCHANGED);
            Printf("Edge7: returned %ld (%ld), buff[] = \"%lc%lc\",0x%lx,0x%lx,0x%lx\n", ret, ioerr, (LONG)buff[0], (LONG)buff[1], (LONG)buff[2], (LONG)buff[3], (LONG)buff[4]);
        }
        failed += lfailed;
    }

    /* Edge8: Buffer size is one more than input length */
    {
        int lfailed = 0;
        LONG ioerr;
        BYTE buff[] = { 0x11, 0x22, 0x33, 0x44, 0x55 };
        BYTE input[] = { '1', '2', '3', '4' };
        struct CSource cs = {
            .CS_Buffer = input,
            .CS_Length = 4,
            .CS_CurChr = 0,
        };
        SetIoErr(IOERR_UNCHANGED);
        ret = ReadItem(buff, 5, &cs);
        ioerr = IoErr();
        tests++;
        lfailed |= (ret != ITEM_UNQUOTED) ? 1 : 0;
        lfailed |= (ioerr != IOERR_UNCHANGED) ? 1 : 0;
        lfailed |= (buff[0] != '1') ? 1 : 0;
        lfailed |= (buff[1] != '2') ? 1 : 0;
        lfailed |= (buff[2] != '3') ? 1 : 0;
        lfailed |= (buff[3] != '4') ? 1 : 0;
        lfailed |= (buff[4] != 0) ? 1 : 0;
        if (lfailed) {
            Printf("Edge8: expected %ld (%ld), buff[] = \"1234\",0x0\n", 0, IOERR_UNCHANGED);
            Printf("Edge8: returned %ld (%ld), buff[] = \"%lc%lc%lc%lc\",0x%lx\n", ret, ioerr, (LONG)buff[0], (LONG)buff[1], (LONG)buff[2], (LONG)buff[3], (LONG)buff[4]);
        }
        failed += lfailed;
    }

    if (failed == 0)
        Printf("All %ld tests passed\n", tests);
    else
        Printf("FAILED: %ld out of %ld tests.\n", failed, tests);

    return (failed) ? RETURN_WARN : RETURN_OK;
}
