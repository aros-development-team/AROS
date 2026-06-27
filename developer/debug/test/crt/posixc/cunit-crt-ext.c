/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    CUnit tests for the CRT functions added on top of the C89 baseline:
      - POSIX.1-2008 getline()/getdelim()/dprintf() (posixc.library)
      - C23/POSIX memccpy() and C11 timespec_get()/at_quick_exit() (stdc.library)
*/

#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <fcntl.h>
#include <unistd.h>

#include <CUnit/Basic.h>
#include <CUnit/Automated.h>

#define TESTFILE "T:cunit-crt-ext.txt"

int init_suite(void)
{
    return 0;
}

int clean_suite(void)
{
    remove(TESTFILE);
    return 0;
}

/* ---- memccpy() (stdc) ---------------------------------------------- */

void testMEMCCPY(void)
{
    char dst[16];
    char *r;

    memset(dst, 0, sizeof(dst));
    /* stop after copying 'c' */
    r = memccpy(dst, "abcdef", 'c', sizeof(dst));
    CU_ASSERT_PTR_NOT_NULL_FATAL(r);
    CU_ASSERT_PTR_EQUAL(r, dst + 3);        /* points just past the 'c' */
    CU_ASSERT_EQUAL(memcmp(dst, "abc", 3), 0);

    /* byte not present within n -> NULL, whole n bytes copied */
    memset(dst, 0, sizeof(dst));
    r = memccpy(dst, "abcdef", 'z', 6);
    CU_ASSERT_PTR_NULL(r);
    CU_ASSERT_EQUAL(memcmp(dst, "abcdef", 6), 0);
}

/* ---- timespec_get() (stdc, C11) ------------------------------------ */

void testTIMESPEC_GET(void)
{
    struct timespec ts = { 0, -1 };
    int r = timespec_get(&ts, TIME_UTC);

    CU_ASSERT_EQUAL(r, TIME_UTC);
    CU_ASSERT_TRUE(ts.tv_sec > 0);                 /* a real epoch time */
    CU_ASSERT_TRUE(ts.tv_nsec >= 0 && ts.tv_nsec < 1000000000L);

    /* an unsupported base returns 0 */
    CU_ASSERT_EQUAL(timespec_get(&ts, 0), 0);
}

/* ---- getline()/getdelim() (posixc) --------------------------------- */

void testGETLINE(void)
{
    FILE *f = fopen(TESTFILE, "w");
    CU_ASSERT_PTR_NOT_NULL_FATAL(f);
    fputs("alpha\nbravo\ncharlie\n", f);
    fclose(f);

    f = fopen(TESTFILE, "r");
    CU_ASSERT_PTR_NOT_NULL_FATAL(f);

    char *line = NULL;
    size_t cap = 0;
    ssize_t len;

    len = getline(&line, &cap, f);
    CU_ASSERT_EQUAL(len, 6);                /* "alpha\n" */
    CU_ASSERT_STRING_EQUAL(line, "alpha\n");

    len = getline(&line, &cap, f);
    CU_ASSERT_EQUAL(len, 6);                /* "bravo\n" */
    CU_ASSERT_STRING_EQUAL(line, "bravo\n");

    len = getline(&line, &cap, f);
    CU_ASSERT_EQUAL(len, 8);                /* "charlie\n" */
    CU_ASSERT_STRING_EQUAL(line, "charlie\n");

    /* end of file */
    len = getline(&line, &cap, f);
    CU_ASSERT_EQUAL(len, -1);

    free(line);
    fclose(f);
}

void testGETDELIM(void)
{
    FILE *f = fopen(TESTFILE, "w");
    CU_ASSERT_PTR_NOT_NULL_FATAL(f);
    fputs("one;two;three", f);
    fclose(f);

    f = fopen(TESTFILE, "r");
    CU_ASSERT_PTR_NOT_NULL_FATAL(f);

    char *tok = NULL;
    size_t cap = 0;
    ssize_t len;

    len = getdelim(&tok, &cap, ';', f);
    CU_ASSERT_EQUAL(len, 4);                /* "one;" */
    CU_ASSERT_STRING_EQUAL(tok, "one;");

    len = getdelim(&tok, &cap, ';', f);
    CU_ASSERT_STRING_EQUAL(tok, "two;");

    len = getdelim(&tok, &cap, ';', f);
    CU_ASSERT_STRING_EQUAL(tok, "three");   /* last field, no delimiter */

    len = getdelim(&tok, &cap, ';', f);
    CU_ASSERT_EQUAL(len, -1);

    free(tok);
    fclose(f);
}

/* ---- dprintf() (posixc) -------------------------------------------- */

