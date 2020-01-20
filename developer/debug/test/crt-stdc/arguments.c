/*
    Copyright © 1995-2014, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <exec/tasks.h>
#include <proto/exec.h>

#include <stdio.h>

extern char *_ProgramName;

int main(int argc, char **argv)
{
    int i;
    struct Task *me = FindTask(NULL);

    printf("Task name: %s\n", me->tc_Node.ln_Name);
    printf("Program name: %s\n", _ProgramName);
    printf("Got %d arguments:\n", argc);

    for (i = 0; i < argc; i++)
    	printf("%d\t%s\n", i, argv[i]);

    return 0;
}
