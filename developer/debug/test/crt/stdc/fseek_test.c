/*
    Copyright © 1995-2014, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>

#define SHOW_PROGRESS 0

// Global variables
int global_test_counter = 0;
int global_test_step_counter = 0;
int global_failure_indicator = 0;

FILE * testfilepointer = NULL;
size_t buffersize = 1024;
char writedata[1024] = {0};
char * testfilename = "fseek_test_file";

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

void erase_buffer(char * buffer, size_t size)
{
    memset(buffer, 0, size);
}

int compare_buffers(const char * buffer1, const char * buffer2, size_t bufferssize)
{
    return memcmp(buffer1, buffer2, bufferssize);
}

void close_test_file()
{
    if (testfilepointer != NULL)
    {
        if (fclose(testfilepointer) != 0)
        {
            report_failure_strerror();
            report_failure("Failed to close the file.");
            return;
        }
        testfilepointer = NULL;
    }
}

// Tests
int test_fseek_ftell_preparation(const char * fopenflags)
{
    size_t operationsize = 0;

    report_progress("Starting test_fseek_ftell_preparation");
    report_progress(fopenflags);

    if (testfilepointer != NULL)
    {
        report_failure("Pointer for test file not closed.");
        return -1;
    }   

    // zero buffer
    erase_buffer(writedata, buffersize);
    
    // open file, write data
    testfilepointer = fopen(testfilename, fopenflags);
    if (testfilepointer == NULL)
    {
        report_failure_strerror();
        report_failure("Failed to open file for writting.");
        return -1;
    }
    operationsize = fwrite(writedata, 1, buffersize, testfilepointer);
    if (operationsize != buffersize)
    {
        report_failure_strerror();
        report_failure("Failed to write data.");
        return -1;
    }

    // Seek to beginning
    if (fseek(testfilepointer, 0, SEEK_SET) != 0)
    {
        report_failure_strerror();
        report_failure("Failed to seek to file beginning.");           
        return -1;
    }

    return 0;

}

int test_fseek_ftell()
{

    /* Expects that testfilepointer is opened in correct mode */
    int ftellposition = 0;
    char conv[16] = {0};    

    report_progress("Start test_fseek_ftell");

    /* 
     * TEST
     */

    next_test_step();

    test_report_description("seek to begin, tell location");
    
    // seek to begin, tell location
    if (fseek(testfilepointer, 0, SEEK_SET) != 0)
    {
        test_report_failure_strerror();
        test_report_failure("Failed to seek to beginning of file");
        return -1;
    }
    test_report_progress("Seek to beginning of file.");
    ftellposition = ftell(testfilepointer);
    if (ftellposition == -1)
    {
        test_report_failure_strerror();
        test_report_failure("Failed to tell position");
        return -1;
    }
    else
        if (ftellposition != 0)
        {
            test_report_failure("Wrong position in stream reported");
            sprintf(conv, "%d", ftellposition);
            test_report_failure(conv);
            return -1;
        }
    test_report_success("Correct position in stream reported");
    test_report_success("SUCCESS");

    /* 
     * TEST END
     */ 

    /* 
     * TEST
     */

    next_test_step();

    test_report_description("seek foreward, tell location");
    
    // seek to begin, tell location
    if (fseek(testfilepointer, 153, SEEK_CUR) != 0)
    {
        test_report_failure_strerror();
        test_report_failure("Failed to seek foreward");
        return -1;
    }
    test_report_progress("Seek foreward made.");
    ftellposition = ftell(testfilepointer);
    if (ftellposition == -1)
    {
        test_report_failure_strerror();
        test_report_failure("Failed to tell position");
        return -1;
    }
    else
        if (ftellposition != 153)
        {
            test_report_failure("Wrong position in stream reported");
            sprintf(conv, "%d", ftellposition);
            test_report_failure(conv);
            return -1;
        }
    test_report_success("Correct position in stream reported");
    test_report_success("SUCCESS");

    /* 
     * TEST END
     */ 

    /* 
     * TEST
     */

    next_test_step();

    test_report_description("seek backwards, tell location");
    
    // seek backwards, tell location
    if (fseek(testfilepointer, -80, SEEK_CUR) != 0)
    {
        test_report_failure_strerror();
        test_report_failure("Failed to seek backwards");
        return -1;
    }
    test_report_progress("Seek backwards made.");
    ftellposition = ftell(testfilepointer);
    if (ftellposition == -1)
    {
        test_report_failure_strerror();
        test_report_failure("Failed to tell position");
        return -1;
    }
    else
        if (ftellposition != (153-80))
        {
            test_report_failure("Wrong position in stream reported");
            sprintf(conv, "%d", ftellposition);
            test_report_failure(conv);
            return -1;
        }
    test_report_success("Correct position in stream reported");
    test_report_success("SUCCESS");

    /* 
     * TEST END
     */

    /* 
     * TEST
     */

    next_test_step();

    test_report_description("seek to the end, tell location");
    
    // seek to begin, tell location
    if (fseek(testfilepointer, 0, SEEK_END) != 0)
    {
        test_report_failure_strerror();
        test_report_failure("Failed to seek to end of file");
        return -1;
    }
    test_report_progress("Seek to end of file made.");
    ftellposition = ftell(testfilepointer);
    if (ftellposition == -1)
    {
        test_report_failure_strerror();
        test_report_failure("Failed to tell position");
        return -1;
    }
    else
        if (ftellposition != buffersize)
        {
            test_report_failure("Wrong position in stream reported");
            sprintf(conv, "%d", ftellposition);
            test_report_failure(conv);
            return -1;
        }
    test_report_success("Correct position in stream reported");
    test_report_success("SUCCESS");

    /* 
     * TEST END
     */

    /* 
     * TEST
     */

    next_test_step();

    test_report_description("seek foreward(beyond file), tell location");
    
    // seek foreward(beyond file), tell location
    if (fseek(testfilepointer, 120, SEEK_CUR) != 0)
    {
        test_report_failure_strerror();
        test_report_failure("Failed to seek beyond file end");
        return -1;
    }
    test_report_progress("Seek beyond file end made.");
    ftellposition = ftell(testfilepointer);
    if (ftellposition == -1)
    {
        test_report_failure_strerror();
        test_report_failure("Failed to tell position");
        return -1;
    }
    else
        if (ftellposition != buffersize + 120)
        {
            test_report_failure("Wrong position in stream reported");
            sprintf(conv, "%d", ftellposition);
            test_report_failure(conv);
            return -1;
        }
    test_report_success("Correct position in stream reported");
    test_report_success("SUCCESS");

    /* 
     * TEST END
     */

    /* 
     * TEST
     */

    next_test_step();

    test_report_description("seek backwards, tell location");
    
    // seek backwards, tell location
    if (fseek(testfilepointer, -240, SEEK_CUR) != 0)
    {
        test_report_failure_strerror();
        test_report_failure("Failed to seek backwards");
        return -1;
    }
    test_report_progress("Seek backwards made.");
    ftellposition = ftell(testfilepointer);
    if (ftellposition == -1)
    {
        test_report_failure_strerror();
        test_report_failure("Failed to tell position");
        return -1;
    }
    else
        if (ftellposition != buffersize - 120)
        {
            test_report_failure("Wrong position in stream reported");
            sprintf(conv, "%d", ftellposition);
            test_report_failure(conv);
            return -1;
        }
    test_report_success("Correct position in stream reported");
    test_report_success("SUCCESS");

    /* 
     * TEST END
     */

    /* 
     * TEST
     */

    next_test_step();

    test_report_description("seek to end with positive offset, tell location");
    
    // seek to end with positive offset, tell location
    if (fseek(testfilepointer, 120, SEEK_END) != 0)
    {
        test_report_failure_strerror();
        test_report_failure("Failed to seek to end with positive offset");
        return -1;
    }
    test_report_progress("Seek to end with positive offset made.");
    ftellposition = ftell(testfilepointer);
    if (ftellposition == -1)
    {
        test_report_failure_strerror();
        test_report_failure("Failed to tell position");
        return -1;
    }
    else
        if (ftellposition != buffersize + 120)
        {
            test_report_failure("Wrong position in stream reported");
            sprintf(conv, "%d", ftellposition);
            test_report_failure(conv);
            return -1;
        }
    test_report_success("Correct position in stream reported");
    test_report_success("SUCCESS");

    /* 
     * TEST END
     */

    return 0;
}

