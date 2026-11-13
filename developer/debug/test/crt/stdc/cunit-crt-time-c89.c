/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    CUnit tests for the C89 (ISO/IEC 9899:1990) <time.h> functions:
    asctime(), ctime(), mktime(), gmtime(), localtime() and difftime().

    These cover the standard-mandated behaviour:
      - asctime() produces the fixed 26-byte "Www Mmm dd hh:mm:ss yyyy\n"
        representation (C89 7.12.3.1), independent of locale,
      - ctime() is asctime(localtime(t)),
      - mktime() normalises an out-of-range struct tm and recomputes
        tm_wday / tm_yday (C89 7.12.2.3),
      - gmtime() converts a time_t to UTC broken-down time.
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include <CUnit/Basic.h>
#include <CUnit/Automated.h>

int init_suite(void)
{
    return 0;
}

int clean_suite(void)
{
    return 0;
}

/* The canonical example from the C standard: a struct tm describing
   21:49:08 on Wednesday 30 June 1993 must format as
   "Wed Jun 30 21:49:08 1993\n". */
void test_asctime(void)
{
    struct tm tm;
    char *s;

    memset(&tm, 0, sizeof(tm));
    tm.tm_sec  = 8;
    tm.tm_min  = 49;
    tm.tm_hour = 21;
    tm.tm_mday = 30;
    tm.tm_mon  = 5;     /* June */
    tm.tm_year = 93;    /* 1993 */
    tm.tm_wday = 3;     /* Wednesday */
    tm.tm_yday = 180;
    tm.tm_isdst = 0;

    s = asctime(&tm);
    CU_ASSERT_PTR_NOT_NULL_FATAL(s);
    CU_ASSERT_STRING_EQUAL(s, "Wed Jun 30 21:49:08 1993\n");
    /* Exactly 24 visible characters + newline. */
    CU_ASSERT_EQUAL(strlen(s), 25);
}

/* asctime() must space-pad a single-digit day of month. */
void test_asctime_pad(void)
{
    struct tm tm;
    char *s;

    memset(&tm, 0, sizeof(tm));
    tm.tm_sec  = 7;
    tm.tm_min  = 5;
    tm.tm_hour = 3;
    tm.tm_mday = 6;
    tm.tm_mon  = 0;     /* January */
    tm.tm_year = 100;   /* 2000 */
    tm.tm_wday = 4;     /* Thursday */

    s = asctime(&tm);
    CU_ASSERT_PTR_NOT_NULL_FATAL(s);
    CU_ASSERT_STRING_EQUAL(s, "Thu Jan  6 03:05:07 2000\n");
}

/* ctime(t) == asctime(localtime(t)); with no timezone offset on AROS this
   also equals asctime(gmtime(t)).  Verify the shape and the epoch value. */
void test_ctime(void)
{
    time_t t = 0;       /* 1970-01-01 00:00:00 UTC, a Thursday */
    char *s = ctime(&t);

    CU_ASSERT_PTR_NOT_NULL_FATAL(s);
    CU_ASSERT_EQUAL(strlen(s), 25);
    /* The year must be present and the string newline-terminated. */
    CU_ASSERT_PTR_NOT_NULL(strstr(s, "1970"));
    CU_ASSERT_EQUAL(s[24], '\n');
}

/* gmtime() of the epoch is 1970-01-01 00:00:00, Thursday (tm_wday==4). */
void test_gmtime_epoch(void)
{
    time_t t = 0;
    struct tm *tm = gmtime(&t);

    CU_ASSERT_PTR_NOT_NULL_FATAL(tm);
    CU_ASSERT_EQUAL(tm->tm_year, 70);
    CU_ASSERT_EQUAL(tm->tm_mon, 0);
    CU_ASSERT_EQUAL(tm->tm_mday, 1);
    CU_ASSERT_EQUAL(tm->tm_hour, 0);
    CU_ASSERT_EQUAL(tm->tm_min, 0);
    CU_ASSERT_EQUAL(tm->tm_sec, 0);
    CU_ASSERT_EQUAL(tm->tm_wday, 4);    /* Thursday */
    CU_ASSERT_EQUAL(tm->tm_yday, 0);
}

/* mktime() must recompute tm_wday and tm_yday for an otherwise valid date.
   2023-03-15 is a Wednesday (tm_wday==3) and the 73rd day of the year
   (tm_yday==73, zero-based). */
