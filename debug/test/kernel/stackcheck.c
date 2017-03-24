/*
    Copyright © 1995-2014, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <exec/tasks.h>
#include <proto/dos.h>
#include <proto/exec.h>

#include <stdio.h>

int __nocommandline = 1;

int main(void)
{
    int i;
    struct Task *t = FindTask(NULL);

    printf("Attempting weird suicide...\n");

    /* Simulate runaway stack */
    t->tc_SPLower = (APTR)10;
    t->tc_SPUpper = (APTR)16;

    for (i = 1; i < 6; i++)
    {
	printf("%d...\n", i);
	/* Delay() should eventually end up in core_Dispatch() which should kill us */
	Delay(50);
    }

    printf("Stack checking FAILED! GO FIX IT!\n");
    return 0;
}