int test_fseek_ftell_wrapper(const char * fopenflags)
{
    next_test();

    int result = test_fseek_ftell_preparation(fopenflags);
    if (result != 0)
    {   
        report_failure("Preparation failed");
    }
    else
        result = test_fseek_ftell();

    close_test_file();

    return result;
}



















int test_fseek_and_fread_preparation(const char * fopenflags)
{
    char readdatabuffer1[1024] = {0};
    int i = 0;
    size_t operationsize = 0;

    report_progress("Starting test_fseek_and_fread_preparation");
    report_progress(fopenflags);

    if (testfilepointer != NULL)
    {
        report_failure("Pointer for test file not closed.");
        return -1;
    }   

    // initialize with random data (always the same because of const seed)
    srand(1);
    for(i = 0; i < buffersize; i++)
        writedata[i] = (char)rand();
    
    // open file, write data, close file
    testfilepointer = fopen(testfilename, "wb+");
    if (testfilepointer == NULL)
    {
        report_failure_strerror();
        report_failure("Failed to open file for writting.");
        return -1;
    }
    operationsize = fwrite(writedata, 1, buffersize, testfilepointer);
    if (operationsize != buffersize)
    {
        report_failure_strerror();
        report_failure("Failed to write data.");
        return -1;
    }

    if ((strcmp(fopenflags, "w+b") == 0) ||
        (strcmp(fopenflags, "wb+") == 0))
    {
        // Seek to beginning
        if (fseek(testfilepointer, 0, SEEK_SET) != 0)
        {
            report_failure_strerror();
            report_failure("Failed to seek to file beginning (w+b).");           
            return -1;
        }
    }
    else
    {

        if (fclose(testfilepointer) != 0)
        {
            report_failure_strerror();
            report_failure("Failed to close file.");
            return -1;
        }
    
        // open file
        testfilepointer = fopen(testfilename, "rb");
        if (testfilepointer == NULL)
        {
            report_failure_strerror();
            report_failure("Failed to open file for reading.");
            return -1;
        }
    }

    //read data, compare data, close file
    erase_buffer(readdatabuffer1, buffersize);
    operationsize = fread(readdatabuffer1, 1, buffersize, testfilepointer);
    if (operationsize != buffersize)
    {
        report_failure_strerror();
        report_failure("Failed to read data.");
        return -1;
    }
    if (compare_buffers(readdatabuffer1, writedata, buffersize) != 0)
    {
        report_failure("Read data differs from written data");
        return -1;
    }

    if ((strcmp(fopenflags, "w+b") == 0) ||
        (strcmp(fopenflags, "wb+") == 0))
    {
        // Seek to beginning
        if (fseek(testfilepointer, 0, SEEK_SET) != 0)
        {
            report_failure_strerror();
            report_failure("Failed to seek to file beginning (w+b).");           
            return -1;
        }
    }
    else
    {
        // Close file and open it with new flags
        if (fclose(testfilepointer) != 0)
        {
            report_failure_strerror();
            report_failure("Failed to close file.");
            return -1;
        }
    
        testfilepointer = fopen(testfilename, fopenflags);
        if (testfilepointer == NULL)
        {
            report_failure_strerror();
            report_failure("Failed to open file for seek testing.");
            return -1;
        }
    }

    return 0;

}

