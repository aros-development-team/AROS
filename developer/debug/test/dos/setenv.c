/*
    Copyright (C) 1995-2015, The AROS Development Team. All rights reserved.
*/

#include <string.h>
#include <stdio.h>
#include <proto/dos.h>

static LONG testvalue(CONST_STRPTR var, STRPTR expval, ULONG explen,
    BOOL expnull, LONG cnt)
{
    TEXT buffer[10];
    LONG len, failed = 0;
    struct LocalVar *lv;

    if ((len = GetVar(var, buffer, sizeof(buffer), 0)) < 0)
    {
        printf("test %d ERROR getvar %d\n", (int)cnt, (int)len);
        failed++;
    }
    else
        printf("test %d getvar '%s'\n", (int)cnt, buffer);

    if ((lv = FindVar(var, 0)) == NULL)
    {
        printf("test %d ERROR findvar\n", (int)cnt);
        failed++;
    }
    else
        printf("test %d findvar lv_Value=%p, lv_Len=%d\n", (int)cnt,
            lv->lv_Value, (int)lv->lv_Len);

    if (lv->lv_Len != explen)
    {
        printf("test %d ERROR lv_Len, expected %d, found %d\n", (int)cnt,
            (int)explen, (int)lv->lv_Len);
        failed++;
    }

    if (expnull && lv->lv_Value != NULL)
    {
        printf("test %d ERROR lv_Value expected NULL, found %p\n", (int)cnt,
            lv->lv_Value);
        failed++;
    }

    if (!expnull && lv->lv_Value == NULL)
    {
        printf("test %d ERROR lv_Value expected not NULL, found %p\n",
            (int)cnt, lv->lv_Value);
        failed++;
    }

    return failed;
}

int main(void)
{
    CONST_STRPTR var = "abc";
    STRPTR val = "cde";
    LONG cnt = 1, failed = 0;

    /* Behavior validated with OS3.x */

    printf("test %d setvar '%s'\n", (int)cnt, val);
    if (SetVar(var, val, strlen(val), 0) == DOSFALSE)
    {
        printf("error setvar\n");
        failed++;
    }
    failed += testvalue(var, val, 3, FALSE, cnt++);

    val = "";
    printf("test %d setvar '%s'\n", (int)cnt, val);
    if (SetVar(var, val, strlen(val), 0) == DOSFALSE)
    {
        printf("error setvar\n");
        failed++;
    }
    failed += testvalue(var, val, 0, TRUE, cnt++);

    val = "abcd";
    printf("test %d setvar '%s'\n", (int)cnt, val);
    if (SetVar(var, val, strlen(val), 0) == DOSFALSE)
    {
        printf("error setvar\n");
        failed++;
    }
    failed += testvalue(var, val, 4, FALSE, cnt++);

    val = "";
    printf("test %d setvar '%s'\n", (int)cnt, val);
    if (SetVar(var, val, strlen(val), 0) == DOSFALSE)
    {
        printf("error setvar\n");
        failed++;
    }
    failed += testvalue(var, val, 0, TRUE, cnt++);

    val = "";
    printf("test %d setvar '%s'\n", (int)cnt, val);
    if (SetVar(var, val, strlen(val), 0) == DOSFALSE)
    {
        printf("error setvar\n");
        failed++;
    }
    failed += testvalue(var, val, 0, TRUE, cnt++);

    return failed == 0 ? RETURN_OK : RETURN_ERROR;
}
