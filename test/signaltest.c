/*
    (C) 1995-96 AROS - The Amiga Replacement OS
    $Id$
    $Log$
    Revision 1.2  1996/08/01 17:41:40  digulla
    Added standard header for all files

    Desc:
    Lang:
*/
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
