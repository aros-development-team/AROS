/*
 * Copyright (C) 2011, The AROS Development Team.  All rights reserved.
 * Author: Jason S. McMullan <jason.mcmullan@gmail.com>
 *
 * Licensed under the AROS PUBLIC LICENSE (APL) Version 1.1
 */

#include <aros/debug.h>
#include <proto/alib.h>
#include <proto/exec.h>
#include <proto/dos.h>

#include <dos/stdio.h>
#include <exec/lists.h>

#ifdef __mc68000
/* For compatability with AOS, our test reference */
static AROS_UFH2(VOID, _SPutC,
        AROS_UFHA(BYTE, c, D0),
        AROS_UFHA(BYTE **, ptr, A3))
{
    AROS_USERFUNC_INIT

    **ptr = c;

    (*ptr)++;

    AROS_USERFUNC_EXIT
}

VOID SPrintf( char *target, const char *format, ...)
{
    RawDoFmt( format, (APTR)&(((ULONG *)&format)[1]), _SPutC, &target);
}
#else
#define SPrintf(buff,format,args...) __sprintf(buff,format ,##args )
#endif
    
#define TEST_START(name) do { \
    CONST_STRPTR test_name = #name ; \
    struct List expr_list; \
    int failed = 0; \
    tests++; \
    NEWLIST(&expr_list);

#define VERIFY_EQ(retval, expected) \
    do { \
        __typeof__(retval) val = retval; \
        if (val != (expected)) { \
            static char buff[128]; \
            static struct Node expr_node; \
            SPrintf(buff, "%s (%ld) != %ld", #retval , (LONG)val, (LONG)expected); \
            expr_node.ln_Name = buff; \
            AddTail(&expr_list, &expr_node); \
            failed |= 1; \
        } \
    } while (0)

#define VERIFY_RET(func, ret, reterr) \
    do { \
        SetIoErr(-1); \
        VERIFY_EQ(func, ret); \
        VERIFY_EQ(IoErr(), reterr); \
    } while (0)

#define TEST_END() \
    if (failed) { \
        struct Node *node; \
        tests_failed++; \
        Printf("Test %ld: Failed (%s)\n", (LONG)tests, test_name); \
        ForeachNode(&expr_list, node) { \
            Printf("\t%s\n", node->ln_Name); \
        } \
    } \
} while (0)

int main(int argc, char **argv)
{
    BPTR fd;
    int tests = 0, tests_failed = 0;

    TEST_START("fd = BNULL");
        fd = BNULL;
        VERIFY_RET(IsInteractive(fd), DOSFALSE, -1);
    TEST_END();

    TEST_START("fd = Open(\"NIL:\", MODE_OLDFILE)");
        fd = Open("NIL:", MODE_OLDFILE);
        VERIFY_RET(IsInteractive(fd), DOSFALSE, -1);
        Close(fd);
    TEST_END();

    TEST_START("fd = Open(\"NIL:\", MODE_NEWFILE)");
        fd = Open("NIL:", MODE_NEWFILE);
        VERIFY_RET(IsInteractive(fd), DOSFALSE, -1);
        Close(fd);
    TEST_END();

    TEST_START("fd = Open(\"CON:0/0/100/100/Test/CLOSE/AUTO\", MODE_OLDFILE)");
        fd = Open("CON:0/0/100/100/Test/CLOSE/AUTO", MODE_OLDFILE);
        VERIFY_RET(IsInteractive(fd), DOSTRUE, -1);
        Close(fd);
    TEST_END();

    return 0;
}
    
