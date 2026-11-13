/*
 * Copyright (C) 2011-2014, The AROS Development Team.  All rights reserved.
 * Author: Jason S. McMullan <jason.mcmullan@gmail.com>
 *
 * Licensed under the AROS PUBLIC LICENSE (APL) Version 1.1
 */

#ifndef TESTFRAME_H
#define TESTFRAME_H

#include <proto/alib.h>
#include <proto/exec.h>
#include <proto/dos.h>

#include <dos/stdio.h>
#include <exec/lists.h>

#ifdef __mc68000
/* For compatability with AOS, our test reference,
 * use this SPrintf()
 */
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
#define SPrintf(target,format,args...) __sprintf(target,format ,##args )
#endif

#define TESTING_BEGINS() \
    do { \
        int tests = 0, tests_failed = 0;

#define TESTING_ENDS() \
        if (tests_failed == 0) Printf("All %ld tests passed\n", (LONG)tests); \
        else Printf("%ld of %ld tests failed\n", (LONG)tests_failed, (LONG)tests); \
        Flush(Output()); \
        return (tests_failed == 0) ? RETURN_OK : RETURN_FAIL; \
    } while (0)

 
#define TEST_START(name) do { \
    CONST_STRPTR test_name = #name ; \
    struct List expr_list; \
    int failed = 0; \
    int subtest = 0; \
    tests++; \
    NEWLIST(&expr_list);

#define _STRINGED(x)    #x
#define STRINGED(x)     _STRINGED(x)

#define VERIFY_EQ(retval, expected) \
    do { \
        __typeof__(retval) val = retval; \
        subtest++; \
        if (val != (expected)) { \
            static char buff[128]; \
            static struct Node expr_node; \
            SPrintf(buff, "%ld.%ld %s (%ld) != %ld", tests, subtest, STRINGED(retval), (LONG)(IPTR)val, (LONG)(IPTR)expected); \
            expr_node.ln_Name = buff; \
            AddTail(&expr_list, &expr_node); \
            failed |= 1; \
        } \
    } while (0)

#define VERIFY_NEQ(retval, expected) \
    do { \
        __typeof__(retval) val = retval; \
        subtest++; \
        if (val == (expected)) { \
            static char buff[128]; \
            static struct Node expr_node; \
            SPrintf(buff, "%ld.%ld %s (%ld) == %ld", tests, subtest, STRINGED(retval) , (LONG)(IPTR)val, (LONG)(IPTR)expected); \
            expr_node.ln_Name = buff; \
            AddTail(&expr_list, &expr_node); \
            failed |= 1; \
        } \
    } while (0)

#define VERIFY_RET(func, ret, reterr) \
    do { \
        SetIoErr(-1); \
        VERIFY_EQ(func, ret); \
        subtest--; \
        VERIFY_EQ(IoErr(), reterr); \
    } while (0)

#define TEST_END() \
    (void)subtest; \
    if (failed) { \
        struct Node *node; \
        tests_failed++; \
        Printf("Test %ld: Failed (%s)\n", (LONG)tests, test_name); \
        ForeachNode(&expr_list, node) { \
            Printf("\t%s\n", node->ln_Name); \
        } \
    } \
} while (0)

#endif /* TESTFRAME_H */
