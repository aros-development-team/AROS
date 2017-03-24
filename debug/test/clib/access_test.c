/*
    Copyright © 1995-2014, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>

#define SHOW_PROGRESS 0

// Global variables
int global_test_counter = 0;
int global_test_step_counter = 0;
int global_failure_indicator = 0;

/* 2008-03-22 - All test pass (if manual configuration is done correclty) */

/* COMPILE WITH '-nix' for unix2amiga path conversion */

/* These values are arbitrary and influence test results! */
/* Existing objects should have RWED permitions (even the volume!) */

#if defined __AROS__
const char * existing_volume = "/Work/";
const char * existing_directory = "/SYS/Classes";
const char * existing_file = "/SYS/Classes/Datatypes/bmp.datatype";
const char * non_existing_volume = "/sw243";
const char * non_existing_directory = "/SYS/sdgfer";
const char * non_existing_file = "/SYS/Classes/ggg.txt";
#else
const char * existing_volume = "/usr";
const char * existing_directory = "/usr/bin";
const char * existing_file = "/usr/bin/gcc";
const char * non_existing_volume = "/sw243";
const char * non_existing_directory = "/usr/sdgfer";
const char * non_existing_file = "/usr/lib/ggg.txt";
#endif 

/* These files need to be prepared in application directory with appriopriate access rights! */
const char * read_only_file = "read_only_file";
const char * write_only_file = "write_only_file";
const char * execute_only_file = "execute_only_file";
const char * all_access_file = "all_access_file";

void reset_global_test_counter()
{
    global_test_counter = 0;
    global_test_step_counter = 0;
}

void next_test()
{
    global_test_counter++;
    global_test_step_counter = 0;
}

void next_test_step()
{
    global_test_step_counter++;
}

// Reporting functions
void report(const char * status, const char * message)
{
    printf("REPORT : %s : %s \n", status, message);
}

void report_progress(const char * message)
{
    report("PROGRESS", message);
}

void report_failure(const char * message)
{
    report("FAILED", message);
}

void report_failure_strerror()
{
    report_failure((const char*)strerror(errno));
}

void test_report(const char * status, const char * message)
{
    printf("TEST %d-%d : %s : %s \n", global_test_counter , global_test_step_counter, status, message);
}

void test_report_progress(const char * message)
{
#if SHOW_PROGRESS == 1
    test_report("PROGRESS", message);
#endif
}

void test_report_failure(const char * message)
{
    test_report("FAILED", message);
}

void test_report_success(const char * message)
{
    test_report("OK", message);
}

void test_report_description(const char * message)
{
    test_report("TEST DESCRIPTION", message);
}

void test_report_failure_strerror()
{
    test_report_failure((const char*)strerror(errno));
}

// Test
int test_access_preparation()
{
}