int test_fseek_and_fread()
{

    /* Expects that testfilepointer is opened in correct mode */
    size_t chunksize = 128;
    char readdatabuffer1[1024] = {0};
    int i = 0;
    size_t operationsize = 0;

    /* 
     * TEST
     */

    report_progress("Start test_fseek_and_fread");

    next_test_step();

    test_report_description("seek to begin, read some data , check data");
    
    // seek to begin, read some data, check data
    if (fseek(testfilepointer, 0, SEEK_SET) != 0)
    {
        test_report_failure_strerror();
        test_report_failure("Failed to seek to beginning of file");
        return -1;
    }
    test_report_progress("Seek to beginning of file.");
    erase_buffer(readdatabuffer1, buffersize);
    operationsize = fread(readdatabuffer1, 1, chunksize, testfilepointer);
    if (operationsize != chunksize)
    {
        test_report_failure_strerror();
        test_report_failure("Failed to read chunk of data.");
        return -1;
    }
    test_report_progress("Chunk read.");
    if (compare_buffers(readdatabuffer1, writedata, chunksize) != 0)
    {
        test_report_failure("Read data does not match written data");
        return -1;
    }
    test_report_success("Read data match written data.");
    test_report_success("SUCCESS");   

    /* 
     * TEST END
     */  


    /* 
     * TEST
     */
    
    next_test_step();

    test_report_description("make absolute seek, read some data , check data");
    
    // make absolute seek, read some data, check data
    if (fseek(testfilepointer, 350, SEEK_SET) != 0)
    {
        test_report_failure_strerror();
        test_report_failure("Failed to make absolute seek");
        return -1;
    }
    test_report_progress("Absolute seek made.");
    erase_buffer(readdatabuffer1, buffersize);
    operationsize = fread(readdatabuffer1, 1, chunksize, testfilepointer);
    if (operationsize != chunksize)
    {
        test_report_failure_strerror();
        test_report_failure("Failed to read chunk of data.");
        return -1;
    }
    test_report_progress("Chunk read.");
    if (compare_buffers(readdatabuffer1, writedata + 350, chunksize) != 0)
    {
        test_report_failure("Read data does not match written data");
        return -1;
    }
    test_report_success("Read data match written data.");
    test_report_success("SUCCESS"); 

    /* 
     * TEST END
     */  


    /* 
     * TEST
     */
    
    next_test_step();

    test_report_description("make relative seek backwards, read some data , check data");
    // make relative seek backwards, read some data, check data
    if (fseek(testfilepointer, - (120 + chunksize), SEEK_CUR) != 0)
    {
        test_report_failure_strerror();
        test_report_failure("Failed to make relative seek");
        return -1;
    }
    test_report_progress("Relative seek made.");
    erase_buffer(readdatabuffer1, buffersize);
    operationsize = fread(readdatabuffer1, 1, chunksize, testfilepointer);
    if (operationsize != chunksize)
    {
        test_report_failure_strerror();
        test_report_failure("Failed to read chunk of data.");
        return -1;
    }
    test_report_progress("Chunk read.");
    if (compare_buffers(readdatabuffer1, writedata + (350 - 120), chunksize) != 0)
    {
        test_report_failure("Read data does not match written data");
        return -1;
    }
    test_report_success("Read data match written data.");
    test_report_success("SUCCESS");

    /* 
     * TEST END
     */  


    /* 
     * TEST
     */
    
    next_test_step();

    test_report_description("make relative seek forewards, read some data , check data");
    // make relative seek forewards, read some data, check data
    if (fseek(testfilepointer, 138, SEEK_CUR) != 0)
    {
        test_report_failure_strerror();
        test_report_failure("Failed to make relative seek");
        return -1;
    }
    test_report_progress("Relative seek made.");
    erase_buffer(readdatabuffer1, buffersize);
    operationsize = fread(readdatabuffer1, 1, chunksize, testfilepointer);
    if (operationsize != chunksize)
    {
        test_report_failure_strerror();
        test_report_failure("Failed to read chunk of data.");
        return -1;
    }
    test_report_progress("Chunk read.");
    if (compare_buffers(readdatabuffer1, writedata + (350 - 120 + chunksize + 138), chunksize) != 0)
    {
        test_report_failure("Read data does not match written data");
        return -1;
    }
    test_report_success("Read data match written data.");
    test_report_success("SUCCESS");


    /* 
     * TEST END
     */  


    /* 
     * TEST
     */
    
    next_test_step();

    test_report_description("seek to end, make relative seek backwards, read some data , check data");
    // seek to end, make relative seek backwards, read some data, check data
    if (fseek(testfilepointer, 0, SEEK_END) != 0)
    {
        test_report_failure_strerror();
        test_report_failure("Failed to seek to end");
        return -1;
    }
    test_report_progress("Seek to end made");
    if (fseek(testfilepointer, -210, SEEK_CUR) != 0)
    {
        test_report_failure_strerror();
        test_report_failure("Failed to make relative seek");
        return -1;
    }
    test_report_progress("Relative seek made.");
    erase_buffer(readdatabuffer1, buffersize);
    operationsize = fread(readdatabuffer1, 1, chunksize, testfilepointer);
    if (operationsize != chunksize)
    {
        test_report_failure_strerror();
        test_report_failure("Failed to read chunk of data.");
        return -1;
    }
    test_report_progress("Chunk read.");
    if (compare_buffers(readdatabuffer1, writedata + buffersize - 210, chunksize) != 0)
    {
        test_report_failure("Read data does not match written data");
        return -1;
    }
    test_report_success("Read data match written data.");
    test_report_success("SUCCESS");

    /* 
     * TEST END
     */  


    /* 
     * TEST
     */
    
    next_test_step();

    test_report_description("read some data (more than is in file) , check error code data");
    // read some data (more than is in file) , check error code data
    erase_buffer(readdatabuffer1, buffersize);
    operationsize = fread(readdatabuffer1, 1, chunksize, testfilepointer);
    if (operationsize != chunksize)
    {
        if (errno != 0/*rb*/ && errno != EINVAL/*a+b, w+b, r+b*/)
        {
            test_report_failure_strerror();
            test_report_failure("Error different than expected reported");
            return -1;
        }   
    }
    else
    {
        test_report_failure("Number of read data should be different than requested data");
        return -1;
    }
    test_report_success("Error reported");
    test_report_success("SUCCESS");

    /* 
     * TEST END
     */  


    /* 
     * TEST
     */
    
    next_test_step();

    test_report_description("seek to beginning, seek forewards, seek backwards beyond file start, check error code");
    // seek to beginning, seek forewards, seek backwards beyond file start, check error code
    if (fseek(testfilepointer, 0, SEEK_SET) != 0)
    {
        test_report_failure_strerror();
        test_report_failure("Failed to seek to beginning");
        return -1;
    }
    test_report_progress("Seek to beginning made.");
    if (fseek(testfilepointer, 99, SEEK_CUR) != 0)
    {
        test_report_failure_strerror();
        test_report_failure("Failed to make relative seek forewards");
        return -1;
    }
    test_report_progress("Relative seek forewards made.");
    if (fseek(testfilepointer, -150, SEEK_CUR) != 0)
    {
        if (errno != EINVAL)
        {
            test_report_failure_strerror();
            test_report_failure("Error different than expected reported");
            return -1;
        }
    }
    else
    {
        test_report_progress("Should not made relative seek backwards beyond file start.");
        return -1;
    }
    test_report_success("Error reported");
    test_report_success("SUCCESS");

    /* 
     * TEST END
     */  


    /* 
     * TEST
     */
    
    next_test_step();

    test_report_description("seek to end, seek forewards, read some data, check error code");
    // seek to end, seek forewards, read some data, check error code
    if (fseek(testfilepointer, 0, SEEK_END) != 0)
    {
        test_report_failure_strerror();
        test_report_failure("Failed to seek to end");
        return -1;
    }
    test_report_progress("Seek to end made.");
    if (fseek(testfilepointer, 99, SEEK_CUR) != 0)
    {
        test_report_failure_strerror();
        test_report_failure("Failed to make relative seek forewards");
        return -1;
    }
    test_report_progress("Relative seek forewards made.");
    erase_buffer(readdatabuffer1, buffersize);
    operationsize = fread(readdatabuffer1, 1, chunksize, testfilepointer);
    if (operationsize != chunksize)
    {
        if (errno != EINVAL)
        {
            test_report_failure_strerror();
            test_report_failure("Error different than expected reported");
            return -1;
        }   
    }
    else
    {
        test_report_failure("Number of read data should be different than requested data");
        return -1;
    }
    test_report_success("Error reported");
    test_report_success("SUCCESS");

   /* 
     * TEST END
     */  


    /* 
     * TEST
     */
    
    next_test_step();

    test_report_description("seek to end with offset, read some data, check error code");
    // seek to end with offset, read some data, check error code
    if (fseek(testfilepointer, 100, SEEK_END) != 0)
    {
        test_report_failure_strerror();
        test_report_failure("Failed to seek to end with offset");
        return -1;
    }
    test_report_progress("Seek to end with offset made.");
    erase_buffer(readdatabuffer1, buffersize);
    operationsize = fread(readdatabuffer1, 1, chunksize, testfilepointer);
    if (operationsize != chunksize)
    {
        if (errno != EINVAL)
        {
            test_report_failure_strerror();
            test_report_failure("Error different than expected reported");
            return -1;
        }   
    }
    else
    {
        test_report_failure("Number of read data should be different than requested data");
        return -1;
    }
    test_report_success("Error reported");
    test_report_success("SUCCESS");
    return 0;
}