void testDPRINTF(void)
{
    int fd = open(TESTFILE, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    CU_ASSERT_TRUE_FATAL(fd >= 0);

    int n = dprintf(fd, "x=%d y=%s", 42, "hi");
    close(fd);
    CU_ASSERT_EQUAL(n, 9);                  /* "x=42 y=hi" */

    FILE *f = fopen(TESTFILE, "r");
    CU_ASSERT_PTR_NOT_NULL_FATAL(f);
    char buf[32] = {0};
    fgets(buf, sizeof(buf), f);
    fclose(f);
    CU_ASSERT_STRING_EQUAL(buf, "x=42 y=hi");
}

/* tzset() parses the POSIX TZ variable into the timezone/daylight/tzname
   globals.  Offsets in TZ are seconds *West* of UTC, so a leading '-' (East of
   UTC) yields a negative timezone.  DST transition rules are not applied, but
   the presence of a DST zone name must still set daylight. */
void testTZSET(void)
{
    char *saved = getenv("TZ");
    if (saved)
        saved = strdup(saved);              /* getenv() buffer may be reused */

    /* Standard zone, positive (West) offset, no DST. */
    setenv("TZ", "EST5", 1);
    tzset();
    CU_ASSERT_EQUAL(timezone, 5 * 3600);
    CU_ASSERT_EQUAL(daylight, 0);
    CU_ASSERT_STRING_EQUAL(tzname[0], "EST");
    CU_ASSERT_STRING_EQUAL(tzname[1], "EST");

    /* A DST zone name sets daylight and tzname[1] (offset unchanged). */
    setenv("TZ", "EST5EDT", 1);
    tzset();
    CU_ASSERT_EQUAL(timezone, 5 * 3600);
    CU_ASSERT_NOT_EQUAL(daylight, 0);
    CU_ASSERT_STRING_EQUAL(tzname[0], "EST");
    CU_ASSERT_STRING_EQUAL(tzname[1], "EDT");

    /* Negative (East) offset with minutes, given as "-hh:mm". */
    setenv("TZ", "IST-5:30", 1);
    tzset();
    CU_ASSERT_EQUAL(timezone, -(5 * 3600 + 30 * 60));
    CU_ASSERT_EQUAL(daylight, 0);
    CU_ASSERT_STRING_EQUAL(tzname[0], "IST");

    /* Modern bracketed numeric abbreviation "<+05>-5" (East of UTC). */
    setenv("TZ", "<+05>-5", 1);
    tzset();
    CU_ASSERT_EQUAL(timezone, -5 * 3600);
    CU_ASSERT_STRING_EQUAL(tzname[0], "+05");

    /* Empty TZ means UTC0. */
    setenv("TZ", "", 1);
    tzset();
    CU_ASSERT_EQUAL(timezone, 0);
    CU_ASSERT_EQUAL(daylight, 0);
    CU_ASSERT_STRING_EQUAL(tzname[0], "UTC");

    /* Named zero-offset zone keeps its abbreviation. */
    setenv("TZ", "GMT0", 1);
    tzset();
    CU_ASSERT_EQUAL(timezone, 0);
    CU_ASSERT_EQUAL(daylight, 0);
    CU_ASSERT_STRING_EQUAL(tzname[0], "GMT");

    if (saved)
    {
        setenv("TZ", saved, 1);
        free(saved);
    }
    else
        unsetenv("TZ");
    tzset();
}

int main(void)
{
    CU_pSuite pSuite = NULL;

    if (CUE_SUCCESS != CU_initialize_registry())
        return CU_get_error();

    pSuite = CU_add_suite("CRT_Ext_Suite", init_suite, clean_suite);
    if (NULL == pSuite) {
        CU_cleanup_registry();
        return CU_get_error();
    }

    if ((NULL == CU_add_test(pSuite, "memccpy()", testMEMCCPY)) ||
        (NULL == CU_add_test(pSuite, "timespec_get()", testTIMESPEC_GET)) ||
        (NULL == CU_add_test(pSuite, "getline()", testGETLINE)) ||
        (NULL == CU_add_test(pSuite, "getdelim()", testGETDELIM)) ||
        (NULL == CU_add_test(pSuite, "dprintf()", testDPRINTF)) ||
        (NULL == CU_add_test(pSuite, "tzset()", testTZSET)))
    {
        CU_cleanup_registry();
        return CU_get_error();
    }

    CU_basic_set_mode(CU_BRM_VERBOSE);
    CU_basic_run_tests();
    CU_basic_set_mode(CU_BRM_SILENT);
    CU_automated_package_name_set("CRTUnitTests");
    CU_set_output_filename("CRT-Ext");
    CU_automated_enable_junit_xml(CU_TRUE);
    CU_automated_run_tests();
    CU_cleanup_registry();

    return CU_get_error();
}
