/*
    Copyright © 1995-2014, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <stdlib.h>
#include <sys/types.h>
#include <sys/mount.h>
#include <stdio.h>
#include "test.h"

struct statfs *buf = NULL;

int main() 
{
    int fscount, newfscount;
    int i;
    fscount = getfsstat(NULL, 0, 0);
    TEST((fscount != -1));
    printf("Number of filesystems: %d\n", fscount);
    buf = malloc(sizeof(struct statfs) * fscount);
    TEST(buf);
    newfscount = getfsstat(buf, (long) sizeof(struct statfs) * fscount, 0);
    TEST((newfscount != -1));
    TEST((newfscount == fscount));
    printf("Printing filesystem data:\n\n");
    for(i = 0; i < newfscount; i++) 
    {
	printf("Record number:\t\t%d\n", i+1);
	printf("Name:\t\t\t%s\n", buf[i].f_mntonname);
	printf("Fundamental block size:\t%ld\n", buf[i].f_fsize);
	printf("Optimal block size:\t%ld\n", buf[i].f_bsize);
	printf("Number of blocks:\t%ld\n", buf[i].f_blocks);
	printf("Free blocks:\t\t%ld\n", buf[i].f_bfree);
	printf("Available blocks:\t%ld\n", buf[i].f_bavail);
	printf("\n");
    }
    newfscount = getfsstat(buf, 1, 0);
    TEST((newfscount == 0));
    cleanup();
    
    return OK;
}

void cleanup() 
{
    if(buf)
	free(buf);
}