int test_fseek_and_fread_wrapper(const char * fopenflags)
{
    next_test();

    int result = test_fseek_and_fread_preparation(fopenflags);
    if (result != 0)
    {   
        report_failure("Preparation failed");
    }
    else
        result = test_fseek_and_fread();

    close_test_file();

    return result;
}

int test_fseek_fwrite_fread_preparation()
{
    char readdatabuffer1[1024] = {0};
    int i = 0;
    size_t operationsize = 0;

    report_progress("Starting test_fseek_fwrite_fread_preparation");

    if (testfilepointer != NULL)
    {
        report_failure("Pointer for test file not closed.");
        return -1;
    }   

    // Initilize random write buffer
    srand(1);
    for (i = 0; i < buffersize; i++)
        writedata[i] = (char)rand();
    
    // open file, write data
    testfilepointer = fopen(testfilename, "wb+");
    if (testfilepointer == NULL)
    {
        report_failure_strerror();
        report_failure("Failed to open file for writting.");
        return -1;
    }
    operationsize = fwrite(writedata, 1, buffersize, testfilepointer);
    if (operationsize != buffersize)
    {
        report_failure_strerror();
        report_failure("Failed to write data.");
        return -1;
    }

    // Seek to beginning
    if (fseek(testfilepointer, 0, SEEK_SET) != 0)
    {
        report_failure_strerror();
        report_failure("Failed to seek to file beginning (w+b).");           
        return -1;
    }

    //read data, compare data
    erase_buffer(readdatabuffer1, buffersize);
    operationsize = fread(readdatabuffer1, 1, buffersize, testfilepointer);
    if (operationsize != buffersize)
    {
        report_failure_strerror();
        report_failure("Failed to read data.");
        return -1;
    }
    if (compare_buffers(readdatabuffer1, writedata, buffersize) != 0)
    {
        report_failure("Read data differs from written data");
        return -1;
    }

    // Seek to beginning
    if (fseek(testfilepointer, 0, SEEK_SET) != 0)
    {
        report_failure_strerror();
        report_failure("Failed to seek to file beginning (w+b).");           
        return -1;
    }

    return 0;

}