int test_access(int mode)
{
    report_progress("Start test_access");
    if (mode & R_OK)
        report_progress("R_OK");
    if (mode & W_OK)
        report_progress("W_OK");
    if (mode & X_OK)
        report_progress("X_OK");
    if (mode == 0)
        report_progress("F_OK");
    

   /* 
     * TEST
     */

    next_test_step();

    test_report_description("access existing volume");
    
    // access existing volume
    if (access(existing_volume, mode) != 0)
    {
        test_report_failure_strerror();
        test_report_failure("Failed accessing existing volume");
        return -1;
    }
    test_report_success("Accessed existing volume");
    test_report_success("SUCCESS");   

    /* 
     * TEST END
     */  

   /* 
     * TEST
     */

    next_test_step();

    test_report_description("access existing directory");
    
    // access existing directory
    if (access(existing_directory, mode) != 0)
    {
        test_report_failure_strerror();
        test_report_failure("Failed accessing existing directory");
        return -1;
    }
    test_report_success("Accessed existing directory");
    test_report_success("SUCCESS");   

    /* 
     * TEST END
     */  

   /* 
     * TEST
     */

    next_test_step();

    test_report_description("access existing file");
    
    // access existing file
    if (access(existing_file, mode) != 0)
    {
        test_report_failure_strerror();
        test_report_failure("Failed accessing existing file");
        return -1;
    }
    test_report_success("Accessed existing file");
    test_report_success("SUCCESS");   

    /* 
     * TEST END
     */  

   /* 
     * TEST
     */

    next_test_step();

    test_report_description("access non existing volume");
    
    // access non existing volume
    if (access(non_existing_volume, mode) == 0)
    {
        test_report_failure("This call should fail");
        return -1;
    }
    else
    {
        if (errno != ENOENT)
        {
            test_report_failure_strerror();
            test_report_failure("Different error expected");
            return -1;
        }
    }
    test_report_success("Correct error reported");
    test_report_success("SUCCESS");   

    /* 
     * TEST END
     */  

   /* 
     * TEST
     */

    next_test_step();

    test_report_description("access non existing directory");
    
    // access non existing directory
    if (access(non_existing_directory, mode) == 0)
    {
        test_report_failure("This call should fail");
        return -1;
    }
    else
    {
        if (errno != ENOENT)
        {
            test_report_failure_strerror();
            test_report_failure("Different error expected");
            return -1;
        }
    }
    test_report_success("Correct error reported");
    test_report_success("SUCCESS");   

    /* 
     * TEST END
     */  

   /* 
     * TEST
     */

    next_test_step();

    test_report_description("access non existing file");
    
    // access non existing file
    if (access(non_existing_file, mode) == 0)
    {
        test_report_failure("This call should fail");
        return -1;
    }
    else
    {
        if (errno != ENOENT)
        {
            test_report_failure_strerror();
            test_report_failure("Different error expected");
            return -1;
        }
    }
    test_report_success("Correct error reported");
    test_report_success("SUCCESS");   

    /* 
     * TEST END
     */  

    return 0;
}

int test_access_wrapper(int mode)
{
    next_test();
    return test_access(mode);
}

