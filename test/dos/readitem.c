/*
 * Copyright (C) 2011, The AROS Development Team.  All rights reserved.
 * Author: Jason S. McMullan <jason.mcmullan@gmail.com>
 *
 * Licensed under the AROS PUBLIC LICENSE (APL) Version 1.1
 */
#include <string.h>

#include <proto/exec.h>
#include <proto/dos.h>

static int ritest(ULONG test, CONST_STRPTR in, LONG ret, CONST_STRPTR out, LONG cur)
{
    static char buff[256];
    struct CSource cs;
    int failed = 0;
    LONG err;

    cs.CS_Buffer = (APTR)in;
    cs.CS_Length = strlen(in);
    cs.CS_CurChr = 0;

    err = ReadItem(buff, sizeof(buff), &cs);
    failed |= (err != ret) ? 1 : 0;
    failed |= (strcmp(buff, out) != 0) ? 1 : 0;
    failed |= (cur != cs.CS_CurChr) ? 1 : 0;

    if (failed) {
        Printf("%ld: in: (%s)\n", test, in);
        Printf("%ld: expected: ret %ld, buf (%s), cur %ld\n", test, ret, out, cur);
        Printf("%ld: returned: ret %ld, buf (%s), cur %ld\n", test, err, buff, cs.CS_CurChr);
    }

    return failed;
}

#define RITEST(in, ret, out, cur) \
    do { \
        failed += ritest(__LINE__, in, ret, out, cur); \
        tests++; \
    } while (0)

int main(int argc, char **argv)
{
    int failed = 0, tests = 0;

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
    RITEST("\"word",ITEM_ERROR,"word",4);
    RITEST("\"word\n\"\n",ITEM_ERROR,"word",5);
    RITEST("\"word=\"\n",ITEM_QUOTED,"word=",7);
    RITEST("\"word \"\n",ITEM_QUOTED,"word ",7);
    RITEST("\"word*",ITEM_ERROR,"word",5);
    RITEST("\"word*\n",ITEM_ERROR,"word",6);
    RITEST("\"word**",ITEM_ERROR,"word*",6);
    RITEST("\"word*e",ITEM_ERROR,"word\e",6);
    RITEST("\"word*E",ITEM_ERROR,"word\e",6);
    RITEST("\"word*n",ITEM_ERROR,"word\n",6);
    RITEST("\"word*N",ITEM_ERROR,"word\n",6);
    RITEST("\"word**\n",ITEM_ERROR,"word*",7);
    RITEST("\"word*e\n",ITEM_ERROR,"word\e",7);
    RITEST("\"word*E\n",ITEM_ERROR,"word\e",7);
    RITEST("\"word*n\n",ITEM_ERROR,"word\n",7);
    RITEST("\"word*N\n",ITEM_ERROR,"word\n",7);
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

    if (failed == 0)
        Printf("All %ld tests passed\n", tests);
    else
        Printf("FAILED: %ld out of %ld tests.\n", failed, tests);

    return (failed) ? RETURN_WARN : RETURN_OK;
}
