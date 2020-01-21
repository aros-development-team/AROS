/*
    Copyright © 1995-2014, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <string.h>
#include <errno.h>
#include <sys/mount.h>
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include "test.h"

char testfilename[] = "RAM:__TEST__";

int main() 
{
    struct statfs buf;
    int fd;
    
    fd = creat(testfilename, 0777);
    TEST((fd != -1));
    close(fd);
    TEST((statfs(testfilename, &buf) != -1));  
    
    printf("Name:\t\t\t%s\n", buf.f_mntonname);
    printf("Fundamental block size:\t%ld\n", buf.f_fsize);
    printf("Optimal block size:\t%ld\n", buf.f_bsize);
    printf("Number of blocks:\t%ld\n", buf.f_blocks);
    printf("Free blocks:\t\t%ld\n", buf.f_bfree);
    printf("Available blocks:\t%ld\n", buf.f_bavail);
    
    cleanup();
    return OK;
}

void cleanup() 
{
    remove(testfilename);
}