int test_fseek_fwrite_fread()
{
    /* Expects that testfilepointer is opened in correct mode */
    char randomwritebuffer[1024] = {0};
    size_t chunksize = 128;
    char readdatabuffer1[1024] = {0};
    char zerobuffer[1024] = {0};
    int i = 0;
    size_t operationsize = 0;
    
    // Initilize random write buffer
    // Don't initialize srand or randomwritebuffer == writedata
    for (i = 0; i < buffersize; i++)
        randomwritebuffer[i] = (char)rand();

    /* 
     * TEST
     */

    report_progress("Start test_fseek_fwrite_fread");

    next_test_step();

    test_report_description("seek to begin, write some data , seek to begin, read 2x some data, check data");
    
    // seek to begin, write some data , seek to begin, read 2x some data, check data
    if (fseek(testfilepointer, 0, SEEK_SET) != 0)
    {
        test_report_failure_strerror();
        test_report_failure("Failed to seek to beginning of file");
        return -1;
    }
    test_report_progress("Seek to beginning of file.");
    operationsize = fwrite(randomwritebuffer, 1, chunksize, testfilepointer);
    if (operationsize != chunksize)
    {
        test_report_failure_strerror();
        test_report_failure("Failed to write chunk of data.");
        return -1;
    }
    test_report_progress("Chunk written.");
    if (fseek(testfilepointer, 0, SEEK_SET) != 0)
    {
        test_report_failure_strerror();
        test_report_failure("Failed to seek to beginning of file");
        return -1;
    }
    test_report_progress("Seek to beginning of file.");
    erase_buffer(readdatabuffer1, buffersize);
    operationsize = fread(readdatabuffer1, 1, 2 * chunksize, testfilepointer);
    if (operationsize != 2 * chunksize)
    {
        test_report_failure_strerror();
        test_report_failure("Failed to read chunk of data.");
        return -1;
    }
    test_report_progress("Chunk read.");
    if (compare_buffers(readdatabuffer1, randomwritebuffer, chunksize) != 0)
    {
        test_report_failure("Read data does not match written data for first chunk");
        return -1;
    }
    test_report_success("Read data match written data for first chunk.");
    if (compare_buffers(readdatabuffer1 + chunksize, writedata + chunksize, chunksize) != 0)
    {
        test_report_failure("Read data does not match written data for second chunk");
        return -1;
    }
    test_report_success("Read data match written data for second chunk.");
    test_report_success("SUCCESS");   

    /* 
     * TEST END
     */

    /* 
     * TEST
     */

    next_test_step();

    test_report_description("seek foreward, write some data , seek backward, read data, check data");
    
    // seek foreward, write some data , seek backward, read data, check data
    if (fseek(testfilepointer, 3 * chunksize, SEEK_CUR) != 0)
    {
        test_report_failure_strerror();
        test_report_failure("Failed to seek foreward");
        return -1;
    }
    test_report_progress("Seek foreward made.");
    operationsize = fwrite(randomwritebuffer, 1, chunksize, testfilepointer);
    if (operationsize != chunksize)
    {
        test_report_failure_strerror();
        test_report_failure("Failed to write chunk of data.");
        return -1;
    }
    test_report_progress("Chunk written.");
    if (fseek(testfilepointer, - chunksize, SEEK_CUR) != 0)
    {
        test_report_failure_strerror();
        test_report_failure("Failed to seek backwards");
        return -1;
    }
    test_report_progress("Seek backwards made.");
    erase_buffer(readdatabuffer1, buffersize);
    operationsize = fread(readdatabuffer1, 1, chunksize, testfilepointer);
    if (operationsize != chunksize)
    {
        test_report_failure_strerror();
        test_report_failure("Failed to read chunk of data.");
        return -1;
    }
    test_report_progress("Chunk read.");
    if (compare_buffers(readdatabuffer1, randomwritebuffer, chunksize) != 0)
    {
        test_report_failure("Read data does not match written");
        return -1;
    }
    test_report_success("Read data match written data.");
    test_report_success("SUCCESS");   

    /* 
     * TEST END
     */

    /* 
     * TEST
     */

    next_test_step();

    test_report_description("seek backward, write some data , seek backward, read 2 x data, check data");
    
    // seek backward, write some data , seek backward, read 2 x data, check data
    if (fseek(testfilepointer, -2 * chunksize, SEEK_CUR) != 0)
    {
        test_report_failure_strerror();
        test_report_failure("Failed to seek backwards");
        return -1;
    }
    test_report_progress("Seek backwards made.");
    operationsize = fwrite(randomwritebuffer + chunksize, 1, chunksize, testfilepointer);
    if (operationsize != chunksize)
    {
        test_report_failure_strerror();
        test_report_failure("Failed to write chunk of data.");
        return -1;
    }
    test_report_progress("Chunk written.");
    if (fseek(testfilepointer, -chunksize, SEEK_CUR) != 0)
    {
        test_report_failure_strerror();
        test_report_failure("Failed to seek backwards");
        return -1;
    }
    test_report_progress("Seek backwards made.");
    erase_buffer(readdatabuffer1, buffersize);
    operationsize = fread(readdatabuffer1, 1, 2 * chunksize, testfilepointer);
    if (operationsize != 2 * chunksize)
    {
        test_report_failure_strerror();
        test_report_failure("Failed to read chunk of data.");
        return -1;
    }
    test_report_progress("Chunk read.");
    if (compare_buffers(readdatabuffer1, randomwritebuffer + chunksize, chunksize) != 0)
    {
        test_report_failure("Read data does not match written for first chunk");
        return -1;
    }
    test_report_success("Read data match written data for first chunk.");
    if (compare_buffers(readdatabuffer1 + chunksize, randomwritebuffer, chunksize) != 0)
    {
        test_report_failure("Read data does not match written for second chunk");
        return -1;
    }
    test_report_success("Read data match written data for second chunk.");
    test_report_success("SUCCESS");   

    /* 
     * TEST END
     */

    /* 
     * TEST
     */

    next_test_step();

    test_report_description("seek to end, write some data , seek backward, read data, check data");
    
    // seek to end, write some data , seek backward, read data, check data
    if (fseek(testfilepointer, 0, SEEK_END) != 0)
    {
        test_report_failure_strerror();
        test_report_failure("Failed to seek to end");
        return -1;
    }
    test_report_progress("Seek to end of file made.");
    operationsize = fwrite(randomwritebuffer + 2 * chunksize, 1, chunksize, testfilepointer);
    if (operationsize != chunksize)
    {
        test_report_failure_strerror();
        test_report_failure("Failed to write chunk of data.");
        return -1;
    }
    test_report_progress("Chunk written.");
    if (fseek(testfilepointer, -chunksize, SEEK_CUR) != 0)
    {
        test_report_failure_strerror();
        test_report_failure("Failed to seek backwards");
        return -1;
    }
    test_report_progress("Seek backwards made.");
    erase_buffer(readdatabuffer1, buffersize);
    operationsize = fread(readdatabuffer1, 1, chunksize, testfilepointer);
    if (operationsize != chunksize)
    {
        test_report_failure_strerror();
        test_report_failure("Failed to read chunk of data.");
        return -1;
    }
    test_report_progress("Chunk read.");
    if (compare_buffers(readdatabuffer1, randomwritebuffer + 2 * chunksize, chunksize) != 0)
    {
        test_report_failure("Read data does not match written data");
        return -1;
    }
    test_report_success("Read data match written data.");
    test_report_success("SUCCESS");   

    /* 
     * TEST END
     */

    /* 
     * TEST
     */

    next_test_step();

    test_report_description("seek to end, seek foreward, write some data , seek backward, read 2 x data, check data");
    
    // seek to end, seek foreward, write some data , seek backward, read 2 x data, check data
    if (fseek(testfilepointer, 0, SEEK_END) != 0)
    {
        test_report_failure_strerror();
        test_report_failure("Failed to seek to end");
        return -1;
    }
    test_report_progress("Seek to end of file made.");
    if (fseek(testfilepointer, chunksize, SEEK_CUR) != 0)
    {
        test_report_failure_strerror();
        test_report_failure("Failed to seek foreward");
        return -1;
    }
    test_report_progress("Seek foreward made.");
    operationsize = fwrite(randomwritebuffer, 1, chunksize, testfilepointer);
    if (operationsize != chunksize)
    {
        test_report_failure_strerror();
        test_report_failure("Failed to write chunk of data.");
        return -1;
    }
    test_report_progress("Chunk written.");
    if (fseek(testfilepointer, -2 * chunksize, SEEK_CUR) != 0)
    {
        test_report_failure_strerror();
        test_report_failure("Failed to seek backwards");
        return -1;
    }
    test_report_progress("Seek backwards made.");
    erase_buffer(readdatabuffer1, buffersize);
    erase_buffer(zerobuffer, buffersize);
    operationsize = fread(readdatabuffer1, 1, 2 * chunksize, testfilepointer);
    if (operationsize != 2 * chunksize)
    {
        test_report_failure_strerror();
        test_report_failure("Failed to read chunk of data.");
        return -1;
    }
    test_report_progress("Chunk read.");        
    if (compare_buffers(readdatabuffer1, zerobuffer, chunksize) != 0)
    {
        test_report_failure("Read data does not match written data for first chunk");
        return -1;
    }
    test_report_success("Read data match written data for first chunk.");
    if (compare_buffers(readdatabuffer1 + chunksize, randomwritebuffer, chunksize) != 0)
    {
        test_report_failure("Read data does not match written data for second chunk");
        return -1;
    }
    test_report_success("Read data match written data for second chunk.");
    test_report_success("SUCCESS");   

    /* 
     * TEST END
     */

    /* 
     * TEST
     */

    next_test_step();

    test_report_description("seek beoynd end, write some data , seek backward, read 2 x data, check data");
    
    // seek beoynd end, write some data , seek backward, read 2 x data, check data
    if (fseek(testfilepointer, chunksize, SEEK_END) != 0)
    {
        test_report_failure_strerror();
        test_report_failure("Failed to seek beyond end of file");
        return -1;
    }
    test_report_progress("Seek beyond end of file made.");
    operationsize = fwrite(randomwritebuffer + chunksize, 1, chunksize, testfilepointer);
    if (operationsize != chunksize)
    {
        test_report_failure_strerror();
        test_report_failure("Failed to write chunk of data.");
        return -1;
    }
    test_report_progress("Chunk written.");
    if (fseek(testfilepointer, -2 * chunksize, SEEK_CUR) != 0)
    {
        test_report_failure_strerror();
        test_report_failure("Failed to seek backwards");
        return -1;
    }
    test_report_progress("Seek backwards made.");
    erase_buffer(readdatabuffer1, buffersize);
    erase_buffer(zerobuffer, buffersize);
    operationsize = fread(readdatabuffer1, 1, 2 * chunksize, testfilepointer);
    if (operationsize != 2 * chunksize)
    {
        test_report_failure_strerror();
        test_report_failure("Failed to read chunk of data.");
        return -1;
    }
    test_report_progress("Chunk read.");
    if (compare_buffers(readdatabuffer1, zerobuffer, chunksize) != 0)
    {
        test_report_failure("Read data does not match written data for first chunk");
        return -1;
    }
    test_report_success("Read data match written data for first chunk.");
    if (compare_buffers(readdatabuffer1 + chunksize, randomwritebuffer + chunksize, chunksize) != 0)
    {
        test_report_failure("Read data does not match written data for second chunk");
        return -1;
    }
    test_report_success("Read data match written data for second chunk.");
    test_report_success("SUCCESS");   

    /* 
     * TEST END
     */

    return 0;
}

