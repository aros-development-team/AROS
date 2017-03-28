/*
    Copyright © 1995-2014, The AROS Development Team. All rights reserved.
    $Id$
*/

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
