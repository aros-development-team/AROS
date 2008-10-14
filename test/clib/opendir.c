#include <sys/types.h>
#include <dirent.h>
#include <stdio.h>
#include "test.h"

DIR *dir;

int main()
{
    dir = opendir("RAM:T");
    TEST((dir));
    closedir(dir);
    return OK;
}

void cleanup()
{
    if(dir)
	closedir(dir);
}
