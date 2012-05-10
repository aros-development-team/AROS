/*
 * Copyright (C) 2012, The AROS Development Team
 * All right reserved.
 * Author: Jason S. McMullan <jason.mcmullan@gmail.com>
 *
 * Licensed under the AROS PUBLIC LICENSE (APL) Version 1.1
 *
 * BCPL string function tests
 */

#include <string.h>     /* memset */

#define BCPL(x,func) const int BCPL_##func = (x) / 4;
#include "bcpl.inc"
#undef BCPL

#include <aros/asmcall.h>
#include <proto/exec.h>
#include <proto/dos.h>

static ULONG BCPL_Thunk(struct Library *DOSBase, int gvfunc, ULONG d1, ULONG d2, ULONG d3, ULONG d4)
{
    struct Process *me = (struct Process *)FindTask(NULL);
    ULONG *gv = (ULONG *)me->pr_GlobVec;

    return AROS_UFC12(ULONG, (APTR)((struct DosLibrary *)DOSBase)->dl_A5,
            AROS_UFCA(ULONG,  0, D0),
            AROS_UFCA(ULONG,  d1, D1),
            AROS_UFCA(ULONG,  d2, D2),
            AROS_UFCA(ULONG,  d3, D3),
            AROS_UFCA(ULONG,  d4, D4),
            AROS_UFCA(APTR, 0, A0),
            AROS_UFCA(APTR, me->pr_Task.tc_SPLower, A1),
            AROS_UFCA(APTR, gv, A2),
            AROS_UFCA(APTR, 0, A3),
            AROS_UFCA(APTR, gv[gvfunc], A4),
            AROS_UFCA(APTR, ((struct DosLibrary *)DOSBase)->dl_A5, A5),
            AROS_UFCA(APTR, ((struct DosLibrary *)DOSBase)->dl_A6, A6));
}

#define BCPL_THUNK1(func, a1)               BCPL_Thunk(DOSBase, BCPL_##func, a1, 0, 0, 0)
#define BCPL_THUNK2(func, a1, a2)           BCPL_Thunk(DOSBase, BCPL_##func, a1, a2, 0, 0)
#define BCPL_THUNK3(func, a1, a2, a3)       BCPL_Thunk(DOSBase, BCPL_##func, a1, a2, a3, 0)
#define BCPL_THUNK4(func, a1, a2, a3, a4)   BCPL_Thunk(DOSBase, BCPL_##func, a1, a2, a3, a4)

#define VERIFY(f)       do { total++; int _val = ({f}); if (_val) { Printf("x");} else { Printf("."); passed++; } } while (0)

__startup int _main(void)
{
    struct Library *DOSBase;
    TEXT buff[512];
    STRPTR cp = &buff[128], val;
    BSTR bval;
    int total = 0, passed = 0;

    if ((DOSBase = OpenLibrary("dos.library", 0))) {
        Printf("Testing BCPL string functions:\n");

        /* toCStr Testing */
        Printf("toCStr: ");

        VERIFY(
            memset(buff, 0x99, sizeof(buff));
            cp[0] = 0;
            val = (STRPTR)BCPL_THUNK1(toCStr, MKBADDR(cp));

            (val == cp) && (val[0] == 0) &&
            (val[-1] == 0x99) && (val[1] == 0x99);
            );

        VERIFY(
            memset(buff, 0x99, sizeof(buff));
            cp[0] = 1;
            cp[1] = 'a';
            val = (STRPTR)BCPL_THUNK1(toCStr, MKBADDR(cp));

            (val == cp) && (val[0] == 'a') &&
            (val[1] == 0) && (val[2] == 0x99) &&
            (val[-1] == 0x99);
            );

        VERIFY(
            memset(buff, 0x99, sizeof(buff));
            memset(cp, 'b', 256);
            cp[0] = 255;
            val = (STRPTR)BCPL_THUNK1(toCStr, MKBADDR(cp));

            (val == cp) && (val[0] == 1) &&
            (val[1] == 'b') && (val[255] == 0x0) &&
            (val[256] == 0x99) && (val[-1] == 0x99);
            );

        Printf("%ld/%ld\n", passed, total);

        /* toBSTR Testing */
        passed = failed = 0;
        Printf("toBSTR: ");

        VERIFY(
            memset(buff, 0x99, sizeof(buff));
            cp[0] = 0;
            bval = (BSTR)BCPL_THUNK2(toBSTR, (IPTR)NULL, MKBADDR(cp));

            (bval == MKBADDR(cp)) && (cp[0] == 0) &&
            (cp[-1] == 0x99) && (cp[1] == 0x99);
            );

        VERIFY(
            memset(buff, 0x99, sizeof(buff));
            bval = (BSTR)BCPL_THUNK2(toBSTR, (IPTR)"", MKBADDR(cp));

            (bval == MKBADDR(cp)) && (cp[0] == 0) &&
            (cp[-1] == 0x99) && (cp[1] == 0x99);
            );

        VERIFY(
            memset(buff, 0x99, sizeof(buff));
            bval = (BSTR)BCPL_THUNK2(toBSTR, (IPTR)"a", MKBADDR(cp));

            (bval == MKBADDR(cp)) && (cp[0] == 1) &&
            (cp[1] == 'a') &&
            (cp[-1] == 0x99) && (cp[2] == 0x99);
            );

        VERIFY(
            memset(buff, 0x99, sizeof(buff));
            bval = (BSTR)BCPL_THUNK2(toBSTR, (IPTR)"123456789001234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678912345", MKBADDR(cp));

            (bval == MKBADDR(cp)) && (cp[0] == 255) &&
            (cp[1] == '1') && (cp[255] == '5') &&
            (cp[-1] == 0x99) && (cp[256] == 0x99);
            );

        VERIFY(
            memset(buff, 0x99, sizeof(buff));
            bval = (BSTR)BCPL_THUNK2(toBSTR, (IPTR)"1234567890012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789123456", MKBADDR(cp));

            (bval == MKBADDR(cp)) && (cp[0] == 0) &&
            (cp[-1] == 0x99) && (cp[256] == 0x99);
            );

        Printf("%ld/%ld\n", passed, total);

        CloseLibrary(DOSBase);
    }

    return (passed == total) ? RETURN_OK : RETURN_FAIL;
}



