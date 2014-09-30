/*
    Copyright © 1995-2014, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include "test.h"

#define BUFLEN 100

int filedes[2] = { 0, 0 };
int dupfiledes[2] = { 0, 0 };

int main()
{
    char source[BUFLEN];
    char destination[BUFLEN];
    char *str = "foo";
    char *str2 = "bar";
    int i;

    for(i = 0; i < BUFLEN; i++)
	source[i] = str[i % strlen(str)];

    TEST((pipe(filedes) == 0));
    TEST((write(filedes[1], source, BUFLEN) == BUFLEN));
    TEST((read(filedes[0], destination, BUFLEN) == BUFLEN));
    TEST((memcmp(source, destination, BUFLEN) == 0));

    for(i = 0; i < BUFLEN; i++)
	source[i] = str2[i % strlen(str2)];

    TEST((write(filedes[1], source, BUFLEN) == BUFLEN));
    TEST((read(filedes[0], destination, BUFLEN) == BUFLEN));
    TEST((memcmp(source, destination, BUFLEN) == 0));
    
    for(i = 0; i < 10000; i++)
    {
	char src = i % 256;
	char dst;
	TESTFALSE((write(filedes[1], &src, 1) == 1));
	TESTFALSE((read(filedes[0], &dst, 1) == 1));
	TESTFALSE((src == dst));
    }
    printf("10000 read-writes passed, line %d.\n", __LINE__);

    dupfiledes[0] = dup(filedes[0]);
    TEST((dupfiledes[0] != -1));

    TEST((write(filedes[1], source, BUFLEN) == BUFLEN));
    TEST((read(dupfiledes[0], destination, BUFLEN) == BUFLEN));
    TEST((memcmp(source, destination, BUFLEN) == 0));

    for(i = 0; i < 10000; i++)
    {
	char src = i % 256;
	char dst;
	TESTFALSE((write(filedes[1], &src, 1) == 1));
	TESTFALSE((read(dupfiledes[0], &dst, 1) == 1));
	TESTFALSE((src == dst));
    }
    printf("10000 read-writes passed, line %d.\n", __LINE__);

    dupfiledes[1] = dup(filedes[1]);
    TEST((dupfiledes[1] != -1));

    TEST((write(dupfiledes[1], source, BUFLEN) == BUFLEN));
    TEST((read(filedes[0], destination, BUFLEN) == BUFLEN));
    TEST((memcmp(source, destination, BUFLEN) == 0));

    for(i = 0; i < 10000; i++)
    {
	char src = i % 256;
	char dst;
	TESTFALSE((write(dupfiledes[1], &src, 1) == 1));
	TESTFALSE((read(filedes[0], &dst, 1) == 1));
	TESTFALSE((src == dst));
    }
    printf("10000 read-writes passed, line %d.\n", __LINE__);

    TEST((write(dupfiledes[1], source, BUFLEN) == BUFLEN));
    TEST((read(dupfiledes[0], destination, BUFLEN) == BUFLEN));
    TEST((memcmp(source, destination, BUFLEN) == 0));

    for(i = 0; i < 10000; i++)
    {
	char src = i % 256;
	char dst;
	TESTFALSE((write(dupfiledes[1], &src, 1) == 1));
	TESTFALSE((read(dupfiledes[0], &dst, 1) == 1));
	TESTFALSE((src == dst));
    }
    printf("10000 read-writes passed, line %d.\n", __LINE__);

    TEST((close(dupfiledes[0]) != -1));
    dupfiledes[0] = 0;
    TEST((close(dupfiledes[1]) != -1));
    dupfiledes[1] = 0;
    
    TEST((write(filedes[1], source, BUFLEN) == BUFLEN));
    TEST((read(filedes[0], destination, BUFLEN) == BUFLEN));
    TEST((memcmp(source, destination, BUFLEN) == 0));
    
    for(i = 0; i < 10000; i++)
    {
	char src = i % 256;
	char dst;
	TESTFALSE((write(filedes[1], &src, 1) == 1));
	TESTFALSE((read(filedes[0], &dst, 1) == 1));
	TESTFALSE((src == dst));
    }
    printf("10000 read-writes passed, line %d.\n", __LINE__);

    cleanup();
    return OK;
}

void cleanup()
{
    if(filedes[0])
	close(filedes[0]);
    if(filedes[1])
	close(filedes[1]);
    if(dupfiledes[0])
	close(dupfiledes[0]);
    if(dupfiledes[1])
	close(dupfiledes[1]);
}
