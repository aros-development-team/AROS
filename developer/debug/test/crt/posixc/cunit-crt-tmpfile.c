/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.
*/

#include <proto/dos.h>
#include <dos/dos.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>

#include <CUnit/Basic.h>
#include <CUnit/Automated.h>

/* The suite initialization function.
 * Returns zero on success, non-zero otherwise.
 */
int init_suite(void)
{
    return 0;
}

/* The suite cleanup function.
 * Returns zero on success, non-zero otherwise.
 */
int clean_suite(void)
{
    return 0;
}

/* Test that tmpfile() returns a valid FILE pointer.
 */
void test_tmpfile_creation(void)
{
    FILE *fp;

    fp = tmpfile();

    CU_ASSERT_PTR_NOT_NULL(fp);

    if (fp != NULL)
        fclose(fp);
}

/* Test writing to and reading from a temporary file.
 */
void test_tmpfile_write_read(void)
{
    FILE *fp;
    const char *test_string = "Hello, AROS temporary file!";
    char buffer[100];
    size_t written, read;

    fp = tmpfile();
    CU_ASSERT_PTR_NOT_NULL(fp);

    if (fp == NULL)
        return;

    /* Write test data */
    written = fwrite(test_string, 1, strlen(test_string), fp);
    CU_ASSERT_EQUAL(written, strlen(test_string));

    /* Rewind to beginning */
    rewind(fp);

    /* Read it back */
    memset(buffer, 0, sizeof(buffer));
    read = fread(buffer, 1, strlen(test_string), fp);
    CU_ASSERT_EQUAL(read, strlen(test_string));
    CU_ASSERT_STRING_EQUAL(buffer, test_string);

    fclose(fp);
}

/* Test that tmpfile() opens in "w+" mode (read/write, binary).
 */
void test_tmpfile_mode(void)
{
    FILE *fp;
    const char *data1 = "First write";
    const char *data2 = "Second write";
    char buffer[100];

    fp = tmpfile();
    CU_ASSERT_PTR_NOT_NULL(fp);

    if (fp == NULL)
        return;

    /* Write first data */
    fputs(data1, fp);
    
    /* Seek back and overwrite */
    fseek(fp, 0, SEEK_SET);
    fputs(data2, fp);

    /* Read back */
    fseek(fp, 0, SEEK_SET);
    memset(buffer, 0, sizeof(buffer));
    fgets(buffer, sizeof(buffer), fp);

    CU_ASSERT_STRING_EQUAL(buffer, data2);

    fclose(fp);
}

/* Test multiple tmpfile() calls create unique files.
 */
void test_tmpfile_multiple(void)
{
    FILE *fp1, *fp2, *fp3;
    const char *test1 = "File 1";
    const char *test2 = "File 2";
    const char *test3 = "File 3";
    char buffer[100];

    fp1 = tmpfile();
    fp2 = tmpfile();
    fp3 = tmpfile();

    CU_ASSERT_PTR_NOT_NULL(fp1);
    CU_ASSERT_PTR_NOT_NULL(fp2);
    CU_ASSERT_PTR_NOT_NULL(fp3);

    if (fp1 == NULL || fp2 == NULL || fp3 == NULL)
    {
        if (fp1) fclose(fp1);
        if (fp2) fclose(fp2);
        if (fp3) fclose(fp3);
        return;
    }

    /* Write different data to each file */
    fputs(test1, fp1);
    fputs(test2, fp2);
    fputs(test3, fp3);

    /* Verify each file has correct data */
    rewind(fp1);
    memset(buffer, 0, sizeof(buffer));
    fgets(buffer, sizeof(buffer), fp1);
    CU_ASSERT_STRING_EQUAL(buffer, test1);

    rewind(fp2);
    memset(buffer, 0, sizeof(buffer));
    fgets(buffer, sizeof(buffer), fp2);
    CU_ASSERT_STRING_EQUAL(buffer, test2);

    rewind(fp3);
    memset(buffer, 0, sizeof(buffer));
    fgets(buffer, sizeof(buffer), fp3);
    CU_ASSERT_STRING_EQUAL(buffer, test3);

    fclose(fp1);
    fclose(fp2);
    fclose(fp3);
}