int test_fseek_fwrite_fread_wrapper()
{
    next_test();

    int result = test_fseek_fwrite_fread_preparation();
    if (result != 0)
    {   
        report_failure("Preparation failed");
    }
    else
        result = test_fseek_fwrite_fread();

    close_test_file();

    return result;
}

int test_fseek_fwrite_fread_append_preparation()
{
    char readdatabuffer1[1024] = {0};
    int i = 0;
    size_t operationsize = 0;

    report_progress("Starting test_fseek_fwrite_fread_append_preparation");

    if (testfilepointer != NULL)
    {
        report_failure("Pointer for test file not closed.");
        return -1;
    }   

    // Initilize random write buffer
    srand(1);
    for (i = 0; i < buffersize; i++)
        writedata[i] = (char)rand();
    
    // open file, write data
    testfilepointer = fopen(testfilename, "wb+");
    if (testfilepointer == NULL)
    {
        report_failure_strerror();
        report_failure("Failed to open file for writting.");
        return -1;
    }
    operationsize = fwrite(writedata, 1, buffersize, testfilepointer);
    if (operationsize != buffersize)
    {
        report_failure_strerror();
        report_failure("Failed to write data.");
        return -1;
    }

    // Seek to beginning
    if (fseek(testfilepointer, 0, SEEK_SET) != 0)
    {
        report_failure_strerror();
        report_failure("Failed to seek to file beginning (w+b).");           
        return -1;
    }

    //read data, compare data
    erase_buffer(readdatabuffer1, buffersize);
    operationsize = fread(readdatabuffer1, 1, buffersize, testfilepointer);
    if (operationsize != buffersize)
    {
        report_failure_strerror();
        report_failure("Failed to read data.");
        return -1;
    }
    if (compare_buffers(readdatabuffer1, writedata, buffersize) != 0)
    {
        report_failure("Read data differs from written data");
        return -1;
    }

    // close
    if (fclose(testfilepointer) != 0)
    {
        report_failure_strerror();
        report_failure("Failed to close the file.");
        return;
    }

    // open
    testfilepointer = fopen(testfilename, "ab+");
    if (testfilepointer == NULL)
    {
        report_failure_strerror();
        report_failure("Failed to open file for appending.");
        return -1;
    }

    return 0;

}

