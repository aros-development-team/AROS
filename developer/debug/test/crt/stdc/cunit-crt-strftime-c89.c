/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    CUnit tests for the C89/C99 (ISO/IEC 9899) strftime() conversion
    specifiers.  In addition to the long-supported set (%a %A %b %B %d %H %I
    %m %M %p %S %y %Y %% %n %t) this exercises the specifiers that used to be
    silently dropped: %C %e %j %u %w %k %l %U %W %V %G %g %P %z and the
    compound forms %D %F %R %T %r %c %x %X (C99 7.23.3.5).
*/

#include <stdio.h>
#include <string.h>
#include <time.h>
#include <locale.h>

#include <CUnit/Basic.h>
#include <CUnit/Automated.h>

/* Wednesday 30 June 1993, 21:49:08 (tm_yday 180, 0-based). */
static struct tm ref_tm;

int init_suite(void)
{
    setlocale(LC_ALL, "C");

    memset(&ref_tm, 0, sizeof(ref_tm));
    ref_tm.tm_sec  = 8;
    ref_tm.tm_min  = 49;
    ref_tm.tm_hour = 21;
    ref_tm.tm_mday = 30;
    ref_tm.tm_mon  = 5;     /* June */
    ref_tm.tm_year = 93;    /* 1993 */
    ref_tm.tm_wday = 3;     /* Wednesday */
    ref_tm.tm_yday = 180;
    ref_tm.tm_isdst = 0;

    return 0;
}

int clean_suite(void) { return 0; }

/* Helper: format ref_tm with 'fmt' and compare against 'expect'. */
static void chk(const char *fmt, const char *expect)
{
    char buf[64];
    size_t n = strftime(buf, sizeof(buf), fmt, &ref_tm);

    CU_ASSERT_EQUAL(n, strlen(expect));
    CU_ASSERT_STRING_EQUAL(buf, expect);
}

void test_strftime_numeric(void)
{
    chk("%Y", "1993");
    chk("%y", "93");
    chk("%C", "19");
    chk("%m", "06");
    chk("%d", "30");
    chk("%e", "30");
    chk("%H", "21");
    chk("%I", "09");
    chk("%M", "49");
    chk("%S", "08");
    chk("%j", "181");
    chk("%u", "3");
    chk("%w", "3");
}

void test_strftime_names(void)
{
    chk("%a", "Wed");
    chk("%A", "Wednesday");
    chk("%b", "Jun");
    chk("%B", "June");
    /* AROS strftime takes the AM/PM strings from the system locale, whose
       English default is lowercase ("am"/"pm"); %P is lowercase by definition. */
    chk("%p", "pm");
    chk("%P", "pm");
}

void test_strftime_space_padded(void)
{
    /* %e/%k/%l space-pad to width 2.  Build an early-month single-digit date. */
    struct tm tm = ref_tm;
    char buf[16];

    tm.tm_mday = 6;
    tm.tm_hour = 3;     /* 03:.. -> %k " 3", %l " 3" */
    strftime(buf, sizeof(buf), "%e", &tm);
    CU_ASSERT_STRING_EQUAL(buf, " 6");
    strftime(buf, sizeof(buf), "%k", &tm);
    CU_ASSERT_STRING_EQUAL(buf, " 3");
    strftime(buf, sizeof(buf), "%l", &tm);
    CU_ASSERT_STRING_EQUAL(buf, " 3");
}

void test_strftime_week(void)
{
    chk("%U", "26");
    chk("%W", "26");
    chk("%V", "26");
    chk("%G", "1993");
    chk("%g", "93");
}

/* ISO 8601 edge case: 2021-01-01 is a Friday and belongs to week 53 of the
   ISO week-based year 2020. */
void test_strftime_iso_edge(void)
{
    struct tm tm;
    char buf[16];

    memset(&tm, 0, sizeof(tm));
    tm.tm_year = 121;   /* 2021 */
    tm.tm_mon  = 0;
    tm.tm_mday = 1;
    tm.tm_wday = 5;     /* Friday */
    tm.tm_yday = 0;

    strftime(buf, sizeof(buf), "%G", &tm);
    CU_ASSERT_STRING_EQUAL(buf, "2020");
    strftime(buf, sizeof(buf), "%V", &tm);
    CU_ASSERT_STRING_EQUAL(buf, "53");
    strftime(buf, sizeof(buf), "%g", &tm);
    CU_ASSERT_STRING_EQUAL(buf, "20");
}

void test_strftime_compound(void)
{
    chk("%F", "1993-06-30");
    chk("%D", "06/30/93");
    chk("%T", "21:49:08");
    chk("%R", "21:49");
    chk("%r", "09:49:08 pm");
    chk("%c", "Wed Jun 30 21:49:08 1993");
    chk("%x", "06/30/93");
    chk("%X", "21:49:08");
}

void test_strftime_literal(void)
{
    chk("%Y-%m-%d", "1993-06-30");
    chk("100%%", "100%");
    chk("%z", "+0000");

    /* An unknown specifier is skipped, not echoed. */
    chk("a%Qb", "ab");
}

int main(void)
{
    CU_pSuite pSuite = NULL;

    if (CUE_SUCCESS != CU_initialize_registry())
        return CU_get_error();

    pSuite = CU_add_suite("STRFTIME_C89_Suite", init_suite, clean_suite);
    if (NULL == pSuite) {
        CU_cleanup_registry();
        return CU_get_error();
    }

    if ((NULL == CU_add_test(pSuite, "numeric", test_strftime_numeric)) ||
        (NULL == CU_add_test(pSuite, "names", test_strftime_names)) ||
        (NULL == CU_add_test(pSuite, "space_padded", test_strftime_space_padded)) ||
        (NULL == CU_add_test(pSuite, "week", test_strftime_week)) ||
        (NULL == CU_add_test(pSuite, "iso_edge", test_strftime_iso_edge)) ||
        (NULL == CU_add_test(pSuite, "compound", test_strftime_compound)) ||
        (NULL == CU_add_test(pSuite, "literal", test_strftime_literal)))
    {
        CU_cleanup_registry();
        return CU_get_error();
    }

    CU_basic_set_mode(CU_BRM_VERBOSE);
    CU_basic_run_tests();
    CU_basic_set_mode(CU_BRM_SILENT);
    CU_automated_package_name_set("CRTUnitTests");
    CU_set_output_filename("CRT-STRFTIME-C89");
    CU_automated_enable_junit_xml(CU_TRUE);
    CU_automated_run_tests();
    CU_cleanup_registry();

    return CU_get_error();
}
