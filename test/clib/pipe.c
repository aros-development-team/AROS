#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include "test.h"

#define BUFLEN 100

int filedes[2] = { 0, 0 };

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

    for(i = 0; i < BUFLEN; i++)
	source[i] = str2[i % strlen(str2)];

    TEST((write(filedes[1], source, BUFLEN) == BUFLEN));
    TEST((read(filedes[0], destination, BUFLEN) == BUFLEN));
    TEST((memcmp(source, destination, BUFLEN) == 0));
    cleanup();
    return OK;
}

void cleanup()
{
    if(filedes[0])
	close(filedes[0]);
    if(filedes[1])
	close(filedes[1]);
}