int test_fseek_fwrite_fread_append()
{
    /* Expects that testfilepointer is opened in correct mode */
    char randomwritebuffer[1024] = {0};
    size_t chunksize = 128;
    char readdatabuffer1[1024] = {0};
    char zerobuffer[1024] = {0};
    int i = 0;
    size_t operationsize = 0;
    
    // Initilize random write buffer
    // Don't initialize srand or randomwritebuffer == writedata
    for (i = 0; i < buffersize; i++)
        randomwritebuffer[i] = (char)rand();

    /* 
     * TEST
     */

    report_progress("Start test_fseek_fwrite_fread_append");

    next_test_step();

    test_report_description("seek to begin, write some data , seek to begin, read 2x some data, check data");
    
    // seek to begin, write some data , seek to begin, read 2x some data, check data
    if (fseek(testfilepointer, 0, SEEK_SET) != 0)
    {
        test_report_failure_strerror();
        test_report_failure("Failed to seek to beginning of file");
        return -1;
    }
    test_report_progress("Seek to beginning of file.");
    operationsize = fwrite(randomwritebuffer, 1, chunksize, testfilepointer);
    if (operationsize != chunksize)
    {
        test_report_failure_strerror();
        test_report_failure("Failed to write chunk of data.");
        return -1;
    }
    test_report_progress("Chunk written.");
    if (fseek(testfilepointer, 0, SEEK_SET) != 0)
    {
        test_report_failure_strerror();
        test_report_failure("Failed to seek to beginning of file");
        return -1;
    }
    test_report_progress("Seek to beginning of file.");
    erase_buffer(readdatabuffer1, buffersize);
    operationsize = fread(readdatabuffer1, 1, 2 * chunksize, testfilepointer);
    if (operationsize != 2 * chunksize)
    {
        test_report_failure_strerror();
        test_report_failure("Failed to read chunk of data.");
        return -1;
    }
    test_report_progress("Chunk read.");
    if (compare_buffers(readdatabuffer1, writedata, chunksize) != 0)
    {
        test_report_failure("Read data does not match written data for first chunk");
        return -1;
    }
    test_report_success("Read data match written data for first chunk.");
    if (compare_buffers(readdatabuffer1 + chunksize, writedata + chunksize, chunksize) != 0)
    {
        test_report_failure("Read data does not match written data for second chunk");
        return -1;
    }
    test_report_success("Read data match written data for second chunk.");
    test_report_success("SUCCESS");   

    /* 
     * TEST END
     */

    /* 
     * TEST
     */

    next_test_step();

    test_report_description("seek foreward, write some data , seek backward, read data, check data");
    
    // seek foreward, write some data , seek backward, read data, check data
    if (fseek(testfilepointer, 3 * chunksize, SEEK_CUR) != 0)
    {
        test_report_failure_strerror();
        test_report_failure("Failed to seek foreward");
        return -1;
    }
    test_report_progress("Seek foreward made.");
    operationsize = fwrite(randomwritebuffer, 1, chunksize, testfilepointer);
    if (operationsize != chunksize)
    {
        test_report_failure_strerror();
        test_report_failure("Failed to write chunk of data.");
        return -1;
    }
    test_report_progress("Chunk written.");
    if (fseek(testfilepointer, - chunksize, SEEK_CUR) != 0)
    {
        test_report_failure_strerror();
        test_report_failure("Failed to seek backwards");
        return -1;
    }
    test_report_progress("Seek backwards made.");
    erase_buffer(readdatabuffer1, buffersize);
    operationsize = fread(readdatabuffer1, 1, chunksize, testfilepointer);
    if (operationsize != chunksize)
    {
        test_report_failure_strerror();
        test_report_failure("Failed to read chunk of data.");
        return -1;
    }
    test_report_progress("Chunk read.");
    if (compare_buffers(readdatabuffer1, randomwritebuffer, chunksize) != 0)
    {
        test_report_failure("Read data does not match written");
        return -1;
    }
    test_report_success("Read data match written data.");
    test_report_success("SUCCESS");   

    /* 
     * TEST END
     */

    /* 
     * TEST
     */

    next_test_step();

    test_report_description("seek backward, write some data , seek backward, read 2 x data, check error");
    
    // seek backward, write some data , seek backward, read 2 x data, check error
    if (fseek(testfilepointer, -2 * chunksize, SEEK_CUR) != 0)
    {
        test_report_failure_strerror();
        test_report_failure("Failed to seek backwards");
        return -1;
    }
    test_report_progress("Seek backwards made.");
    operationsize = fwrite(randomwritebuffer + chunksize, 1, chunksize, testfilepointer);
    if (operationsize != chunksize)
    {
        test_report_failure_strerror();
        test_report_failure("Failed to write chunk of data.");
        return -1;
    }
    test_report_progress("Chunk written.");
    if (fseek(testfilepointer, -chunksize, SEEK_CUR) != 0)
    {
        test_report_failure_strerror();
        test_report_failure("Failed to seek backwards");
        return -1;
    }
    test_report_progress("Seek backwards made.");
    erase_buffer(readdatabuffer1, buffersize);
    operationsize = fread(readdatabuffer1, 1, 2 * chunksize, testfilepointer);
    if (operationsize != 2 * chunksize)
    {
        if (errno != EINVAL)
        {
            test_report_failure_strerror();
            test_report_failure("Different than expected error reported.");
            return -1;
        }
    }
    else
    {
        test_report_failure("Expected error");
        return -1;
    }
    test_report_success("Error reported.");
    test_report_success("SUCCESS");   

    /* 
     * TEST END
     */

    /* 
     * TEST
     */

    next_test_step();

    test_report_description("seek to end, write some data , seek backward, read data, check data");
    
    // seek to end, write some data , seek backward, read data, check data
    if (fseek(testfilepointer, 0, SEEK_END) != 0)
    {
        test_report_failure_strerror();
        test_report_failure("Failed to seek to end");
        return -1;
    }
    test_report_progress("Seek to end of file made.");
    operationsize = fwrite(randomwritebuffer + 2 * chunksize, 1, chunksize, testfilepointer);
    if (operationsize != chunksize)
    {
        test_report_failure_strerror();
        test_report_failure("Failed to write chunk of data.");
        return -1;
    }
    test_report_progress("Chunk written.");
    if (fseek(testfilepointer, -chunksize, SEEK_CUR) != 0)
    {
        test_report_failure_strerror();
        test_report_failure("Failed to seek backwards");
        return -1;
    }
    test_report_progress("Seek backwards made.");
    erase_buffer(readdatabuffer1, buffersize);
    operationsize = fread(readdatabuffer1, 1, chunksize, testfilepointer);
    if (operationsize != chunksize)
    {
        test_report_failure_strerror();
        test_report_failure("Failed to read chunk of data.");
        return -1;
    }
    test_report_progress("Chunk read.");
    if (compare_buffers(readdatabuffer1, randomwritebuffer + 2 * chunksize, chunksize) != 0)
    {
        test_report_failure("Read data does not match written data");
        return -1;
    }
    test_report_success("Read data match written data.");
    test_report_success("SUCCESS");   

    /* 
     * TEST END
     */

    /* 
     * TEST
     */

    next_test_step();

    test_report_description("seek to end, seek foreward, write some data , seek backward, read 2 x data, check data");
    
    // seek to end, seek foreward, write some data , seek backward, read 2 x data, check data
    if (fseek(testfilepointer, 0, SEEK_END) != 0)
    {
        test_report_failure_strerror();
        test_report_failure("Failed to seek to end");
        return -1;
    }
    test_report_progress("Seek to end of file made.");
    if (fseek(testfilepointer, chunksize, SEEK_CUR) != 0)
    {
        test_report_failure_strerror();
        test_report_failure("Failed to seek foreward");
        return -1;
    }
    test_report_progress("Seek foreward made.");
    operationsize = fwrite(randomwritebuffer + 3 * chunksize, 1, chunksize, testfilepointer);
    if (operationsize != chunksize)
    {
        test_report_failure_strerror();
        test_report_failure("Failed to write chunk of data.");
        return -1;
    }
    test_report_progress("Chunk written.");
    if (fseek(testfilepointer, -2 * chunksize, SEEK_CUR) != 0)
    {
        test_report_failure_strerror();
        test_report_failure("Failed to seek backwards");
        return -1;
    }
    test_report_progress("Seek backwards made.");
    erase_buffer(readdatabuffer1, buffersize);
    erase_buffer(zerobuffer, buffersize);
    operationsize = fread(readdatabuffer1, 1, 2 * chunksize, testfilepointer);
    if (operationsize != 2 * chunksize)
    {
        test_report_failure_strerror();
        test_report_failure("Failed to read chunk of data.");
        return -1;
    }
    test_report_progress("Chunk read.");
    if (compare_buffers(readdatabuffer1, randomwritebuffer + 2 * chunksize, chunksize) != 0)
    {
        test_report_failure("Read data does not match written data for first chunk");
        return -1;
    }
    test_report_success("Read data match written data for first chunk.");
    if (compare_buffers(readdatabuffer1 + chunksize, randomwritebuffer + 3 * chunksize, chunksize) != 0)
    {
        test_report_failure("Read data does not match written data for second chunk");
        return -1;
    }
    test_report_success("Read data match written data for second chunk.");
    test_report_success("SUCCESS");   

    /* 
     * TEST END
     */

    /* 
     * TEST
     */

    next_test_step();

    test_report_description("seek beyond end, write some data , seek backward, read 2 x data, check data");
    
    // seek beoynd end, write some data , seek backward, read 2 x data, check data
    if (fseek(testfilepointer, chunksize, SEEK_END) != 0)
    {
        test_report_failure_strerror();
        test_report_failure("Failed to seek beyond end of file");
        return -1;
    }
    test_report_progress("Seek beyond end of file made.");
    operationsize = fwrite(randomwritebuffer + chunksize, 1, chunksize, testfilepointer);
    if (operationsize != chunksize)
    {
        test_report_failure_strerror();
        test_report_failure("Failed to write chunk of data.");
        return -1;
    }
    test_report_progress("Chunk written.");
    if (fseek(testfilepointer, -2 * chunksize, SEEK_CUR) != 0)
    {
        test_report_failure_strerror();
        test_report_failure("Failed to seek backwards");
        return -1;
    }
    test_report_progress("Seek backwards made.");
    erase_buffer(readdatabuffer1, buffersize);
    erase_buffer(zerobuffer, buffersize);
    operationsize = fread(readdatabuffer1, 1, 2 * chunksize, testfilepointer);
    if (operationsize != 2 * chunksize)
    {
        test_report_failure_strerror();
        test_report_failure("Failed to read chunk of data.");
        return -1;
    }
    test_report_progress("Chunk read.");
    if (compare_buffers(readdatabuffer1, randomwritebuffer + 3 * chunksize, chunksize) != 0)
    {
        test_report_failure("Read data does not match written data for first chunk");
        return -1;
    }
    test_report_success("Read data match written data for first chunk.");
    if (compare_buffers(readdatabuffer1 + chunksize, randomwritebuffer + chunksize, chunksize) != 0)
    {
        test_report_failure("Read data does not match written data for second chunk");
        return -1;
    }
    test_report_success("Read data match written data for second chunk.");
    test_report_success("SUCCESS");   

    /* 
     * TEST END
     */

    return 0;
}