/* Test that tmpfile() can handle large amounts of data.
 */
void test_tmpfile_large_data(void)
{
    FILE *fp;
    unsigned char buffer[4096];
    unsigned char verify[4096];
    size_t i;

    fp = tmpfile();
    CU_ASSERT_PTR_NOT_NULL(fp);

    if (fp == NULL)
        return;

    /* Fill buffer with test pattern */
    for (i = 0; i < sizeof(buffer); i++)
        buffer[i] = (unsigned char)(i & 0xFF);

    /* Write multiple blocks */
    for (i = 0; i < 10; i++)
    {
        size_t written = fwrite(buffer, 1, sizeof(buffer), fp);
        CU_ASSERT_EQUAL(written, sizeof(buffer));
    }

    /* Verify file position */
    long pos = ftell(fp);
    CU_ASSERT_EQUAL(pos, 10 * sizeof(buffer));

    /* Rewind and verify some data */
    rewind(fp);
    memset(verify, 0, sizeof(verify));
    size_t read = fread(verify, 1, sizeof(verify), fp);
    CU_ASSERT_EQUAL(read, sizeof(verify));
    CU_ASSERT_EQUAL(memcmp(buffer, verify, sizeof(buffer)), 0);

    fclose(fp);
}

/* Test tmpfile() with fseek/ftell operations.
 */
void test_tmpfile_seek(void)
{
    FILE *fp;
    const char *data = "0123456789ABCDEF";
    char ch;

    fp = tmpfile();
    CU_ASSERT_PTR_NOT_NULL(fp);

    if (fp == NULL)
        return;

    /* Write test data */
    fputs(data, fp);

    /* Test SEEK_SET */
    fseek(fp, 5, SEEK_SET);
    ch = fgetc(fp);
    CU_ASSERT_EQUAL(ch, '5');

    /* Test SEEK_CUR */
    fseek(fp, 2, SEEK_CUR);
    ch = fgetc(fp);
    CU_ASSERT_EQUAL(ch, '8');

    /* Test SEEK_END */
    fseek(fp, -3, SEEK_END);
    ch = fgetc(fp);
    CU_ASSERT_EQUAL(ch, 'D');

    /* Test ftell */
    fseek(fp, 10, SEEK_SET);
    long pos = ftell(fp);
    CU_ASSERT_EQUAL(pos, 10);

    fclose(fp);
}

int main(void)
{
    CU_pSuite pSuite = NULL;

    /* initialize the CUnit test registry */
    if (CUE_SUCCESS != CU_initialize_registry())
        return CU_get_error();

    /* add a suite to the registry */
    pSuite = CU_add_suite("PosixC_tmpfile_Suite", init_suite, clean_suite);
    if (NULL == pSuite) {
        CU_cleanup_registry();
        return CU_get_error();
    }

    /* add the tests to the suite */
    if (
        (NULL == CU_add_test(pSuite, "test tmpfile() creation", test_tmpfile_creation)) ||
        (NULL == CU_add_test(pSuite, "test tmpfile() write/read", test_tmpfile_write_read)) ||
        (NULL == CU_add_test(pSuite, "test tmpfile() mode", test_tmpfile_mode)) ||
        (NULL == CU_add_test(pSuite, "test tmpfile() multiple files", test_tmpfile_multiple)) ||
        (NULL == CU_add_test(pSuite, "test tmpfile() large data", test_tmpfile_large_data)) ||
        (NULL == CU_add_test(pSuite, "test tmpfile() seek operations", test_tmpfile_seek)))
    {
        CU_cleanup_registry();
        return CU_get_error();
    }

    /* Run all tests using the CUnit Basic & Automated interfaces */
    CU_basic_set_mode(CU_BRM_VERBOSE);
    CU_basic_run_tests();
    CU_basic_set_mode(CU_BRM_SILENT);
    CU_automated_package_name_set("CRTUnitTests");
    CU_set_output_filename("CRT-PosixC_tmpfile");
    CU_automated_enable_junit_xml(CU_TRUE);
    CU_automated_run_tests();
    CU_cleanup_registry();

    return CU_get_error();
}