int test_single_file_access_modes()
{
    next_test();
    
    /* Manual action required for test. See top of file */

    report_progress("Start test_file_access_modes");
    
   /* 
     * TEST
     */

    next_test_step();

    test_report_description("access F_OK for read_only_file");
    
    // access F_OK for read_only_file
    if (access(read_only_file, F_OK) != 0)
    {
        test_report_failure_strerror();
        test_report_failure("Failed accessing file");
        return -1;
    }
    test_report_success("File accessed");
    test_report_success("SUCCESS");   

    /* 
     * TEST END
     */

   /* 
     * TEST
     */

    next_test_step();

    test_report_description("access F_OK for write_only_file");
    
    // access F_OK for write_only_file
    if (access(write_only_file, F_OK) != 0)
    {
        test_report_failure_strerror();
        test_report_failure("Failed accessing file");
        return -1;
    }
    test_report_success("File accessed");
    test_report_success("SUCCESS");   

    /* 
     * TEST END
     */

   /* 
     * TEST
     */

    next_test_step();

    test_report_description("access F_OK for execute_only_file");
    
    // access F_OK for execute_only_file
    if (access(execute_only_file, F_OK) != 0)
    {
        test_report_failure_strerror();
        test_report_failure("Failed accessing file");
        return -1;
    }
    test_report_success("File accessed");
    test_report_success("SUCCESS");   

    /* 
     * TEST END
     */

   /* 
     * TEST
     */

    next_test_step();

    test_report_description("access R_OK for read_only_file");
    
    // access R_OK for read_only_file
    if (access(read_only_file, R_OK) != 0)
    {
        test_report_failure_strerror();
        test_report_failure("Failed accessing file");
        return -1;
    }
    test_report_success("File accessed");
    test_report_success("SUCCESS");   

    /* 
     * TEST END
     */

   /* 
     * TEST
     */

    next_test_step();

    test_report_description("access R_OK | W_OK for read_only_file");
    
    // access R_OK | W_OK for read_only_file
    if (access(read_only_file, R_OK | W_OK) != 0)
    {
        if (errno != EACCES)
        {
            test_report_failure_strerror();
            test_report_failure("Different error expected");
            return -1;
        }
    }
    else
    {
        test_report_failure("This operation should fail");
        return -1;
    }
    test_report_success("Correct error reported");
    test_report_success("SUCCESS");   

    /* 
     * TEST END
     */

   /* 
     * TEST
     */

    next_test_step();

    test_report_description("access R_OK | X_OK for read_only_file");
    
    // access R_OK | X_OK for read_only_file
    if (access(read_only_file, R_OK | X_OK) != 0)
    {
        if (errno != EACCES)
        {
            test_report_failure_strerror();
            test_report_failure("Different error expected");
            return -1;
        }
    }
    else
    {
        test_report_failure("This operation should fail");
        return -1;
    }
    test_report_success("Correct error reported");
    test_report_success("SUCCESS");   

    /* 
     * TEST END
     */

   /* 
     * TEST
     */

    next_test_step();

    test_report_description("access W_OK for write_only_file");
    
    // access W_OK for write_only_file
    if (access(write_only_file, W_OK) != 0)
    {
        test_report_failure_strerror();
        test_report_failure("Failed accessing file");
        return -1;
    }
    test_report_success("File accessed");
    test_report_success("SUCCESS");   

    /* 
     * TEST END
     */

   /* 
     * TEST
     */

    next_test_step();

    test_report_description("access W_OK | R_OK for write_only_file");
    
    // access W_OK | R_OK for write_only_file
    if (access(write_only_file, W_OK | R_OK) != 0)
    {
        if (errno != EACCES)
        {
            test_report_failure_strerror();
            test_report_failure("Different error expected");
            return -1;
        }
    }
    else
    {
        test_report_failure("This operation should fail");
        return -1;
    }
    test_report_success("Correct error reported");
    test_report_success("SUCCESS");   

    /* 
     * TEST END
     */

   /* 
     * TEST
     */

    next_test_step();

    test_report_description("access W_OK | X_OK for write_only_file");
    
    // access W_OK | X_OK for write_only_file
    if (access(write_only_file, W_OK | X_OK) != 0)
    {
        if (errno != EACCES)
        {
            test_report_failure_strerror();
            test_report_failure("Different error expected");
            return -1;
        }
    }
    else
    {
        test_report_failure("This operation should fail");
        return -1;
    }
    test_report_success("Correct error reported");
    test_report_success("SUCCESS");   

    /* 
     * TEST END
     */


   /* 
     * TEST
     */

    next_test_step();

    test_report_description("access X_OK for execute_only_file");
    
    // access X_OK for execute_only_file
    if (access(execute_only_file, X_OK) != 0)
    {
        test_report_failure_strerror();
        test_report_failure("Failed accessing file");
        return -1;
    }
    test_report_success("File accessed");
    test_report_success("SUCCESS");   

    /* 
     * TEST END
     */

   /* 
     * TEST
     */

    next_test_step();

    test_report_description("access X_OK | R_OK for execute_only_file");
    
    // access X_OK | R_OK for execute_only_file
    if (access(execute_only_file, X_OK | R_OK) != 0)
    {
        if (errno != EACCES)
        {
            test_report_failure_strerror();
            test_report_failure("Different error expected");
            return -1;
        }
    }
    else
    {
        test_report_failure("This operation should fail");
        return -1;
    }
    test_report_success("Correct error reported");
    test_report_success("SUCCESS");   

    /* 
     * TEST END
     */

   /* 
     * TEST
     */

    next_test_step();

    test_report_description("access X_OK | W_OK for execute_only_file");
    
    // access X_OK | W_OK for execute_only_file
    if (access(execute_only_file, X_OK | W_OK) != 0)
    {
        if (errno != EACCES)
        {
            test_report_failure_strerror();
            test_report_failure("Different error expected");
            return -1;
        }
    }
    else
    {
        test_report_failure("This operation should fail");
        return -1;
    }
    test_report_success("Correct error reported");
    test_report_success("SUCCESS");   

    /* 
     * TEST END
     */

    return 0;
}