int test_fseek_fwrite_fread_append_wrapper()
{
    next_test();

    int result = test_fseek_fwrite_fread_append_preparation();
    if (result != 0)
    {   
        report_failure("Preparation failed");
    }
    else
        result = test_fseek_fwrite_fread_append();

    close_test_file();

    return result;
}

int main()
{
    report_progress("Starting tests");

    reset_global_test_counter();

    // seek/tell
    if (test_fseek_ftell_wrapper("wb") != 0)
        global_failure_indicator = 1;
    if (test_fseek_ftell_wrapper("w+b") != 0)
        global_failure_indicator = 1;

    // seek/read test
    // no tested:
    // wb - can only write
    // ab - can only append
    if (test_fseek_and_fread_wrapper("rb") != 0)
        global_failure_indicator = 1;

    if (test_fseek_and_fread_wrapper("r+b") != 0)
        global_failure_indicator = 1;

    if (test_fseek_and_fread_wrapper("a+b") != 0)
        global_failure_indicator = 1;

    if (test_fseek_and_fread_wrapper("w+b") != 0)
        global_failure_indicator = 1;

    // seek/write/read
    if (test_fseek_fwrite_fread_wrapper() != 0)
        global_failure_indicator = 1;

    // seek/write/read (append)
    if (test_fseek_fwrite_fread_append_wrapper() != 0)
        global_failure_indicator = 1;
    
    if (global_failure_indicator == 1)
        report_failure("One of the tests FAILED");
    else
        report_progress("All tests SUCCEEDED");

    report_progress("Tests finished");
}