void test_mktime_wday_yday(void)
{
    struct tm tm;
    time_t t;

    memset(&tm, 0, sizeof(tm));
    tm.tm_year = 123;   /* 2023 */
    tm.tm_mon  = 2;     /* March */
    tm.tm_mday = 15;
    tm.tm_hour = 12;
    tm.tm_min  = 0;
    tm.tm_sec  = 0;
    tm.tm_wday = -1;    /* deliberately wrong; mktime must fix */
    tm.tm_yday = -1;
    tm.tm_isdst = 0;

    t = mktime(&tm);
    CU_ASSERT_NOT_EQUAL_FATAL(t, (time_t)-1);
    CU_ASSERT_EQUAL(tm.tm_wday, 3);     /* Wednesday */
    CU_ASSERT_EQUAL(tm.tm_yday, 73);
}

/* mktime() must normalise out-of-range members.  60 seconds rolls into the
   next minute; 24 hours rolls into the next day. */
void test_mktime_normalise(void)
{
    struct tm tm;

    memset(&tm, 0, sizeof(tm));
    tm.tm_year = 123;   /* 2023 */
    tm.tm_mon  = 0;     /* January */
    tm.tm_mday = 1;
    tm.tm_hour = 0;
    tm.tm_min  = 0;
    tm.tm_sec  = 60;    /* -> 00:01:00 */
    tm.tm_isdst = 0;

    CU_ASSERT_NOT_EQUAL_FATAL(mktime(&tm), (time_t)-1);
    CU_ASSERT_EQUAL(tm.tm_sec, 0);
    CU_ASSERT_EQUAL(tm.tm_min, 1);
}

/* mktime() round-trips with gmtime() when the broken-down time is treated as
   UTC (AROS has no timezone offset): gmtime(mktime(tm)) reproduces tm. */
void test_mktime_roundtrip(void)
{
    struct tm tm, *back;
    time_t t;

    memset(&tm, 0, sizeof(tm));
    tm.tm_year = 130;   /* 2030 */
    tm.tm_mon  = 6;     /* July */
    tm.tm_mday = 4;
    tm.tm_hour = 1;
    tm.tm_min  = 2;
    tm.tm_sec  = 3;
    tm.tm_isdst = 0;

    t = mktime(&tm);
    CU_ASSERT_NOT_EQUAL_FATAL(t, (time_t)-1);

    back = gmtime(&t);
    CU_ASSERT_PTR_NOT_NULL_FATAL(back);
    CU_ASSERT_EQUAL(back->tm_year, 130);
    CU_ASSERT_EQUAL(back->tm_mon, 6);
    CU_ASSERT_EQUAL(back->tm_mday, 4);
    CU_ASSERT_EQUAL(back->tm_hour, 1);
    CU_ASSERT_EQUAL(back->tm_min, 2);
    CU_ASSERT_EQUAL(back->tm_sec, 3);
}

void test_difftime(void)
{
    CU_ASSERT_DOUBLE_EQUAL(difftime((time_t)100, (time_t)40), 60.0, 0.0001);
    CU_ASSERT_DOUBLE_EQUAL(difftime((time_t)40, (time_t)100), -60.0, 0.0001);
}

int main(void)
{
    CU_pSuite pSuite = NULL;

    if (CUE_SUCCESS != CU_initialize_registry())
        return CU_get_error();

    pSuite = CU_add_suite("TIME_C89_Suite", init_suite, clean_suite);
    if (NULL == pSuite) {
        CU_cleanup_registry();
        return CU_get_error();
    }

    if ((NULL == CU_add_test(pSuite, "asctime", test_asctime)) ||
        (NULL == CU_add_test(pSuite, "asctime_pad", test_asctime_pad)) ||
        (NULL == CU_add_test(pSuite, "ctime", test_ctime)) ||
        (NULL == CU_add_test(pSuite, "gmtime_epoch", test_gmtime_epoch)) ||
        (NULL == CU_add_test(pSuite, "mktime_wday_yday", test_mktime_wday_yday)) ||
        (NULL == CU_add_test(pSuite, "mktime_normalise", test_mktime_normalise)) ||
        (NULL == CU_add_test(pSuite, "mktime_roundtrip", test_mktime_roundtrip)) ||
        (NULL == CU_add_test(pSuite, "difftime", test_difftime)))
    {
        CU_cleanup_registry();
        return CU_get_error();
    }

    CU_basic_set_mode(CU_BRM_VERBOSE);
    CU_basic_run_tests();
    CU_basic_set_mode(CU_BRM_SILENT);
    CU_automated_package_name_set("CRTUnitTests");
    CU_set_output_filename("CRT-TIME-C89");
    CU_automated_enable_junit_xml(CU_TRUE);
    CU_automated_run_tests();
    CU_cleanup_registry();

    return CU_get_error();
}
