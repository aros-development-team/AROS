/*
    Copyright © 1995-2002, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <proto/exec.h>
#include <clib/exec_protos.h>
#include <stdio.h>

int main(int argc, char* argv[])
{
    int sig[32];
    int i;

    for(i=0;i<32;i++)
    {
	sig[i]=AllocSignal(-1);
	printf("%d\n",sig[i]);
    }

    for(i=0;i<32;i++)
	FreeSignal(sig[i]);

    for(i=0;i<32;i++)
    {
	sig[i]=AllocSignal(i);
	printf("%d\n",sig[i]);
    }

    for(i=0;i<32;i++)
	FreeSignal(sig[i]);

    return 0;
}