int test_combined_file_access_modes()
{
    next_test();
    
    /* Manual action required for test. See top of file */

    report_progress("Start test_combined_file_access_modes");
    

   /* 
     * TEST
     */

    next_test_step();

    test_report_description("access R_OK | W_OK for all_access_file");
    
    // access R_OK | W_OK for all_access_file
    if (access(all_access_file, R_OK | W_OK) != 0)
    {
        test_report_failure_strerror();
        test_report_failure("Failed accessing file");
        return -1;
    }
    test_report_success("File accessed");
    test_report_success("SUCCESS");   

    /* 
     * TEST END
     */

   /* 
     * TEST
     */

    next_test_step();

    test_report_description("access R_OK | X_OK for all_access_file");
    
    // access R_OK | X_OK for all_access_file
    if (access(all_access_file, R_OK | X_OK) != 0)
    {
        test_report_failure_strerror();
        test_report_failure("Failed accessing file");
        return -1;
    }
    test_report_success("File accessed");
    test_report_success("SUCCESS");   

    /* 
     * TEST END
     */

   /* 
     * TEST
     */

    next_test_step();

    test_report_description("access W_OK | X_OK for all_access_file");
    
    // access W_OK | X_OK for all_access_file
    if (access(all_access_file, W_OK | X_OK) != 0)
    {
        test_report_failure_strerror();
        test_report_failure("Failed accessing file");
        return -1;
    }
    test_report_success("File accessed");
    test_report_success("SUCCESS");   

    /* 
     * TEST END
     */

   /* 
     * TEST
     */

    next_test_step();

    test_report_description("access R_OK | W_OK | X_OK for all_access_file");
    
    // access R_OK | W_OK | X_OK for all_access_file
    if (access(all_access_file, R_OK | W_OK | X_OK) != 0)
    {
        test_report_failure_strerror();
        test_report_failure("Failed accessing file");
        return -1;
    }
    test_report_success("File accessed");
    test_report_success("SUCCESS");   

    /* 
     * TEST END
     */

   /* 
     * TEST
     */

    next_test_step();

    test_report_description("access R_OK | W_OK | X_OK | F_OK for all_access_file");
    
    // access R_OK | W_OK | X_OK | F_OK for all_access_file
    if (access(all_access_file, R_OK | W_OK | X_OK | F_OK) != 0)
    {
        test_report_failure_strerror();
        test_report_failure("Failed accessing file");
        return -1;
    }
    test_report_success("File accessed");
    test_report_success("SUCCESS");   

    /* 
     * TEST END
     */

    return 0;
}
int main()
{
    report_progress("Starting tests");

    reset_global_test_counter();

    // access test
    if (test_access_wrapper(F_OK) != 0)
        global_failure_indicator = 1;

    if (test_access_wrapper(R_OK) != 0)
        global_failure_indicator = 1;

    if (test_access_wrapper(W_OK) != 0)
        global_failure_indicator = 1;

    if (test_access_wrapper(X_OK) != 0)
        global_failure_indicator = 1;

    if (test_access_wrapper(R_OK | W_OK | X_OK) != 0)
        global_failure_indicator = 1;

    if (test_single_file_access_modes() != 0)
        global_failure_indicator = 1;

    if (test_combined_file_access_modes() != 0)
        global_failure_indicator = 1;

    if (global_failure_indicator == 1)
        report_failure("One of the tests FAILED");
    else
        report_progress("All tests SUCCEEDED");

    report_progress("Tests finished");
}
