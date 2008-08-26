#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include "test.h"

char testfilename[] = "RAM:__TEST__";

int main() 
{
    struct stat buf;
    
    TEST(creat(testfilename, 0700) < 0);
    TEST(stat(testfilename, &buf) < 0);
    TEST(buf.st_mode & 0777 != 0700);
    TEST(chmod(testfilename, 0100) < 0);
    TEST(stat(testfilename, &buf) < 0);
    TEST(buf.st_mode & 0777 != 0100);
    TEST(chmod(testfilename, 0200) < 0);
    TEST(stat(testfilename, &buf) < 0);
    TEST(buf.st_mode & 0777 != 0200);
    TEST(chmod(testfilename, 0300) < 0);
    TEST(stat(testfilename, &buf) < 0);
    TEST(buf.st_mode & 0777 != 0300);
    TEST(chmod(testfilename, 0400) < 0);
    TEST(stat(testfilename, &buf) < 0);
    TEST(buf.st_mode & 0777 != 0400);
    TEST(chmod(testfilename, 0500) < 0);
    TEST(stat(testfilename, &buf) < 0);
    TEST(buf.st_mode & 0777 != 0500);
    TEST(chmod(testfilename, 0600) < 0);
    TEST(stat(testfilename, &buf) < 0);
    TEST(buf.st_mode & 0777 != 0600);
    TEST(chmod(testfilename, 0700) < 0);
    TEST(stat(testfilename, &buf) < 0);
    TEST(buf.st_mode & 0777 != 0700);
    cleanup();
    return OK;
}

void cleanup() 
{
    remove(testfilename);
}